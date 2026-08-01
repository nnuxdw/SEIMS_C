#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Build the SWMM_DW node/link collections from a river-network shapefile.

The generated collections use the canonical names read by the SWMM_DW DLL:
``SWMM_NODES`` and ``SWMM_REACHES``.  Line endpoints are snapped to form
nodes, so the input network must already be noded: confluences and bifurcations
must be line endpoints rather than intersections in the middle of a line.

Typical use (the ``--replace`` switch is deliberately explicit)::

    python build_swmm_dw_network.py river.shp \
        --mongo-uri mongodb://127.0.0.1:27017 \
        --database taihu_model --replace

When no field mapping is supplied, common WISE/SEIMS and SWMM names are found
case-insensitively (for example ``LINKNO``, ``CH_LEN``, ``CH_N``, ``CH_WIDTH``
and ``CH_DEPTH``).  A different field name can be mapped without modifying the
script, for example::

    --field REACH_ID=ARC_ID --field FULL_DEPTH=BKF_DEPTH \
    --field FROM_INVERT=UP_ELEV --field TO_INVERT=DN_ELEV

The default cross section is a 10 m wide, 2 m deep trapezoidal open channel
with 2H:1V side slopes and Manning n=0.030.  These defaults make a usable
plain-river network but are modelling assumptions; replace them with surveyed
geometry where it is available.
"""
from __future__ import print_function

import argparse
import datetime as dt
import math
import os
import sys
from collections import defaultdict

try:
    from osgeo import ogr
except ImportError as exc:  # pragma: no cover - depends on the local GDAL installation.
    raise SystemExit("GDAL/OGR is required: %s" % exc)


NODE_COLLECTION = "SWMM_NODES"
REACH_COLLECTION = "SWMM_REACHES"

# Canonical SWMM_DW key -> accepted shapefile attributes.  Shapefile field
# names are limited to ten characters in many writers, hence the short aliases.
FIELD_ALIASES = {
    "REACH_ID": ("REACH_ID", "LINK_ID", "LINKNO", "ID", "ARC_ID"),
    "SUBBASINID": ("SUBBASINID", "SUBBASIN_ID", "SUBBASIN", "SUB_ID", "BASINID"),
    "FROM_NODE": ("FROM_NODE", "FROM_NODE_ID", "NODE1", "FROMID", "US_NODE", "USNODE"),
    "TO_NODE": ("TO_NODE", "TO_NODE_ID", "NODE2", "TOID", "DS_NODE", "DSNODE"),
    "DOWNSTREAM": ("DSLINKNO", "DOWNSTREAM", "DS_LINK", "TO_REACH", "DN_LINK"),
    "SHAPE": ("SHAPE", "SHAPE_CODE", "XSECT_TYPE", "XSECTTYP"),
    "BARRELS": ("BARRELS", "NUM_BARRELS", "N_BARRELS"),
    "LENGTH": ("LENGTH", "CH_LEN", "LENGTH_M", "LEN", "LEN_M"),
    "MANNING_N": ("MANNING_N", "CH_N", "ROUGHNESS", "MANNING", "N_VALUE"),
    "INLET_OFFSET": ("INLET_OFFSET", "OFFSET1", "IN_OFFSET", "IN_OFF"),
    "OUTLET_OFFSET": ("OUTLET_OFFSET", "OFFSET2", "OUT_OFFSET", "OUT_OFF"),
    "FULL_DEPTH": ("FULL_DEPTH", "CH_DEPTH", "DEPTH", "GEOM1", "DIAMETER"),
    "BOTTOM_WIDTH": ("BOTTOM_WIDTH", "CH_WIDTH", "WIDTH", "GEOM2", "BOT_WIDTH"),
    "SIDE_SLOPE": ("SIDE_SLOPE", "SIDE_SLOPES", "CH_SSLP", "GEOM3", "SIDE_SLP"),
    "INIT_FLOW": ("INIT_FLOW", "INITIAL_FLOW", "INIT_Q"),
    "MAX_FLOW": ("MAX_FLOW", "QMAX", "Q_LIMIT", "MAX_Q"),
    "INLET_LOSS": ("INLET_LOSS", "LOSS_INLET", "K_INLET"),
    "OUTLET_LOSS": ("OUTLET_LOSS", "LOSS_OUTLET", "K_OUTLET"),
    "AVERAGE_LOSS": ("AVERAGE_LOSS", "LOSS_AVERAGE", "K_AVERAGE"),
    # Elevation attributes are normally recorded at the two ends of a line.
    "FROM_INVERT": ("FROM_INVERT", "FROM_ELEV", "US_ELEV", "UP_ELEV", "START_ELEV", "BED_START"),
    "TO_INVERT": ("TO_INVERT", "TO_ELEV", "DS_ELEV", "DN_ELEV", "END_ELEV", "BED_END"),
    "INVERT_ELEV": ("INVERT_ELEV", "BED_ELEV", "BED_MEAN_E", "ELEVATION", "ELEV"),
    # On a line feature, an outfall flag/stage describes its downstream node.
    "NODE_TYPE": ("NODE_TYPE", "IS_OUTFALL", "OUTFALL", "OUTFALL_FL"),
    "BOUNDARY_STAGE": ("BOUNDARY_STAGE", "OUTFALL_STAGE", "TAILWATER", "STAGE"),
}

SHAPE_NAMES = {
    "OPEN_RECT": 0,
    "OPEN_RECTANGLE": 0,
    "RECT_OPEN": 0,
    "RECTANGULAR_OPEN": 0,
    "TRAPEZOID": 1,
    "TRAPEZOIDAL": 1,
    "TRAP": 1,
    "CIRCULAR": 2,
    "CIRCLE": 2,
    "CIRCULAR_CONDUIT": 2,
    "RECT_CLOSED": 3,
    "CLOSED_RECT": 3,
    "CLOSED_RECTANGLE": 3,
    "梯形": 1,
    "明渠矩形": 0,
    "圆形": 2,
    "圆管": 2,
    "封闭矩形": 3,
}


def parse_field_overrides(items):
    """Return canonical-key -> physical-field overrides from KEY=FIELD items."""
    overrides = {}
    for item in items:
        if "=" not in item:
            raise ValueError("--field must use KEY=FIELD, received %r" % item)
        key, field = item.split("=", 1)
        key = key.strip().upper()
        field = field.strip()
        if key not in FIELD_ALIASES:
            raise ValueError("Unsupported --field key %s" % key)
        if not field:
            raise ValueError("The field name for %s is empty" % key)
        overrides[key] = field
    return overrides


def resolve_fields(layer, overrides):
    """Resolve configured/candidate fields against a layer definition."""
    definition = layer.GetLayerDefn()
    existing = {}
    for index in range(definition.GetFieldCount()):
        name = definition.GetFieldDefn(index).GetNameRef()
        existing[name.upper()] = name

    resolved = {}
    for key, candidates in FIELD_ALIASES.items():
        if key in overrides:
            actual = existing.get(overrides[key].upper())
            if actual is None:
                raise ValueError("Mapped field %s=%s is not present in the shapefile" %
                                 (key, overrides[key]))
            resolved[key] = actual
            continue
        resolved[key] = None
        for candidate in candidates:
            if candidate.upper() in existing:
                resolved[key] = existing[candidate.upper()]
                break
    return resolved


def feature_value(feature, field_name):
    """Read an optional OGR attribute, treating null and blank strings as missing."""
    if field_name is None:
        return None
    index = feature.GetFieldIndex(field_name)
    if index < 0 or not feature.IsFieldSet(index):
        return None
    value = feature.GetField(index)
    if isinstance(value, str) and not value.strip():
        return None
    return value


def as_number(value, field_name):
    """Convert a supplied number and reject NaN/inf rather than silently defaulting it."""
    try:
        number = float(value)
    except (TypeError, ValueError):
        raise ValueError("%s must be numeric, received %r" % (field_name, value))
    if not math.isfinite(number):
        raise ValueError("%s must be finite, received %r" % (field_name, value))
    return number


def as_integer(value, field_name):
    """Convert an identifier/count/code while rejecting fractional values."""
    number = as_number(value, field_name)
    integer = int(round(number))
    if abs(number - integer) > 1.0e-9:
        raise ValueError("%s must be an integer, received %r" % (field_name, value))
    return integer


def optional_number(values, key, default, defaults_applied):
    """Use a source value if supplied; otherwise record that a model default was used."""
    value = values.get(key)
    if value is None:
        defaults_applied.append(key)
        return default
    return as_number(value, key)


def optional_integer(values, key, default, defaults_applied):
    value = values.get(key)
    if value is None:
        defaults_applied.append(key)
        return default
    return as_integer(value, key)


def shape_code(value, default, defaults_applied):
    """Parse numeric and descriptive section types into SWMM_DW's four codes."""
    if value is None:
        defaults_applied.append("SHAPE")
        return default
    if isinstance(value, str):
        name = value.strip().upper().replace("-", "_").replace(" ", "_")
        if name in SHAPE_NAMES:
            return SHAPE_NAMES[name]
    code = as_integer(value, "SHAPE")
    if code not in (0, 1, 2, 3):
        raise ValueError("SHAPE must be 0=open rectangle, 1=trapezoid, "
                         "2=circular, or 3=closed rectangle; received %r" % value)
    return code


