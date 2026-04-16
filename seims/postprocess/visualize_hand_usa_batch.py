
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
import os
import numpy as np
from datetime import datetime, timedelta
from concurrent.futures import ThreadPoolExecutor, as_completed
import rasterio

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
    if not matched_files:
        print(f"\033[31m错误:{start_time}-{end_time} 没有匹配的文件符合时间范围！\033[0m")

    matched_files.sort(key=lambda x: x[0])
    for date,f in matched_files:
        print(f'{date} 成功匹配到 {f}')
    return [f for _, f in matched_files]

def gen_gif_by_tifs():
    # tif生成gif
    OUTPUT_file = r'gen_62_cali_5'
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

def get_previous_day(date_str):
    """给定日期字符串，获取前一天的日期"""
    event_date = datetime.strptime(date_str, "%Y-%m-%d")
    previous_day = event_date - timedelta(days=1)
    return previous_day.strftime("%Y-%m-%d")


    # create_gif_by_tif_with_shp(image_files, years, output_file, bakgrnd_tif,
    #                            'arcgis_elevation', -9999,shapefile_path=extent_shp,river_shapefile=river_shapefile)
    # create_gif_by_tif(image_files, years, output_file, bakgrnd_tif, 'arcgis_elevation', -9999)
    # for colormap in colormap_options:
    #     output_file = output_file_base + colormap + '.gif'
    #     create_gif_by_tif(image_files, years,output_file,bakgrnd_tif,colormap,-9999)

"""
    脚本功能：把WISE输出的txt时间序列数据转为tif，例如把流域内所有HAND上的淹没水深输出为txt后，将其转为tif
    你需要改的：
    work_dir: 项目目录
    longter_model_name: 你的项目名称
    calibration_name: 我用的是某次率定的结果
    directory：我用的是某次率定的路径拼接的路径，你可以直接写到自己txt时间序列所在的路径
    max_workers：线程数，支持多线程
    pairs：txt文件的前缀，用于识别txt
    start，end：你要可视化哪些时间范围的

"""


