from collections import OrderedDict
import datetime
import os
import re
import glob
from datetime import datetime

import numpy as np
import geopandas as gpd
import rasterio
from rasterio import mask
from shapely.geometry import mapping
from shapely.geometry import box
from rasterio.warp import calculate_default_transform, reproject, Resampling
import shutil
from osgeo import gdal, osr
from concurrent.futures import ThreadPoolExecutor, as_completed,ProcessPoolExecutor
from functools import partial
import pandas as pd
from pymongo import MongoClient, UpdateOne, ASCENDING
from datetime import timezone
from pathlib import Path
from rasterio.mask import mask
from typing import List
from shapely.geometry import Polygon, MultiPolygon

gdal.UseExceptions()
def _to_month_start(dt_or_str):
    """把 'YYYY-MM' / 'YYYY-MM-DD' 或 datetime 转成当月1日 00:00:00 的 datetime"""
    if isinstance(dt_or_str, str):
        s = dt_or_str.strip()
        s = s[:7]  # 只取到月份
        dt = datetime.strptime(s, "%Y-%m")
        return dt.replace(day=1, hour=0, minute=0, second=0, microsecond=0)
    else:
        return dt_or_str.replace(day=1, hour=0, minute=0, second=0, microsecond=0)


def _month_iter(start_dt, end_dt):
    """闭区间 [start_dt, end_dt] 的逐月迭代器（datetime，仅看年月）"""
    y, m = start_dt.year, start_dt.month
    while (y < end_dt.year) or (y == end_dt.year and m <= end_dt.month):
        yield datetime(y, m, 1, 0, 0, 0)
        if m == 12:
            y, m = y + 1, 1
        else:
            m += 1



def load_monthly_tifs(root_dir,
                      subbasin_ids,
                      start, end,
                      band=1,
                      on_missing="warn",  # "warn" | "skip" | "raise"
                      rename='flood',
                      readonly=True):
    """
    从 root_dir 读取命名为 SUBBASINID_YYYYMM.tif 的月尺度栅格。

    返回：
        OrderedDict:
           key  = datetime(YYYY,MM,1,00:00:00)
           value= { subbasin_id(str): numpy.ndarray(H,W) }

    说明：
      - 本函数把影像读入内存；数据特别大时请改为分块流式或 memmap/shared_memory。
      - 假定同月不同分区的影像在投影/分辨率/尺寸上已一致。
    """
    root_dir = os.path.abspath(root_dir)
    start_dt = _to_month_start(start)
    end_dt   = _to_month_start(end)
    sub_ids = [str(s) for s in subbasin_ids]

    data_dict = OrderedDict()

    ref_shape = {}
    ref_dtype = {}

    for month_dt in _month_iter(start_dt, end_dt):
        yyyymm = "{:04d}{:02d}".format(month_dt.year, month_dt.month)
        per_month = {}

        for sid in sub_ids:
            tif_path = os.path.join(root_dir, rename+"_{}_{}.tif".format(sid, yyyymm))
            if not os.path.exists(tif_path):
                msg = "[load_monthly_tifs] missing file: {}".format(tif_path)
                if on_missing == "raise":
                    raise FileNotFoundError(msg)
                if on_missing == "warn":
                    print(msg)
                continue

            with rasterio.open(tif_path) as ds:
                arr = ds.read(band)  # 读取第 band 个波段为 numpy.ndarray

            if readonly:
                try:
                    arr.setflags(write=False)
                except Exception:
                    pass

            if ref_shape.get(sid) is None:
                ref_shape[sid] = arr.shape
            else:
                if arr.shape != ref_shape[sid]:
                    print("[load_monthly_tifs] shape mismatch: {} {} != {}, skip."
                          .format(tif_path, arr.shape, ref_shape[sid]))
                    continue

            if ref_dtype.get(sid) is None:
                ref_dtype[sid] = arr.dtype
            else:
                # dtype 不一致一般问题不大，这里仅提示
                if arr.dtype != ref_dtype[sid]:
                    print("[load_monthly_tifs] dtype mismatch: {} {} != {}"
                          .format(tif_path, arr.dtype, ref_dtype[sid]))

            per_month[sid] = arr

        if len(per_month) > 0:
            data_dict[yyyymm] = per_month

    return data_dict


def _same_srs(wkt1, wkt2):
    s1 = gdal.osr.SpatialReference()
    s2 = gdal.osr.SpatialReference()
    if not wkt1 or not wkt2:
        return False
    s1.ImportFromWkt(wkt1)
    s2.ImportFromWkt(wkt2)
    return bool(s1.IsSame(s2))

def _ref_bounds_and_size(ref_ds):
    gt = ref_ds.GetGeoTransform()
    w = ref_ds.RasterXSize
    h = ref_ds.RasterYSize
    xmin = gt[0]
    ymax = gt[3]
    xmax = xmin + w * gt[1]
    ymin = ymax + h * gt[5]
    # GDAL 期望 (minX, minY, maxX, maxY)
    return (min(xmin, xmax), min(ymin, ymax), max(xmin, xmax), max(ymin, ymax)), w, h

def _creation_opts_from_src(src_ds):
    """继承源GTiff的压缩/瓦片设置；若缺失给合理默认。"""
    meta = src_ds.GetMetadata('IMAGE_STRUCTURE') or {}
    comp  = meta.get('COMPRESSION', 'LZW')          # LZW/DEFLATE/ZSTD/...
    tiled = meta.get('TILED', 'YES')
    bx    = meta.get('BLOCKXSIZE')
    by    = meta.get('BLOCKYSIZE')

    b1 = src_ds.GetRasterBand(1)
    gdt_name = gdal.GetDataTypeName(b1.DataType)    # 'Int16'/'Float32'...
    is_float = gdt_name.startswith('Float')

    opts = [
        'COMPRESS={}'.format(comp),
        'TILED={}'.format('YES' if str(tiled).upper() == 'YES' else 'NO'),
        'BIGTIFF=IF_SAFER'
    ]
    if bx and by:
        opts += ['BLOCKXSIZE={}'.format(bx), 'BLOCKYSIZE={}'.format(by)]
    # 对可预测压缩设置 PREDICTOR：整数=2，浮点=3
    if comp.upper() in ('LZW', 'DEFLATE', 'ZSTD'):
        opts.append('PREDICTOR={}'.format('3' if is_float else '2'))
    return opts