def line_parts(geometry, source_fid):
    """Return non-empty LineString components from a line or multilinestring feature."""
    if geometry is None or geometry.IsEmpty():
        raise ValueError("Feature FID %s has no geometry" % source_fid)
    name = geometry.GetGeometryName().upper()
    if name == "LINESTRING":
        parts = [geometry]
    elif name == "MULTILINESTRING":
        parts = [geometry.GetGeometryRef(i) for i in range(geometry.GetGeometryCount())]
    else:
        raise ValueError("Feature FID %s has geometry type %s; a line is required" %
                         (source_fid, geometry.GetGeometryName()))
    valid_parts = []
    for part in parts:
        if part is not None and part.GetPointCount() >= 2 and part.Length() > 0.0:
            valid_parts.append(part.Clone())
    if not valid_parts:
        raise ValueError("Feature FID %s has no non-zero line component" % source_fid)
    return valid_parts


def endpoint(geometry, point_index):
    return float(geometry.GetX(point_index)), float(geometry.GetY(point_index))


def point_distance(left, right):
    return math.hypot(left[0] - right[0], left[1] - right[1])


class NodeRegistry(object):
    """Create unique node IDs by endpoint snapping, or preserve supplied IDs."""

    def __init__(self, snap_tolerance):
        if snap_tolerance < 0.0:
            raise ValueError("snap tolerance must be >= 0")
        self.tolerance = snap_tolerance
        self.nodes = {}
        self._buckets = defaultdict(list)
        self._next_id = 1
        self.coordinate_warnings = []

    def _bucket_key(self, coordinate):
        if self.tolerance == 0.0:
            return coordinate
        return (int(math.floor(coordinate[0] / self.tolerance)),
                int(math.floor(coordinate[1] / self.tolerance)))

    def _new_id(self):
        while self._next_id in self.nodes:
            self._next_id += 1
        value = self._next_id
        self._next_id += 1
        return value

    def _store(self, node_id, coordinate):
        self.nodes[node_id] = {"NODE_ID": node_id, "NODE_X": coordinate[0], "NODE_Y": coordinate[1]}
        self._buckets[self._bucket_key(coordinate)].append(node_id)
        return node_id

    def resolve(self, coordinate, explicit_id=None):
        """Return an existing snapped node or create one at the endpoint."""
        if explicit_id is not None:
            if explicit_id in self.nodes:
                existing = self.nodes[explicit_id]
                old_coordinate = (existing["NODE_X"], existing["NODE_Y"])
                if point_distance(old_coordinate, coordinate) > self.tolerance:
                    self.coordinate_warnings.append(
                        "NODE_ID %s is attached to endpoints farther than the snap tolerance" % explicit_id)
                return explicit_id
            return self._store(explicit_id, coordinate)

        key = self._bucket_key(coordinate)
        candidate_ids = []
        if self.tolerance == 0.0:
            candidate_ids.extend(self._buckets.get(key, []))
        else:
            for delta_x in (-1, 0, 1):
                for delta_y in (-1, 0, 1):
                    candidate_ids.extend(self._buckets.get((key[0] + delta_x, key[1] + delta_y), []))
        closest_id = None
        closest_distance = None
        for node_id in candidate_ids:
            node = self.nodes[node_id]
            distance = point_distance((node["NODE_X"], node["NODE_Y"]), coordinate)
            if distance <= self.tolerance and (closest_distance is None or distance < closest_distance):
                closest_id = node_id
                closest_distance = distance
        if closest_id is not None:
            return closest_id
        return self._store(self._new_id(), coordinate)


