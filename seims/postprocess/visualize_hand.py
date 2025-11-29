
import numpy as np
import rasterio
import os
from PIL import Image, ImageDraw, ImageFont
import matplotlib.pyplot as plt
import re
import matplotlib.cm as cm
from matplotlib.colors import LinearSegmentedColormap
import pandas as pd
import geopandas as gpd
import os
from pymongo import MongoClient
from datetime import datetime
from concurrent.futures import ThreadPoolExecutor, as_completed
from rasterio.enums import Resampling
from pathlib import Path


def build_stats(tif_path):
    with rasterio.open(tif_path, 'r+') as ds:
        band = ds.read(1, resampling=Resampling.nearest)
        mask = band != ds.nodata if ds.nodata is not None else np.ones_like(band, dtype=bool)
        stats = {
            'min': float(band[mask].min()),
            'max': float(band[mask].max()),
            'mean': float(band[mask].mean()),
            'std': float(band[mask].std())
        }
        ds.update_tags(1, STATISTICS_MINIMUM=str(stats['min']),
                          STATISTICS_MAXIMUM=str(stats['max']),
                          STATISTICS_MEAN=str(stats['mean']),
                          STATISTICS_STDDEV=str(stats['std']))
    print("成功构建统计信息")

def gen_hand_tif_by_txt_fast(txt_path, input_tif_path, output_tif_path):
    # 1) 读映射
    keys = []
    vals = []
    with open(txt_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            k, v = line.split(',', 1)
            keys.append(int(k))
            vals.append(float(v))
    keys = np.asarray(keys, dtype=np.int64)
    vals = np.asarray(vals, dtype=np.float32)

    max_id = int(keys.max()) if len(keys) else -1
    lut = np.full(max_id + 1, np.nan, dtype=np.float32)
    lut[keys] = vals

    # 2) 读栅格
    with rasterio.open(input_tif_path) as src:
        arr = src.read(1)
        profile = src.profile.copy()
        nodata = src.nodata

    # 2.1) nodata 掩膜（兼容 NaN）
    if nodata is None:
        is_nd = np.zeros_like(arr, dtype=bool)
    elif isinstance(nodata, float) and np.isnan(nodata):
        is_nd = np.isnan(arr)
    else:
        is_nd = (arr == nodata)

    # 2.2) 如果 hand_id 是浮点，确保是“整数值的浮点”（如 3.0）
    if np.issubdtype(arr.dtype, np.floating):
        # 对非 nodata 像元检查是否为整数值
        frac = np.modf(arr[~is_nd])[0]
        if not np.all(np.isclose(frac, 0.0, atol=1e-6)):
            # 出现了 3.7 这种非整数 hand_id —— 会映射不到
            # 这里直接取整（更安全是报错让你检查数据源）
            arr_rounded = np.rint(arr).astype(np.int64)
        else:
            arr_rounded = arr.astype(np.int64, copy=False)
    else:
        arr_rounded = arr.astype(np.int64, copy=False)

    # 3) 查表映射
    out = np.empty_like(arr, dtype=np.float32)
    out[:] = np.nan

    valid = (~is_nd) & (arr_rounded >= 0)
    idx = arr_rounded[valid]

    in_range = (idx <= max_id)
    tmp = np.full(idx.shape, np.nan, dtype=np.float32)
    tmp[in_range] = lut[idx[in_range]]
    out[valid] = tmp

    # 4) 处理输出 nodata / 写出
    if nodata is None or (isinstance(nodata, float) and np.isnan(nodata)):
        nodata_out = -9999.0
        profile.update(nodata=nodata_out)
    else:
        nodata_out = float(nodata)

    profile.update(dtype='float32', tiled=True, compress='DEFLATE', predictor=2, num_threads='all_cpus')

    out[np.isnan(out)] = nodata_out
    with rasterio.open(output_tif_path, 'w', **profile) as dst:
        dst.write(out, 1)

    try:
        build_stats(output_tif_path)
    except NameError:
        pass
    print(f"成功生成输出文件：{output_tif_path}")

def gen_hand_tif_by_txt(txt_path, input_tif_path, output_tif_path):
    # 1. 读取 hand_id -> value 映射表
    id_to_value = {}
    with open(txt_path, 'r') as f:
        for line in f:
            parts = line.strip().split(',')
            if len(parts) == 2:
                hand_id = int(parts[0].strip())
                value = float(parts[1].strip())
                id_to_value[hand_id] = value

    # 2. 打开原始 TIF 文件
    with rasterio.open(input_tif_path) as src:
        input_array = src.read(1)
        profile = src.profile.copy()  # 完整复制 profile
        nodata = src.nodata

    # 3. 创建输出数组（默认保留 nodata 值）
    output_array = np.full_like(input_array, nodata if nodata is not None else 0, dtype=np.float32)

    # 4. 替换 hand_id 为对应的值
    for hand_id, value in id_to_value.items():
        output_array[input_array == hand_id] = value

    # 5. 更新 profile 的数据类型为 float32（避免精度丢失）
    profile.update(dtype=rasterio.float32)

    # 6. 写出输出 TIF 文件，保持元数据不变
    with rasterio.open(output_tif_path, 'w', **profile) as dst:
        dst.write(output_array, 1)
    build_stats(output_tif_path)
    print(f"成功生成输出文件：{output_tif_path}")

def get_files_by_prefix_suffix(directory, prefix='', suffix=''):
    """
    获取指定目录下，所有以 prefix 开头、suffix 结尾的文件名列表。

    :param directory: 目录路径
    :param prefix: 文件名前缀（可选）
    :param suffix: 文件名后缀（如 .tif, .txt，可选）
    :return: 符合条件的文件名列表（包含完整路径）
    """
    matched_files = []
    for filename in os.listdir(directory):
        if filename.startswith(prefix) and filename.endswith(suffix):
            matched_files.append(os.path.join(directory, filename))
    return matched_files

def replace_txt_with_tif(path):
    base, ext = os.path.splitext(path)
    return base + '.tif'

def replace_txt_with_shp(path):
    base, ext = os.path.splitext(path)
    return base + '.shp'

def create_gif_by_tif(image_files, years, output_file, dem_path, colormap, nodata_value=None):
    from PIL import ImageDraw, ImageFont, ImageEnhance

    frames = []
    dem_image = load_and_colorize_dem(dem_path, colormap=colormap, debug=False)

    for image_file, year in zip(image_files, years):
        im = Image.open(image_file).convert("L")
        image_array = np.array(im)
        image_array, nodata_mask = linear_stretch(image_array, nodata_value=nodata_value, ignore_zero=True)

        im_colored = apply_color_map(image_array, nodata_mask=nodata_mask)

        if im_colored.size != dem_image.size:
            im_colored = im_colored.resize(dem_image.size)

        # 合成 DEM + 水深图
        combined_im = Image.alpha_composite(dem_image, im_colored)

        # 叠加到白色背景（防止透明变黑）
        white_bg = Image.new("RGBA", combined_im.size, (255, 255, 255, 255))
        final_im = Image.alpha_composite(white_bg, combined_im)

        # 添加时间戳
        draw = ImageDraw.Draw(final_im)
        try:
            font = ImageFont.truetype("arial.ttf", 80)
        except:
            font = ImageFont.load_default()

        width, height = final_im.size
        text_position = (width - 800, height - 120)
        draw.text((text_position[0]+1, text_position[1]+1), year, fill="black", font=font)
        draw.text(text_position, year, fill="black", font=font)

        # frames.append(final_im.convert("P", palette=Image.ADAPTIVE))
        frames.append(final_im)

    print("GIF 已保存:", output_file)
    frames[0].save(output_file, save_all=True, append_images=frames[1:], duration=100, loop=0, optimize=True)


def render_chwtrdepth_on_image(base_img, shp_path, field="CHWTRDEPTH",
                               transform=None, crs=None,
                               color_map="Blues", vmin=0.0, vmax=1.0):
    import geopandas as gpd
    from matplotlib import cm
    from matplotlib.colors import Normalize

    # 读取 shapefile
    gdf = gpd.read_file(shp_path)
    if crs and gdf.crs != crs:
        gdf = gdf.to_crs(crs)

    # 获取 colormap 映射器
    cmap = cm.get_cmap(color_map)
    norm = Normalize(vmin=vmin, vmax=vmax)

    draw = ImageDraw.Draw(base_img)

    for _, row in gdf.iterrows():
        value = row.get(field)
        if value is None or np.isnan(value):
            continue

        color = tuple((np.array(cmap(norm(value))[:3]) * 255).astype(int))
        geom = row.geometry

        if geom is None:
            continue

        # 统一处理 MultiLineString 和 LineString
        lines = geom.geoms if geom.type == "MultiLineString" else [geom]
        for line in lines:
            coords = list(line.coords)
            px_coords = [~transform * (x, y) for x, y in coords]
            px_coords = [(int(px), int(py)) for px, py in px_coords]
            if len(px_coords) >= 2:
                draw.line(px_coords, fill=color, width=2)

    return base_img


def create_gif_by_tif_with_shp_bak(image_files, years, output_file, dem_path, colormap,
                                nodata_value=None, shapefile_path=None, river_shapefile=None):
    from PIL import ImageDraw, ImageFont, Image
    import numpy as np
    import rasterio
    import geopandas as gpd

    frames = []

    # --- 读取 DEM 图像和地理信息 ---
    with rasterio.open(dem_path) as dem_src:
        dem_image = load_and_colorize_dem(dem_path, colormap=colormap, debug=False)
        transform = dem_src.transform
        dem_crs = dem_src.crs
        dem_width, dem_height = dem_src.width, dem_src.height

    # --- 读取行政区划边界 shapefile ---
    boundaries = []
    if shapefile_path:
        gdf = gpd.read_file(shapefile_path)
        if gdf.crs != dem_crs:
            gdf = gdf.to_crs(dem_crs)
        for geom in gdf.geometry:
            if geom is not None:
                coords_list = []
                if geom.type in ["Polygon", "MultiPolygon"]:
                    for part in geom.geoms if geom.type == "MultiPolygon" else [geom]:
                        coords = list(part.exterior.coords)
                        pixel_coords = [~transform * (x, y) for x, y in coords]
                        pixel_coords = [(int(px), int(py)) for px, py in pixel_coords]
                        coords_list.append(pixel_coords)
                if coords_list:
                    boundaries.extend(coords_list)

    # --- 读取河道 shapefile，转为像素坐标 ---
    river_lines = []
    if river_shapefile:
        r_gdf = gpd.read_file(river_shapefile)
        if r_gdf.crs != dem_crs:
            r_gdf = r_gdf.to_crs(dem_crs)
        for geom in r_gdf.geometry:
            if geom is not None:
                if geom.type == "LineString":
                    coords = list(geom.coords)
                    pixel_coords = [~transform * (x, y) for x, y in coords]
                    pixel_coords = [(int(px), int(py)) for px, py in pixel_coords]
                    river_lines.append(pixel_coords)
                elif geom.type == "MultiLineString":
                    for line in geom.geoms:
                        coords = list(line.coords)
                        pixel_coords = [~transform * (x, y) for x, y in coords]
                        pixel_coords = [(int(px), int(py)) for px, py in pixel_coords]
                        river_lines.append(pixel_coords)

    # --- 扫描所有水深图，确定统一色阶 ---
    global_min, global_max = np.inf, -np.inf
    for tif in image_files:
        arr = np.array(Image.open(tif).convert("L")).astype(float)
        if nodata_value is not None:
            arr[arr == nodata_value] = np.nan
        valid = arr[np.isfinite(arr)]
        if valid.size > 0:
            global_min = min(global_min, np.nanmin(valid))
            global_max = max(global_max, np.nanmax(valid))

    for image_file, year in zip(image_files, years):
        im = Image.open(image_file).convert("L")
        image_array = np.array(im)
        image_array, nodata_mask = linear_stretch(
            image_array, nodata_value=nodata_value,
            ignore_zero=True,
            fixed_min=global_min,
            fixed_max=global_max
        )

        im_colored = apply_color_map(image_array, nodata_mask=nodata_mask)

        if im_colored.size != dem_image.size:
            im_colored = im_colored.resize(dem_image.size)

        # --- 将 DEM 拷贝一份用于绘制河道 ---
        dem_with_river = dem_image.copy()

        # --- 在 DEM 图层上绘制河道（在水深图层下）---
        if river_lines:
            draw_river = ImageDraw.Draw(dem_with_river)
            for line in river_lines:
                draw_river.line(line, fill="blue", width=2)

        # --- 合成 DEM+河道 + 水深图 ---
        combined_im = Image.alpha_composite(dem_with_river, im_colored)

        # --- 添加白色背景 ---
        white_bg = Image.new("RGBA", combined_im.size, (255, 255, 255, 255))
        final_im = Image.alpha_composite(white_bg, combined_im)

        # --- 画行政区划在最上层 ---
        if shapefile_path:
            draw = ImageDraw.Draw(final_im)
            for polygon in boundaries:
                draw.line(polygon, fill="black", width=3)

        # --- 添加时间戳 ---
        draw = ImageDraw.Draw(final_im)
        try:
            font = ImageFont.truetype("arial.ttf", 38)
        except:
            font = ImageFont.load_default()

        width, height = final_im.size
        text_position = (width - 220, height - 50)
        draw.text((text_position[0] + 1, text_position[1] + 1), year.split()[0], fill="black", font=font)
        draw.text(text_position, year.split()[0], fill="black", font=font)

        # --- 添加图例 ---
        dem_legend = generate_colorbar_legend(80, colormap, 0, 255, "Elevation (m)")
        depth_legend = generate_colorbar_legend(80, "Blues", global_min, global_max, "Depth (m)")

        final_im.paste(dem_legend, (20, 20), dem_legend)
        final_im.paste(depth_legend, (20, 160), depth_legend)

        frames.append(final_im)

    print("GIF 已保存:", output_file)
    frames[0].save(output_file, save_all=True, append_images=frames[1:], duration=100, loop=0, optimize=True)



def create_gif_by_tif_with_shp_and_chwtrdepth(
    image_files, years, output_file, dem_path, colormap,
    nodata_value=None, shapefile_path=None, river_shapefile_dir=None,
    chwtr_min=0, chwtr_max=1
):
    from PIL import ImageDraw, ImageFont, Image
    import numpy as np
    import rasterio
    import geopandas as gpd
    import os

    frames = []

    # 读取 DEM 图像和地理信息
    with rasterio.open(dem_path) as dem_src:
        dem_image = load_and_colorize_dem(dem_path, colormap=colormap, debug=False)
        transform = dem_src.transform
        dem_crs = dem_src.crs
        dem_width, dem_height = dem_src.width, dem_src.height

    # 读取行政边界 shapefile
    boundaries = []
    if shapefile_path:
        gdf = gpd.read_file(shapefile_path)
        if gdf.crs != dem_crs:
            gdf = gdf.to_crs(dem_crs)
        for geom in gdf.geometry:
            if geom is not None:
                coords_list = []
                if geom.type in ["Polygon", "MultiPolygon"]:
                    for part in geom.geoms if geom.type == "MultiPolygon" else [geom]:
                        coords = list(part.exterior.coords)
                        pixel_coords = [~transform * (x, y) for x, y in coords]
                        pixel_coords = [(int(px), int(py)) for px, py in pixel_coords]
                        coords_list.append(pixel_coords)
                if coords_list:
                    boundaries.extend(coords_list)

    # 扫描所有水深图，获取统一拉伸范围
    global_min, global_max = np.inf, -np.inf
    for tif in image_files:
        arr = np.array(Image.open(tif).convert("L")).astype(float)
        if nodata_value is not None:
            arr[arr == nodata_value] = np.nan
        valid = arr[np.isfinite(arr)]
        if valid.size > 0:
            global_min = min(global_min, np.nanmin(valid))
            global_max = max(global_max, np.nanmax(valid))

    for image_file, year in zip(image_files, years):
        im = Image.open(image_file).convert("L")
        image_array = np.array(im)
        image_array, nodata_mask = linear_stretch(
            image_array,
            nodata_value=nodata_value,
            ignore_zero=True,
            fixed_min=global_min,
            fixed_max=global_max,
        )

        im_colored = apply_color_map(image_array, nodata_mask=nodata_mask)

        if im_colored.size != dem_image.size:
            im_colored = im_colored.resize(dem_image.size)

        # 渲染河道水深到 DEM 图层上
        year_str = year.replace("-", "_")
        shp_name = f"CHWTRDEPTH_TS_{year_str}_000000.shp"
        shp_path = os.path.join(river_shapefile_dir, shp_name)
        base_im = dem_image.copy()
        base_with_river = render_chwtrdepth_on_image(
            base_im, shp_path,
            field="CHWTRDEPTH", transform=transform, crs=dem_crs,
            vmin=chwtr_min, vmax=chwtr_max
        )

        # 合成：DEM+河道 + 水深图
        combined_im = Image.alpha_composite(base_with_river, im_colored)

        # 添加白色背景
        white_bg = Image.new("RGBA", combined_im.size, (255, 255, 255, 255))
        final_im = Image.alpha_composite(white_bg, combined_im)

        # 添加行政边界
        if shapefile_path:
            draw = ImageDraw.Draw(final_im)
            for polygon in boundaries:
                draw.line(polygon, fill="black", width=3)

        # 添加时间戳
        draw = ImageDraw.Draw(final_im)
        try:
            font = ImageFont.truetype("arial.ttf", 38)
        except:
            font = ImageFont.load_default()
        width, height = final_im.size
        date_text = year.split()[0]  # 仅保留年月日
        text_position = (width - 220, height - 50)
        draw.text((text_position[0] + 1, text_position[1] + 1), date_text, fill="black", font=font)
        draw.text(text_position, date_text, fill="black", font=font)

        # 添加图例：DEM、水深、河道水深
        dem_legend = generate_colorbar_legend(80,  35,colormap, 0, 255, "Elevation (m)")
        depth_legend = generate_colorbar_legend(85,  40,"Blues", global_min, global_max, "HAND Depth (m)")
        chwtr_legend = generate_colorbar_legend(85, 40, "Blues", chwtr_min, chwtr_max, "River Depth (m)")

        final_im.paste(dem_legend, (20, 20), dem_legend)
        final_im.paste(depth_legend, (20, 160), depth_legend)
        final_im.paste(chwtr_legend, (20, 300), chwtr_legend)

        frames.append(final_im)

    print("GIF 已保存:", output_file)
    frames[0].save(output_file, save_all=True, append_images=frames[1:], duration=300, loop=0, optimize=True)



def generate_colorbar_legend(height,width, colormap, vmin, vmax, label):
    import numpy as np
    from PIL import Image, ImageDraw, ImageFont
    import matplotlib.pyplot as plt

    gradient = np.linspace(1, 0, height).reshape(-1, 1)
    cmap = get_arcgis_elevation_colormap() if colormap == 'arcgis_elevation' else plt.get_cmap(colormap)
    rgba_img = (cmap(gradient)[:, 0, :3] * 255).astype(np.uint8)

    legend_img = Image.new("RGBA", (width, height), (255, 255, 255, 0))
    draw = ImageDraw.Draw(legend_img)
    for y in range(height):
        color = tuple(rgba_img[y])
        draw.rectangle([(0, y), (width, y + 1)], fill=color)

    try:
        font = ImageFont.truetype("arial.ttf", 16)
    except:
        font = ImageFont.load_default()

    final_img = Image.new("RGBA", (width + 80, height + 50), (255, 255, 255, 0))
    final_img.paste(legend_img, (0, 0))
    draw = ImageDraw.Draw(final_img)
    draw.text((width + 4, 0), f"{vmax:.1f}", fill="black", font=font)
    draw.text((width + 4, height // 2 - 8), f"{(vmin + vmax)/2:.1f}", fill="black", font=font)
    draw.text((width + 4, height - 18), f"{vmin:.1f}", fill="black", font=font)
    draw.text((0, height + 5), label, fill="black", font=font)

    return final_img




def get_arcgis_elevation_colormap():

    # 模拟 ArcGIS 的绿色 → 黄色 → 棕色 → 粉红色
    colors = [
        (0.0, '#8CCB5E'),  # 绿色
        (0.3, '#FFFFB2'),  # 黄色
        (0.6, '#B07C5A'),  # 棕色
        (1.0, '#F2D2DA')   # 粉红色
    ]
    return LinearSegmentedColormap.from_list('ArcGIS_Elevation', colors)

def load_and_colorize_dem(dem_path, colormap='terrain', debug=False):
    with rasterio.open(dem_path) as src:
        dem_array = src.read(1).astype(float)

    nodata = src.nodata
    if nodata is not None:
        dem_array[dem_array == nodata] = np.nan

    min_val = np.nanmin(dem_array)
    max_val = np.nanmax(dem_array)
    print(f"DEM 范围: {min_val:.2f} - {max_val:.2f}, 使用色带: {colormap}")

    standardized = (dem_array - min_val) / (max_val - min_val + 1e-6)

    # 获取 colormap
    if colormap == 'arcgis_elevation':
        cmap = get_arcgis_elevation_colormap()
    else:
        cmap = plt.get_cmap(colormap)
    colorized = cmap(standardized)
    colorized = (colorized * 255).astype(np.uint8)

    if debug:
        plt.imshow(colorized)
        plt.title(f"DEM 渲染效果（colormap={colormap}）")
        plt.axis("off")
        plt.show()

    r, g, b, a = [Image.fromarray(colorized[:, :, i]) for i in range(4)]
    alpha = Image.new("L", r.size, 255)
    return Image.merge("RGBA", (r, g, b, alpha))

def linear_stretch(image_array, nodata_value=None, ignore_zero=False, fixed_min=None, fixed_max=None):
    nodata_mask = np.zeros_like(image_array, dtype=bool)
    if nodata_value is not None:
        nodata_mask |= (image_array == nodata_value)
    if ignore_zero:
        nodata_mask |= (image_array == 0)

    valid = image_array[~nodata_mask].astype(float)
    if fixed_min is None or fixed_max is None:
        if valid.size == 0:
            return np.zeros_like(image_array, dtype=np.uint8), nodata_mask
        vmin, vmax = np.nanmin(valid), np.nanmax(valid)
    else:
        vmin, vmax = fixed_min, fixed_max

    stretched = 255 * (image_array - vmin) / (vmax - vmin + 1e-6)
    stretched = np.clip(stretched, 0, 255)
    stretched[nodata_mask] = 0
    return stretched.astype(np.uint8), nodata_mask




def apply_color_map(image_array, nodata_mask=None, water_mask_threshold=1):
    cmap = cm.get_cmap('Blues')
    rgba_img = cmap(image_array / 255.0)
    rgba_img = (rgba_img * 255).astype(np.uint8)

    # 基于阈值设定水体掩膜：例如像素值 < 5 认为是非水体
    water_mask = image_array > water_mask_threshold  # 更宽容，0也可能是有效水，但视觉干扰大

    # 默认先全设为透明
    rgba_img[:, :, 3] = 0  # alpha 通道

    # 只有真正的水体区域设为不透明
    rgba_img[water_mask, 3] = 255

    # 可以可选保留 nodata_mask 的处理（兼容其他情况）
    if nodata_mask is not None:
        rgba_img[nodata_mask] = [0, 0, 0, 0]

    return Image.fromarray(rgba_img, 'RGBA')

def extract_time_from_filename(filename):
    try:
        time_part = filename.split('_TS_')[1].replace('.tif', '')
        year = time_part[:4]
        month = time_part[5:7]
        day = time_part[8:10]
        hour = time_part[11:13]
        minute = time_part[13:15]
        second = time_part[15:17]
        formatted_time = f"{year}-{month}-{day} {hour}:{minute}:{second}"
        return year, formatted_time
    except Exception as e:
        print(f"错误: 无法解析文件名 {filename}，原因: {e}")
        return None, None

def get_tif_files_and_years(directory):
    image_files = []
    years = []

    for filename in sorted(os.listdir(directory)):
        if filename.endswith(".tif"):
            year, formatted_time = extract_time_from_filename(filename)
            if year and formatted_time:
                image_files.append(os.path.join(directory, filename))
                # years.append(formatted_time)
                years.append(formatted_time.split(" ")[0])  # 只保留日期部分，如 2015-03-06

    return image_files, years

def gen_chwtrdepth_timeseries_by_txt(txt_path, shp_path, output_dir,
                                     mongo_uri, db_name, collection_name):
    # Step 1: 读取河道 shapefile
    gdf = gpd.read_file(shp_path)
    if 'LINKNO' not in gdf.columns:
        raise ValueError("Shapefile must contain 'LINKNO' field.")

    # Step 2: 从 MongoDB 读取 SUBBASINID -> Is_Lake 映射
    client = MongoClient(mongo_uri)
    collection = client[db_name][collection_name]

    is_lake_map = {
        doc['SUBBASINID']: doc.get('Is_Lake', 0)
        for doc in collection.find({}, {'SUBBASINID': 1, 'Is_Lake': 1})
    }

    print(f"✅ Loaded {len(is_lake_map)} Is_Lake flags from MongoDB.")

    # Step 3: 读取 TXT，跳过前3行
    df = pd.read_csv(txt_path, sep='\s+', skiprows=3, header=None, engine='python')

    # Step 4: 第一列是时间，拼接为 datetime
    df[0] = pd.to_datetime(df[0] + ' ' + df[1])
    df = df.drop(columns=[1])  # 删除时间的第二部分

    # Step 5: 创建输出目录
    os.makedirs(output_dir, exist_ok=True)

    # Step 6: 遍历每一行（每个时间步），生成新的 shapefile
    for idx, row in df.iterrows():
        timestamp = row[0]  # 时间
        values = row[1:].reset_index(drop=True)  # 水深值

        # 你的原始赋值逻辑，不变
        gdf_copy = gdf.copy()
        gdf_copy['CHWTRDEPTH'] = gdf_copy['LINKNO'].apply(
            lambda linkno: values[linkno - 1] if 1 <= linkno <= len(values) else None
        )

        # 强制覆盖湖泊为 0
        gdf_copy.loc[
            gdf_copy['LINKNO'].isin([k for k, v in is_lake_map.items() if v == 1]),
            'CHWTRDEPTH'
        ] = 0.0

        # 生成文件名
        filename = f"CHWTRDEPTH_TS_{timestamp.strftime('%Y_%m_%d_%H%M%S')}.shp"
        output_path = os.path.join(output_dir, filename)

        # 保存 shapefile
        gdf_copy = gdf_copy[gdf_copy.geometry.notnull()]
        gdf_copy = gdf_copy[gdf_copy.is_valid]
        gdf_copy = gdf_copy.set_crs(gdf.crs)
        gdf_copy.to_file(output_path)
        print(f"✅ Saved: {output_path}")

    """
    根据 SNAC_TS_YYYY_MM_DD_hhmmss.txt 文件名和时间段，返回时间段内的文件路径列表

    参数:
        folder_path (str | Path): 存放文件的文件夹路径
        start_time (datetime): 起始时间
        end_time (datetime): 结束时间
        pattern_prefix (str): 文件名前缀（默认 "SNAC_TS_"）
        time_format (str): 时间格式（默认 "%Y_%m_%d_%H%M%S"）

    返回:
        list[Path]: 时间段内的文件路径列表（按时间升序）
    """
def filter_files_by_time(folder_path, start_time, end_time, pattern_prefix="SNAC_TS_", time_format="%Y_%m_%d_%H%M%S"):

    folder_path = Path(folder_path)
    matched_files = []

    for file in folder_path.glob(f"{pattern_prefix}*.txt"):
        try:
            # 提取时间部分
            datetime_str = file.stem.replace(pattern_prefix, "")
            file_time = datetime.strptime(datetime_str, time_format)
            # 时间过滤
            if start_time <= file_time <= end_time:
                matched_files.append((file_time, file))
        except ValueError:
            # 文件名不符合规则的跳过
            continue

    # 按时间排序后返回 Path 列表
    matched_files.sort(key=lambda x: x[0])
    return [f for _, f in matched_files]

""" 把SEIMS输出每个HRU的TS txt文件, 赋值给shp对应的value属性，输出为新的shp（即每个时刻输出一个shp） """
def write_value_to_hrushp(shp_path, txt_path, out_shp_path, id_field="FIELDID",
                       value_field="value", fill_missing=None):
    shp_path = Path(shp_path)
    txt_path = Path(txt_path)
    out_shp_path = Path(out_shp_path)
    out_shp_path.parent.mkdir(parents=True, exist_ok=True)

    # 1) 读取 txt
    df = pd.read_csv(
        txt_path,
        header=None,
        names=[id_field, value_field],
        sep=",",
        engine="python",
        skipinitialspace=True,
    )

    df[id_field] = df[id_field].astype(int)
    df[value_field] = pd.to_numeric(df[value_field], errors="coerce")
    df = df.drop_duplicates(subset=[id_field], keep="last")

    # 2) 读取 shp
    gdf = gpd.read_file(shp_path)

    if id_field not in gdf.columns:
        raise KeyError(f"在 {shp_path.name} 中未找到字段 `{id_field}`。现有字段：{list(gdf.columns)}")

    # 3) 合并
    gdf = gdf.merge(df, how="left", on=id_field)

    if fill_missing is not None:
        gdf[value_field] = gdf[value_field].fillna(fill_missing)

    gdf[value_field] = gdf[value_field].astype("float32")

    # 4) 输出到指定路径
    gdf.to_file(out_shp_path, driver="ESRI Shapefile", encoding="utf-8")
    print(f"✅ 已写出: {out_shp_path}")


def filter_paths_by_time(path_list, start_time, end_time, pattern_prefix="", time_format="%Y_%m_%d_%H%M%S"):
    """
    根据 SNAC_TS_YYYY_MM_DD_hhmmss.txt 文件名和时间段，返回时间段内的文件路径列表

    参数:
        path_list (list[Path|str]): 文件路径列表
        start_time (datetime): 起始时间
        end_time (datetime): 结束时间
        pattern_prefix (str): 文件名前缀（默认 "SNAC_TS_"）
        time_format (str): 时间格式（默认 "%Y_%m_%d_%H%M%S"）

    返回:
        list[Path]: 时间段内的文件路径列表（按时间升序）
    """
    matched_files = []

    for p in path_list:
        file = Path(p)
        try:
            # 提取时间部分
            datetime_str = file.stem.replace(pattern_prefix, "")
            file_time = datetime.strptime(datetime_str, time_format)
            # 时间过滤
            if start_time <= file_time <= end_time:
                matched_files.append((file_time, file))
        except ValueError:
            # 文件名不符合规则的跳过
            continue

    matched_files.sort(key=lambda x: x[0])
    return [f for _, f in matched_files]


if __name__ == '__main__':
    # missouri
    # work_dir = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347'
    # longter_model_name = '-90_124556_38_819347_longterm_model'
    # 鄱阳湖
    work_dir = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1'
    longter_model_name = 'poyang_lake1_longterm_model'
    longterm_model_dir = os.path.join(work_dir,longter_model_name)
    directory = os.path.join(longterm_model_dir,'OUTPUT0')
    # prefix = 'SNAC_TS'
    # pattern_prefix = 'SNAC_TS_'
    # prefix = 'SNME_TS'
    # pattern_prefix = 'SNME_TS_'
    # prefix = 'TMAX_TS'
    # pattern_prefix = 'TMAX_TS_'
    #('SNAC_TS', 'SNAC_TS_'),,('SNME_TS','SNME_TS_'),('TMAX_TS','TMAX_TS_')
    # pairs = [('SNAC_TS', 'SNAC_TS_'),('SNME_TS', 'SNME_TS_')]
    # pairs = [('OL_Hand_WTRDEP_TS_AVG', 'OL_Hand_WTRDEP_TS_AVG_')]
    pairs = [('OL_Hand_WTRDEP_TS', 'OL_Hand_WTRDEP_TS_')]
    suffix = 'txt'

    #########################  将HAND输出的结果生成为shp  ################################
    shp_path = os.path.join(work_dir,r"workspace\HRU_file\HRU_mollwede.shp")
    # start = datetime(2019, 3, 10, 0, 0, 0)
    # end = datetime(2019, 3, 22, 0, 0, 0)
    start = datetime(2010, 1, 1, 0, 0, 0)
    end = datetime(2012, 1, 30, 0, 0, 0)
    # 多线程容易报错
    # files_in_range = []
    # for prefix, pattern_prefix in pairs:
    #     txt_paths = get_files_by_prefix_suffix(directory,prefix,suffix)
    #     files_in_range.extend(filter_paths_by_time(txt_paths, start, end, pattern_prefix))
    # def run_gen_hand_shp(txt_path, shp_path):
    #     output_tif_path = replace_txt_with_shp(txt_path)
    #     write_value_to_hrushp(shp_path, txt_path, output_tif_path, id_field="FIELDID", value_field="value",
    #                           fill_missing=None)
    # max_workers = 2
    # with ThreadPoolExecutor(max_workers=max_workers) as executor:
    #     futures = [
    #         executor.submit(run_gen_hand_shp, txt_path, shp_path)
    #         for txt_path in files_in_range
    #     ]
    #
    #     # 可选：显示进度 & 捕捉错误
    #     for future in as_completed(futures):
    #         try:
    #             future.result()
    #         except Exception as e:
    #             print(f"发生错误：{e}")
    ### 单线程
    # for prefix, pattern_prefix in pairs:
    #     txt_paths = get_files_by_prefix_suffix(directory,prefix,suffix)
    #     files_in_range = filter_paths_by_time(txt_paths, start, end, pattern_prefix)
    #     for txt_path in files_in_range:
    #         output_tif_path = replace_txt_with_shp(txt_path)
    #         write_value_to_hrushp(shp_path, txt_path, output_tif_path, id_field="FIELDID", value_field="value", fill_missing=None)

    #########################  将HAND输出的结果生成为tif  ################################
    work_dir = r'G:\program\seims\SEIMS_HAND\data\poyang_lake1'
    longter_model_name = 'poyang_lake1_longterm_model'
    # prefix = 'OL_Hand_WTRDEP_TS'
    # prefix = 'SNAC_TS_'

    input_tif_path = os.path.join(work_dir,'workspace\HRU_file\ALL_HRU_final.tif')

    # HAND水深 txt 转 tif
    # txt_paths = get_files_by_prefix_suffix(directory,prefix,suffix)
    # for txt_path in txt_paths:
    #     output_tif_path = replace_txt_with_tif(txt_path)
    #     gen_hand_tif_by_txt(
    #         txt_path=txt_path,
    #         input_tif_path=input_tif_path,
    #         output_tif_path=output_tif_path
    #     )
    # 多线程生成tif
    # 设置最大线程数（建议不超过 CPU 核心数的 2~4 倍）
    def run_gen_hand_tif(txt_path, input_tif_path):
        output_tif_path = replace_txt_with_tif(txt_path)
        # gen_hand_tif_by_txt(
        #     txt_path=txt_path,
        #     input_tif_path=input_tif_path,
        #     output_tif_path=output_tif_path
        # )
        gen_hand_tif_by_txt_fast(
            txt_path=txt_path,
            input_tif_path=input_tif_path,
            output_tif_path=output_tif_path
                                 )
    max_workers = 1
    files_in_range = []
    for prefix, pattern_prefix in pairs:
        txt_paths = get_files_by_prefix_suffix(directory,prefix,suffix)
        files_in_range.extend(filter_paths_by_time(txt_paths, start, end, pattern_prefix))
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            futures = [
                executor.submit(run_gen_hand_tif, txt_path, input_tif_path)
                for txt_path in files_in_range
            ]

            # 可选：显示进度 & 捕捉错误
            for future in as_completed(futures):
                try:
                    future.result()
                except Exception as e:
                    print(f"发生错误：{e}")

    # tif生成gif
    OUTPUT_file = r'OUTPUT0'
    tif_folder = os.path.join(longterm_model_dir,OUTPUT_file)
    output_file = os.path.join(longterm_model_dir,f'{OUTPUT_file}\gif\OL_Hand_WTRDEP.gif')
    output_file_base = os.path.join(longterm_model_dir,f'{OUTPUT_file}\gif\OL_Hand_WTRDEP_')
    # bakgrnd_tif = r'G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\workspace\spatial_raster\dem.tif'
    bakgrnd_tif = os.path.join(work_dir,r'merit_dem\merit_bounding_dem.tif')
    extent_shp = os.path.join(work_dir,r'workspace\spatial_shp\basin.shp')
    river_shapefile = os.path.join(work_dir,r'workspace\spatial_shp\reach.shp')

    chwtrdepth_file = os.path.join(longterm_model_dir,f'{OUTPUT_file}\CHWTRDEPTH_TS.txt')
    chwtrdepth_tif_folder = os.path.join(longterm_model_dir,f'{OUTPUT_file}\CHWTRDEPTH')

    # gen_chwtrdepth_timeseries_by_txt(chwtrdepth_file, river_shapefile, chwtrdepth_tif_folder,
    #                                  mongo_uri="mongodb://localhost:27017",
    #                                  db_name=longter_model_name,
    #                                  collection_name="REACHES"
    #                                  )

    image_files, years = get_tif_files_and_years(tif_folder)
    colormap_options = [
        # 地形真实风格（适合 DEM）
        'terrain',  # 红-绿-蓝色高程渐变，经典地形色带
        'gist_earth',  # 类似地形图，偏绿棕黄，自然真实
        'BrBG',  # 棕绿分布，表现坡地/水域对比好

        # 🌫柔和、背景不抢眼（推荐用于衬底）
        'cividis',  # 色弱友好，蓝→黄，柔和低对比
        'viridis',  # 蓝绿黄通透渐变，现代感强
        'YlGnBu',  # 浅黄→绿→蓝，适合水系背景
        'bone',  # 灰蓝色调，非常适合作为淡背景
        'Greys',  # 单纯灰度，强调高程线条感
        'Blues',  # 浅蓝→深蓝，适合模拟水体或阴影

        # 晕渲/立体感强（用于视觉提升）
        'cubehelix',  # 有种光照感，适合配合 hillshade
        'plasma',  # 紫-红-黄高对比，不适合做背景但高辨识
        'magma',  # 黑-红-黄热感色带，适合晕渲增强地势

        # 中性科研色带（柔和易读）
        'coolwarm',  # 蓝-红对比，用于坡向也可作 DEM
        'Spectral',  # 彩虹顺序渐变，有时用于水文学图层
        'PuBuGn',  # 紫蓝绿渐变，柔和通透
    ]
    river_shapefile_dir = os.path.join(longterm_model_dir,r'OUTPUT0\CHWTRDEPTH')
    create_gif_by_tif_with_shp_and_chwtrdepth(image_files, years, output_file, bakgrnd_tif,
                               'arcgis_elevation', -9999,shapefile_path=extent_shp,river_shapefile_dir=river_shapefile_dir)



    # create_gif_by_tif_with_shp(image_files, years, output_file, bakgrnd_tif,
    #                            'arcgis_elevation', -9999,shapefile_path=extent_shp,river_shapefile=river_shapefile)
    # create_gif_by_tif(image_files, years, output_file, bakgrnd_tif, 'arcgis_elevation', -9999)
    # for colormap in colormap_options:
    #     output_file = output_file_base + colormap + '.gif'
    #     create_gif_by_tif(image_files, years,output_file,bakgrnd_tif,colormap,-9999)

