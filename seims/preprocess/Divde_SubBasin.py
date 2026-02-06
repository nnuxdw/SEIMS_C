import numpy as np
from osgeo import gdal,ogr,osr
import WBT.whitebox_tools
import Raster
import os
import util_ZB
import math
import pandas as pd
import sys
import json

dmove=[(0,1),(1,1),(1,0),(1,-1),(0,-1),(-1,-1),(-1,0),(-1,1)]
dmove_dic = {1: (0, 1), 2: (1, 1), 4: (1, 0), 8: (1, -1), 16: (0, -1), 32: (-1, -1), 64: (-1, 0), 128: (-1, 1)}
wbt=WBT.whitebox_tools.WhiteboxTools()
def Project84(file):

    _,geo,nodata=Raster.get_proj_geo_nodata(file)
    band=Raster.get_raster(file)
    # 定义投影
    source_srs = osr.SpatialReference()
    source_srs.ImportFromEPSG(4326)
    proj = source_srs.ExportToWkt()
    # os.remove(file)
    Raster.save_raster(file,band,proj,geo,Raster.get_raster_DataType(file),nodata)

def find_link(Stream,Dir,Dir_nodata,link_list,geo):
    # print(link_list)
    Stream_dic={}
    row,col=Stream.shape
    
    id=1
    # link_list=[]
    # for link in link_list:
        # Stream_dic.setdefault(id, [link])
        # id+=1
        
    Vis = np.zeros((row,col)) # 防止用户指定子流域出口和自动监测出来的出口重合
    print((row,col))
    new_link_list={}
    cells=[]
    link_list_index = 0
    # 自定义节点
    for link in link_list:
        # print(link)
        link_list_index += 1
        # 追溯到河道上，若超出一定距离，则舍弃
        row1,col1 = link
        if row1 > row - 1 or col1 > col - 1 or row1 < 0 or col1 < 0 or Dir[row1, col1] == -9999:
            continue
        # print((row1,col1))
        pop_cells=[(row1,col1)]
        length = 0
        while length < 5:
            length = length + 1
            pop_cell=pop_cells.pop()
            row1 = pop_cell[0]
            col1 = pop_cell[1]
            print(pop_cell)
            # 若下一个栅格是流域出口，则也不加入
            now_dir=Dir[pop_cell[0],pop_cell[1]]
            if now_dir in dmove_dic:
                next_cell = (pop_cell[0] + dmove_dic[now_dir][0], pop_cell[1] + dmove_dic[now_dir][1])
                print(next_cell)
                if Dir[next_cell[0],next_cell[1]] == -9999:
                    break
            if Stream[pop_cell[0],pop_cell[1]] != -9999 and Dir[pop_cell[0],pop_cell[1]] != -9999 and row1 <= row - 1 and col1 <= col - 1 and row1 >= 0 and col1 >= 0:
                if (pop_cell[0], pop_cell[1]) not in cells:
                    # 加入节点
                    Stream_dic.setdefault(id, [(pop_cell[0], pop_cell[1])])
                    link_list.append((pop_cell[0], pop_cell[1]))
                    cells.append((pop_cell[0], pop_cell[1]))
                    Vis[pop_cell[0], pop_cell[1]] = 1
                    print("add")
                    new_lat = (pop_cell[0] + 0.5) * geo[5] + geo[3]
                    new_lon = (pop_cell[1] + 0.5) * geo[1] + geo[0]
                    print(id,new_lon,new_lat)
                    new_link_list[link_list_index] = (new_lon, new_lat, id)
                    id+=1
                break
                # return new_lon,new_lat
            else:
                # print(pop_cell)
                now_dir=Dir[pop_cell[0],pop_cell[1]]
                if now_dir in dmove_dic:
                    next_cell = (pop_cell[0] + dmove_dic[now_dir][0], pop_cell[1] + dmove_dic[now_dir][1])
                    pop_cells.insert(0, next_cell)
                    # print(next_cell)
                else:
                    # 流域外或者流向为0
                    print(now_dir)
                    break
    print(Stream_dic)    
    # 河道节点
    for i in range(row):
        for j in range(col):
            if Stream[i,j] != -9999:
                # 检验上游
                up_cells=util_ZB.get_rever_D8(Dir,i,j)
                n=0
                
                for cell in up_cells:
                    if Stream[cell[0],cell[1]] != -9999:
                        n+=1
                          
                if n>1 or n==0:
                    if n>1:
                        # 判断周围一个是否有节点
                        isin = False
                        directions = [(-1, -1), (-1, 0), (-1, 1),
                                      (0, -1),          (0, 1),
                                      (1, -1),  (1, 0), (1, 1)]
                        for di, dj in directions:
                            ni, nj = i + di, j + dj        
                            if (ni, nj) in link_list:
                                isin = True
                                print("周围一格有河道节点")
                                break
                        if not isin:
                            link_list.append((i,j))
                            Stream_dic.setdefault(id,[(i,j)])
                            id+=1
                    # 源头节点
                    else:
                        Stream_dic.setdefault(id,[(i,j)])
                        id+=1
    print(Stream_dic)
    
    
    Vis = np.zeros((row,col))
    for id in Stream_dic:
        head=Stream_dic[id][0]
        while True:
            # print(head)
            now_dir=int(Dir[head[0],head[1]])
            Vis[head[0],head[1]] = 1
            if now_dir != Dir_nodata and now_dir != 0:
                next_cell=(head[0]+dmove_dic[now_dir][0],head[1]+dmove_dic[now_dir][1])
                # print(next_cell)
                if Dir[next_cell[0],next_cell[1]] != Dir_nodata and Vis[next_cell[0],next_cell[1]] == 0:
                    Stream_dic[id].append(next_cell)
                    Vis[next_cell[0],next_cell[1]] = 1
                    head = next_cell
                    if head in link_list:
                        break
                else:
                    break

            else:
                break
    # print(Stream_dic)

    return Stream_dic