def read_raw_parts(shapefile, field_names, args):
    """Read each physical line part and retain canonical source values."""
    datasource = ogr.Open(shapefile, 0)
    if datasource is None:
        raise ValueError("Cannot open shapefile %s" % shapefile)
    layer = datasource.GetLayer(0)
    if layer is None:
        raise ValueError("Shapefile %s has no layer" % shapefile)

    srs = layer.GetSpatialRef()
    is_projected = bool(srs is not None and srs.IsProjected())
    use_explicit_nodes = field_names["FROM_NODE"] is not None or field_names["TO_NODE"] is not None
    if use_explicit_nodes and (field_names["FROM_NODE"] is None or field_names["TO_NODE"] is None):
        raise ValueError("Both FROM_NODE and TO_NODE fields are required when either one is supplied")
    if not is_projected and field_names["LENGTH"] is None and not args.allow_geographic_length:
        raise ValueError("The layer is not projected and has no LENGTH field. Reproject it or provide "
                         "--field LENGTH=<metres-field>; --allow-geographic-length is only for special cases.")
    if not is_projected and args.snap_tolerance > 0.0:
        print("WARNING: --snap-tolerance is expressed in geographic coordinate units because the layer is not projected.",
              file=sys.stderr)

    records = []
    registry = NodeRegistry(args.snap_tolerance)
    layer.ResetReading()
    feature = layer.GetNextFeature()
    while feature is not None:
        fid = feature.GetFID()
        values = {key: feature_value(feature, field_name) for key, field_name in field_names.items()}
        source_reach_id = None
        if values["REACH_ID"] is not None:
            source_reach_id = as_integer(values["REACH_ID"], "REACH_ID")
        parts = line_parts(feature.GetGeometryRef(), fid)
        for part_index, part in enumerate(parts):
            start = endpoint(part, 0)
            end = endpoint(part, part.GetPointCount() - 1)
            if use_explicit_nodes:
                start_node = registry.resolve(start, as_integer(values["FROM_NODE"], "FROM_NODE"))
                end_node = registry.resolve(end, as_integer(values["TO_NODE"], "TO_NODE"))
            else:
                start_node = registry.resolve(start)
                end_node = registry.resolve(end)
            if start_node == end_node:
                raise ValueError("Feature FID %s part %s has identical start/end nodes" % (fid, part_index))
            records.append({
                "SOURCE_FID": int(fid),
                "SOURCE_PART": part_index,
                "SOURCE_REACH_ID": source_reach_id,
                "VALUES": values,
                "GEOMETRY_LENGTH": float(part.Length()),
                "START_NODE": start_node,
                "END_NODE": end_node,
            })
        feature = layer.GetNextFeature()
    if not records:
        raise ValueError("No non-empty line features were found in %s" % shapefile)
    return records, registry, use_explicit_nodes


