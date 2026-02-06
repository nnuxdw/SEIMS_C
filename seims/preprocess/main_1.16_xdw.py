# -*- coding: utf-8 -*-
import os
import sys
import argparse
import warnings
import traceback
import shutil  # 引入shutil以防需要移动文件

try:
    import Bathymetry
    import util_lp
    import ZB_hillslope_workflow
    import calc_hand
    import calc_hand_by_lakeandwetland
    import new_discretize
    import util_ZB
    import BigScale_hand
except ImportError as e:
    print(f"[Error] 模块导入失败: {e}")
    sys.exit(1)

warnings.filterwarnings("ignore")

# ================= 服务器环境配置 =================
# BASE_ROOT_DIR = "/data/xujs/WEB/basins/test"
BASE_ROOT_DIR = "/data/user/longp/wise_data/wise_project"
GLOBAL_LAKE_DEPTH_FILE = "/data/user/longp/data/lake_depth_max_file/GLOBathy_basic_parameters(ALL_LAKES).csv"
GLOBAL_GLAD_SOURCE = "/data/user/longp/data/GLAD"


# ===============================================
def create_empty_lake_raster(dem_file, out_lake_file):
    """根据 DEM 创建一个同尺寸、同投影的空湖泊栅格"""
    import rasterio
    import numpy as np

    with rasterio.open(dem_file) as src:
        profile = src.profile.copy()
        nodata = profile.get("nodata", -9999)
        data = np.full((src.height, src.width), nodata, dtype=profile["dtype"])
        profile.update(count=1, nodata=nodata, compress="lzw")

        # 确保输出目录存在
        os.makedirs(os.path.dirname(out_lake_file), exist_ok=True)

        with rasterio.open(out_lake_file, "w", **profile) as dst:
            dst.write(data, 1)


