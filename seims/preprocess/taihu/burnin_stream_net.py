# -*- coding: utf-8 -*-
"""Burn flat-river cells between reliable original-DEM channel anchors.

The source SHP features are never merged. Features whose raw DEM endpoint
elevations differ by over 5 m are excluded. For the remaining features,
reliable anchors come from original-DEM channel fits. When a local endpoint
lacks such evidence, connected flat features are followed to another anchor;
an isolated terminal end can use a distinctly low original-DEM endpoint if
its opposite end is independently anchored. Interpolation then uses cumulative
river distance. A flat terminal reach with only one reliable connected end
can extend that bed level without raising any DEM cells. Other unanchored
features stay unchanged. Original SHP
feature identities are retained in review outputs.

Run with basin_flood_py37::

    python burnin_stream_net.py --self-test
    python burnin_stream_net.py --rivers PATH_TO_SHP --out EMPTY_DIRECTORY

Set FORCE_MOUNTAIN_INPUT_FIDS and FORCE_PLAIN_INPUT_FIDS below to override
terrain class by the input SHP's native FID (0-based in ArcGIS). All parts of
an original multipart feature share one input FID and receive the same class.

No input raster or vector is overwritten. Both exact interpolation (which can
raise DEM) and lowering-only burn are exported for resolved flat features.
Lake-covered river cells retain the original DEM. A reach with at least the
configured fraction of its length in the lake mask is excluded before manual
terrain-FID overrides.
Review SHPs identify mountains, cross-reach interpolation, uncertain endpoint
evidence, large potential raising and large lowering.
"""
from __future__ import print_function

import argparse
import ctypes
import csv
import heapq
import json
import math
import os
import shutil
import tempfile

import numpy as np
import rasterio
from affine import Affine
from osgeo import ogr, osr
from pyproj import CRS, Transformer
from rasterio.features import rasterize
from rasterio.warp import transform_geom
from scipy.spatial import cKDTree
from burnin_stream_net import (GridMetric, channel_info, get_paths,
                               write_tif)


# Fill these arrays with FIDs from the INPUT river SHP's ArcGIS FID column.
# A manually classified mountain reach is left unchanged; a manually
# classified plain reach follows the same anchor-search/interpolation rules
# as automatically classified plain reaches.
FORCE_MOUNTAIN_INPUT_FIDS = [55,56,4716,4678,4681,4679,4669,4984,4644,4630,4628,4629,4603,4586,4582,4565,
                             4568,4569,4570,4572,4531,4528,4529,4575]
FORCE_PLAIN_INPUT_FIDS = [4916,3978,3943,3940,3843,3783,3784,3997,4241,3061,1591,1834,923,1727,4156,860,859,350,588,
                          146,145]



CONFIG = {
    "dem": r"G:\program\seims\SEIMS_HAND\data\taihu\张斌提取的真实平原河网\HydroLakes\DEM_cliped.tif",
    "rivers": r"J:\G\data\太湖数据\Data\天地图\天地图100万水系\HYDL_clip_wgs84_merge_手动新增河道.shp",
    'lakes': r'G:\program\seims\SEIMS_HAND\data\taihu\张斌提取的真实平原河网\HydroLakes\Taihu_lakes.shp',
    'output_dir': r'G:\program\seims\SEIMS_HAND\data\taihu\HYDL_burned_DEM',
    'bank_offsets_cells': (2.0, 3.0, 4.0, 5.0, 6.0),
    'min_bank_samples_each_side': 2,
    'min_channel_depth_m': 0.5,
    'endpoint_window_cells': 12.0,
    'endpoint_nearest_channel_cells': 3.0,
    'min_endpoint_samples': 3,
    'max_endpoint_mad_m': 0.5,
    'max_endpoint_grade': 0.02,
    'terminal_anchor_min_relief_m': 0.5,
    'node_snap_cells': 0.35,
    'node_agreement_m': 0.75,
    'target_agreement_m': 0.5,
    'review_large_cut_m': 10.0,
    'review_large_raise_m': 10.0,
    'mountain_endpoint_drop_m': 5.0,
    'lake_exclusion_fraction': 0.5,
    'force_mountain_fids': FORCE_MOUNTAIN_INPUT_FIDS,
    'force_plain_fids': FORCE_PLAIN_INPUT_FIDS,
}


class RiverLine(object):
    """Minimal line container; production geometry never enters GEOS/Shapely."""
    def __init__(self, coords):
        self.coords = tuple((float(p[0]), float(p[1])) for p in coords)
        if len(self.coords) < 2:
            raise ValueError('A river line needs at least two points.')

    def as_geojson(self):
        return {'type': 'LineString', 'coordinates': self.coords}

    def as_ogr(self):
        geom = ogr.Geometry(ogr.wkbLineString)
        for x, y in self.coords:
            geom.AddPoint_2D(x, y)
        return geom


def iter_ogr_line_coords(geom):
    """Yield each line part without creating a Shapely/GEOS geometry."""
    kind = ogr.GT_Flatten(geom.GetGeometryType())
    if kind in (ogr.wkbLineString, ogr.wkbLinearRing):
        points = geom.GetPoints()
        if len(points) >= 2:
            yield points
    elif kind in (ogr.wkbMultiLineString, ogr.wkbGeometryCollection):
        for i in range(geom.GetGeometryCount()):
            for coords in iter_ogr_line_coords(geom.GetGeometryRef(i)):
                yield coords
    else:
        raise ValueError('River input must contain line geometries.')


def read_lines_with_source_fids(path, target_crs):
    """Read SHP with GDAL/OGR, preserving native FIDs and multipart features."""
    lines, source_fids = [], []
    src = ogr.Open(path, 0)
    if src is None:
        raise IOError('Cannot open river SHP: ' + path)
    layer = src.GetLayer(0)
    source_ref = layer.GetSpatialRef()
    if source_ref is None:
        raise ValueError('Vector has no CRS: ' + path)
    source_crs = CRS.from_user_input(source_ref.ExportToWkt())
    target_crs = CRS.from_user_input(target_crs)
    tf = None if source_crs == target_crs else Transformer.from_crs(
        source_crs, target_crs, always_xy=True)
    for feat in layer:
        source_fid = int(feat.GetFID())
        geom = feat.GetGeometryRef()
        if geom is None or geom.IsEmpty():
            print('WARNING: skip feature {}: empty geometry'.format(source_fid))
            continue
        try:
            parts = list(iter_ogr_line_coords(geom))
            feature_lines = []
            for coords in parts:
                xy = ((p[0], p[1]) for p in coords)
                if tf is not None:
                    xy = (tf.transform(x, y) for x, y in xy)
                line = RiverLine(xy)
                if not np.all(np.isfinite(np.asarray(line.coords))):
                    raise ValueError('non-finite coordinates')
                feature_lines.append(line)
            lines.extend(feature_lines)
            source_fids.extend([source_fid] * len(feature_lines))
        except (TypeError, ValueError, OSError) as exc:
            print('WARNING: skip feature {}: {}'.format(source_fid, exc))
    src = None
    return lines, source_fids


def read_river_projection(path):
    """Retain both the source CRS and its exact SHP .prj contents."""
    src = ogr.Open(path, 0)
    if src is None:
        raise IOError('Cannot open river SHP: ' + path)
    layer = src.GetLayer(0)
    source_ref = layer.GetSpatialRef()
    if source_ref is None:
        raise ValueError('River SHP has no CRS: ' + path)
    source_crs = CRS.from_user_input(source_ref.ExportToWkt())
    layer = None
    src = None
    prj_path = os.path.splitext(path)[0] + '.prj'
    if not os.path.isfile(prj_path):
        raise IOError('River SHP projection file is missing: ' + prj_path)
    with open(prj_path, 'rb') as stream:
        prj_bytes = stream.read()
    if not prj_bytes:
        raise ValueError('River SHP projection file is empty: ' + prj_path)
    return source_crs, prj_bytes