def assign_reach_ids(records):
    """Preserve unique source reach IDs; create deterministic IDs where that is impossible."""
    occurrence_count = defaultdict(int)
    for record in records:
        if record["SOURCE_REACH_ID"] is not None:
            occurrence_count[record["SOURCE_REACH_ID"]] += 1
    used = set()
    for record in records:
        source_id = record["SOURCE_REACH_ID"]
        if source_id is not None and occurrence_count[source_id] == 1 and source_id not in used:
            record["REACH_ID"] = source_id
            used.add(source_id)
    next_id = 1
    for record in records:
        if "REACH_ID" in record:
            continue
        while next_id in used:
            next_id += 1
        record["REACH_ID"] = next_id
        used.add(next_id)
        next_id += 1
    if len(used) != len(records):
        raise AssertionError("Failed to build unique reach identifiers")


def orient_records(records, use_explicit_nodes, field_names, reverse_direction):
    """Set FROM_NODE/TO_NODE from explicit topology, downstream links, or line direction."""
    if use_explicit_nodes:
        for record in records:
            record["FROM_NODE"] = record["START_NODE"]
            record["TO_NODE"] = record["END_NODE"]
        return

    has_downstream = field_names["DOWNSTREAM"] is not None
    source_index = defaultdict(list)
    for record in records:
        if record["SOURCE_REACH_ID"] is not None:
            source_index[record["SOURCE_REACH_ID"]].append(record)

    for record in records:
        start_node = record["START_NODE"]
        end_node = record["END_NODE"]
        record["FROM_NODE"] = end_node if reverse_direction else start_node
        record["TO_NODE"] = start_node if reverse_direction else end_node
        record["DOWNSTREAM_ID"] = None
        if not has_downstream:
            continue
        downstream_value = record["VALUES"]["DOWNSTREAM"]
        if downstream_value is None:
            continue
        downstream_id = as_integer(downstream_value, "DOWNSTREAM")
        record["DOWNSTREAM_ID"] = downstream_id
        if downstream_id <= 0:
            continue  # Terminal link: retain digitized direction unless upstream links resolve it below.
        targets = source_index.get(downstream_id, [])
        if len(targets) != 1:
            raise ValueError("Reach %s refers to DOWNSTREAM=%s, but that source reach does not map "
                             "to exactly one line part" % (record["REACH_ID"], downstream_id))
        target = targets[0]
        shared_nodes = set((start_node, end_node)).intersection((target["START_NODE"], target["END_NODE"]))
        if len(shared_nodes) != 1:
            raise ValueError("Reach %s and its DOWNSTREAM reach %s do not share exactly one snapped endpoint. "
                             "Check topology or increase --snap-tolerance." %
                             (record["REACH_ID"], downstream_id))
        record["TO_NODE"] = shared_nodes.pop()
        record["FROM_NODE"] = end_node if record["TO_NODE"] == start_node else start_node

    if not has_downstream:
        return
    # For terminal links, use the node where an upstream link joins the terminal
    # reach when this identifies its direction unambiguously.
    incoming_nodes = defaultdict(set)
    for record in records:
        downstream_id = record["DOWNSTREAM_ID"]
        if downstream_id is not None and downstream_id > 0:
            incoming_nodes[downstream_id].add(record["TO_NODE"])
    for record in records:
        downstream_id = record["DOWNSTREAM_ID"]
        if downstream_id is None or downstream_id > 0 or record["SOURCE_REACH_ID"] is None:
            continue
        candidates = incoming_nodes.get(record["SOURCE_REACH_ID"], set())
        endpoints = set((record["START_NODE"], record["END_NODE"]))
        candidates = candidates.intersection(endpoints)
        if len(candidates) == 1:
            record["FROM_NODE"] = candidates.pop()
            record["TO_NODE"] = (endpoints - {record["FROM_NODE"]}).pop()