def batch_reproject_tif(source_dir, target_dir, ref_tif_path,
                        align_to_ref=False, resampling='nearest'):
    """
    将 source_dir 下所有 .tif 重投影到 ref_tif_path 的坐标系；
    若 align_to_ref=True，则对齐到参考影像的范围与像素网格。
    输出时继承源 TIF 的压缩/瓦片设置（与 clip 逻辑一致）。
    """
    os.makedirs(target_dir, exist_ok=True)

    ref_ds = gdal.Open(ref_tif_path, gdal.GA_ReadOnly)
    if ref_ds is None:
        raise RuntimeError("无法打开参考影像: {}".format(ref_tif_path))
    dst_wkt = ref_ds.GetProjection()
    if not dst_wkt:
        raise ValueError("参考影像缺少投影信息: {}".format(ref_tif_path))

    if align_to_ref:
        ref_bounds, ref_w, ref_h = _ref_bounds_and_size(ref_ds)

    resample_map = {
        'nearest': gdal.GRA_NearestNeighbour,
        'bilinear': gdal.GRA_Bilinear,
        'cubic': gdal.GRA_Cubic,
        'cubicspline': gdal.GRA_CubicSpline,
        'lanczos': gdal.GRA_Lanczos,
        'average': gdal.GRA_Average,
        'mode': gdal.GRA_Mode,
        'max': gdal.GRA_Max,
        'min': gdal.GRA_Min,
        'med': gdal.GRA_Med,
        'q1': gdal.GRA_Q1,
        'q3': gdal.GRA_Q3,
    }
    if resampling not in resample_map:
        raise ValueError("不支持的 resampling: {}".format(resampling))
    resample_alg = resample_map[resampling]

    for root, _, files in os.walk(source_dir):
        for fn in files:
            if not fn.lower().endswith('.tif'):
                continue
            src_path = os.path.join(root, fn)
            rel = os.path.relpath(root, source_dir)
            out_dir = os.path.join(target_dir, rel)
            os.makedirs(out_dir, exist_ok=True)
            dst_path = os.path.join(out_dir, fn)

            src_ds = gdal.Open(src_path, gdal.GA_ReadOnly)
            if src_ds is None:
                print("[skip] 打不开：{}".format(src_path))
                continue

            # 源数据类型/NoData（保持一致），以及压缩参数
            b1 = src_ds.GetRasterBand(1)
            src_gdt = b1.DataType
            dst_nodata = b1.GetNoDataValue()
            creation_opts = _creation_opts_from_src(src_ds)

            # 如果不需要对齐且 CRS 已一致，也不要直接 copy —— 仍然 warp 一次以保证压缩一致
            if align_to_ref:
                warp_opts = gdal.WarpOptions(
                    dstSRS=dst_wkt,
                    outputBounds=ref_bounds,
                    width=ref_w,
                    height=ref_h,
                    resampleAlg=resample_alg,
                    dstNodata=dst_nodata,
                    outputType=src_gdt,
                    creationOptions=creation_opts
                )
            else:
                warp_opts = gdal.WarpOptions(
                    dstSRS=dst_wkt,
                    resampleAlg=resample_alg,
                    dstNodata=dst_nodata,
                    outputType=src_gdt,
                    creationOptions=creation_opts
                )

            print("[proj] {} -> {} | align={} resampling={}".format(
                src_path, dst_path, align_to_ref, resampling))

            out_ds = gdal.Warp(destNameOrDestDS=dst_path,
                               srcDSOrSrcDSTab=src_ds,
                               options=warp_opts)
            if out_ds is None:
                raise RuntimeError("gdal.Warp 失败：{}".format(src_path))

            out_ds = None
            src_ds = None

    ref_ds = None


def batch_resample_tif(dem_path, source_dir, target_dir, geonodata=-9999, scale_factor=1.0):
    dem_nodata = geonodata
    with rasterio.open(dem_path) as dem_source:
        dem_nodata = dem_source.nodata
    # 打开DEM文件
    with rasterio.open(dem_path) as dem:
        dem_data = dem.read(1)
        dem_transform = dem.transform
        dem_crs = dem.crs
        dem_dtype = dem.dtypes[0]  # 获取DEM的数据类型

        # 创建DEM的边界框多边形
        dem_bounds = dem.bounds
        dem_geom = box(dem_bounds.left, dem_bounds.bottom, dem_bounds.right, dem_bounds.top)
        dem_gdf = gpd.GeoDataFrame({'geometry': [dem_geom]}, crs=dem_crs)

        # 遍历source_dir目录下的所有.tif文件
        for filename in os.listdir(source_dir):
            if filename.endswith('.tif'):
                source_path = os.path.join(source_dir, filename)
                target_path = os.path.join(target_dir, filename)

                # 确保目标文件夹存在
                os.makedirs(os.path.dirname(target_path), exist_ok=True)

                # 如果目标文件已存在，删除它
                if os.path.exists(target_path):
                    os.remove(target_path)

                # 打开源文件
                with rasterio.open(source_path) as source:
                    # 检查坐标系是否相同
                    if dem_crs != source.crs:
                        raise ValueError(f"CRS mismatch between DEM and source file: {filename}")

                    # 使用DEM的掩膜裁剪源文件
                    out_image, out_transform = mask.mask(source, dem_gdf.geometry, all_touched=True,crop=True, nodata=source.nodata)

                    # 将源文件中的nodata值转换为-9999
                    out_image = np.where(out_image == source.nodata, geonodata, out_image)

                    # 创建目标数组，使用合适的数据类型
                    destination = np.full((dem.height, dem.width), geonodata, dtype=dem_dtype)
                    # 应用缩放系数
                    destination = destination * scale_factor
                    # 重采样到DEM的分辨率
                    rasterio.warp.reproject(
                        source=out_image,
                        destination=destination,
                        src_transform=out_transform,
                        src_crs=source.crs,
                        dst_transform=dem_transform,
                        dst_crs=dem_crs,
                        resampling=Resampling.nearest
                    )

                    # 将DEM中的nodata像元掩膜到目标栅格
                    destination = np.where(dem_data == dem_nodata, geonodata, destination)

                    # 将处理后的数据写入新的TIFF文件
                    with rasterio.open(
                            target_path, 'w',
                            driver='GTiff',
                            height=dem.height,
                            width=dem.width,
                            count=1,
                            dtype=dem_dtype,
                            crs=dem_crs,
                            transform=dem_transform,
                            nodata=geonodata  # 设置nodata值为-9999
                    ) as target:
                        target.write(destination, 1)