def run_main_process(user_name, project_name):
    print(f"{'=' * 50}")
    print(f"开始处理任务: {project_name} ({user_name})")

    # --- 1. 定义路径 ---

    # [Input] 输入目录: 读取基础空间数据 (spatial_raster)
    input_dir = os.path.join(BASE_ROOT_DIR, user_name, project_name, "workspace", "spatial_raster")

    # [Output] 输出目录: 存放所有生成的数据 (rundata)
    output_dir = os.path.join(BASE_ROOT_DIR, user_name, project_name, "workspace", "rundata")

    print(f"输入路径 (只读): {input_dir}")
    print(f"输出路径 (写入): {output_dir}")

    # 检查输入目录
    if not os.path.exists(input_dir):
        print(f"[Error] 输入目录不存在: {input_dir}")
        sys.exit(1)

    # 创建输出目录
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"已创建输出目录: {output_dir}")

    # --- 2. 变量映射 ---
    # 大尺度阈值区间
    breaks =  [ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20, 30, 40, 50,60,70,80,90, 100,110,120,130,140, 150,
                160,170,180,190,200,250,300,350,400,450,500,550,600,650,700,750,800,850,900,950,1000,1100,1200,1300,1400,1500,
                1600,1700,1800,1900,2000,2500,3000,3500,4000,4500,5000]
    # 超细阈值区间
    # breaks = [0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9, 1,1.5, 2,2.5, 3,3.5, 4,5,6,7, 8,9,10, 12,15, 20, 25,30,35, 40, 45,50,60,70,80,90,100,110,120,130,140,150,200,250,300, 500, 1000, 2000, 5000]

    # >>> A. 输入文件 (从 input_dir 读取) <<<
    # 根据你的要求，严格限定这些文件从 spatial_raster 读取
    dem_file = os.path.join(input_dir, 'dem.tif')
    dir_file = os.path.join(input_dir, 'flow_dir.tif')
    stream_file = os.path.join(input_dir, 'stream_link.tif')
    acc_file = os.path.join(input_dir, 'acc.tif')
    subbasin_tif_path = os.path.join(input_dir, "subbasin.tif")

    # 原始湖泊文件 (通常在项目根目录)
    orgin_lake_file = os.path.join(BASE_ROOT_DIR, user_name, project_name, 'lake2.tif')

    # >>> B. 输出文件 (全部指向 output_dir) <<<
    Basin_path = os.path.join(output_dir, '1')
    # 自动创建文件夹逻辑
    if not os.path.exists(Basin_path):
        os.makedirs(Basin_path)
        print(f"已自动创建子流域文件夹: {Basin_path}")
    # 湖泊相关
    lake_SWBD_file = os.path.join(output_dir, "SWBD_lake.tif")
    lake_shp_dir = os.path.join(output_dir, "lakeshp")  # 专门的文件夹
    lake_shp_file = os.path.join(lake_shp_dir, 'SWBD_lake.shp')
    Longterm_lake_file = os.path.join(output_dir, "Longterm_lake.tif")
    GLADlakefile = os.path.join(output_dir, "GLAD_lake.tif")
    Permanent_lake_file = os.path.join(output_dir, "Permanent_lake.tif")

    # 水深与 HAND 相关
    lake_depth_file = os.path.join(output_dir, 'depth.csv')
    Bathymetry_file = os.path.join(output_dir, 'Bathymetry.tif')
    HAND_file = os.path.join(output_dir, "hand.tif")
    HAND_therehold_file = os.path.join(output_dir, "HAND_therehold.txt")

    # 汇流与 HRU 相关
    lake_confluence_dir = os.path.join(output_dir, "LakeHRU")
    confluence_file = os.path.join(output_dir, "confluence.txt")
    HRU_file = os.path.join(output_dir, "HRU.tif")

    # 确保子文件夹存在
    os.makedirs(lake_shp_dir, exist_ok=True)
    os.makedirs(lake_confluence_dir, exist_ok=True)

    # --- 3. 执行逻辑 ---
    try:
        print(">>> Step 1: 处理湖泊...")
        has_lake = os.path.exists(orgin_lake_file)
        if has_lake:
            print(f"使用湖泊文件: {orgin_lake_file}")
            # 输出文件都在 output_dir
            util_lp.extract_lake_by_dem_extent(dem_file, orgin_lake_file, lake_SWBD_file)
            util_lp.AlertlakeBySubbasin(lake_SWBD_file, subbasin_tif_path)
            util_lp.raster_to_vector(lake_SWBD_file, lake_shp_file)
            util_lp.GetlakeDepth_maxBylakeid(lake_SWBD_file, GLOBAL_LAKE_DEPTH_FILE)
            Bathymetry.calculate_bathymetry_merged(lake_shp_file, lake_depth_file, dem_file)
            util_lp.AlertSWBDlakeBySubbasin(lake_SWBD_file, subbasin_tif_path)
        else:
            print("未检测到湖泊文件，生成空栅格")
            create_empty_lake_raster(dem_file, lake_SWBD_file)

        print(">>> Step 2: 生成坡面 HAND...")
        # 关键点：这里传入了 input 的 dem/dir/stream 和 output 的 lake_SWBD_file
        # 你的 new_discretize 模块需要根据参数生成 HAND。
        # 如果模块内部没有提供指定 HAND 输出路径的参数，它可能会生成在 DEM 旁边或 Lake 旁边。
        # new_discretize.divide_lake_hillslope(dem_file, dir_file, lake_SWBD_file, stream_file, acc_file)
        util_lp.generate_hand_raster(dem_file, dir_file, lake_SWBD_file, stream_file,subbasin_tif_path)

        # [防错补丁]: 检查 hand.tif 是否被生成到了 input_dir，如果是，强制移动到 output_dir
        wrong_hand_path = os.path.join(input_dir, "hand.tif")
        if os.path.exists(wrong_hand_path):
            print(f"[注意] 检测到 hand.tif 生成在输入目录，正在移动到: {HAND_file}")
            shutil.move(wrong_hand_path, HAND_file)
        elif not os.path.exists(HAND_file):
            # 如果 input 目录没有，output 目录也没有，可能模块生成在了脚本运行目录，或者生成在了 lake_SWBD_file 所在目录(如果是后者则正确)
            # 假设模块逻辑是“生成在 lake_SWBD_file 同级目录”，那我们已经安全了。
            # 如果还是找不到，尝试查找默认名称
            pass

        print(">>> Step 3: 检查自相交...")
        util_ZB.Check_self_intersect(Basin_path)  # Basin_path 指向 input_dir

        print(">>> Step 4: 更新 HAND 水深...")
        # 确保 HAND_file 存在 (如果是从 step 2 移动过来的或者直接生成的)
        if has_lake and os.path.exists(Bathymetry_file):
            util_lp.updateHandBybathymetry(HAND_file, Bathymetry_file)

        print(">>> Step 5: 划分 HAND 带...")
        BigScale_hand.calc_hand_bythreshold_subbasin(HAND_file, subbasin_tif_path, stream_file, lake_SWBD_file, breaks)

        print(">>> Step 6: 处理 GLAD 与长期水体...")
        util_lp.ReNameLake(lake_SWBD_file)
        util_lp.create_masked_glad_from_dem(dem_file, GLOBAL_GLAD_SOURCE, GLADlakefile)
        util_lp.GetMinlakeFormGLAD(GLADlakefile, Longterm_lake_file)

        print(">>> Step 7: 划分 HRU...")
        util_lp.New_divideHruByHand(HAND_file, Longterm_lake_file, Permanent_lake_file, 16, 10, 6, 5000, 15000, 400)

        print(">>> Step 8: 合并汇流与更新编号...")
        util_lp.New_merge_confluence_for_lake(confluence_file, lake_confluence_dir, HRU_file)
        util_lp.new_update_subbasin_ids(confluence_file, HRU_file, subbasin_tif_path)
        util_lp.update_hru_and_confluence(HRU_file, confluence_file)

        print(">>> Step 9: 计算 HRU 高程与淹没顺序...")
        util_lp.calc_HRU_threshold(HRU_file, HAND_file, HAND_therehold_file, breaks)
        util_lp.calc_floodStep(confluence_file, HAND_therehold_file)

        print(f"\n[Success] 处理完成！数据已保存至: {output_dir}")

    except Exception as e:
        print(f"\n[Fatal Error] 错误: {e}")
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-u', '--user', type=str, required=True)
    parser.add_argument('-p', '--project', type=str, required=True)
    args = parser.parse_args()

    run_main_process(args.user, args.project)
