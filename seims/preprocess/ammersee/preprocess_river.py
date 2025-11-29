import geopandas as gpd
import rasterio
from rasterstats import zonal_stats
from pymongo import MongoClient
import os
from shapely.geometry import Point

"""判断河道shp文件是否有bed_elev属性字段，如果没有就根据dem计算河道平均高程，如果有就覆盖"""
def add_river_bed_elevation(river_shp_path, dem_tif_path, output_shp_path=None):
    # 1. 读取河道线矢量数据
    gdf = gpd.read_file(river_shp_path)

    # 2. 计算 zonal stats: 平均、最大、最小高程
    stats = zonal_stats(
        vectors=gdf.geometry,
        raster=dem_tif_path,
        stats=['mean', 'max', 'min'],
        nodata=None,
        all_touched=True,
        geojson_out=False
    )

    # 3. 分别提取并保留3位小数，确保为 float 类型
    bed_elev_list = []
    max_elev_list = []
    min_elev_list = []

    for stat in stats:
        mean_val = stat['mean']
        max_val = stat['max']
        min_val = stat['min']

        bed_elev_list.append(round(float(mean_val), 3) if mean_val is not None else None)
        max_elev_list.append(round(float(max_val), 3) if max_val is not None else None)
        min_elev_list.append(round(float(min_val), 3) if min_val is not None else None)

    # 4. 添加到 GeoDataFrame，并设置字段类型为 float64
    gdf['mean_elev'] = bed_elev_list
    gdf['max_elev'] = max_elev_list
    gdf['min_elev'] = min_elev_list

    gdf['mean_elev'] = gdf['mean_elev'].astype('float64')
    gdf['max_elev'] = gdf['max_elev'].astype('float64')
    gdf['min_elev'] = gdf['min_elev'].astype('float64')

    # 5. 写出新的 shapefile
    if output_shp_path is None:
        base, ext = os.path.splitext(river_shp_path)
        output_shp_path = base + "_with_elevation_stats.shp"

    gdf.to_file(output_shp_path, driver='ESRI Shapefile')

    print(f"平均、最大、最小高程已写入到: {output_shp_path}")


def add_river_start_end_elevation_from_topology(river_shp_path, dem_tif_path, output_shp_path=None):
    # 1. 读取河道矢量数据
    gdf = gpd.read_file(river_shp_path)

    # 2. 提取 DOWNSTREAM 字段，并构建上下游映射
    downstream_map = gdf.set_index("LINKNO")["DSLINKNO"].to_dict()
    linkno_to_geom = gdf.set_index("LINKNO").geometry.to_dict()

    # 3. 创建空字段列表
    start_elev_list = []
    end_elev_list = []

    # 4. 读取 DEM 数据
    with rasterio.open(dem_tif_path) as dem_src:
        dem_array = dem_src.read(1)
        transform = dem_src.transform

        def get_dem_value(pt: Point):
            row, col = dem_src.index(pt.x, pt.y)
            value = dem_array[row, col]
            return float(value) if value != dem_src.nodata else None

        # 5. 遍历河道进行起点/终点计算
        for idx, row in gdf.iterrows():
            linkno = row["LINKNO"]
            geom = row.geometry
            coords = list(geom.coords)
            start_pt = Point(coords[0])
            end_pt = Point(coords[-1])

            # 默认起点/终点为线两端
            upstream_linknos = [k for k, v in downstream_map.items() if v == linkno]
            downstream_linkno = downstream_map.get(linkno)

            # 如果有上游，则找上游的终点与当前的起点对比 → 得到起点
            if upstream_linknos:
                shared_pt = None
                for up_link in upstream_linknos:
                    up_geom = linkno_to_geom.get(up_link)
                    if up_geom:
                        shared = set(up_geom.coords) & set(geom.coords)
                        if shared:
                            shared_pt = Point(list(shared)[0])
                            break
                start_pt = shared_pt if shared_pt else start_pt

            # 如果有下游，则找下游的起点与当前的终点对比 → 得到终点
            if downstream_linkno != -1:
                dn_geom = linkno_to_geom.get(downstream_linkno)
                if dn_geom:
                    shared = set(dn_geom.coords) & set(geom.coords)
                    if shared:
                        end_pt = Point(list(shared)[0])
            else:
                # 是最下游河道，用非共享点作为终点
                shared_pts = []
                for up_link, up_geom in linkno_to_geom.items():
                    if up_link == linkno:
                        continue
                    shared = set(geom.coords) & set(up_geom.coords)
                    if shared:
                        shared_pts.extend(shared)

                # 先设定默认终点
                candidate_pts = [Point(coords[-1]), Point(coords[0])]
                for pt in candidate_pts:
                    if pt.coords[0] not in shared_pts:
                        end_pt = pt
                        break

                # 检查 end_pt 的 DEM 值是否有效
                val = get_dem_value(end_pt)
                if val is None or val == 0 or val == dem_src.nodata:
                    # 使用相邻点替代
                    fallback_pt = Point(coords[0]) if end_pt.equals(Point(coords[-1])) else Point(coords[-1])
                    fallback_val = get_dem_value(fallback_pt)
                    print(f"LINKNO={linkno} 的终点位置 {end_pt.x:.6f}, {end_pt.y:.6f} 无效（DEM缺值/为0），使用相邻点替代，高程为: {fallback_val}")
                    end_pt = fallback_pt
            # 采样高程值
            start_elev = get_dem_value(start_pt)
            end_elev = get_dem_value(end_pt)

            start_elev_list.append(round(start_elev, 3) if start_elev is not None else None)
            end_elev_list.append(round(end_elev, 3) if end_elev is not None else None)

    # 6. 添加字段
    gdf['start_elev'] = start_elev_list
    gdf['end_elev'] = end_elev_list
    gdf = gdf.astype({'start_elev': 'float64', 'end_elev': 'float64'})

    # 7. 写出新的 SHP 文件
    if output_shp_path is None:
        base, ext = os.path.splitext(river_shp_path)
        output_shp_path = base + "_with_topo_elev.shp"

    gdf.to_file(output_shp_path, driver='ESRI Shapefile')
    print(f"起点终点高程（基于拓扑）已写入到: {output_shp_path}")