def find_nearest_stream(lon,lat,Stream_file):

    Stream=Raster.get_raster(Stream_file)
    proj,geo,Stream_nodata=Raster.get_proj_geo_nodata(Stream_file)
    # print(geo)
    row=math.ceil((lat-geo[3])/geo[5])
    col=math.ceil((lon-geo[0])/geo[1])

    # 用bfs寻找
    pop_cells=[(row,col)]
    # print(pop_cells)
    while True:
        pop_cell=pop_cells.pop()

        if Stream[pop_cell[0],pop_cell[1]]!=Stream_nodata:
            # print(pop_cell)
            return pop_cell
        else:
            for i in range(8):
                next_cell=(pop_cell[0]+dmove[i][0],pop_cell[1]+dmove[i][1])
                pop_cells.insert(0,next_cell)



def Divide_subbasin(Dir_file,Acc_file,Stream_file,Subbasin_file,threshold=30):
    # Project(Dir_file)
    # Project(Acc_file)
    # 用户指定流域出口
    # outlet = find_nearest_stream(102.277, 34.131, Stream_file)
    # outlets=[outlet]
    outlets=[]


    Dir=Raster.get_raster(Dir_file)
    Acc=Raster.get_raster(Acc_file)
    Stream=np.zeros_like(Acc)
    Stream[:,:]=-9999
    Stream[Acc>=threshold]=1
    Stream[Acc==-9999]=-9999
    proj,geo,Dir_nodata=Raster.get_proj_geo_nodata(Dir_file)
    # find_link(Stream,Dir,Dir_nodata)

    # Raster.save_raster(r'E:\SEIMS\HLG\DATA\basin_Stream2.tif',Stream,proj,geo,gdal.GDT_Float64,-9999)
    # 给河流链接做地址
    Stream_dic = find_link(Stream, Dir, Dir_nodata, outlets, geo)
    print(Stream_dic[1])
    for id in Stream_dic:
        for cell in Stream_dic[id]:
            Stream[cell[0], cell[1]] = id
    Raster.save_raster(Stream_file,Stream,proj,geo,gdal.GDT_Float64,-9999)


    # Stream_dic = {}
    # Stream_dic.setdefault(0, [outlet])



    # 划子流域
    Vis=np.zeros_like(Stream)
    Subbasin=np.zeros_like(Stream)
    Subbasin[:,:]=-9999
    # print(len(Stream_dic))
    for id in Stream_dic:

        stream=Stream_dic[id].copy()
        # print(stream)
        while stream:
            pop_cell=stream.pop()
            Subbasin[pop_cell[0],pop_cell[1]]=id
            up_cells=util_ZB.get_rever_D8(Dir,pop_cell[0],pop_cell[1])
            for cell in up_cells:
                if Stream[cell[0],cell[1]]==-9999:
                    # print(cell)
                    stream.insert(0,cell)

    Raster.save_raster(Subbasin_file,Subbasin,proj,geo,gdal.GDT_Float64,-9999)