def clip_tifs_batch_singlethread(input_dir, shp_path, output_dir, algrithem):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    tif_files = glob.glob(os.path.join(input_dir, "*.tif"))
    for tif_file in tif_files:
        output_tif = os.path.join(output_dir, os.path.basename(tif_file))
        if algrithem == 'gdal' or algrithem == 'GDAL':
            clip_raster_by_polygon_gdal(tif_file, shp_path, output_tif)
        else:
            clip_raster_by_polygon(tif_file, shp_path, output_tif)
        print(f"Cliped {tif_file} to {output_tif}")

def clip_raster_by_polygon(dem_tif_path, polygon_shp_path, output_tif_path):

    # 加载矢量数据
    gdf = gpd.read_file(polygon_shp_path)

    # 读取DEM数据
    with rasterio.open(dem_tif_path) as src:
        # 使用多边形作为掩模裁剪DEM
        # crop=True：该选项确保裁剪后的 TIF 文件范围仅限于矢量覆盖的区域。
        # all_touched=False：确保仅裁剪矢量覆盖的像素，避免包含多余的外部区域。
        out_image, out_transform = rasterio.mask.mask(src, gdf.geometry, crop=False)
        out_meta = src.meta.copy()

        # 更新元数据信息
        out_meta.update({
            "driver": "GTiff",
            "height": out_image.shape[1],
            "width": out_image.shape[2],
            "transform": out_transform
        })

        # 保存裁剪后的TIF文件
        with rasterio.open(output_tif_path, "w", **out_meta) as dest:
            dest.write(out_image)
            print(f"HRU裁剪DEM已完成，保存到: {output_tif_path}")


def clip_raster_by_polygon_gdal(
    dem_tif_path: str,
    polygon_shp_path: str,
    output_tif_path: str,
    crop_to_cutline: bool = True,   # True=只保留多边形范围
    keep_src_res: bool = True,      # True=保持像元大小
    resampling: str = "nearest"     # DEM/整数建议 'nearest'
):
    src = gdal.Open(dem_tif_path, gdal.GA_ReadOnly)
    if src is None:
        raise RuntimeError(f"无法打开栅格: {dem_tif_path}")

    # 源像元大小
    gt = src.GetGeoTransform()
    xres, yres = abs(gt[1]), abs(gt[5])

    # 源数据类型 & NoData
    b1 = src.GetRasterBand(1)
    src_gdt = b1.DataType           # 保持与原始一致（你的例子是 Int16）
    dst_nodata = b1.GetNoDataValue()

    # 选择重采样算法
    resample_map = {
        "nearest": gdal.GRA_NearestNeighbour, "bilinear": gdal.GRA_Bilinear,
        "cubic": gdal.GRA_Cubic, "cubicspline": gdal.GRA_CubicSpline,
        "lanczos": gdal.GRA_Lanczos
    }
    resample_alg = resample_map[resampling]

    # 压缩与瓦片：与原始一致（LZW），整数用 PREDICTOR=2
    creation_opts = [
        "COMPRESS=LZW",
        "TILED=YES",
        "BLOCKXSIZE=512",
        "BLOCKYSIZE=512",
        "PREDICTOR=2",       # 整数
        "BIGTIFF=IF_SAFER"
    ]

    warp_kwargs = dict(
        cutlineDSName=polygon_shp_path,
        cropToCutline=crop_to_cutline,
        resampleAlg=resample_alg,
        dstNodata=dst_nodata,
        outputType=src_gdt,              # 保持 16 位整数
        creationOptions=creation_opts
    )
    if keep_src_res:
        warp_kwargs.update(xRes=xres, yRes=yres)

    os.makedirs(os.path.dirname(output_tif_path) or ".", exist_ok=True)
    out = gdal.Warp(output_tif_path, src, options=gdal.WarpOptions(**warp_kwargs))
    if out is None:
        raise RuntimeError("gdal.Warp 执行失败。")
    out, src = None, None