def read_lake_mask(path, target_crs, transform, shape):
    """Rasterize lake interiors plus boundary-touching cells for protection."""
    src = ogr.Open(path, 0)
    if src is None:
        raise IOError('Cannot open lake polygon SHP: ' + path)
    layer = src.GetLayer(0)
    source_ref = layer.GetSpatialRef()
    if source_ref is None:
        raise ValueError('Lake SHP has no CRS: ' + path)
    source_crs = CRS.from_user_input(source_ref.ExportToWkt())
    target_crs = CRS.from_user_input(target_crs)
    polygons = []
    for feat in layer:
        geom = feat.GetGeometryRef()
        if geom is None or geom.IsEmpty():
            continue
        kind = ogr.GT_Flatten(geom.GetGeometryType())
        if kind not in (ogr.wkbPolygon, ogr.wkbMultiPolygon):
            raise ValueError('Lake SHP must contain only polygons (FID {}).'.format(
                feat.GetFID()))
        polygon = json.loads(geom.ExportToJson())
        if source_crs != target_crs:
            polygon = transform_geom(source_crs.to_wkt(), target_crs.to_wkt(),
                                     polygon)
        polygons.append((polygon, 1))
    src = None
    if not polygons:
        raise ValueError('Lake SHP contains no polygon geometries: ' + path)
    return rasterize(polygons, out_shape=shape, transform=transform,
                     all_touched=True, fill=0, dtype='uint8').astype(bool)


def parse_fid_list(value):
    """Validate the in-script list of native input-SHP FIDs."""
    if value is None:
        return set()
    if not isinstance(value, (list, tuple, set)):
        raise ValueError('Manual FIDs must be an in-script array of integers.')
    result = set()
    for item in value:
        if isinstance(item, bool) or not isinstance(item, int) or item < 0:
            raise ValueError('Each input-SHP FID must be a nonnegative integer.')
        result.add(item)
    return result


def cell_size_m(metric, transform, shape):
    r, c = shape[0] // 2, shape[1] // 2
    xy = [transform * (c + .5, r + .5),
          transform * (c + 1.5, r + .5),
          transform * (c + .5, r + 1.5)]
    horizontal = metric.distances_m(xy[:2])[-1]
    vertical = metric.distances_m([xy[0], xy[2]])[-1]
    return float((horizontal + vertical) / 2.0)


def all_touched_stations(line, transform, shape, metric):
    """Return each touched (row, col) once, with distance along source line.

    Rasterization happens in the line's local bounding window. Dense samples
    assign chainage to the touched cells, including diagonal edge cells.
    """
    coords = np.asarray(line.coords, dtype='float64')[:, :2]
    if len(coords) < 2:
        return np.empty((0, 3), dtype='float64'), 0.0
    lengths = metric.distances_m(coords)
    length_m = float(lengths[-1])
    inverse = ~transform
    grid = np.asarray([inverse * (float(x), float(y)) for x, y in coords])
    cmin = max(0, int(math.floor(np.min(grid[:, 0]))) - 1)
    cmax = min(shape[1], int(math.ceil(np.max(grid[:, 0]))) + 2)
    rmin = max(0, int(math.floor(np.min(grid[:, 1]))) - 1)
    rmax = min(shape[0], int(math.ceil(np.max(grid[:, 1]))) + 2)
    if cmax <= cmin or rmax <= rmin:
        return np.empty((0, 3), dtype='float64'), length_m
    local_transform = transform * Affine.translation(cmin, rmin)
    mask = rasterize([(line.as_geojson(), 1)], out_shape=(rmax-rmin, cmax-cmin),
                     transform=local_transform, all_touched=True)
    rr, cc = np.where(mask != 0)
    if not len(rr):
        return np.empty((0, 3), dtype='float64'), length_m
    rr, cc = rr + rmin, cc + cmin
    samples, station = [], []
    for i in range(len(grid)-1):
        step = float(np.linalg.norm(grid[i+1]-grid[i]))
        count = max(1, int(math.ceil(step / 0.2)))
        for j in range(count+1):
            t = float(j)/count
            samples.append(grid[i] + t*(grid[i+1]-grid[i]))
            station.append(lengths[i] + t*(lengths[i+1]-lengths[i]))
    if not samples:
        return np.empty((0, 3), dtype='float64'), length_m
    center = np.column_stack((cc + .5, rr + .5))
    _, near = cKDTree(np.asarray(samples)).query(center)
    s = np.asarray(station)[near]
    order = np.lexsort((cc, rr, s))
    return np.column_stack((rr[order], cc[order], s[order])), length_m


def lake_length_fraction(line, transform, lake_mask, metric):
    """Approximate the proportion of line length in lakes at 0.2-cell steps."""
    coords = np.asarray(line.coords, dtype='float64')[:, :2]
    grid = np.asarray([(~transform) * (float(x), float(y)) for x, y in coords])
    chainage = metric.distances_m(coords)
    total = float(chainage[-1])
    if total <= 0:
        return 0.0
    inside_length = 0.0
    height, width = lake_mask.shape
    for i in range(len(coords)-1):
        segment_length = float(chainage[i+1]-chainage[i])
        if segment_length <= 0:
            continue
        steps = max(1, int(math.ceil(np.linalg.norm(grid[i+1]-grid[i]) / 0.2)))
        t = (np.arange(steps, dtype='float64') + 0.5) / steps
        points = grid[i] + t[:, None] * (grid[i+1]-grid[i])
        cc = np.floor(points[:, 0]).astype(int)
        rr = np.floor(points[:, 1]).astype(int)
        on_grid = (rr >= 0) & (rr < height) & (cc >= 0) & (cc < width)
        if np.any(on_grid):
            inside_length += segment_length * float(np.count_nonzero(
                lake_mask[rr[on_grid], cc[on_grid]])) / steps
    return min(1.0, inside_length / total)


def robust_endpoint(stations, end_s, side, observed, z, channel_tree,
                    channel_rc, cell_m, cfg):
    """Fit the original channel elevations inward of one source endpoint."""
    if not len(stations):
        return None
    window = cfg['endpoint_window_cells'] * cell_m
    distance_from_end = (stations[:, 2] if side == 'start'
                         else end_s-stations[:, 2])
    local = stations[distance_from_end <= window + 1e-9]
    if not len(local):
        return None
    rows, cols = local[:, 0].astype(int), local[:, 1].astype(int)
    query_rc = np.column_stack((rows, cols))
    if channel_tree is None:
        return None
    d, idx = channel_tree.query(query_rc)
    allowed = d <= cfg['endpoint_nearest_channel_cells'] + 1e-9
    if not np.any(allowed):
        return None
    source_rc = channel_rc[idx[allowed]]
    if len(np.unique(source_rc, axis=0)) < cfg['min_endpoint_samples']:
        return None
    along = local[allowed, 2]
    values = z[source_rc[:, 0], source_rc[:, 1]].astype('float64')
    offsets = d[allowed]
    # Several all-touched cells can occupy the same along-line position.
    # One median per half-cell station prevents them from faking sample count.
    bins = np.rint((along-end_s) / (0.5*cell_m)).astype('int64')
    unique = np.unique(bins)
    if len(unique) < cfg['min_endpoint_samples']:
        return None
    x, y, lateral = [], [], []
    for group in unique:
        selected = bins == group
        x.append(float(np.median(along[selected]-end_s)))
        y.append(float(np.median(values[selected])))
        lateral.append(float(np.median(offsets[selected])))
    x, y = np.asarray(x), np.asarray(y)
    if min(abs(x)) > cfg['endpoint_nearest_channel_cells']*cell_m:
        return None
    if max(x)-min(x) < cell_m:
        return None
    slopes = [(y[j]-y[i])/(x[j]-x[i])
              for i in range(len(x)) for j in range(i+1, len(x))
              if abs(x[j]-x[i]) > 0.25*cell_m]
    if not slopes:
        return None
    slope = float(np.median(slopes))
    intercept = float(np.median(y-slope*x))
    mad = float(np.median(np.abs(y-(intercept+slope*x))))
    if (not np.isfinite(intercept) or mad > cfg['max_endpoint_mad_m']
            or abs(slope) > cfg['max_endpoint_grade']):
        return None
    return dict(value=intercept, samples=len(x), mad=mad,
                slope=slope, lateral_cells=float(np.median(lateral)),
                method='ROBUST_CHANNEL')