def validate_positive(value, field_name, reach_id):
    if value <= 0.0:
        raise ValueError("Reach %s has %s=%s; it must be > 0" % (reach_id, field_name, value))
    return value


def validate_nonnegative(value, field_name, reach_id):
    if value < 0.0:
        raise ValueError("Reach %s has %s=%s; it must be >= 0" % (reach_id, field_name, value))
    return value


def make_reach_document(record, args):
    """Build one canonical SWMM_REACHES document, tracking all assumed values."""
    values = record["VALUES"]
    defaults_applied = []
    reach_id = record["REACH_ID"]
    shape = shape_code(values["SHAPE"], 1, defaults_applied)  # 1 = trapezoidal open channel.
    barrels = optional_integer(values, "BARRELS", 1, defaults_applied)
    if barrels < 1:
        raise ValueError("Reach %s has BARRELS=%s; it must be >= 1" % (reach_id, barrels))

    if values["LENGTH"] is None:
        length = record["GEOMETRY_LENGTH"]
        defaults_applied.append("LENGTH_FROM_GEOMETRY")
    else:
        length = as_number(values["LENGTH"], "LENGTH")
    length = validate_positive(length, "LENGTH", reach_id)

    manning_n = validate_positive(optional_number(values, "MANNING_N", args.default_manning_n, defaults_applied),
                                  "MANNING_N", reach_id)
    inlet_offset = optional_number(values, "INLET_OFFSET", 0.0, defaults_applied)
    outlet_offset = optional_number(values, "OUTLET_OFFSET", 0.0, defaults_applied)
    full_depth = validate_positive(optional_number(values, "FULL_DEPTH", args.default_full_depth, defaults_applied),
                                   "FULL_DEPTH", reach_id)
    if shape == 2:  # Circular: the dynamic-wave module uses FULL_DEPTH as diameter.
        bottom_width = full_depth
        if values["BOTTOM_WIDTH"] is None:
            defaults_applied.append("BOTTOM_WIDTH=FULL_DEPTH_FOR_CIRCULAR")
    else:
        bottom_width = validate_positive(optional_number(values, "BOTTOM_WIDTH", args.default_bottom_width,
                                                           defaults_applied), "BOTTOM_WIDTH", reach_id)
    side_default = args.default_side_slope if shape == 1 else 0.0
    side_slope = validate_nonnegative(optional_number(values, "SIDE_SLOPE", side_default, defaults_applied),
                                      "SIDE_SLOPE", reach_id)
    init_flow = optional_number(values, "INIT_FLOW", 0.0, defaults_applied)
    max_flow = validate_nonnegative(optional_number(values, "MAX_FLOW", 0.0, defaults_applied),
                                    "MAX_FLOW", reach_id)
    inlet_loss = validate_nonnegative(optional_number(values, "INLET_LOSS", 0.0, defaults_applied),
                                      "INLET_LOSS", reach_id)
    outlet_loss = validate_nonnegative(optional_number(values, "OUTLET_LOSS", 0.0, defaults_applied),
                                       "OUTLET_LOSS", reach_id)
    average_loss = validate_nonnegative(optional_number(values, "AVERAGE_LOSS", 0.0, defaults_applied),
                                        "AVERAGE_LOSS", reach_id)
    if values["SUBBASINID"] is None:
        subbasin_id = reach_id if args.default_subbasin_id is None else args.default_subbasin_id
        defaults_applied.append("SUBBASINID")
    else:
        subbasin_id = as_integer(values["SUBBASINID"], "SUBBASINID")
    if subbasin_id < 0:
        raise ValueError("Reach %s has SUBBASINID=%s; it must be >= 0" % (reach_id, subbasin_id))

    document = {
        "REACH_ID": reach_id,
        "SUBBASINID": subbasin_id,
        "FROM_NODE": record["FROM_NODE"],
        "TO_NODE": record["TO_NODE"],
        "SHAPE": shape,
        "BARRELS": barrels,
        "LENGTH": length,
        "MANNING_N": manning_n,
        "INLET_OFFSET": inlet_offset,
        "OUTLET_OFFSET": outlet_offset,
        "FULL_DEPTH": full_depth,
        "BOTTOM_WIDTH": bottom_width,
        "SIDE_SLOPE": side_slope,
        "INIT_FLOW": init_flow,
        "MAX_FLOW": max_flow,
        "INLET_LOSS": inlet_loss,
        "OUTLET_LOSS": outlet_loss,
        "AVERAGE_LOSS": average_loss,
        # Provenance fields are ignored by SWMM_DW but make defaulted records auditable.
        "SOURCE_FID": record["SOURCE_FID"],
        "SOURCE_PART": record["SOURCE_PART"],
        "SOURCE_REACH_ID": record["SOURCE_REACH_ID"],
        "DEFAULT_FIELDS": sorted(defaults_applied),
    }
    record["REACH_DOCUMENT"] = document
    return document