def update_mongodb_field_from_shp(
    shp_path,
    field_linkno,        # e.g., 'LINKNO'
    field_source,        # e.g., 'bed_elev'（从shp中读取）
    mongo_uri,
    db_name,
    collection_name,
    mongo_match_field,   # e.g., 'link_id'（用于匹配）
    mongo_target_field   # e.g., 'bed_elev'（写入字段）
):
    # 1. 读取 SHP 文件
    gdf = gpd.read_file(shp_path)

    if field_linkno not in gdf.columns or field_source not in gdf.columns:
        raise ValueError(f"字段 {field_linkno} 或 {field_source} 不存在于 shapefile 中")

    # 2. 构建一个 LINKNO → A 字段值的映射字典
    value_map = dict(zip(gdf[field_linkno], gdf[field_source]))

    # 3. 连接 MongoDB
    client = MongoClient(mongo_uri)
    db = client[db_name]
    col = db[collection_name]

    # 4. 遍历并更新 MongoDB 中的字段
    updated_count = 0
    for linkno, value in value_map.items():
        result = col.update_one(
            {mongo_match_field: linkno},
            {"$set": {mongo_target_field: value}}
        )
        if result.modified_count > 0:
            updated_count += 1
    print(f"已更新 MongoDB 中 {updated_count} 条记录。")


def find_field_in_collections(mongo_uri, db_name, field_name):
    client = MongoClient(mongo_uri)
    db = client[db_name]
    found = []

    for coll_name in db.list_collection_names():
        if db[coll_name].find_one({field_name: {"$exists": True}}):
            found.append(coll_name)

    return found




if __name__=='__main__':
    """根据dem计算河道平均、最大、最小高程，并输出到reach_with_bedelev.shp"""
    river_shp_path = r"G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\workspace\spatial_shp\reach.shp"
    dem_tif_path = r"G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\workspace\spatial_raster\dem.tif"
    reach_with_bedelev = r"G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\workspace\spatial_shp\reach_with_bedelev.shp"
    add_river_bed_elevation(
        river_shp_path=river_shp_path,
        dem_tif_path=dem_tif_path,
        output_shp_path=reach_with_bedelev
    )

    reach_with_topo_elev = r"G:\program\seims\SEIMS_HAND\data\11.159084_48.120933\workspace\spatial_shp\reach_with_topo_elev.shp"
    """根据dem计算河道平均、最大、最小高程，并输出到out_shp_with_topo_elev.shp"""
    add_river_start_end_elevation_from_topology(
        river_shp_path=river_shp_path,
        dem_tif_path=dem_tif_path,
        output_shp_path=reach_with_topo_elev)

    """更新mongodb"""
    fields = {"mean_elev":"Bed_Mean_Elev","max_elev":"Bed_Max_Elev","min_elev":"Bed_Min_Elev"}
    for field_source,mongo_target_field in fields.items():
        update_mongodb_field_from_shp(
            shp_path=reach_with_bedelev,
            field_linkno="LINKNO",
            field_source=field_source,
            mongo_uri="mongodb://localhost:27017",
            db_name="11_159084_48_120933_longterm_model",
            collection_name="REACHES",
            mongo_match_field="SUBBASINID",
            mongo_target_field=mongo_target_field
        )
    """更新mongodb"""
    fields2 = {"start_elev":"Bed_Start_Elev","end_elev":"Bed_End_Elev"}
    for field_source,mongo_target_field in fields2.items():
        update_mongodb_field_from_shp(
            shp_path=reach_with_topo_elev,
            field_linkno="LINKNO",
            field_source=field_source,
            mongo_uri="mongodb://localhost:27017",
            db_name="11_159084_48_120933_longterm_model",
            collection_name="REACHES",
            mongo_match_field="SUBBASINID",
            mongo_target_field=mongo_target_field
        )



    """ 查找一个字段在哪个表里 """
    # collections_with_field = find_field_in_collections(
    #     mongo_uri="mongodb://localhost:27017",
    #     db_name="11_159084_48_120933_longterm_model",
    #     field_name="bed_elev"
    # )