def _extract_yyyymm(path_or_name):
    """从文件名里抽取 YYYYMM（优先匹配第一个 YYYY-MM-DD / YYYYMMDD，其次 YYYY-MM / YYYYMM）。"""
    base = os.path.basename(path_or_name)

    # 1) 优先：YYYY-MM-DD
    m = re.search(r'(\d{4})-(\d{2})-(\d{2})', base)
    if m:
        return "{}{}".format(m.group(1), m.group(2))

    # 2) 次优：YYYYMMDD（紧邻数字，不夹杂分隔符）
    m = re.search(r'(?<!\d)(\d{4})(\d{2})(\d{2})(?!\d)', base)
    if m:
        return "{}{}".format(m.group(1), m.group(2))

    # 3) 再试：YYYY-MM（避免把 YYYY-MM-DD 的前两段再匹配到）
    m = re.search(r'(\d{4})-(\d{2})(?!-\d)', base)
    if m:
        return "{}{}".format(m.group(1), m.group(2))

    # 4) 最后：YYYYMM
    m = re.search(r'(?<!\d)(\d{4})(\d{2})(?!\d)', base)
    if m:
        return "{}{}".format(m.group(1), m.group(2))

    return None

""" rename表示重命名的前缀，如果为None则不重命名 """
def _worker_clip_thread(tif_file, shp_path, output_dir, algorithm,rename=None):
    try:
        os.makedirs(output_dir, exist_ok=True)
        # 计算输出名：是否重命名为 flood_YYYYMM.tif
        if rename != None:
            shpfile_name = os.path.basename(shp_path)
            m = re.search(r'subbasin[_-]?(\d+)', shpfile_name, flags=re.IGNORECASE)
            if not m:
                print(f"无法从shp文件名解析subbasin id: {shpfile_name}")
            subbasin_id = int(m.group(1))

            yyyymm = _extract_yyyymm(tif_file)
            if yyyymm:
                out_name = rename + '_' + str(subbasin_id) +  "_{}.tif".format(yyyymm)
            else:
                # 没找到日期就退回原名
                out_name = os.path.basename(tif_file)
        else:
            out_name = os.path.basename(tif_file)


        output_tif = os.path.join(output_dir, out_name)

        # 已存在就跳过（避免同月多文件被覆盖）
        if os.path.exists(output_tif) and os.path.getsize(output_tif) > 0:
            return "[SKIP] exists: {}".format(output_tif)
        if os.path.exists(output_tif) and os.path.getsize(output_tif) > 0:
            return f"[SKIP] exists: {output_tif}"

        if algorithm.lower() == 'gdal':
            clip_raster_by_polygon_gdal(tif_file, shp_path, output_tif)
        else:
            clip_raster_by_polygon(tif_file, shp_path, output_tif)

        return f"[OK] {tif_file} -> {output_tif}"
    except Exception as e:
        return f"[ERR] {tif_file}: {e}"

def clip_tifs_batch_multithread(input_dir, shp_path, output_dir, algorithm, rename=None,max_workers=None):
    tif_files = glob.glob(os.path.join(input_dir, "*.tif"))
    if not tif_files:
        print("[WARN] no tif found")
        return

    if max_workers is None:
        # I/O 密集可适当放大线程数；不确定时 4~8 比较稳
        max_workers = 6

    worker = partial(_worker_clip_thread, shp_path=shp_path, output_dir=output_dir, algorithm=algorithm,rename=rename)

    print(f"[INFO] start (threaded) batch clipping: {len(tif_files)} files, workers={max_workers}")
    results = []
    with ThreadPoolExecutor(max_workers=max_workers) as ex:
        fut_map = {ex.submit(worker, tif): tif for tif in tif_files}
        for fut in as_completed(fut_map):
            msg = fut.result()
            results.append(msg)
            print(msg)

    ok = sum(m.startswith("[OK]") for m in results)
    skip = sum(m.startswith("[SKIP]") for m in results)
    err = sum(m.startswith("[ERR]") for m in results)
    print(f"[DONE] total={len(results)}, ok={ok}, skip={skip}, err={err}")

def _to_int_like(x, tol=1e-6):
    """把 1171.0 / '1171' 等转成 int；非整形语义返回 None。"""
    try:
        f = float(x)
        i = int(round(f))
        if abs(f - i) <= tol:
            return i
    except Exception:
        pass
    return None
def export_subbasins_as_shp(shp_path, id_field, subbasin_ids, out_dir, dissolve=False):
    """
    从总 shp 中按 id_field 选出 subbasin_ids，每个 ID 导出成独立 shp：
      out_dir/subbasin_<ID>.shp
    """
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    gdf = gpd.read_file(shp_path)
    if gdf is None or len(gdf) == 0:
        print("[ERROR] 读取不到要素：{}".format(shp_path))
        return

    gdf["_id_int"] = gdf[id_field].apply(_to_int_like)
    want_ids = [_to_int_like(x) for x in subbasin_ids]
    want_ids = [i for i in want_ids if i is not None]
    want_set = set(want_ids)

    # 修复几何，避免自交导致失败
    try:
        gdf["geometry"] = gdf["geometry"].buffer(0)
    except Exception:
        pass

    for sid in sorted(want_set):
        sub = gdf[gdf["_id_int"] == sid].copy()
        if sub is None or len(sub) == 0:
            print("[WARN] shp 中找不到 ID={} 的要素，跳过。".format(sid))
            continue
        if dissolve:
            try:
                sub = sub.dissolve(by="_id_int").reset_index()
            except Exception:
                # 某些几何异常时 dissolve 可能失败，降级不溶解
                pass

        out_path = os.path.join(out_dir, "subbasin_{}.shp".format(sid))
        try:
            sub.to_file(out_path, driver="ESRI Shapefile", encoding="utf-8")
            print("[OK] 导出 {}".format(out_path))
        except Exception as e:
            print("[ERROR] 写文件失败 ID={} -> {} : {}".format(sid, out_path, e))