def Divide_subbasin_user(Dir_file,Acc_file,Stream_file,json_obj,Subbasin_file,threshold=30):

    # # 读取csv文件
    # df=pd.read_csv(outlets_file)
    # # print(df.values)
    # con=df.values
    outlets=[]

    # Project(Dir_file)
    # Project(Acc_file)
    
    # 用户指定流域出口
    # outlet = find_nearest_stream(102.277, 34.131, Stream_file)
    # outlets=[outlet]


    Dir=Raster.get_raster(Dir_file)
    Acc=Raster.get_raster(Acc_file)
    Stream=np.zeros_like(Acc)
    Stream[:,:]=-9999
    Stream[Acc>=threshold]=1
    proj,geo,Dir_nodata=Raster.get_proj_geo_nodata(Dir_file)
    
    # find_link(Stream,Dir,Dir_nodata)
    Stream_file1 = Stream_file.replace("basin_Stream.tif", "basin_Stream_1.tif")
    print(Stream_file1)
    Raster.save_raster(Stream_file1,Stream,proj,geo,gdal.GDT_Float64,-9999)
    
    # for lon_lat in json_obj:
        # # print(lon_lat['lon'])
        # outlet=find_nearest_stream(lon_lat['lon'],lon_lat['lat'],Stream_file)
        # outlets.append(outlet)
    
    for lon_lat in json_obj:
        # print(lon_lat['lon'])
        # outlet=find_nearest_stream(lon_lat['lon'],lon_lat['lat'],Stream_file)
        
        row=math.ceil((float(lon_lat['lat']) - geo[3]) / geo[5])
        col=math.ceil((float(lon_lat['lon']) - geo[0]) / geo[1])
        outlets.append((row,col))
    
    
    # 给河流链接做地址
    Stream_dic = find_link(Stream, Dir, Dir_nodata, outlets, geo)
    # print(Stream_dic.keys())
    # print(Stream_dic[6])
    for id in Stream_dic:
        for cell in Stream_dic[id]:
            Stream[cell[0], cell[1]] = id
    Raster.save_raster(Stream_file,Stream,proj,geo,gdal.GDT_Float64,-9999)


    # Stream_dic = {}
    # Stream_dic.setdefault(0, [outlet])



    # 划子流域
    Vis=np.zeros_like(Stream)
    Subbasin=np.zeros_like(Stream)
    Subbasin[:,:]=-9999
    # print(len(Stream_dic))
    for id in Stream_dic:

        stream=Stream_dic[id].copy()
        # print(stream)
        while stream:
            pop_cell=stream.pop()
            Subbasin[pop_cell[0],pop_cell[1]]=id
            up_cells=util_ZB.get_rever_D8(Dir,pop_cell[0],pop_cell[1])
            for cell in up_cells:
                if Stream[cell[0],cell[1]]==-9999:
                    # print(cell)
                    stream.insert(0,cell)

    Raster.save_raster(Subbasin_file,Subbasin,proj,geo,gdal.GDT_Float64,-9999)

def project(input_file,output_file):
    # 定义输入文件路径和名称
    # input_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem.tif"
    # output_file = r"E:\WEB\basins\test\xujs\1018\workspace\spatial_raster\dem2.tif"

    projection = 'PROJCS["Beijing 1954 / 3-degree Gauss-Kruger zone 39",GEOGCS["Beijing 1954",DATUM["Beijing_1954",SPHEROID["Krassowsky 1940",6378245,298.2999999999998,AUTHORITY["EPSG","7024"]],AUTHORITY["EPSG","6214"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",117],PARAMETER["scale_factor",1],PARAMETER["false_easting",39500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]]]'
    # projection = 'PROJCS["Asia_North_Albers_Equal_Area_Conic",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433],AUTHORITY["EPSG","4326"]],PROJECTION["Albers_Conic_Equal_Area"],PARAMETER["standard_parallel_1",15],PARAMETER["standard_parallel_2",65],PARAMETER["latitude_of_center",30],PARAMETER["longitude_of_center",95],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]]]'
    
    # print(projection)
    # 创建目标投影对象（beijing）
    target_srs = osr.SpatialReference()
    target_srs.ImportFromWkt(projection)
     
    # 创建源空间参考对象(WGS1984)
    source_srs = osr.SpatialReference()
    source_srs.ImportFromEPSG(4326)
     

    # 设置重采样方法为双线性插值
    resample_alg = None

    # 重投影
    gdal.Warp(output_file, input_file, 
              format='GTiff', resampleAlg=resample_alg, srcNodata=-9999,
              srcSRS=source_srs.ExportToWkt(),
              dstSRS=target_srs.ExportToWkt()
              )