def fallback_endpoint(line, stations, end_s, side, z, valid, transform,
                      observed, cell_m, cfg):
    """Estimate endpoint from nearby original-DEM cells on this SHP feature."""
    xy = line.coords[0] if side == 'start' else line.coords[-1]
    col, row = (~transform) * xy[:2]
    r, c = int(math.floor(row)), int(math.floor(col))
    raw = (float(z[r, c]) if 0 <= r < z.shape[0] and
           0 <= c < z.shape[1] and valid[r, c] else None)
    if not len(stations):
        return None
    rr, cc = stations[:, 0].astype(int), stations[:, 1].astype(int)
    distance = np.abs(stations[:, 2]-end_s)
    local = valid[rr, cc] & (distance <= cfg['endpoint_window_cells']*cell_m)
    if not np.any(local):
        local = valid[rr, cc]
    if not np.any(local):
        return None
    channel_local = local & observed[rr, cc]
    selected = channel_local if np.any(channel_local) else local
    values = z[rr[selected], cc[selected]].astype('float64')
    # The lower quartile suppresses high bridge/embankment pixels while
    # avoiding reliance on a single anomalously deep DEM pixel.
    value = float(np.percentile(values, 25))
    if raw is not None:
        value = min(raw, value)
    method = ('LOCAL_CHANNEL_QUARTILE' if np.any(channel_local)
              else 'LOCAL_DEM_QUARTILE')
    return dict(value=value, samples=len(values), mad=None, slope=None,
                lateral_cells=0.0, method=method)


def raw_endpoint_elevation(line, stations, end_s, side, z, valid, transform):
    """DEM cell containing the SHP endpoint; nearest touched cell at an edge."""
    xy = line.coords[0] if side == 'start' else line.coords[-1]
    col, row = (~transform) * xy[:2]
    r, c = int(math.floor(row)), int(math.floor(col))
    if 0 <= r < z.shape[0] and 0 <= c < z.shape[1] and valid[r, c]:
        return float(z[r, c])
    if not len(stations):
        return None
    rr, cc = stations[:, 0].astype(int), stations[:, 1].astype(int)
    usable = valid[rr, cc]
    if not np.any(usable):
        return None
    local = stations[usable]
    k = int(np.argmin(np.abs(local[:, 2]-end_s)))
    r, c = local[k, :2].astype(int)
    return float(z[r, c])


def nearest_anchor(start_node, blocked_edge, adjacency, anchors):
    """Shortest flat-river route to a reliable DEM endpoint, not via this edge."""
    queue = [(0.0, start_node)]
    best = {start_node: 0.0}
    while queue:
        distance, node = heapq.heappop(queue)
        if distance > best[node] + 1e-9:
            continue
        if node in anchors:
            return node, distance, anchors[node]
        for neighbor, length, edge in adjacency.get(node, ()):
            if edge == blocked_edge:
                continue
            candidate = distance + length
            if candidate + 1e-9 < best.get(neighbor, float('inf')):
                best[neighbor] = candidate
                heapq.heappush(queue, (candidate, neighbor))
    return None


def terminal_dem_anchor(node, adjacency, stations, end_s, raw, other,
                        z, valid, lake_mask, cell_m, length, cfg):
    """Use a clear original-DEM endpoint pit at an isolated open river end.

    This only resolves one missing end when the other end already has an
    independent reliable anchor. It does not turn every fallback estimate
    into a network-wide anchor.
    """
    if (len(adjacency.get(node, ())) != 1 or raw is None or other is None or
            abs(raw-other) > cfg['mountain_endpoint_drop_m'] or
            length <= 0 or abs(raw-other)/length > cfg['max_endpoint_grade']):
        return None
    if not len(stations):
        return None
    rr, cc = stations[:, 0].astype(int), stations[:, 1].astype(int)
    local = (np.abs(stations[:, 2]-end_s) <=
             cfg['endpoint_window_cells']*cell_m)
    local &= valid[rr, cc] & ~lake_mask[rr, cc]
    values = z[rr[local], cc[local]].astype('float64')
    if len(values) < cfg['min_endpoint_samples']:
        return None
    lower_quartile, median = np.percentile(values, [25, 50])
    if (raw > lower_quartile + 0.1 or
            median-raw < cfg['terminal_anchor_min_relief_m']):
        return None
    return node, 0.0, float(raw)


def one_sided_flat_anchor(missing_node, known_node, known_route, adjacency,
                          anchors, node_rows, raw_missing, length, cfg):
    """Extend a directly supported flat-river bed to one open terminal end."""
    if (known_route is None or known_route[0] != known_node or
            known_route[1] > 1e-9 or known_node not in anchors or
            len(adjacency.get(missing_node, ())) != 1 or
            raw_missing is None or length <= 0):
        return None
    # At least one original-DEM channel fit must support the connected node.
    if node_rows[known_node-1][2] < 1:
        return None
    value = float(known_route[2])
    if (abs(raw_missing-value) > cfg['mountain_endpoint_drop_m'] or
            abs(raw_missing-value)/length > cfg['max_endpoint_grade']):
        return None
    return missing_node, 0.0, value


def cluster_endpoints(lines, estimates, transform, cfg):
    """Share elevation evidence at coincident source endpoints, not features."""
    inverse = ~transform
    positions = []
    for line in lines:
        coords = list(line.coords)
        for xy in (coords[0], coords[-1]):
            c, r = inverse * xy[:2]
            positions.append((r, c))
    parent = list(range(len(positions)))
    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    def unite(i, j):
        a, b = root(i), root(j)
        if a != b:
            parent[b] = a
    tree = cKDTree(np.asarray(positions))
    for i, j in tree.query_pairs(cfg['node_snap_cells']):
        unite(i, j)
    groups = {}
    for i in range(len(positions)):
        groups.setdefault(root(i), []).append(i)
    node_value = [None] * len(positions)
    node_source = ['NO_EVIDENCE'] * len(positions)
    node_id = [None] * len(positions)
    node_rows = []
    for number, members in enumerate(groups.values(), 1):
        claims = [(i, estimates[i]['value']) for i in members
                  if estimates[i] is not None]
        value, state = None, 'NO_EVIDENCE'
        if claims:
            vals = np.asarray([v for _, v in claims])
            median = float(np.median(vals))
            if len(vals) == 1:
                value, state = median, 'DIRECT'
            else:
                ordered = np.sort(vals)
                best = ordered[:1]
                for left in range(len(ordered)):
                    right = np.searchsorted(ordered,
                                            ordered[left]+cfg['node_agreement_m'],
                                            side='right')
                    if right-left > len(best):
                        best = ordered[left:right]
                if len(best) > len(vals)/2.0:
                    value, state = float(np.median(best)), 'CONSENSUS'
                else:
                    # A shared pixel needs one value. The lower original-DEM
                    # estimate avoids leaving a dam at a feature junction.
                    value, state = float(np.min(vals)), 'DISAGREEMENT_MIN'
        for i in members:
            node_id[i] = number
            node_value[i] = value
            node_source[i] = state if estimates[i] is not None else (
                'BORROWED' if value is not None else state)
        node_rows.append((number, len(members), len(claims), value, state,
                          min((v for _, v in claims), default=None),
                          max((v for _, v in claims), default=None)))
    return node_id, node_value, node_source, node_rows