def _find_subbasin_shp(sub_shp_dir, sid_int):
    """
    在目录下寻找分区 shp，支持两种命名：
      - subbasin_<ID>.shp
      - <ID>.shp
    返回存在的路径或 None。
    """
    cand1 = os.path.join(sub_shp_dir, "subbasin_{}.shp".format(sid_int))
    if os.path.exists(cand1):
        return cand1
    cand2 = os.path.join(sub_shp_dir, "{}.shp".format(sid_int))
    if os.path.exists(cand2):
        return cand2
    return None

def batch_clip_by_each_subbasin(tif_dir,
                                sub_shp_dir,
                                subbasin_ids,
                                out_root,
                                algorithm="gdal",
                                rename='flood',
                                max_workers=None):
    """
    遍历 sub_shp_dir 下各个分区的 shp（按 subbasin_ids 指定的 ID），
    逐个调用 clip_tifs_batch_multithread 对 tif_dir 里的所有 TIF 进行裁剪。
    每个 ID 的结果放在 out_root/<ID>/ 目录下。
    """
    if not os.path.isdir(out_root):
        os.makedirs(out_root)

    # 归一化 ID 列表
    id_list = []
    for x in subbasin_ids:
        v = _to_int_like(x)
        if v is not None:
            id_list.append(v)
        else:
            print("[WARN] 非整数语义的 ID 被忽略：{}".format(x))

    if not id_list:
        print("[ERROR] subbasin_ids 为空或无有效 ID")
        return

    print("[INFO] total subbasins to process:", id_list)

    for sid in id_list:
        shp_path = _find_subbasin_shp(sub_shp_dir, sid)
        if shp_path is None:
            print("[SKIP] 未找到分区 shp：ID={} 于 {}".format(sid, sub_shp_dir))
            continue

        out_dir = os.path.join(out_root,str(sid))
        if not os.path.isdir(out_dir):
            os.makedirs(out_dir)

        print("[INFO] clipping for subbasin {} using {}".format(sid, shp_path))
        # 调用你已有的多线程批量裁剪方法（对同一批 TIF，用同一个 shp）
        clip_tifs_batch_multithread(
            input_dir=tif_dir,
            shp_path=shp_path,
            output_dir=out_dir,
            algorithm=algorithm,
            rename=rename,
            max_workers=max_workers
        )



def calculate_inundation_area(folder, pixel_size=None, output_csv="flood_area.csv"):
    """
    计算每个subbasin每个月的淹没面积（像元值=1）

    参数:
    - folder: 存放 .tif 文件的目录
    - pixel_size: 单个像元的边长（米），如果不传，则从.tif元数据中获取
    - output_csv: 输出结果的csv文件路径
    """
    results = []
    files = sorted(glob.glob(os.path.join(folder, "**", "*.tif"), recursive=True))


    for f in files:
        with rasterio.open(f) as src:
            arr = src.read(1)  # 读取第一波段
            transform = src.transform
            # 如果用户没传 pixel_size，就从文件里获取分辨率
            if pixel_size is None:
                pixel_size = abs(transform[0])  # 假设像元是方形，取 x 分辨率
            pixel_area = pixel_size * pixel_size

            # 统计值为1的像元数量
            flooded_cells = np.sum(arr == 1)
            flooded_area = flooded_cells * pixel_area * 0.000001

            # 从文件名提取 subbasin 和时间信息
            fname = os.path.basename(f)
            # 例: flood_1171_201001.tif
            parts = fname.replace(".tif", "").split("_")
            subbasin = parts[1]
            yearmonth = parts[2]

            # 转换 yearmonth -> 时间戳
            # "201001" -> "2010-01-01 00:00:00"
            timestamp = pd.to_datetime(yearmonth, format="%Y%m") \
                           .strftime("%Y-%m-%d %H:%M:%S")

            results.append({
                "subbasin": subbasin,
                "time": timestamp,
                "flooded_area_km2": flooded_area
            })

    df = pd.DataFrame(results)
    df.to_csv(output_csv, index=False)
    return df


def load_flood_csv_to_mongo(
    csv_path: str,
    mongo_uri: str,
    db_name: str,
    collection_name: str = "MEASUREMENT",
    type_code: str = "F",
    batch_size: int = 1000,
    create_unique_index: bool = True,
):
    """
    将 calculate_inundation_area() 生成的 CSV 写入 MongoDB.MEASUREMENT

    CSV字段要求：
      - subbasin: 子流域ID（字符串或整数）
      - time: 'YYYY-MM-DD HH:MM:SS'（每月1日0点）
      - flooded_area_m2: 浮点数（淹没面积，单位 m^2）

    写入Mongo字段：
      - STATIONID      <- subbasin
      - VALUE          <- flooded_area_m2
      - TYPE           <- 传入的 type_code（如 "Q"/"A"/自定义）
      - LOCALDATETIME  <- 与 UTCDATETIME 一致（UTC）
      - UTCDATETIME    <- UTC（BSON datetime）
      - UTCOFFSET      <- 0
    """
    # 读取并规范化
    df = pd.read_csv(csv_path, dtype={"subbasin": str})
    # 解析为 pandas datetime，再转为 UTC tz-aware（Z）
    dt = pd.to_datetime(df["time"], format="%Y-%m-%d %H:%M:%S", errors="raise")
    df["dt_utc"] = dt.dt.tz_localize(timezone.utc)  # 带UTC时区

    # 建立连接
    client = MongoClient(mongo_uri)
    col = client[db_name][collection_name]

    # 可选：创建唯一索引，避免重复
    if create_unique_index:
        col.create_index(
            [("STATIONID", ASCENDING), ("UTCDATETIME", ASCENDING), ("TYPE", ASCENDING)],
            unique=True,
            name="uniq_station_time_type",
        )

    ops = []
    for _, row in df.iterrows():
        station_id = str(row["subbasin"]).strip()
        value = float(row["flooded_area_km2"])
        ts_utc = row["dt_utc"].to_pydatetime()  # tz-aware datetime，pymongo会保存为UTC BSON datetime

        # 以 (STATIONID, UTCDATETIME, TYPE) 作为唯一键进行 upsert
        ops.append(
            UpdateOne(
                {
                    "STATIONID": int(station_id),
                    "UTCDATETIME": ts_utc,
                    "TYPE": type_code,
                },
                {
                    "$set": {
                        "STATIONID": int(station_id),
                        "VALUE": value,
                        "TYPE": type_code,
                        "LOCALDATETIME": ts_utc,  # 与UTCDATETIME一致
                        "UTCDATETIME": ts_utc,
                        "UTCOFFSET": 0,
                    }
                },
                upsert=True,
            )
        )

        # 分批提交
        if len(ops) >= batch_size:
            col.bulk_write(ops, ordered=False)
            ops = []

    if ops:
        col.bulk_write(ops, ordered=False)

    client.close()
    print("写入完成。")