def supplied_invert(values, end_key, args):
    """Get an endpoint invert, using a reach-wide elevation only when necessary."""
    value = values[end_key]
    if value is None:
        value = values["INVERT_ELEV"]
    if value is None:
        return None
    return as_number(value, end_key)


def build_node_documents(records, registry, args):
    """Derive node elevations, bank depths, outfalls and one lateral-inflow index."""
    invert_candidates = defaultdict(list)
    crown_depth = defaultdict(float)
    outgoing_count = defaultdict(int)
    incoming_count = defaultdict(int)
    lateral_candidates = defaultdict(set)
    forced_outfalls = set()
    stage_candidates = defaultdict(list)

    for record in records:
        link = record["REACH_DOCUMENT"]
        values = record["VALUES"]
        from_node = link["FROM_NODE"]
        to_node = link["TO_NODE"]
        outgoing_count[from_node] += 1
        incoming_count[to_node] += 1
        from_invert = supplied_invert(values, "FROM_INVERT", args)
        to_invert = supplied_invert(values, "TO_INVERT", args)
        if from_invert is not None:
            invert_candidates[from_node].append(from_invert)
        if to_invert is not None:
            invert_candidates[to_node].append(to_invert)
        crown_depth[from_node] = max(crown_depth[from_node], link["INLET_OFFSET"] + link["FULL_DEPTH"])
        crown_depth[to_node] = max(crown_depth[to_node], link["OUTLET_OFFSET"] + link["FULL_DEPTH"])

        if args.node_subbasin_placement == "from":
            lateral_candidates[from_node].add(link["SUBBASINID"])
        elif args.node_subbasin_placement == "to":
            lateral_candidates[to_node].add(link["SUBBASINID"])

        node_type = values["NODE_TYPE"]
        if node_type is not None:
            node_type = as_integer(node_type, "NODE_TYPE")
            if node_type not in (0, 1):
                raise ValueError("NODE_TYPE must be 0 (junction) or 1 (fixed-stage outfall)")
            if node_type == 1:
                forced_outfalls.add(to_node)
        stage = values["BOUNDARY_STAGE"]
        if stage is not None:
            stage_candidates[to_node].append(as_number(stage, "BOUNDARY_STAGE"))

    node_documents = []
    lateral_conflicts = []
    for node_id in sorted(registry.nodes):
        node = registry.nodes[node_id]
        defaults_applied = ["INIT_DEPTH", "SURCHARGE_DEPTH", "PONDED_AREA"]
        if invert_candidates[node_id]:
            # At a junction the lowest connected invert is the physically
            # conservative common datum.  The source elevations are retained in
            # the shapefile; use explicit node data when that distinction matters.
            invert_elev = min(invert_candidates[node_id])
        else:
            invert_elev = args.default_invert_elev
            defaults_applied.append("INVERT_ELEV")
        max_depth = max(crown_depth[node_id], args.minimum_node_depth)
        defaults_applied.append("MAX_DEPTH_FROM_CONNECTED_REACHES")

        is_terminal = incoming_count[node_id] > 0 and outgoing_count[node_id] == 0
        is_outfall = node_id in forced_outfalls or is_terminal
        if is_outfall:
            if stage_candidates[node_id]:
                boundary_stage = min(stage_candidates[node_id])
            else:
                boundary_stage = invert_elev + args.default_outfall_stage_offset
                defaults_applied.append("BOUNDARY_STAGE")
        else:
            boundary_stage = invert_elev
            defaults_applied.append("BOUNDARY_STAGE_UNUSED")

        candidates = lateral_candidates[node_id]
        if len(candidates) == 1:
            subbasin_id = next(iter(candidates))
        elif len(candidates) == 0:
            subbasin_id = 0
            defaults_applied.append("SUBBASINID")
        else:
            # SWMM_DW currently accepts one SUBBASINID per node.  Do not choose
            # one arbitrarily: 0 makes the required manual mapping visible.
            subbasin_id = 0
            defaults_applied.append("SUBBASINID_CONFLICT")
            lateral_conflicts.append((node_id, sorted(candidates)))

        node_documents.append({
            "NODE_ID": node_id,
            "SUBBASINID": subbasin_id,
            "INVERT_ELEV": invert_elev,
            "MAX_DEPTH": max_depth,
            "INIT_DEPTH": 0.0,
            "SURCHARGE_DEPTH": 0.0,
            "PONDED_AREA": 0.0,
            "BOUNDARY_STAGE": boundary_stage,
            "NODE_TYPE": 1 if is_outfall else 0,
            # Coordinates/provenance are not required by the DLL but help QC.
            "NODE_X": node["NODE_X"],
            "NODE_Y": node["NODE_Y"],
            "DEFAULT_FIELDS": sorted(defaults_applied),
        })
    return node_documents, lateral_conflicts