def get_digit_count(n):
    if n == 0:
        return 1
    count = 0
    while n > 0:
        n //= 10
        count += 1
    return count

def find_top5_percent(matrix, return_values=True):
    arr = np.array(matrix[matrix != -9999]).flatten()
    if len(arr) == 0:
        return (None, []) if return_values else None
    
    # sorted_arr = np.sort(arr)
    # threshold_index = int(len(sorted_arr) * 0.9985) - 1
    # threshold = sorted_arr[threshold_index]
    
    threshold = np.percentile(arr, 99.85) 
    
    if return_values:
        return threshold, sorted_arr[threshold_index+1:].tolist()
    return threshold  

if __name__=='__main__':
    # input:
    # 流向
    Dir_file = sys.argv[1]
    # Dir_file=r'E:\WEB\basins\test\xujs\mulanxi\data_prepare\spatial\basin_dir.tif'
    # 汇流累积量
    Acc_file = sys.argv[2]
    # Acc_file=r'E:\WEB\basins\test\xujs\mulanxi\data_prepare\spatial\basin_upa.tif'
    # 子流域出口
    outlets = sys.argv[3]
    # outlets_file=r'E:\WEB\basins\test\xujs\mulanxi\data_prepare\spatial\outlets.csv'
    outlets = outlets.replace("'", '"')
    json_obj = json.loads(outlets)
    
    
    
    
    # output:
    # 河网
    Stream_file1 = sys.argv[4]
    # Stream_file1 = r'E:\WEB\basins\test\xujs\mulanxi\data_prepare\spatial\basin_Stream.tif'
    # 子流域_tif
    Subbasin_file1 = sys.argv[5]
    # Subbasin_file1 = r'E:\WEB\basins\test\xujs\mulanxi\data_prepare\spatial\Subbasin.tif'
    
    # 子流域_shp
    Subbasin_file2 = sys.argv[6]
    # 河网_shp
    Stream_file2 = sys.argv[7]
    # 河网阈值
    stramValue = sys.argv[8]
    
    # if stramValue=="":
        # print("stramValue is None")
        # # 打开栅格文件   
        # array = Raster.get_raster(Acc_file)  
        # # 获取栅格数据的最大值  
        # max_value = np.max(array)
        # print(max_value)
        # if max_value < 10000:
            # stramValue = max_value
        # else:
            # stramValue = (10 ** (get_digit_count(max_value) - 2)) * 1.5
    if stramValue=="":
        # print("stramValue is None")
        # 打开栅格文件   
        array = Raster.get_raster(Acc_file) 
    
        stramValue = find_top5_percent(array,False)
        print("streamValue:" + str(stramValue))
    
    projedtFile = sys.argv[9]
    
    
    
    # # 输入投影为beijing
    # Dir_file1 = projedtFile + r'\basin_dir.tif'
    # project(Dir_file,Dir_file1)
    # Acc_file1 = projedtFile + r'\basin_upa.tif'
    # project(Acc_file,Acc_file1)
    # #Todo：投影outlets
    
    
    
    # ************ 1、默认划分 **************
    # if
    # Divide_subbasin(Dir_file,Acc_file,Stream_file1,Subbasin_file1)
    
    
    #************* 2、用户指定子流域出口 ******
    Divide_subbasin_user(Dir_file,Acc_file,Stream_file1,json_obj,Subbasin_file1,int(stramValue))
    # Divide_subbasin(Dir_file,Acc_file,Stream_file1,Subbasin_file1,int(stramValue))
    
    # 定义投影为wgs1984
    Project84(Stream_file1)
    Project84(Subbasin_file1)
    
    wbt.raster_to_vector_polygons(Subbasin_file1,Subbasin_file2)   # 子流域矢量化
    wbt.raster_to_vector_lines(Stream_file1,Stream_file2)   # 河网矢量化。仅用于前端渲染
    # Project(Dir_file)