if __name__ == '__main__':

    # 美国小流域
    if os.name == 'nt':  # Windows
        work_dir = r'G:\program\seims\SEIMS_HAND\data'
    else:
        work_dir = f'/data/user/xiaodw/software/WISE/data'
    longter_model = '_longterm_model'
    calibration_name = 'OUTPUT0-0'


    BASIN_CONFIG = {

        # "US_2": {
        #     "events": {
        #         "2017": ("2017-06-28", "2017-07-07"),
        #         "2019": ("2019-10-29", "2019-11-07"),
        #         "2023": ("2023-07-07", "2023-07-16"),
        #         "2024": ("2024-07-08", "2024-07-17"),
        #     },
        # },
        # "US_3": {
        #     "events": {
        #         "2016": ("2016-06-20", "2016-06-29"),
        #         "2017": ("2017-10-21", "2017-10-30"),
        #         "2021": ("2021-02-26", "2021-03-07"),
        #         "2024": ("2024-01-06", "2024-01-15"),
        #     },
        # },
        # "US_4": {
        #     "events": {
        #         "2016_1": ("2016-01-03", "2016-01-12"),  # Jan event
        #         "2016_2": ("2016-12-14", "2016-12-23"),  # Dec event
        #         "2018": ("2018-08-15", "2018-08-24"),
        #         "2019": ("2019-02-11", "2019-02-20"),
        #     },
        # },
        # "US_5": {
        #     "events": {
        #         "2019": ("2019-02-12", "2019-02-21"),
        #         "2021": ("2021-07-22", "2021-07-31"),
        #         "2022": ("2022-08-17", "2022-08-26"),
        #         "2023": ("2023-01-14", "2023-01-23"),
        #     },
        # },
        # "US_6": {
        #     "events": {
        #         "2017": ("2017-02-14", "2017-02-23"),
        #         "2019": ("2019-01-14", "2019-01-23"),
        #         "2023": ("2023-01-06", "2023-01-15"),
        #         "2024": ("2024-02-01", "2024-02-10"),
        #     },
        # },
        #
        # "US_10": {
        #     "events": {
        #         "2019": ("2019-04-14", "2019-04-24"),  # 洪水日期前后各5天
        #         "2020": ("2020-04-08", "2020-04-18"),  # 洪水日期前后各5天
        #         "2021": ("2021-08-12", "2021-08-22"),  # 洪水日期前后各5天
        #         "2022": ("2022-05-22", "2022-06-01"),  # 洪水日期前后各5天
        #     },
        # },
        #
        # "US_11": {
        #     "events": {
        #         "2017": ["2017-10-18", "2017-10-28"],
        #         "2020": ["2020-04-08", "2020-04-18"],
        #         "2021": ["2021-08-12", "2021-08-22"],
        #         "2022": ["2022-05-22", "2022-06-01"]
        #     }
        # },
        #
        # "US_12": {
        #     "events": {
        #         "2018": ["2018-05-25", "2018-06-04"],
        #         "2020": ["2020-05-14", "2020-05-24"],
        #         "2021": ["2021-10-02", "2021-10-12"],
        #         "2022": ["2022-11-06", "2022-11-16"]
        #     }
        # },
        # "US_14": {
        #     "events": {
        #         "2018": ["2018-05-25", "2018-06-04"],
        #         "2020": ["2020-05-15", "2020-05-25"],
        #         "2021": ["2021-10-02", "2021-10-12"],
        #         "2022": ["2022-11-06", "2022-11-16"]
        #     },
        # },
        "US_15": {
            "events": {
                # "20180912": ["2018-09-12", "2018-09-22"],
                # "20200106": ["2020-01-06", "2020-01-16"],
                # "20210813": ["2021-08-13", "2021-08-23"],
                # "20221106": ["2022-11-06", "2022-11-16"],
                # ### 新加的
                # "20150414": ["2015-04-14", "2015-04-24"],
                # "20160129": ["2016-01-29", "2016-02-08"],
                # "20161006": ["2016-10-06", "2016-10-16"],
                # "20170430": ["2017-04-30", "2017-05-10"],
                # "20181006": ["2018-10-06", "2018-10-16"],
                # "20190424": ["2019-04-24", "2019-05-04"],
                # "20211002": ["2021-10-02", "2021-10-12"],
                # "20230104": ["2023-01-04", "2023-01-14"],
                # "20240922": ["2024-09-22", "2024-10-02"],
                # "20250507": ["2025-05-07", "2025-05-17"]
                # ### 第二次新加的
                "20160113": ["2016-01-13", "2016-01-23"],
                "20160221": ["2016-02-21", "2016-03-02"],
                "20160805": ["2016-08-05", "2016-08-15"],

                "20170120": ["2017-01-20", "2017-01-30"],
                "20170328": ["2017-03-28", "2017-04-07"],
                "20170521": ["2017-05-21", "2017-05-31"],
                "20170909": ["2017-09-09", "2017-09-19"],
                "20171006": ["2017-10-06", "2017-10-16"],
                "20171020": ["2017-10-20", "2017-10-30"],
                "20171101": ["2017-11-01", "2017-11-11"],

                "20180109": ["2018-01-09", "2018-01-19"],
                "20180208": ["2018-02-08", "2018-02-18"],
                "20180323": ["2018-03-23", "2018-04-02"],
                "20180421": ["2018-04-21", "2018-05-01"],
                "20180516": ["2018-05-16", "2018-05-26"],
                "20180527": ["2018-05-27", "2018-06-06"],
                "20180731": ["2018-07-31", "2018-08-10"],
                "20180818": ["2018-08-18", "2018-08-28"],
                "20181024": ["2018-10-24", "2018-11-03"],
                "20181112": ["2018-11-12", "2018-11-22"],
                "20181218": ["2018-12-18", "2018-12-28"],

                "20190102": ["2019-01-02", "2019-01-12"],
                "20190121": ["2019-01-21", "2019-01-31"],
                "20190221": ["2019-02-21", "2019-03-03"],
                "20190307": ["2019-03-07", "2019-03-17"],
                "20190412": ["2019-04-12", "2019-04-22"],
                "20190606": ["2019-06-06", "2019-06-16"],
                "20191028": ["2019-10-28", "2019-11-07"],
                "20191128": ["2019-11-28", "2019-12-08"],
                "20191221": ["2019-12-21", "2019-12-31"],

                "20180925": ["2018-09-25", "2018-10-05"],
                "20181129": ["2018-11-29", "2018-12-09"],
                "20190509": ["2019-05-09", "2019-05-19"],
                "20190210": ["2019-02-10", "2019-02-20"],
                "20180624": ["2018-06-24", "2018-07-04"],
                "20180227": ["2018-02-27", "2018-03-09"],
                "20170315": ["2017-03-15", "2017-03-25"],
                "20180126": ["2018-01-26", "2018-02-05"],
                "20190709": ["2019-07-09", "2019-07-19"],
                "20190720": ["2019-07-20", "2019-07-30"],

                "20160430": ["2016-04-30", "2016-05-10"],
                "20180610": ["2018-06-10", "2018-06-20"],
                "20171218": ["2017-12-18", "2017-12-28"],
                "20170611": ["2017-06-11", "2017-06-21"],
                "20180505": ["2018-05-05", "2018-05-15"],
                "20160329": ["2016-03-29", "2016-04-08"],
                "20160306": ["2016-03-06", "2016-03-16"],
                "20161204": ["2016-12-04", "2016-12-14"],
                "20180831": ["2018-08-31", "2018-09-10"],
                "20190323": ["2019-03-23", "2019-04-02"],

                "20170811": ["2017-08-11", "2017-08-21"],
                "20180310": ["2018-03-10", "2018-03-20"],
                "20180404": ["2018-04-04", "2018-04-14"],
                "20160531": ["2016-05-31", "2016-06-10"],
                "20170710": ["2017-07-10", "2017-07-20"],
                "20180710": ["2018-07-10", "2018-07-20"],
                "20160518": ["2016-05-18", "2016-05-28"],
                "20171116": ["2017-11-16", "2017-11-26"],
                "20191017": ["2019-10-17", "2019-10-27"],
                "20170413": ["2017-04-13", "2017-04-23"]
            },
        },
        # "US_16": {
        #     "events": {
        #         "2018": ["2018-05-14", "2018-05-24"],
        #         "2019": ["2019-06-04", "2019-06-14"],
        #         "2020": ["2020-04-08", "2020-04-18"],
        #         "2022": ["2022-11-06", "2022-11-16"]
        #     },
        # },
        # "US_17": {
        #     "events": {
        #         "2017": ["2017-05-19", "2017-05-29"],
        #         "2018": ["2018-04-10", "2018-04-20"],
        #         "2019": ["2019-06-04", "2019-06-14"],
        #         "2020": ["2020-04-08", "2020-04-18"]
        #     },
        # },
        # "US_18": {
        #     "events": {
        #         "2017": ["2017-05-19", "2017-05-29"],
        #         "2019": ["2019-06-04", "2019-06-14"],
        #         "2020": ["2020-04-08", "2020-04-18"],
        #         "2023": ["2023-02-27", "2023-03-08"]
        #     },
        # },
    }


    pairs_arr = [

        # [('Perco200_TS', 'Perco200_TS_')],
        # [('Runoff_co_TS', 'Runoff_co_TS_')],
        # [('RUNOFF_PERCENTAGE_TS', 'RUNOFF_PERCENTAGE_TS_')],
        # [('RUNOFF_PERCENTAGE_TS', 'RUNOFF_PERCENTAGE_TS_')],

        [('solmoist1_TS', 'solmoist1_TS_')],
        [('solmoist5_TS', 'solmoist5_TS_')],
        [('solmoist15_TS', 'solmoist15_TS_')],
        [('solmoist30_TS', 'solmoist30_TS_')],
        [('solmoist60_TS', 'solmoist60_TS_')],
        [('solmoist100_TS', 'solmoist100_TS_')],
        [('solmoist200_TS', 'solmoist200_TS_')],

        # [('solawc1_TS', 'solawc1_TS_')],
        # [('solawc5_TS', 'solawc5_TS_')],
        # [('solawc30_TS', 'solawc30_TS_')],
        # [('solawc60_TS', 'solawc60_TS_')],
        # [('solawc100_TS', 'solawc100_TS_')],
        # [('solawc200_TS', 'solawc200_TS_')],
        #
        # [('solsat1_TS', 'solsat1_TS_')],
        # [('solsat5_TS', 'solsat5_TS_')],
        # [('solsat30_TS', 'solsat30_TS_')],
        # [('solsat60_TS', 'solsat60_TS_')],
        # [('solsat100_TS', 'solsat100_TS_')],
        # [('solsat200_TS', 'solsat200_TS_')],
        #
        # [('ks1_TS', 'ks1_TS_')],
        # [('ks5_TS', 'ks5_TS_')],
        # [('ks30_TS', 'ks30_TS_')],
        # [('ks60_TS', 'ks60_TS_')],
        # [('ks100_TS', 'ks100_TS_')],
        # [('ks200_TS', 'ks200_TS_')]
    ]
    suffix = 'txt'

    #########################  将HAND输出的结果生成为tif  ################################

    # 设置最大线程数（建议不超过 CPU 核心数的 2~4 倍）
    max_workers = 10
    def run_gen_hand_tif(txt_path, input_tif_path):
        output_tif_path = replace_txt_with_tif(txt_path)
        gen_hand_tif_by_txt_fast(
            txt_path=txt_path,
            input_tif_path=input_tif_path,
            output_tif_path=output_tif_path
                                 )


    for basin_name, cfg in BASIN_CONFIG.items():
        events = cfg["events"]
        input_tif_path = os.path.join(work_dir, f'{basin_name}',r'workspace/HRU_file/ALL_HRU_final.tif')
        for event_name, event_dates in events.items():
            # 获取事件开始日期
            event_start = event_dates[0]
            # 获取前一天的日期
            previous_day_str = get_previous_day(event_start)
            previous_day = datetime.strptime(previous_day_str, "%Y-%m-%d")
            print(f"\n===== {basin_name} - {event_name} 的前一天 {previous_day} 开始处理 =====")
            longterm_model_dir = os.path.join(work_dir, f'{basin_name}',f'{basin_name}_longterm_model')
            directory = os.path.join(longterm_model_dir, calibration_name)
            ## txt转tif
            files_in_range = []
            for pairs in pairs_arr:
                for prefix, pattern_prefix in pairs:
                    txt_paths = get_files_by_prefix_suffix(directory,prefix,suffix)
                    files_in_range.extend(filter_paths_by_time(txt_paths, previous_day, previous_day, pattern_prefix))
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

    """ HAND淹没水深tif叠加观测范围tif绘图 """


    """ HAND淹没水深tif叠加观测范围tif的图转gif """

