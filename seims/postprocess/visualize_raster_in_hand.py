import geopandas as gpd
from shapely.geometry import box
import numpy as np
import os

""" 在shpfile面内部生成加密网格，输出网格shp，类似经纬网 """
def generate_grid_within_shapefile(boundary_shp, output_shp, resolution):
    # 读取边界
    gdf = gpd.read_file(boundary_shp)
    gdf = gdf.to_crs(epsg=3857)  # 建议使用米制坐标系，便于网格大小控制

    bounds = gdf.total_bounds  # [minx, miny, maxx, maxy]
    minx, miny, maxx, maxy = bounds
    print(f"边界范围: {bounds}")

    # 构建网格
    rows = int(np.ceil((maxy - miny) / resolution))
    cols = int(np.ceil((maxx - minx) / resolution))

    polygons = []
    for i in range(cols):
        for j in range(rows):
            x1 = minx + i * resolution
            y1 = miny + j * resolution
            x2 = x1 + resolution
            y2 = y1 + resolution
            polygons.append(box(x1, y1, x2, y2))

    grid = gpd.GeoDataFrame({'geometry': polygons})
    grid.set_crs(gdf.crs, inplace=True)

    # 裁剪保留内部网格
    try:
        clipped = gpd.overlay(grid, gdf, how='intersection')  # 需要 rtree 或 pygeos
    except ImportError as e:
        print("需要安装 rtree 或 pygeos 才能裁剪，请使用 `conda install rtree`")
        return

    # 保存输出
    clipped.to_file(output_shp)
    print(f"生成完成：共 {len(clipped)} 个网格，保存为 {output_shp}")

if __name__ == '__main__':

    important_subbasin_shp = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347\workspace\spatial_shp\subbasin_omaha.shp'
    output_shp = r'G:\program\seims\SEIMS_HAND\data\-90.124556_38.819347\nested_modeling\important_subbasin\grid_within_sub'
    generate_grid_within_shapefile(
        boundary_shp=important_subbasin_shp,  # 替换为你的子流域边界 shp 路径
        output_shp=output_shp,  # 输出路径
        resolution=5000  # 网格大小，例如 1000 米
    )