def read_shp_field(shp_path: str, field_name: str):
    """
    读取指定 shp 文件的某个字段，并以列表形式返回

    参数:
    - shp_path: shapefile 文件路径 (.shp)
    - field_name: 要提取的字段名

    返回:
    - values: list，字段值列表
    """
    gdf = gpd.read_file(shp_path)
    if field_name not in gdf.columns:
        raise ValueError(f"字段 '{field_name}' 不存在，shp字段包括: {list(gdf.columns)}")
    return gdf[field_name].tolist()

def process_subbasin(sbid, subbasin_flood_map_path, subbasin_flood_map_mollweide_path, mollweide_ref_path):
    clp_path = os.path.join(subbasin_flood_map_path, str(int(sbid)))
    proj_path = os.path.join(subbasin_flood_map_mollweide_path, str(int(sbid)))
    batch_reproject_tif(clp_path, proj_path, mollweide_ref_path)
    return sbid

def parallel_reproject(subbasin_ids, subbasin_flood_map_path, subbasin_flood_map_mollweide_path, mollweide_ref_path, max_workers=4):
    """
    并行执行每个 subbasin_id 的重投影
    """
    results = []
    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        future_to_sbid = {
            executor.submit(
                process_subbasin,
                sbid,
                subbasin_flood_map_path,
                subbasin_flood_map_mollweide_path,
                mollweide_ref_path
            ): sbid
            for sbid in subbasin_ids
        }
        for future in as_completed(future_to_sbid):
            sbid = future_to_sbid[future]
            try:
                result = future.result()
                results.append(result)
                print(f"Subbasin {sbid} 完成")
            except Exception as e:
                print(f"Subbasin {sbid} 出错: {e}")
    return results


def read_subbasin_mapping(txt_file: str):
    """
    读取指定txt文件，解析出HRU_ID和Subbasin的对应关系。

    参数:
    - txt_file: 指定的txt文件路径。

    返回:
    - 一个字典，键为HRU_ID，值为Subbasin。
    """
    # 读取txt文件
    df = pd.read_csv(txt_file, sep="\t")  # 使用\t分隔符读取
    # 创建HRU_ID到Subbasin的映射字典
    subbasin_mapping = dict(zip(df['HRU_ID'], df['Subbasin']))

    return subbasin_mapping


def calculate_hand_flood_status(
    hand_shp_path: str,
    flood_tif_dir: str,
    hand_subbasinid_map,
    output_csv: str,
    threshold: float = 0.5
):
    """
    计算每个hand的淹没状态，并将结果保存到指定的CSV文件。
    支持MultiPolygon类型的hand（多个多边形）。

    参数:
    - hand_shp_path: hand多边形shp文件路径。
    - flood_tif_dir: 包含子流域淹没范围tif文件的目录，子流域编号作为文件夹名称。
    - hand_subbasinid_map: 字典，用于根据hand的ID查找对应的子流域ID。
    - output_csv: 输出结果的CSV文件路径。
    - threshold: 淹没状态判断的比例阈值，默认50%。

    返回:
    - None: 将结果写入CSV文件。
    """
    # 读取hand的shp文件
    hand_gdf = gpd.read_file(hand_shp_path)

    flood_status_list = []
    hand_id_list = []
    subbasin_id_list = []

    # 遍历每个hand，多边形区域
    for idx, row in hand_gdf.iterrows():
        # 获取当前hand的geometry（可能是Polygon或MultiPolygon）
        hand_geometry = row['geometry']
        hand_id = row['FIELDID']
        subbasin_id = hand_subbasinid_map.get(hand_id)
        print(f"Calculating for hand ID {hand_id} subbasin {subbasin_id}...")

        # 定位对应的淹没tif文件路径
        flood_tif_path = Path(flood_tif_dir) / str(int(subbasin_id)) / f"flood_{str(int(subbasin_id))}_201001.tif"

        if not flood_tif_path.exists():
            print(f"Warning: {flood_tif_path} 不存在，跳过该子流域。")
            flood_status_list.append(0)  # 如果没有该文件，设置为0
            hand_id_list.append(hand_id)
            subbasin_id_list.append(subbasin_id)
            continue

        # 读取tif文件
        with rasterio.open(flood_tif_path) as src:
            # 获取栅格的NoData值
            nodata_value = src.nodata

            # 使用hand_geometry来掩膜tif，得到交集部分
            out_image, out_transform = mask(src, [hand_geometry], crop=True)
            out_image = out_image[0]  # 假设只处理一个波段

            # 如果有NoData值，则将NoData区域排除
            if nodata_value is not None:
                out_image = np.ma.masked_equal(out_image, nodata_value)  # 忽略NoData值

            # 统计hand区域内有效的栅格总数
            total_pixels = np.count_nonzero(~out_image.mask)  # 统计非NoData的像元

            # 计算淹没区域的像元数（像元值>0.9表示淹没区域）
            flooded_pixels = np.sum(out_image > 0.9)  # 可以根据需要调整阈值

            # 计算交集区域内淹没区域的比例
            flood_ratio = flooded_pixels / total_pixels if total_pixels > 0 else 0

            # 如果淹没区域的像元数占比大于给定比例阈值，则淹没状态为1，否则为0
            if flood_ratio > threshold:
                flood_status_list.append(flood_ratio)
            else:
                flood_status_list.append(0)

        hand_id_list.append(hand_id)
        subbasin_id_list.append(subbasin_id)

    # 创建DataFrame并保存为CSV
    result_df = pd.DataFrame({
        'Hand_ID': hand_id_list,
        'Subbasin': subbasin_id_list,
        'Flood_Status': flood_status_list
    })

    # 将结果写入CSV文件
    result_df.to_csv(output_csv, index=False)
    print(f"Results have been written to {output_csv}")