def calculate(z, valid, transform, crs, lines, cfg, source_fids=None,
              lake_mask=None):
    if source_fids is None:
        source_fids = list(range(len(lines)))
    if len(source_fids) != len(lines):
        raise ValueError('source_fids must match the number of river lines.')
    if lake_mask is None:
        lake_mask = np.zeros(z.shape, bool)
    elif lake_mask.shape != z.shape:
        raise ValueError('lake_mask must match the DEM shape.')
    else:
        lake_mask = lake_mask.astype(bool, copy=False)
    lake_threshold = float(cfg.get('lake_exclusion_fraction', 0.5))
    if not 0.0 < lake_threshold <= 1.0:
        raise ValueError('lake_exclusion_fraction must be in (0, 1].')
    force_mountain = parse_fid_list(cfg.get('force_mountain_fids'))
    force_plain = parse_fid_list(cfg.get('force_plain_fids'))
    available = set(source_fids)
    unknown = (force_mountain | force_plain) - available
    if unknown:
        raise ValueError('Manual FIDs not found in input SHP: {}'.format(
            sorted(unknown)))
    metric = GridMetric(crs)
    cell_m = cell_size_m(metric, transform, z.shape)
    potential = np.zeros(z.shape, bool)
    station_lines, lengths = [], []
    for number, line in enumerate(lines, 1):
        if number % 500 == 0:
            print('Rasterizing feature', number, '/', len(lines))
        stations, length = all_touched_stations(line, transform, z.shape, metric)
        station_lines.append(stations)
        lengths.append(length)
        if len(stations):
            potential[stations[:, 0].astype(int), stations[:, 1].astype(int)] = True
    raw_ends, mountain, lake_reach, lake_fraction, terrain_source = [], [], [], [], []
    for i, (line, stations, length) in enumerate(zip(lines, station_lines, lengths)):
        raw_start = raw_endpoint_elevation(line, stations, 0.0, 'start',
                                           z, valid, transform)
        raw_end = raw_endpoint_elevation(line, stations, length, 'end',
                                         z, valid, transform)
        raw_ends.append((raw_start, raw_end))
        auto_mountain = (raw_start is not None and raw_end is not None and
                         abs(raw_end-raw_start) > cfg['mountain_endpoint_drop_m'])
        key = source_fids[i]
        # Measure along the source line; counts of all-touched cells would
        # over-weight diagonals and shoreline edge cells.
        fraction = (lake_length_fraction(line, transform, lake_mask, metric)
                    if len(stations) and np.any(lake_mask[
                        stations[:, 0].astype(int), stations[:, 1].astype(int)])
                    else 0.0)
        lake_fraction.append(fraction)
        inside_lake = fraction >= lake_threshold
        lake_reach.append(bool(inside_lake))
        if inside_lake:
            mountain.append(False)
            terrain_source.append('LAKE')
        elif key in force_mountain:
            mountain.append(True)
            terrain_source.append('MANUAL_MOUNTAIN')
        elif key in force_plain:
            mountain.append(False)
            terrain_source.append('MANUAL_PLAIN')
        else:
            mountain.append(auto_mountain)
            terrain_source.append('AUTO')
    active_fids = {source_fids[i] for i, is_lake in enumerate(lake_reach)
                   if not is_lake}
    conflicting = force_mountain & force_plain & active_fids
    if conflicting:
        raise ValueError('The same FID cannot be forced to both terrain types: '
                         + str(sorted(conflicting)))
    empty_protect = np.zeros(z.shape, bool)
    thin = get_paths(lines, transform, z, valid, empty_protect, False)
    banks = channel_info(thin, transform, potential, z, valid,
                         cfg['bank_offsets_cells'],
                         cfg['min_bank_samples_each_side'])
    observed = (potential & valid & np.isfinite(banks) &
                (z <= banks-cfg['min_channel_depth_m']+1e-6))
    estimates = []
    for line, stations, length in zip(lines, station_lines, lengths):
        if len(stations):
            rr, cc = stations[:, 0].astype(int), stations[:, 1].astype(int)
            channel_rc = np.unique(np.column_stack((rr[observed[rr, cc]],
                                                     cc[observed[rr, cc]])),
                                   axis=0)
        else:
            channel_rc = np.empty((0, 2), dtype=int)
        channel_tree = cKDTree(channel_rc) if len(channel_rc) else None
        for end_s, side in ((0.0, 'start'), (length, 'end')):
            estimate = robust_endpoint(stations, end_s, side, observed,
                                       z, channel_tree, channel_rc, cell_m, cfg)
            if estimate is None:
                estimate = fallback_endpoint(line, stations, end_s, side,
                                             z, valid, transform, observed,
                                             cell_m, cfg)
            estimates.append(estimate)
    reliable = [fit if fit is not None and
                fit['method'] == 'ROBUST_CHANNEL' and not mountain[j//2]
                and not lake_reach[j//2]
                else None for j, fit in enumerate(estimates)]
    node_id, node_value, node_source, node_rows = cluster_endpoints(
        lines, reliable, transform, cfg)
    anchors = {}
    for nid, value, source in zip(node_id, node_value, node_source):
        if value is not None and source != 'DISAGREEMENT_MIN':
            anchors[nid] = value
    adjacency = {}
    for i, length in enumerate(lengths):
        if mountain[i] or lake_reach[i] or length <= 0:
            continue
        a, b = node_id[2*i:2*i+2]
        adjacency.setdefault(a, []).append((b, length, i))
        adjacency.setdefault(b, []).append((a, length, i))
    target = np.full(z.shape, np.nan, 'float32')
    conflict = np.zeros(z.shape, bool)
    unresolved = np.zeros(z.shape, bool)
    mountain_pixels = np.zeros(z.shape, bool)
    lake_reach_pixels = np.zeros(z.shape, bool)
    feature_rows = []
    feature_reviews = []
    for i, (stations, length) in enumerate(zip(station_lines, lengths)):
        a, b = node_id[2*i:2*i+2]
        start_route = (nearest_anchor(a, i, adjacency, anchors)
                       if not mountain[i] and not lake_reach[i] else None)
        end_route = (nearest_anchor(b, i, adjacency, anchors)
                     if not mountain[i] and not lake_reach[i] else None)
        terminal_used = False
        if start_route is None and end_route is not None:
            start_route = terminal_dem_anchor(
                a, adjacency, stations, 0.0, raw_ends[i][0], end_route[2],
                z, valid, lake_mask, cell_m, length, cfg)
            terminal_used = start_route is not None
        elif end_route is None and start_route is not None:
            end_route = terminal_dem_anchor(
                b, adjacency, stations, length, raw_ends[i][1],
                start_route[2], z, valid, lake_mask, cell_m, length, cfg)
            terminal_used = end_route is not None
        one_sided = False
        if start_route is None and end_route is not None:
            start_route = one_sided_flat_anchor(
                a, b, end_route, adjacency, anchors, node_rows,
                raw_ends[i][0], length, cfg)
            one_sided = start_route is not None
        elif end_route is None and start_route is not None:
            end_route = one_sided_flat_anchor(
                b, a, start_route, adjacency, anchors, node_rows,
                raw_ends[i][1], length, cfg)
            one_sided = end_route is not None
        start = start_route[2] if start_route else None
        end = end_route[2] if end_route else None
        start_extra = start_route[1] if start_route else 0.0
        end_extra = end_route[1] if end_route else 0.0
        total_length = start_extra + length + end_extra
        bridged = (start_route is not None and end_route is not None and
                   (start_extra > 0 or end_extra > 0))
        start_fit, end_fit = estimates[2*i:2*i+2]
        start_method = start_fit['method'] if start_fit else 'NO_ESTIMATE'
        end_method = end_fit['method'] if end_fit else 'NO_ESTIMATE'
        bad_end = (start_method != 'ROBUST_CHANNEL' or
                   end_method != 'ROBUST_CHANNEL' or
                   node_source[2*i] == 'DISAGREEMENT_MIN' or
                   node_source[2*i+1] == 'DISAGREEMENT_MIN' or
                   start is None or end is None)
        max_raise, max_cut, n_raise, n_cut = 0.0, 0.0, 0, 0
        reason = ('LAKE' if lake_reach[i] else
                  'MOUNTAIN' if mountain[i] else
                  'UNRESOLVED_ANCHORS' if (start_route is None or
                                            end_route is None or
                                            start_route[0] == end_route[0] or
                                            total_length <= 0) else
                  'TERMINAL_ANCHOR' if terminal_used else
                  'ONE_SIDED_FLAT' if one_sided else
                  'BRIDGED' if bridged else 'INTERPOLATED')
        if reason == 'LAKE':
            if len(stations):
                rr, cc = stations[:, 0].astype(int), stations[:, 1].astype(int)
                lake_reach_pixels[rr, cc] = True
        elif reason == 'MOUNTAIN':
            if len(stations):
                rr, cc = stations[:, 0].astype(int), stations[:, 1].astype(int)
                mountain_pixels[rr, cc] = True
        elif reason == 'UNRESOLVED_ANCHORS':
            if len(stations):
                rr, cc = stations[:, 0].astype(int), stations[:, 1].astype(int)
                unresolved[rr, cc] = True
        else:
            rr = stations[:, 0].astype(int)
            cc = stations[:, 1].astype(int)
            usable = valid[rr, cc]
            if np.any(usable):
                ss = stations[usable, 2]
                desired_values = start + (end-start)*((start_extra+ss)/total_length)
                differences = desired_values-z[rr[usable], cc[usable]]
                max_raise = float(max(0.0, np.max(differences)))
                max_cut = float(max(0.0, -np.min(differences)))
                n_raise = int(np.sum(differences > cfg['review_large_raise_m']))
                n_cut = int(np.sum(-differences > cfg['review_large_cut_m']))
            for r, c, s in stations:
                r, c = int(r), int(c)
                if not valid[r, c]:
                    continue
                desired = float(start + (end-start)*((start_extra+s)/total_length))
                if np.isfinite(target[r, c]):
                    if abs(float(target[r, c])-desired) > cfg['target_agreement_m']:
                        conflict[r, c] = True
                    target[r, c] = min(float(target[r, c]), desired)
                else:
                    target[r, c] = desired
        feature_rows.append((i+1, source_fids[i], len(stations), length,
                             a, b, start, end, terrain_source[i],
                             lake_fraction[i], reason))
        feature_reviews.append(dict(fid=i+1, src_fid=source_fids[i],
                                    class_src=terrain_source[i], bad_end=bad_end,
                                    start_src=start_method,
                                    end_src=end_method,
                                    start_z=start, end_z=end,
                                    max_raise=max_raise, max_cut=max_cut,
                                    n_raise=n_raise, n_cut=n_cut,
                                    n_cells=len(stations),
                                    mountain=int(mountain[i]),
                                    lake=int(lake_reach[i]),
                                    lake_frac=lake_fraction[i],
                                    terminal=int(reason == 'TERMINAL_ANCHOR'),
                                    one_sided=int(reason == 'ONE_SIDED_FLAT'),
                                    bridged=int(bridged and reason == 'BRIDGED'),
                                    raw_s=raw_ends[i][0],
                                    raw_e=raw_ends[i][1],
                                    anchor_a=start_route[0] if start_route else None,
                                    anchor_b=end_route[0] if end_route else None,
                                    dist_a=start_extra, dist_b=end_extra))
    # Even at an intersection, a pixel touched by an excluded mountain
    # feature must retain its original DEM elevation.
    target[mountain_pixels | lake_mask | lake_reach_pixels] = np.nan
    required = np.full(z.shape, np.nan, 'float32')
    known = potential & valid & np.isfinite(target)
    required[known] = np.maximum(0.0, z[known]-target[known])
    raising_if_exact = np.zeros(z.shape, 'float32')
    raising_if_exact[known] = np.maximum(0.0, target[known]-z[known])
    large_raise = raising_if_exact > cfg['review_large_raise_m']
    out = z.copy()
    out[known] = np.minimum(z[known], target[known])
    lowering = np.zeros(z.shape, 'float32')
    lowering[known] = np.maximum(0.0, z[known]-out[known])
    large_cut = lowering > cfg['review_large_cut_m']
    status = np.zeros(z.shape, 'uint8')
    status[potential & valid] = 3          # no supported anchor pair
    status[known] = 1                     # already at/below target
    status[lowering > 1e-6] = 2
    status[conflict & valid] = 5
    status[mountain_pixels & valid] = 6
    status[(lake_mask & potential | lake_reach_pixels) & valid] = 7
    status[~valid] = 255
    endpoint_rows = []
    for i, line in enumerate(lines):
        coords = list(line.coords)
        for side, xy, j in (('start', coords[0], 2*i),
                            ('end', coords[-1], 2*i+1)):
            raw_col, raw_row = (~transform) * xy[:2]
            r, c = int(math.floor(raw_row)), int(math.floor(raw_col))
            raw = float(z[r, c]) if (0 <= r < z.shape[0] and
                                     0 <= c < z.shape[1] and valid[r, c]) else None
            fit = estimates[j]
            endpoint_rows.append((i+1, side, xy[0], xy[1], r, c, raw,
                                  fit['value'] if fit else None,
                                  fit['samples'] if fit else 0,
                                  fit['mad'] if fit else None,
                                  fit['lateral_cells'] if fit else None,
                                  fit['method'] if fit else None,
                                  node_id[j], node_value[j], node_source[j]))
    return dict(dem=out, lowering=lowering, large_cut=large_cut,
                raising_if_exact=raising_if_exact, large_raise=large_raise,
                required=required,
                target=target, status=status, potential=potential,
                observed=observed, banks=banks, conflict=conflict,
                unresolved=unresolved, mountain_pixels=mountain_pixels,
                lake_mask=lake_mask, lake_reach_pixels=lake_reach_pixels,
                endpoint_rows=endpoint_rows,
                node_rows=node_rows, feature_rows=feature_rows,
                feature_reviews=feature_reviews)


def export(result, profile, crs, valid, cfg, lines,
           vector_crs=None, vector_prj=None):
    directory = cfg['output_dir']
    os.makedirs(directory, exist_ok=True)
    def path(name):
        return os.path.join(directory, name)
    nd = profile.get('nodata')
    nd = -9999.0 if nd is None else nd
    final = result['dem'].copy()
    final[~valid] = nd
    write_tif(path('dem_burn.tif'), final, profile, nd)
    exact = final.copy()
    targeted = result['potential'] & valid & np.isfinite(result['target'])
    exact[targeted] = result['target'][targeted]
    write_tif(path('dem_interpolated_all_touched.tif'), exact, profile, nd)
    for key, filename in (('lowering', 'lowering_m.tif'),
                          ('raising_if_exact', 'raising_if_exact_m.tif'),
                          ('required', 'required_lowering_m.tif'),
                          ('target', 'target_bed_m.tif'),
                          ('banks', 'bank_reference_m.tif')):
        arr = result[key].copy()
        arr[~valid | ~np.isfinite(arr)] = -9999.0
        write_tif(path(filename), arr, profile, -9999.0)
    for key, filename in (('potential', 'all_touched_river.tif'),
                          ('observed', 'observed_dem_channel.tif'),
                          ('conflict', 'target_conflict.tif'),
                          ('status', 'status.tif')):
        arr = result[key].astype('uint8').copy()
        arr[~valid] = 255
        write_tif(path(filename), arr, profile, 255)
    tables = (
        ('endpoint_estimates.csv', result['endpoint_rows'],
         ('feature_id', 'side', 'x', 'y', 'row', 'col', 'raw_dem_m',
          'local_estimate_m', 'trusted_samples', 'fit_mad_m',
          'lateral_offset_cells', 'estimate_method', 'node_id',
          'node_estimate_m', 'source')),
        ('endpoint_nodes.csv', result['node_rows'],
         ('node_id', 'connected_ends', 'direct_estimates', 'elevation_m',
          'status', 'direct_min_m', 'direct_max_m')),
        ('river_features.csv', result['feature_rows'],
         ('feature_id', 'source_fid', 'all_touched_pixels', 'length_m',
          'start_node', 'end_node', 'start_elevation_m', 'end_elevation_m',
          'terrain_class_source', 'lake_length_fraction', 'result')),
    )
    for filename, rows, headers in tables:
        with open(path(filename), 'w', encoding='utf-8-sig', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(headers)
            writer.writerows(rows)
    review_fields = (
        ('fid', ogr.OFTInteger), ('src_fid', ogr.OFTInteger),
        ('class_src', ogr.OFTString), ('start_src', ogr.OFTString),
        ('end_src', ogr.OFTString), ('start_z', ogr.OFTReal),
        ('end_z', ogr.OFTReal), ('max_raise', ogr.OFTReal),
        ('max_cut', ogr.OFTReal), ('n_raise', ogr.OFTInteger),
        ('n_cut', ogr.OFTInteger), ('n_cells', ogr.OFTInteger),
        ('lake_frac', ogr.OFTReal),
        ('raw_s', ogr.OFTReal), ('raw_e', ogr.OFTReal),
        ('anchor_a', ogr.OFTInteger), ('anchor_b', ogr.OFTInteger),
        ('dist_a', ogr.OFTReal), ('dist_b', ogr.OFTReal))
    review_groups = (
        ('unreliable_endpoint_reaches.shp', lambda row: row['bad_end']),
        ('large_raising_reaches.shp', lambda row: row['n_raise'] > 0),
        ('large_lowering_reaches.shp', lambda row: row['n_cut'] > 0),
        ('mountain_reaches.shp', lambda row: row['mountain']),
        ('plain_reaches.shp', lambda row: not row['mountain'] and not row['lake']),
        ('lake_excluded_reaches.shp', lambda row: row['lake']),
        ('terminal_anchor_reaches.shp', lambda row: row['terminal']),
        ('one_sided_flat_reaches.shp', lambda row: row['one_sided']),
        ('cross_reach_interpolation.shp', lambda row: row['bridged']),
    )
    review_counts = {}
    vector_crs = CRS.from_user_input(vector_crs or crs)
    to_source = (None if vector_crs == CRS.from_user_input(crs) else
                 Transformer.from_crs(crs, vector_crs, always_xy=True))
    spatial_ref = osr.SpatialReference()
    if spatial_ref.ImportFromWkt(vector_crs.to_wkt(version='WKT1_GDAL')) != 0:
        raise ValueError('Cannot write review SHP: invalid river CRS.')
    driver = ogr.GetDriverByName('ESRI Shapefile')
    for filename, predicate in review_groups:
        count = 0
        dst = driver.CreateDataSource(path(filename))
        if dst is None:
            raise IOError('Cannot create review SHP: ' + path(filename))
        layer = dst.CreateLayer(os.path.splitext(filename)[0], spatial_ref,
                                ogr.wkbLineString, options=['ENCODING=UTF-8'])
        if layer is None:
            raise IOError('Cannot create review SHP layer: ' + filename)
        for name, field_type in review_fields:
            field = ogr.FieldDefn(name, field_type)
            if field_type == ogr.OFTString:
                field.SetWidth(32)
            elif field_type == ogr.OFTReal:
                field.SetWidth(18)
                field.SetPrecision(4)
            layer.CreateField(field)
        for line, row in zip(lines, result['feature_reviews']):
            if not predicate(row):
                continue
            feature = ogr.Feature(layer.GetLayerDefn())
            if to_source is None:
                feature.SetGeometry(line.as_ogr())
            else:
                original_coords = [to_source.transform(x, y)
                                   for x, y in line.coords]
                feature.SetGeometry(RiverLine(original_coords).as_ogr())
            for name, _ in review_fields:
                if row[name] is not None:
                    feature.SetField(name, row[name])
            if layer.CreateFeature(feature) != 0:
                raise IOError('Cannot write review SHP feature ' + str(row['fid']))
            feature = None
            count += 1
        dst = None
        if vector_prj is not None:
            # The shapefile driver may normalize WKT to a form older GIS
            # software cannot read. Preserve the input .prj byte-for-byte.
            with open(path(os.path.splitext(filename)[0] + '.prj'), 'wb') as stream:
                stream.write(vector_prj)
        review_counts[filename] = count
    summary = {
        'config': cfg,
        'crs_wkt': crs.to_wkt(),
        'review_shp_crs_wkt': vector_crs.to_wkt(),
        'candidate_pixels': int(np.sum(result['potential'] & valid)),
        'observed_channel_pixels': int(np.sum(result['observed'])),
        'modified_pixels': int(np.sum(result['lowering'] > 1e-6)),
        'large_cut_review_pixels': int(np.sum(result['large_cut'])),
        'exact_interpolation_raised_pixels': int(np.sum(result['raising_if_exact'] > 1e-6)),
        'large_raise_review_pixels': int(np.sum(result['large_raise'])),
        'uncertain_endpoint_pixels': int(np.sum(result['unresolved'] & valid)),
        'conflicting_target_pixels': int(np.sum(result['status'] == 5)),
        'features_with_targets': int(sum(row[-1] in ('INTERPOLATED', 'BRIDGED',
                                                    'TERMINAL_ANCHOR',
                                                    'ONE_SIDED_FLAT')
                                         for row in result['feature_rows'])),
        'terminal_anchor_features': int(sum(row['terminal']
                                            for row in result['feature_reviews'])),
        'one_sided_flat_features': int(sum(row['one_sided']
                                           for row in result['feature_reviews'])),
        'features_total': len(result['feature_rows']),
        'manual_mountain_features': int(sum(row['class_src'] == 'MANUAL_MOUNTAIN'
                                            for row in result['feature_reviews'])),
        'manual_plain_features': int(sum(row['class_src'] == 'MANUAL_PLAIN'
                                         for row in result['feature_reviews'])),
        'lake_excluded_features': int(sum(row['lake']
                                         for row in result['feature_reviews'])),
        'lake_protected_river_pixels': int(np.sum(result['lake_mask'] &
                                                  result['potential'] & valid)),
        'review_shapefile_feature_counts': review_counts,
        'max_applied_lowering_m': float(np.max(result['lowering'][valid])),
        'max_raise_if_exact_m': float(np.max(result['raising_if_exact'][valid])),
        'status_codes': {'0':'outside river', '1':'already at/below target',
                         '2':'lowered', '3':'flat reach without two reliable anchors',
                         '5':'overlapping features disagree; lower target applied',
                         '6':'mountain reach; original DEM retained',
                         '7':'lake cell or lake-dominant reach; original DEM retained',
                         '255':'DEM NoData'},
    }
    with open(path('summary.json'), 'w', encoding='utf-8') as f:
        json.dump(summary, f, ensure_ascii=False, indent=2)
    print('Output:', directory)
    print('Features with target:', summary['features_with_targets'], '/',
          summary['features_total'], '; modified pixels:',
          summary['modified_pixels'])
    if summary['large_cut_review_pixels']:
        print('REVIEW: lowering exceeds', cfg['review_large_cut_m'],
              'm in', summary['large_cut_review_pixels'], 'pixels.')
    if summary['large_raise_review_pixels']:
        print('REVIEW: exact interpolation raises more than',
              cfg['review_large_raise_m'], 'm in',
              summary['large_raise_review_pixels'], 'pixels.')


def prepare_output_directory(cfg):
    """Clear only the configured output folder, after inputs have been read."""
    directory = os.path.abspath(cfg['output_dir'])
    if os.path.islink(directory):
        raise ValueError('Output folder must not be a symlink: ' + directory)
    canonical = os.path.normcase(os.path.realpath(directory))
    if canonical == os.path.normcase(os.path.splitdrive(canonical)[0] + os.sep):
        raise ValueError('Refusing to clear a drive root.')
    for protected in (cfg['dem'], cfg['rivers'], cfg['lakes'], __file__):
        source = os.path.normcase(os.path.realpath(protected))
        try:
            if os.path.commonpath((canonical, source)) == canonical:
                raise ValueError('Output folder contains an input or script: '
                                 + directory)
        except ValueError as exc:
            if 'contains an input or script' in str(exc):
                raise
            # Different Windows drives have no common path.
    os.makedirs(directory, exist_ok=True)
    for name in os.listdir(directory):
        child = os.path.join(directory, name)
        if os.path.islink(child):
            raise ValueError('Refusing to clear output symlink: ' + child)
        resolved = os.path.normcase(os.path.realpath(child))
        if os.path.commonpath((canonical, resolved)) != canonical:
            raise ValueError('Output child resolves outside output folder: ' + child)
    # Windows GIS viewers commonly keep TIFFs open. Check *all* files before
    # removing any, so one locked file cannot leave a half-cleared directory.
    if os.name == 'nt':
        create_file = ctypes.windll.kernel32.CreateFileW
        create_file.argtypes = (ctypes.c_wchar_p, ctypes.c_uint32,
                                ctypes.c_uint32, ctypes.c_void_p,
                                ctypes.c_uint32, ctypes.c_uint32,
                                ctypes.c_void_p)
        create_file.restype = ctypes.c_void_p
        close_handle = ctypes.windll.kernel32.CloseHandle
        close_handle.argtypes = (ctypes.c_void_p,)
        for root, _, files in os.walk(directory):
            for name in files:
                candidate = os.path.join(root, name)
                handle = create_file(candidate, 0x80000000, 0, None, 3, 0, None)
                if handle == ctypes.c_void_p(-1).value:
                    raise PermissionError('Output file is open; close it in GIS '
                                          'or choose another output folder: '
                                          + candidate)
                close_handle(handle)
    for name in os.listdir(directory):
        child = os.path.join(directory, name)
        if os.path.isdir(child):
            shutil.rmtree(child)
        else:
            os.unlink(child)


def run(cfg):
    if not os.path.isfile(cfg['dem']):
        raise IOError('DEM does not exist: ' + cfg['dem'])
    if not os.path.isfile(cfg['rivers']):
        raise IOError('River SHP does not exist or its drive is unavailable: '
                      + cfg['rivers'])
    if not os.path.isfile(cfg['lakes']):
        raise IOError('Lake SHP does not exist: ' + cfg['lakes'])
    with rasterio.open(cfg['dem']) as src:
        if src.crs is None:
            raise ValueError('DEM CRS is missing.')
        crs = CRS.from_user_input(src.crs)
        transform, profile = src.transform, src.profile.copy()
        if transform.b or transform.d or transform.a <= 0 or transform.e >= 0:
            raise ValueError('North-up unrotated DEM required.')
        z = src.read(1).astype('float32')
        valid = (src.read_masks(1) > 0) & np.isfinite(z)
    lines, source_fids = read_lines_with_source_fids(cfg['rivers'], crs)
    if not lines:
        raise ValueError('No valid river line geometries.')
    river_crs, river_prj = read_river_projection(cfg['rivers'])
    lake_mask = read_lake_mask(cfg['lakes'], crs, transform, z.shape)
    print('Lake-protected DEM pixels:', int(np.sum(lake_mask & valid)))
    result = calculate(z, valid, transform, crs, lines, cfg, source_fids,
                       lake_mask)
    prepare_output_directory(cfg)
    export(result, profile, crs, valid, cfg, lines, river_crs, river_prj)
    return result


def self_test():
    from rasterio.transform import from_origin
    transform = from_origin(400000, 3500000, 30, 30)
    z = np.full((25, 45), 10.0, 'float32')
    z[12, 3:39] = 6.0
    z[12, 3] = 8.0             # bad source endpoint
    z[12, 38] = 8.0            # bad source endpoint
    z[12, 19:22] = 7.5         # raised interior cells
    z[12, 15] = 4.0            # existing deeper bed must not be raised
    line = RiverLine([transform * (3.5, 12.5),
                      transform * (38.5, 12.5)])
    cfg = CONFIG.copy()
    cfg['force_mountain_fids'] = []
    cfg['force_plain_fids'] = []
    result = calculate(z, np.ones(z.shape, bool), transform,
                       CRS.from_epsg(32650), [line], cfg)
    assert result['endpoint_rows'][0][7] < 6.5
    assert result['endpoint_rows'][1][7] < 6.5
    assert np.all(result['dem'][12, 19:22] < 7.5)
    assert result['dem'][12, 15] == 4.0
    assert result['status'][12, 19] == 2
    covered = result['potential']
    assert np.allclose(result['dem'][covered],
                       np.minimum(z[covered], result['target'][covered]))
    assert np.all(result['raising_if_exact'][covered] >= 0)
    joined = [RiverLine([(0, 0), (1, 0)]),
              RiverLine([(1, 0), (2, 0)])]
    claims = [None, dict(value=6.0), dict(value=7.5), None]
    _, values, states, _ = cluster_endpoints(
        joined, claims, Affine.identity(), cfg)
    assert values[1] == values[2] == 6.0
    assert states[1] == 'DISAGREEMENT_MIN'
    claims[2] = None
    _, values, states, _ = cluster_endpoints(
        joined, claims, Affine.identity(), cfg)
    assert values[1] == values[2] == 6.0
    assert states[2] == 'BORROWED'
    # A short feature without enough channel samples still gets a target on
    # every all-touched cell, using original DEM endpoint elevations.
    short = RiverLine([transform * (6.5, 8.5),
                       transform * (9.5, 8.5)])
    small = calculate(z, np.ones(z.shape, bool), transform,
                      CRS.from_epsg(32650), [short], cfg)
    assert small['feature_rows'][0][-1] == 'UNRESOLVED_ANCHORS'
    assert np.all(small['dem'] == z)
    graph = {1: [(2, 30.0, 0)], 2: [(1, 30.0, 0), (3, 40.0, 1)],
             3: [(2, 40.0, 1)]}
    assert nearest_anchor(2, 1, graph, {1: 5.0, 3: 4.0}) == (1, 30.0, 5.0)
    pit = np.full(z.shape, 10.0, 'float32')
    pit[12, 3] = 6.0
    pit_stations, pit_length = all_touched_stations(
        line, transform, z.shape, GridMetric(CRS.from_epsg(32650)))
    terminal_graph = {1: [(2, pit_length, 0)]}
    assert terminal_dem_anchor(
        1, terminal_graph, pit_stations, 0.0, 6.0, 7.0, pit,
        np.ones(z.shape, bool), np.zeros(z.shape, bool), 30.0,
        pit_length, cfg) == (1, 0.0, 6.0)
    assert terminal_dem_anchor(
        1, terminal_graph, pit_stations, 0.0, 10.0, 7.0, pit,
        np.ones(z.shape, bool), np.zeros(z.shape, bool), 30.0,
        pit_length, cfg) is None
    supported_nodes = [(1, 2, 1, 7.0, 'DIRECT', 7.0, 7.0),
                       (2, 1, 0, None, 'NO_EVIDENCE', None, None)]
    assert one_sided_flat_anchor(
        2, 1, (1, 0.0, 7.0), {2: [(1, pit_length, 0)]},
        {1: 7.0}, supported_nodes, 10.0, pit_length, cfg) == (2, 0.0, 7.0)
    assert one_sided_flat_anchor(
        2, 1, (1, 30.0, 7.0), {2: [(1, pit_length, 0)]},
        {1: 7.0}, supported_nodes, 10.0, pit_length, cfg) is None
    steep = z.copy()
    steep[8, 9] = 20.0
    mountain_test = calculate(steep, np.ones(z.shape, bool), transform,
                              CRS.from_epsg(32650), [short], cfg)
    assert mountain_test['feature_rows'][0][-1] == 'MOUNTAIN'
    assert np.all(mountain_test['dem'] == steep)
    override = cfg.copy()
    override['force_mountain_fids'] = (0,)
    forced_mountain = calculate(z, np.ones(z.shape, bool), transform,
                                CRS.from_epsg(32650), [line], override,
                                source_fids=[0])
    assert forced_mountain['feature_rows'][0][-1] == 'MOUNTAIN'
    assert forced_mountain['feature_reviews'][0]['class_src'] == 'MANUAL_MOUNTAIN'
    override['force_mountain_fids'] = ()
    override['force_plain_fids'] = (0,)
    forced_plain = calculate(steep, np.ones(z.shape, bool), transform,
                             CRS.from_epsg(32650), [short], override,
                             source_fids=[0])
    assert forced_plain['feature_rows'][0][-1] != 'MOUNTAIN'
    assert forced_plain['feature_reviews'][0]['class_src'] == 'MANUAL_PLAIN'
    lake_all = np.ones(z.shape, bool)
    for forced_key in ('force_mountain_fids', 'force_plain_fids'):
        lake_cfg = cfg.copy()
        lake_cfg[forced_key] = (0,)
        excluded = calculate(z, np.ones(z.shape, bool), transform,
                             CRS.from_epsg(32650), [line], lake_cfg,
                             source_fids=[0], lake_mask=lake_all)
        assert excluded['feature_rows'][0][-1] == 'LAKE'
        assert excluded['feature_reviews'][0]['class_src'] == 'LAKE'
        assert np.array_equal(excluded['dem'], z)
        assert np.all(excluded['status'][excluded['potential']] == 7)
    lake_part = np.zeros(z.shape, bool)
    lake_part[:, 19:22] = True
    partial = calculate(z, np.ones(z.shape, bool), transform,
                        CRS.from_epsg(32650), [line], cfg,
                        lake_mask=lake_part)
    assert partial['feature_rows'][0][-1] != 'LAKE'
    assert np.array_equal(partial['dem'][lake_part], z[lake_part])
    assert np.all(partial['status'][12, 19:22] == 7)
    lake_major = np.zeros(z.shape, bool)
    lake_major[:, :23] = True  # Most, but not all, of the line is in the lake.
    for forced_key in ('force_mountain_fids', 'force_plain_fids'):
        lake_cfg = cfg.copy()
        lake_cfg[forced_key] = (0,)
        excluded = calculate(z, np.ones(z.shape, bool), transform,
                             CRS.from_epsg(32650), [line], lake_cfg,
                             source_fids=[0], lake_mask=lake_major)
        assert excluded['feature_rows'][0][-1] == 'LAKE'
        assert 0.5 < excluded['feature_reviews'][0]['lake_frac'] < 1.0
        assert np.array_equal(excluded['dem'], z)
        assert np.all(excluded['status'][excluded['potential']] == 7)
    override['force_mountain_fids'] = (0,)
    excluded_conflict = calculate(z, np.ones(z.shape, bool), transform,
                                  CRS.from_epsg(32650), [line], override,
                                  source_fids=[0], lake_mask=lake_all)
    assert excluded_conflict['feature_rows'][0][-1] == 'LAKE'
    try:
        calculate(z, np.ones(z.shape, bool), transform,
                  CRS.from_epsg(32650), [line], override,
                  source_fids=[0])
    except ValueError as exc:
        assert 'both terrain types' in str(exc)
    else:
        raise AssertionError('Conflicting manual FIDs must fail.')
    override['force_plain_fids'] = ()
    override['force_mountain_fids'] = (2,)
    try:
        calculate(z, np.ones(z.shape, bool), transform,
                  CRS.from_epsg(32650), [line], override,
                  source_fids=[0])
    except ValueError as exc:
        assert 'not found' in str(exc)
    else:
        raise AssertionError('Unknown manual FIDs must fail.')
    with tempfile.TemporaryDirectory(prefix='burn_endpoint_test_') as temp:
        cfg['output_dir'] = os.path.join(temp, 'result')
        profile = dict(driver='GTiff', height=z.shape[0], width=z.shape[1],
                       count=1, dtype='float32', crs='EPSG:32650',
                       transform=transform, nodata=-9999.0)
        export(result, profile, CRS.from_epsg(32650),
               np.ones(z.shape, bool), cfg, [line])
        for filename in ('unreliable_endpoint_reaches.shp',
                         'large_raising_reaches.shp',
                         'large_lowering_reaches.shp',
                         'mountain_reaches.shp', 'plain_reaches.shp',
                         'lake_excluded_reaches.shp',
                         'terminal_anchor_reaches.shp',
                         'one_sided_flat_reaches.shp',
                         'cross_reach_interpolation.shp'):
            src = ogr.Open(os.path.join(cfg['output_dir'], filename))
            assert src is not None and src.GetLayer(0) is not None
            src = None
        with rasterio.open(os.path.join(cfg['output_dir'], 'dem_burn.tif')) as src:
            assert np.array_equal(src.read(1), result['dem'])
        with rasterio.open(os.path.join(cfg['output_dir'],
                                      'dem_interpolated_all_touched.tif')) as src:
            exact = src.read(1)
            assert np.allclose(exact[covered], result['target'][covered])
        cfg['output_dir'] = os.path.join(temp, 'short_result')
        export(small, profile, CRS.from_epsg(32650),
               np.ones(z.shape, bool), cfg, [short])
        src = ogr.Open(os.path.join(cfg['output_dir'],
                                    'unreliable_endpoint_reaches.shp'))
        assert src.GetLayer(0).GetFeatureCount() == 1
        src = None
        cfg['output_dir'] = os.path.join(temp, 'projection_result')
        vector_crs = CRS.from_epsg(4326)
        source_prj = vector_crs.to_wkt(version='WKT1_ESRI').encode('ascii')
        export(result, profile, CRS.from_epsg(32650),
               np.ones(z.shape, bool), cfg, [line], vector_crs, source_prj)
        review = os.path.join(cfg['output_dir'], 'plain_reaches.shp')
        with open(os.path.splitext(review)[0] + '.prj', 'rb') as stream:
            assert stream.read() == source_prj
        src = ogr.Open(review)
        layer = src.GetLayer(0)
        feature = layer.GetNextFeature()
        geometry = feature.GetGeometryRef().Clone()
        expected = Transformer.from_crs(32650, 4326, always_xy=True).transform(
            *line.coords[0])
        assert np.allclose(geometry.GetPoint(0)[:2], expected, atol=1e-8)
        feature = None
        layer = None
        src = None
        with rasterio.open(os.path.join(cfg['output_dir'], 'dem_burn.tif')) as src:
            assert src.crs.to_epsg() == 32650
            assert src.transform == transform
    print('SELF-TEST PASSED')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dem', default=CONFIG['dem'])
    parser.add_argument('--rivers', default=CONFIG['rivers'])
    parser.add_argument('--lakes', default=CONFIG['lakes'])
    parser.add_argument('--out', default=CONFIG['output_dir'])
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        config = CONFIG.copy()
        config.update(dem=args.dem, rivers=args.rivers, lakes=args.lakes,
                      output_dir=args.out)
        run(config)