def write_to_mongodb(node_documents, reach_documents, args):
    """Write only after validation; replacing non-empty collections needs --replace."""
    if args.dry_run:
        return
    try:
        from pymongo import ASCENDING, MongoClient
    except ImportError as exc:  # pragma: no cover - depends on the local environment.
        raise RuntimeError("pymongo is required to write MongoDB: %s" % exc)

    client = MongoClient(args.mongo_uri, serverSelectionTimeoutMS=10000)
    try:
        client.admin.command("ping")
        database = client[args.database]
        node_collection = database[NODE_COLLECTION]
        reach_collection = database[REACH_COLLECTION]
        node_exists = NODE_COLLECTION in database.list_collection_names()
        reach_exists = REACH_COLLECTION in database.list_collection_names()
        node_count = node_collection.count_documents({}) if node_exists else 0
        reach_count = reach_collection.count_documents({}) if reach_exists else 0
        if (node_count or reach_count) and not args.replace:
            raise RuntimeError("%s/%s already contain records. Re-run with --replace only after "
                               "checking the active model database." % (NODE_COLLECTION, REACH_COLLECTION))
        if args.replace:
            database.drop_collection(NODE_COLLECTION)
            database.drop_collection(REACH_COLLECTION)
            node_collection = database[NODE_COLLECTION]
            reach_collection = database[REACH_COLLECTION]
        node_collection.insert_many(node_documents, ordered=True)
        reach_collection.insert_many(reach_documents, ordered=True)
        node_collection.create_index([("NODE_ID", ASCENDING)], unique=True, name="node_id_unique")
        reach_collection.create_index([("REACH_ID", ASCENDING)], unique=True, name="reach_id_unique")
        reach_collection.create_index([("FROM_NODE", ASCENDING)], name="from_node_index")
        reach_collection.create_index([("TO_NODE", ASCENDING)], name="to_node_index")
    finally:
        client.close()


def make_parser():
    parser = argparse.ArgumentParser(
        description="Import a pre-noded river-line shapefile into SWMM_DW MongoDB collections.")
    parser.add_argument("shapefile", help="Input river network shapefile; endpoints must meet at all junctions.")
    parser.add_argument("--mongo-uri", default="mongodb://127.0.0.1:27017", help="MongoDB URI.")
    parser.add_argument("--database", required=True, help="Active WISE model database name.")
    parser.add_argument("--replace", action="store_true",
                        help="Replace SWMM_NODES and SWMM_REACHES if they already contain records.")
    parser.add_argument("--dry-run", action="store_true", help="Validate and report without writing MongoDB.")
    parser.add_argument("--field", action="append", default=[], metavar="KEY=FIELD",
                        help="Override a field alias; may be repeated (e.g. --field FULL_DEPTH=BKF_DEPTH).")
    parser.add_argument("--snap-tolerance", type=float, default=0.01,
                        help="Endpoint snap tolerance in layer CRS units; default: 0.01.")
    parser.add_argument("--reverse-direction", action="store_true",
                        help="Use end-to-start line direction only when FROM_NODE/TO_NODE and DOWNSTREAM are absent.")
    parser.add_argument("--allow-geographic-length", action="store_true",
                        help="Permit geometric LENGTH from an unprojected layer; normally supply a metre length field instead.")
    parser.add_argument("--default-manning-n", type=float, default=0.030,
                        help="Manning n used where the field is absent; default: 0.030.")
    parser.add_argument("--default-full-depth", type=float, default=2.0,
                        help="Bankfull depth [m] where absent; default: 2.0.")
    parser.add_argument("--default-bottom-width", type=float, default=10.0,
                        help="Trapezoid/rectangle bottom width [m] where absent; default: 10.0.")
    parser.add_argument("--default-side-slope", type=float, default=2.0,
                        help="One-bank H:V side slope for default trapezoids; default: 2.0.")
    parser.add_argument("--default-invert-elev", type=float, default=0.0,
                        help="Relative node invert [m] where no endpoint elevation exists; default: 0.0.")
    parser.add_argument("--minimum-node-depth", type=float, default=1.0e-6,
                        help="Lower bound for auto-derived MAX_DEPTH [m]; default: 1e-6.")
    parser.add_argument("--default-outfall-stage-offset", type=float, default=0.0,
                        help="Outfall stage minus invert [m] when no stage attribute exists; default: 0.0.")
    parser.add_argument("--default-subbasin-id", type=int, default=None,
                        help="SUBBASINID where absent; default uses the generated REACH_ID. Set 0 to disable this mapping.")
    parser.add_argument("--node-subbasin-placement", choices=("from", "to", "none"), default="from",
                        help="Attach a link SUBBASINID to its from node, to node, or neither; default: from.")
    return parser