def calculate_flood_levels(input_txt: str, flood_status_csv: str, output_csv: str):
    """
    计算每个subbasin的淹没层级，淹没必须从下级到上级，
    如果下级未淹没，则上级即使淹没也不算淹没。如果一层也没淹没，设置淹没层级为-9999。

    参数：
    - input_txt: 包含每个subbasin、hand层级、Flood_Level、Depth的txt文件路径。
    - flood_status_csv: 包含每个subbasin的hand淹没状态的CSV文件路径。
    - output_csv: 输出最终结果的CSV文件路径。

    返回：
    - 每个subbasin最终淹没的hand层级（从下向上连续淹没的最上级）以及对应的hand_id。
    """
    # 读取输入的txt文件，假设文件是tab分隔的
    hand_df = pd.read_csv(input_txt, sep="\t")  # 如果是空格分隔，可改为sep="\s+"
    flood_status_df = pd.read_csv(flood_status_csv)

    # 创建一个字典来存储每个hand的Flood_Status（通过HAND_ID做映射）
    flood_status_map = dict(zip(flood_status_df['Hand_ID'], flood_status_df['Flood_Status']))

    # 创建一个字典来存储每个subbasin的淹没层级
    subbasin_flood_level = {}

    # 创建一个字典来存储每个subbasin的最终hand_id
    subbasin_hand_id = {}

    # 遍历每个subbasin
    for subbasin_id in hand_df['Subbasin'].unique():
        # 获取当前subbasin所有的hand层级
        subbasin_data = hand_df[hand_df['Subbasin'] == subbasin_id]

        # 确保按照hand的Flood_Level排序（从低到高）
        subbasin_data = subbasin_data.sort_values(by='Flood_Level')

        # 计算最终淹没层级（从下级到上级）
        final_flood_level = None
        final_hand_id = None
        is_previous_level_flooded = True  # 标记上级是否已经被淹没（初始值为True）

        for _, row in subbasin_data.iterrows():
            hand_id = row['HRU_ID']
            flood_level_range = row['Flood_Level']

            # 从字典中获取对应的淹没状态（通过map查找）
            hand_flood_status = flood_status_map.get(hand_id, 0)  # 默认未淹没（0）

            # 只有当前层级和所有下级都已经淹没，当前层级才算淹没
            if is_previous_level_flooded and hand_flood_status >= 0.5:
                final_flood_level = flood_level_range  # 记录最终的Flood_Level，而不是hand_id
                final_hand_id = hand_id  # 记录最终淹没层级对应的hand_id

            # 如果当前hand没有淹没，后面的hand也不算淹没
            if hand_flood_status < 0.5:
                is_previous_level_flooded = False  # 如果下级没有淹没，则上级也不算淹没

        # 如果没有任何层级淹没，将最终淹没层级和hand_id设为-9999
        if final_flood_level is None:
            final_flood_level = -9999
            final_hand_id = -9999

        # 存储每个subbasin最终淹没层级和对应的hand_id
        subbasin_flood_level[subbasin_id] = final_flood_level
        subbasin_hand_id[subbasin_id] = final_hand_id

    # 将结果存储到一个DataFrame并返回
    result_df = pd.DataFrame({
        'Subbasin': list(subbasin_flood_level.keys()),
        'Final_Flood_Level': list(subbasin_flood_level.values()),
        'Hand_ID': list(subbasin_hand_id.values())
    })

    # 将结果写入CSV文件
    result_df.to_csv(output_csv, index=False)
    print(f"Results have been written to {output_csv}")

    return result_df


def update_lake_hand_level_mongo(csv_file: str, db_config: dict):
    """
    读取CSV文件，更新MongoDB中REACHES集合的Lake_Hand_Level_Ini字段。

    参数：
    - csv_file: 包含Subbasin和Final_Flood_Level的CSV文件路径。
    - db_config: MongoDB连接配置字典，包含host, port, database等信息。
    """
    # 读取CSV文件
    df = pd.read_csv(csv_file)

    # 连接到MongoDB数据库
    client = MongoClient(db_config['host'], db_config['port'])
    db = client[db_config['database']]
    reaches_collection = db['REACHES']  # 假设集合名为REACHES

    # 遍历DataFrame，逐条更新数据库
    for _, row in df.iterrows():
        subbasin = row['Subbasin']
        final_flood_level = row['Final_Flood_Level']

        # 构建更新语句
        update_query = {'SUBBASINID': int(subbasin)}
        update_values = {'$set': {'Lake_Hand_Level_Ini': int(final_flood_level)}}

        # 执行更新操作
        result = reaches_collection.update_one(update_query, update_values)

        # 打印更新结果（可选）
        if result.matched_count > 0:
            print(f"Updated Subbasin {int(subbasin)} with Flood Level {int(final_flood_level)}")
        else:
            print(f"No matching subbasin {int(subbasin)} found, skipping.")

    # 关闭MongoDB连接
    client.close()

    print("Database update complete.")