def check_arguments(args):
    if not os.path.isfile(args.shapefile):
        raise ValueError("Input shapefile does not exist: %s" % args.shapefile)
    for name, value in (("default Manning n", args.default_manning_n),
                        ("default full depth", args.default_full_depth),
                        ("default bottom width", args.default_bottom_width),
                        ("minimum node depth", args.minimum_node_depth)):
        if value <= 0.0:
            raise ValueError("%s must be > 0" % name)
    if args.default_side_slope < 0.0:
        raise ValueError("default side slope must be >= 0")
    if args.default_subbasin_id is not None and args.default_subbasin_id < 0:
        raise ValueError("default subbasin ID must be >= 0")


def main(argv=None):
    parser = make_parser()
    args = parser.parse_args(argv)
    try:
        check_arguments(args)
        overrides = parse_field_overrides(args.field)
        datasource = ogr.Open(args.shapefile, 0)
        if datasource is None:
            raise ValueError("Cannot open shapefile %s" % args.shapefile)
        layer = datasource.GetLayer(0)
        field_names = resolve_fields(layer, overrides)
        datasource = None

        records, registry, use_explicit_nodes = read_raw_parts(args.shapefile, field_names, args)
        assign_reach_ids(records)
        orient_records(records, use_explicit_nodes, field_names, args.reverse_direction)
        reach_documents = [make_reach_document(record, args) for record in records]
        node_documents, lateral_conflicts = build_node_documents(records, registry, args)

        stamp = dt.datetime.utcnow().replace(microsecond=0).isoformat() + "Z"
        for document in node_documents:
            document["GENERATED_AT"] = stamp
            document["SOURCE_SHP"] = os.path.abspath(args.shapefile)
        for document in reach_documents:
            document["GENERATED_AT"] = stamp
            document["SOURCE_SHP"] = os.path.abspath(args.shapefile)
        write_to_mongodb(node_documents, reach_documents, args)

        mode = "DRY RUN; no records written" if args.dry_run else "written to MongoDB"
        print("SWMM_DW network %s: %d nodes, %d reaches." %
              (mode, len(node_documents), len(reach_documents)))
        print("Collections: %s, %s; topology: %s." %
              (NODE_COLLECTION, REACH_COLLECTION,
               "FROM_NODE/TO_NODE fields" if use_explicit_nodes else
               ("DOWNSTREAM field" if field_names["DOWNSTREAM"] else "line digitization direction")))
        if registry.coordinate_warnings:
            print("WARNING: %d explicit-node coordinate mismatch(es); node IDs were kept as supplied." %
                  len(registry.coordinate_warnings), file=sys.stderr)
        if lateral_conflicts:
            print("WARNING: %d node(s) receive multiple candidate SUBBASINID values. Their generated "
                  "node SUBBASINID is 0 because SWMM_DW accepts one lateral-inflow index per node:" %
                  len(lateral_conflicts), file=sys.stderr)
            for node_id, candidates in lateral_conflicts[:20]:
                print("  NODE_ID %s: %s" % (node_id, candidates), file=sys.stderr)
            if len(lateral_conflicts) > 20:
                print("  ... %d more conflicts" % (len(lateral_conflicts) - 20), file=sys.stderr)
        return 0
    except (RuntimeError, ValueError) as exc:
        print("ERROR: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