if __name__ == '__main__':
    inundation_cali_path = r'J:\G\program\seims\SEIMS_HAND\data\poyang_lake\inundation_cali'
    # 1.先裁剪一次，减少计算量
    src_tif_path = r"J:\G\data\鄱阳湖数据\monthly_inundation_image\2010-2019"
    extent_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake\extent\extent_for_clip_proj.shp"
    cliped_all_path = os.path.join(inundation_cali_path,'cliped_all')
    # clip_tifs_batch_multithread(
    #     input_dir=src_tif_path,
    #     shp_path=extent_path,
    #     output_dir=cliped_all_path,
    #     algorithm='gdal',
    #     rename=None,
    #     max_workers=6
    # )
    # 2.投影到DEM
    proj_tif_path = os.path.join(inundation_cali_path, 'proj')
    dem_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_raster\dem.tif"
    # batch_reproject_tif(cliped_all_path,proj_tif_path,dem_path)
    # 3.重采样到DEM
    rsp_tif_path = os.path.join(inundation_cali_path, 'resample')
    # batch_resample_tif(dem_path,proj_tif_path,rsp_tif_path)
    # 4.导出指定subbasin
    subbasin_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_shp\subbasin.shp"
    # subbasin_ids = [1171, 1176, 1193, 1194, 1214]
    subbasin_ids = read_shp_field(subbasin_path,"SUBBASINID")
    extract_subbasins_path = os.path.join(inundation_cali_path, 'subbasins_shp')
    # export_subbasins_as_shp(subbasin_path,"SUBBASINID",subbasin_ids,extract_subbasins_path,False)
    # 5. 使用指定subbasin的shp裁剪龙笛老师的逐月tif
    subbasin_flood_map_path = os.path.join(inundation_cali_path, 'subbasin_flood')
    # batch_clip_by_each_subbasin(
    #     tif_dir=rsp_tif_path,
    #     sub_shp_dir=extract_subbasins_path,  # 里面存放 subbasin_1171.shp 等
    #     subbasin_ids=subbasin_ids,
    #     out_root=subbasin_flood_map_path,
    #     algorithm="gdal",  # 按你 _worker_clip_thread 的实现填写
    #     rename="flood",
    #     max_workers=12
    # )

    # 6 如果用每个子流域逐月的淹没面积率定，用该方法
    # 6.1 先重投影到Mollweide
    subbasin_flood_map_mollweide_path = os.path.join(inundation_cali_path, 'subbasin_flood_mollweide')
    mollweide_ref_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\spatial_raster\dem_mollweide.tif"
    # parallel_reproject(
    #     subbasin_ids,
    #     subbasin_flood_map_path,
    #     subbasin_flood_map_mollweide_path,
    #     mollweide_ref_path,
    #     max_workers=12  # 根据机器 CPU 核数调整
    # )
    # for sbid in subbasin_ids:
    #     clp_path = os.path.join(subbasin_flood_map_path, str(sbid))
    #     proj_path = os.path.join(subbasin_flood_map_mollweide_path, str(sbid))
    #     batch_reproject_tif(clp_path, proj_path, mollweide_ref_path)
    # 6.2 计算每个subbasin每个月的淹没面积，并写入MEASUREMENT表
    csv_path = os.path.join(inundation_cali_path, 'subbasin_flood_area.csv')
    # df = calculate_inundation_area(subbasin_flood_map_mollweide_path,None, csv_path)
    mongo_uri = "mongodb://172.21.124.127:27019"
    db_name = "poyang_lake1_HydroClimate"
    collection_name = "MEASUREMENT"
    load_flood_csv_to_mongo(
        csv_path=csv_path,
        mongo_uri=mongo_uri,
        db_name=db_name,
        collection_name=collection_name,
        type_code="F",      # 你想写入的TYPE
    )

    # 6 如果用每个子流域逐月的淹没范围率定，用该方法
    # 6.1 加载每个子流域逐月淹没的tif,时间为闭区间
    # data_dict = load_monthly_tifs(
    #     root_dir=subbasin_flood_map_path,
    #     subbasin_ids=[1171,1176,1193,1194,1214],
    #     start="2010-01",
    #     end="2010-02",
    #     band=1,
    #     on_missing="warn",
    #     rename='flood',
    #     readonly=True
    # )

    # 7 计算每个subbasin初始水位所在的hand层级
    hand_shp_path = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\workspace\HRU_file\HRU_dissolved.shp"
    flood_tif_dir = os.path.join(inundation_cali_path, 'subbasin_flood')
    FloodStep_dir = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\rundata\FloodStep.txt"
    hand_initial_flood_csv = os.path.join(inundation_cali_path, 'hand_initial_flood.csv')
    threshold = 0.5
    hand_subbasinid_map = read_subbasin_mapping(FloodStep_dir)
    flood_status = calculate_hand_flood_status(hand_shp_path, flood_tif_dir, hand_subbasinid_map, hand_initial_flood_csv, threshold)

    # 计算最终的淹没层级并保存结果
    subbasin_initial_flood = os.path.join(inundation_cali_path, 'subbasin_initial_flood.csv')
    calculate_flood_levels(FloodStep_dir, hand_initial_flood_csv, subbasin_initial_flood)

    # 更新REACHES表的Lake_Hand_Level_Ini字段
    db_config = {
        'host': '127.0.0.1',  # MongoDB的主机
        'port': 27017,  # MongoDB的端口
        'database': 'poyang_lake1_longterm_model'  # MongoDB的数据库名
    }

    # 调用方法更新数据库
    update_lake_hand_level_mongo(subbasin_initial_flood, db_config)
