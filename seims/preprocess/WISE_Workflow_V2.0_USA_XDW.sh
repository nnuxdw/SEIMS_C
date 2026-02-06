#!/bin/bash

# 单个流域批量建模自动化脚本 版本V2.0
# 项目文件目录：项目根目录 + 用户名称 + 项目名称


# -------------------------- 1. 基础配置 --------------------------
# 【需修改】流域项目根目录、项目名称、用户名称、出口坐标
projectRoot="/data/user/longp/wise_data/wise_project" # 项目根目录，【若自己新建根目录，请拷贝/data/xujs/WEB/basins/test下的demo_modelio_configs，demo_model_configs，lookup，runtime_log，other_files】
username='XDW'                           # 用户名称
codePath="/data/xujs/code"        # 预处理代码位置
xdwcodePath="/data/user/longp/wise_data/script/SEIMS"  #大卫淹没SEIMS代码位置
lp_codePath="/data/user/longp/wise_data/script"  #金帅脚本的流域划分部分被写死了
port="27017"                             # 数据库端口
WISE_Path="${codePath}/SEIMS-hulugou"    # WISE模型代码位置
# 遍历多个流域出口
csv_file="/data/user/longp/wise_data/sitesfile/outlet_USA_Watersheds.csv"


mapfile -t lines < "$csv_file"
unset lines[0]

# 记录脚本开始时间（用于计算总耗时）
start_time=$(date +%s.%N)

for line in "${lines[@]}"; do
	IFS=, read -r col0 col1 col2 rest <<< "$line"
	projectname="US_$col0"
	projectname=$(echo "$projectname" | tr -d '\r')
	lon="$col1"                          # 第4列是流域出口经度
	lon=$(echo "$lon" | tr -d '\r')
	lat="$col2"                          # 第3列是流域出口纬度
	lat=$(echo "$lat" | tr -d '\r')
	echo "$lon"
	echo "$lat"
	# 打印项目名称，查看是否正确
	echo "当前项目名称：$projectname"

	# -------------------------- 2. 创建项目目录结构 --------------------------
	# 定义关键路径变量
	userPath="${projectRoot}/${username}"                                        # 用户文件目录
	projectPath="${projectRoot}/${username}/${projectname}"                      # 项目文件目录
	projectSpatialFileSource="${projectPath}/data_prepare/spatial"               # 中间数据目录
	climateFileSource="${projectPath}/data_prepare/climate"                      # 气候数据目录
	lookupFileSource="${projectPath}/data_prepare/lookup"                        # 查找表目录
	observedFileSource="${projectPath}/data_prepare/observed"                    # 观测数据目录
	scenarioFileSource="${projectPath}/data_prepare/scenario"                    # 情景分析目录
	model_configsFileSource="${projectPath}/model_configs"                       # 模型配置目录
	longterm_modelFileSource="${projectPath}/${projectname}_longterm_model"      # 模型运行目录
	workspaceFileSource="${projectPath}/workspace"                               # 工作空间目录
	spatial_shpPath="${workspaceFileSource}/spatial_shp"                         # 空间矢量目录

	echo "当前项目文件目录：$projectPath"

	# 创建所有必要的目录（-p确保目录存在）
	mkdir -p "$userPath" "$projectPath" "$projectSpatialFileSource" "$climateFileSource" \
			 "$lookupFileSource" "$observedFileSource" "$scenarioFileSource" \
			 "$model_configsFileSource" "$longterm_modelFileSource" \
			 "$workspaceFileSource" "$spatial_shpPath"

	# -------------------------- 3. 复制配置文件 --------------------------
	echo "复制模型配置文件..."

	# 复制模型配置文件（preprocess.ini等）
	preprocessConfig="${projectRoot}/demo_model_configs/preprocess.ini"
	postprocessConfig="${projectRoot}/demo_model_configs/postprocess.ini"
	runmodelConfig="${projectRoot}/demo_model_configs/runmodel.ini"
	calibrationConfig="${projectRoot}/demo_model_configs/calibration.ini"
	scenario_analysisConfig="${projectRoot}/demo_model_configs/scenario_analysis.ini"
	sensitivity_analysisConfig="${projectRoot}/demo_model_configs/sensitivity_analysis.ini"

	cd "$model_configsFileSource" || exit
	cp "$preprocessConfig" "$postprocessConfig" "$runmodelConfig" \
	   "$calibrationConfig" "$scenario_analysisConfig" "$sensitivity_analysisConfig" ./

	# 复制模型输入输出配置文件
	fileinConfig="${projectRoot}/demo_modelio_configs/file.in"
	fileoutConfig="${projectRoot}/demo_modelio_configs/file.out"
	paramcaliConfig="${projectRoot}/demo_modelio_configs/param.cali"
	# 【修改：考虑坡面湖泊和不考虑模块文件有区别】
	configfigConfig="${projectRoot}/demo_modelio_configs/config_xdw.fig"
	# 修改这里有问题顺序反了
	cd "$longterm_modelFileSource" || exit
	cp "$fileinConfig" "$fileoutConfig" "$paramcaliConfig" "$configfigConfig" ./
	mv config_xdw.fig config.fig

	# 复制情景分析相关文件
	areal_struct_managementConfig="${projectRoot}/demo_modelio_configs/areal_struct_management.csv"
	BMP_indexConfig="${projectRoot}/demo_modelio_configs/BMP_index.csv"
	BMP_scenariosConfig="${projectRoot}/demo_modelio_configs/BMP_scenarios.csv"
	plant_managementConfig="${projectRoot}/demo_modelio_configs/plant_management.csv"

	cd "$scenarioFileSource" || exit
	cp "$areal_struct_managementConfig" "$BMP_indexConfig" \
	   "$BMP_scenariosConfig" "$plant_managementConfig" ./

	# 复制气候变量文件
	climateVariablesFileSource="${projectRoot}/demo_modelio_configs/Variables.csv"
	cd "$climateFileSource" || exit
	cp "$climateVariablesFileSource" ./

	# -------------------------- 4. 提取流域范围 --------------------------
	echo "提取流域范围..."

	# 生成流域提取配置文件
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/edit_ini.py" "$projectname" "${username}" "${projectRoot}" "${WISE_Path}" "${port}"

	# 检查是否已存在流域边界文件，若不存在则执行提取
	basinshp="${projectSpatialFileSource}/basin.shp"
	if [ ! -e "$basinshp" ]; then
		# # 【可选1】用户上传 -------------------------------------------------------------------------------------------------------------------------
		# dem="${projectSpatialFileSource}/basin_elv.tif"
		# dir="${projectSpatialFileSource}/basin_dir.tif"
		# acc="${projectSpatialFileSource}/basin_upa.tif"

		# # 【可选1.1】用户上传未处理的DEM时打开 --------------------------------------------------
		# # 【修改】用户上传DEM，不使用全球数据集
		# upload_dem="/data/xujs/srp/test/basin_elv.tif"
		# cd $projectSpatialFileSource
		# cp $upload_dem ./
		# # 【修改】注意更改文件名
		# mv basin_elv.tif basin_elv.tif

		# # # 【可选1.1】DEM处理：burn-in。用户上传真实河网shp时打开
		# # river_shp="/data/xujs/srp/LT/StreamNetwork_WGS1984.shp"
		# # echo "/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=FillBurn -v --dem=$upload_dem --streams=$river_shp -o=$dem"
		# # /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=FillBurn -v --dem="$upload_dem" --streams="$river_shp" -o="$dem"
		# # # ---------------------

		# # 填洼
		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=FillDepressionsWangAndLiu -v --dem="$dem" -o="$dem" --fix_flats --compression="NONE"
		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=SetNodataValue -v -i="$dem" -o="$dem" --back_value="-9999"
		# /data/xujs/anaconda/envs/SEIMS/bin/python /data/xujs/code/changeTIFcompression.py "$dem"
		# cd $projectPath
		# cp $dem ./
		# mv basin_elv.tif dem.tif
		# # 生成流向dir
		# echo "/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=D8Pointer -v --dem=$dem -o=$dir --compression=NONE --esri_pntr"
		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=D8Pointer -v --dem="$dem" -o="$dir" --compression="NONE" --esri_pntr
		# /data/xujs/anaconda/envs/SEIMS/bin/python /data/xujs/code/proprecess_dir.py "$projectSpatialFileSource"
		# cd $projectPath
		# cp $dir ./
		# mv basin_dir.tif dir.tif
		# # 生成汇流累积量acc
		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=D8FlowAccumulation -v --input="$dir" -o="$acc" --out_type="cells" --esri_pntr --compression="NONE"
		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=SetNodataValue -v -i="$acc" -o="$acc" --back_value="-9999"
		# cd $projectPath
		# cp $acc ./
		# mv basin_upa.tif acc.tif
		# # --------------------------------------------------------------------------

		# #【可选1.2】用户上传处理好（burn-in & 填洼）的DEM时打开。需要自备landuse，【soiltype】---------------------
		# # 【修改】上传处理好的DEM,dir,acc的路径。三个tif行列要相同，nodata值为-9999
		# upload_dem_preprocess="/data/xujs/srp/LT/preprocess/DEM_30m_WGS1984_1.tif"
		# upload_dir_preprocess="/data/xujs/srp/LT/preprocess/DEM_30m_WGS1984_fdr.tif"
		# upload_acc_preprocess="/data/xujs/srp/LT/preprocess/Acc.tif"

		# cd $projectSpatialFileSource
		# cp $upload_dem_preprocess ./
		# # 【修改】注意更改文件名
		# mv DEM_30m_WGS1984_1.tif basin_elv.tif
		# cd $projectPath
		# cp $dem ./
		# mv basin_elv.tif dem.tif

		# cd $projectSpatialFileSource
		# cp $upload_dir_preprocess ./
		# # 【修改】注意更改文件名
		# mv DEM_30m_WGS1984_fdr.tif basin_dir.tif
		# cd $projectPath
		# cp $dir ./
		# mv basin_dir.tif dir.tif

		# cd $projectSpatialFileSource
		# cp $upload_acc_preprocess ./
		# # 【修改】注意更改文件名
		# mv Acc.tif basin_upa.tif
		# cd $projectPath
		# cp $acc ./
		# mv basin_upa.tif acc.tif
		# #-------------------------------------------------------------------------------

		# # 提取临时河网
		# # stream="${projectSpatialFileSource}/basin_Stream.tif"
		# # /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=ExtractStreams -v --flow_accum="$acc" -o="$stream" --threshold="$threshold"
		# # /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=Reclass -v -i="$dir" -o="$dir" --reclass_vals="-9999;0.0;0.1"

		# echo "latlon2shp"
		# outletshp="${projectRoot}/${username}/$projectname/workspace/spatial_shp/outlet.shp"
		# /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/latlon2shp.py "$lat" "$lon" "$outletshp"

		# # 提取流域范围，根据流域范围裁剪 dem，dir，acc
		# cd $projectSpatialFileSource
		# basin="${projectSpatialFileSource}/basin.tif"
		# basin1="${projectSpatialFileSource}/basin_1.tif"
		# basinshp="${projectRoot}/${username}/${projectname}/data_prepare/spatial/basin.shp"
		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=UnnestBasins -v --d8_pntr="$dir" --pour_pts="$outletshp" -o="$basin" --esri_pntr
		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=SetNodataValue -v -i="$basin1" -o="$basin" --back_value="-9999"
		# # mv "$basin1" "$basin"

		# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=RasterToVectorPolygons -v --input="$basin" -o="$basinshp"

		# dir1="${projectSpatialFileSource}/basin_dir1.tif"
		# dir2="${projectSpatialFileSource}/basin_dir2.tif"
		# /data/xujs/anaconda/envs/python36_2/bin/python /data/xujs/code/clipRaster2.py "$basin" "$dir" "$dir1" "$dir2"
		# # rm "$dir"
		# rm "$dir1"
		# # mv basin_dir2.tif basin_dir.tif

		# acc1="${projectSpatialFileSource}/basin_upa1.tif"
		# acc2="${projectSpatialFileSource}/basin_upa2.tif"
		# /data/xujs/anaconda/envs/python36_2/bin/python /data/xujs/code/clipRaster2.py "$basin" "$acc" "$acc1" "$acc2"
		# # rm "$acc"
		# rm "$acc1"
		# # mv basin_upa2.tif basin_upa.tif

		# dem1="${projectSpatialFileSource}/basin_elv1.tif"
		# dem2="${projectSpatialFileSource}/basin_elv2.tif"
		# /data/xujs/anaconda/envs/python36_2/bin/python /data/xujs/code/clipRaster2.py "$basin" "$dem" "$dem1" "$dem2"
		# # rm "$dem"
		# rm "$dem1"
		# # mv basin_elv2.tif basin_elv.tif

		# # crop
		# basin_temp="${projectSpatialFileSource}/basin_temp.tif"
		# /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/crop_tif_9999.py "$basin" "$basin_temp"
		# mv basin_temp.tif basin.tif

		# /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/crop_tif_9999.py "$dem2" "$dem"
		# /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/crop_tif_9999.py "$acc2" "$acc"
		# /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/crop_tif_9999.py "$dir2" "$dir"

		# rm "$dem2"
		# rm "$acc2"
		# rm "$dir2"
		# # -------------------------------------------------------------------------------------------------------------------------

		# 【可选2】默认全球数据集--------------------------
		echo "确定流域位置，写入配置文件中..."
		/data/xujs/anaconda/envs/python36/bin/python "${lp_codePath}/determineLocation.py" "$lon" "$lat" "$projectname" "${username}"
		echo "提取流域边界，流域基础数据 dem,dir,acc,landuse,soiltype..."
		/data/xujs/anaconda/envs/python36/bin/python "${lp_codePath}/extractBasin.py" "$lon" "$lat" "$projectSpatialFileSource" "$projectname" "${username}"
		# ---------------------------
	fi

	# 投影转换和出口点生成
	echo "投影转换到WGS84..."
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/projectRaster2WGS84.py" "$projectSpatialFileSource"

	echo "生成出口点shapefile..."
	outletshp="${spatial_shpPath}/outlet.shp"
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/latlon2shp.py" "$lat" "$lon" "$outletshp"

	# 复制流域边界文件到工作空间
	echo "复制流域边界文件..."
	cd "$spatial_shpPath" || exit
	cp "$basinshp" "${basinshp%.*}".{dbf,prj,shx} ./

	# -------------------------- 5. 水文处理 --------------------------
	echo "水文处理..."

	# 复制运行时日志目录
	runlogFilesPath="${projectRoot}/runtime_log"
	newrunlogFilesPath="${workspaceFileSource}/runtime_log"
	mkdir -p "$newrunlogFilesPath"
	cd "$newrunlogFilesPath" || exit
	cp -r "$runlogFilesPath" ./

	# 使用WhiteboxTools计算流向和累积量
	echo "计算流向和累积量..."
	dirPath="${projectSpatialFileSource}/basin_dir.tif"
	accPath1="${projectSpatialFileSource}/basin_upa1.tif"
	accPath="${projectSpatialFileSource}/basin_upa2.tif"

	# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
		# -r=D8FlowAccumulation -v --input="$dirPath" -o="$accPath1" --out_type="cells" --esri_pntr
	# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
		# -r=SetNodataValue -v -i="$accPath1" -o="$accPath" --back_value=-9999

	# 生成流域外接范围
	echo "生成流域外接范围..."
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/creat_TS.py" "$projectPath"

	# -------------------------- 6. 子流域划分 --------------------------
	echo "子流域划分..."

	# 【可选】湖泊处理（如不需要考虑湖泊请注释）
	lakeshp="${projectSpatialFileSource}/lake.shp"
	lakeshp_inbasin="${projectSpatialFileSource}/lake_in.shp"
	hydrolake="/data/xujs/data/HydroLakes/HydroLAKES_polys_v10.shp"
	laketif="${projectSpatialFileSource}/lake.tif"

	if [ ! -e "$lakeshp" ]; then
		echo "裁剪湖泊数据..."
		/data/xujs/anaconda/envs/python36/bin/python "${codePath}/clipShp.py" "$hydrolake" "$basinshp" "$lakeshp" # 粗裁剪

		if [ -e "$lakeshp" ]; then
			/data/xujs/anaconda/envs/python36/bin/python "${codePath}/fineClipShp.py" "$lakeshp" "$basinshp" "$lakeshp_inbasin" # 精裁剪
			rm -f "$laketif"
		fi
	fi

	if [ -e "$lakeshp_inbasin" ]; then
		echo "湖泊矢量转栅格..."
		/data/xujs/anaconda/envs/python36/bin/python "${codePath}/vector2raster.py" "$projectSpatialFileSource" "$laketif"
	fi
	# -----------------------------

	# 子流域划分（考虑/不考虑湖泊）
	if [ -e "$laketif" ]; then
		echo "考虑湖泊的子流域划分..."
		# echo "$projectname,$lakeshp" >> /data/xujs/srp/Arctic/Arctic/greatRivers/851_sourceBasin/havelake_sourcebasin.csv # 记录有湖泊流域

		/data/xujs/anaconda/envs/python36/bin/python ${codePath}/project2wgs1984.py "$laketif"

		dirtif="${projectRoot}/${username}/$projectname/data_prepare/spatial/basin_dir.tif"
		accPath="${projectSpatialFileSource}/basin_upa.tif"
		streamPath="${projectSpatialFileSource}/basin_Stream.tif"
		subbasintif="${projectRoot}/${username}/$projectname/data_prepare/spatial/subBasin.tif";
		subbasinshp="${projectRoot}/${username}/$projectname/workspace/spatial_shp/subbasin.shp";
		streamshp="${projectRoot}/${username}/$projectname/workspace/spatial_shp/basin_Stream.shp";
		projedtfile="${projectRoot}/${username}/$projectname/workspace/projectFile";

		# mkdir -p "$projedtfile"

		laketif1="${projectRoot}/${username}/$projectname/lake2.tif"
		/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=Resample -v -i="$laketif" --base="$dirtif" -o="$laketif1" --method="nn"

		GRDC_station="[]"
		# # 【需修改,可选】如不需要指定子流域出口请注释。修改子流域出口csv文件路径，csv请按格式：/data/xujs/srp/zhongshanqiao/tangnaihai.csv
		# GRDC_station=$(awk -F, '
		# BEGIN { print "[" }
		# NR > 1 {
			# gsub(/[ \t\r\n]+$/, "", $3); gsub(/^[ \t\r\n]+/, "", $3)
			# gsub(/[ \t\r\n]+$/, "", $4); gsub(/^[ \t\r\n]+/, "", $4)
			# if (NR > 2) printf ",\n"
			# printf "  {\"lat\":%s, \"lon\":%s}", $3, $4
		# }
		# END { print "\n]" }

		# ' /data/xujs/srp/Arctic/Arctic/Arctic_bigRiver_GRDC_station_filerinlake.csv)
		# -----------------------------------------------

		echo "$GRDC_station"
		# 【需修改,可选】第2个参数为河网阈值，空值代表使用默认值
		/data/xujs/anaconda/envs/python38/bin/python ${codePath}/subBasinLake/Extract_Pound_Basins.py "$projectPath" "" "$GRDC_station"
	else
		echo "不考虑湖泊的子流域划分..."
		# 非湖泊处理流程...
		dirtif="${projectRoot}/${username}/$projectname/data_prepare/spatial/basin_dir.tif"
		accPath="${projectSpatialFileSource}/basin_upa.tif"
		streamPath="${projectSpatialFileSource}/basin_Stream.tif"
		subbasintif="${projectRoot}/${username}/$projectname/data_prepare/spatial/subBasin.tif";
		subbasinshp="${projectRoot}/${username}/$projectname/workspace/spatial_shp/subbasin.shp";
		streamshp="${projectRoot}/${username}/$projectname/workspace/spatial_shp/basin_Stream.shp";
		projedtfile="${projectRoot}/${username}/$projectname/workspace/projectFile";
		# mkdir -p "$projedtfile"

		# echo "$projectname,$lakeshp" >> /data/xujs/srp/Arctic/Arctic/greatRivers/851_sourceBasin/nolake_sourcebasin.csv # 记录无湖泊流域

		GRDC_station="[]"

		# # 【需修改,可选】如不需要指定子流域出口请注释。修改子流域出口csv文件路径，csv请按格式：/data/xujs/srp/zhongshanqiao/tangnaihai.csv
		# GRDC_station=$(awk -F, '
		# BEGIN { print "[" }
		# NR > 1 {
			# gsub(/[ \t\r\n]+$/, "", $3); gsub(/^[ \t\r\n]+/, "", $3)
			# gsub(/[ \t\r\n]+$/, "", $4); gsub(/^[ \t\r\n]+/, "", $4)
			# if (NR > 2) printf ",\n"
			# printf "  {\"lat\":%s, \"lon\":%s}", $3, $4
		# }
		# END { print "\n]" }

		# ' /data/xujs/srp/Arctic/Arctic/Arctic_bigRiver_GRDC_station_filerinlake.csv)

		echo "$GRDC_station"
		echo "/data/xujs/anaconda/envs/python38/bin/python ${codePath}/Divde_SubBasin.py $dirtif $accPath $GRDC_station $streamPath $subbasintif $subbasinshp $streamshp  $projedtfile"
		# 【需修改,可选】第8个参数为河网阈值，空值代表使用默认值
		/data/xujs/anaconda/envs/python38/bin/python ${codePath}/Divde_SubBasin.py "$dirtif" "$accPath" "$GRDC_station" "$streamPath" "$subbasintif" "$subbasinshp" "$streamshp" "20" "$projedtfile"

	fi

	echo "project"
	/data/xujs/anaconda/envs/python36/bin/python ${codePath}/projectRaster2WGS84.py "$projectSpatialFileSource"
	echo "subbasin2shp"
	subbasintif="${projectRoot}/${username}/$projectname/data_prepare/spatial/subBasin.tif";
	subbasinshp="${projectRoot}/${username}/$projectname/workspace/spatial_shp/subbasin.shp";
	/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=RasterToVectorPolygons -v -i="$subbasintif" -o="$subbasinshp"
	echo "merge subbasin"
	/data/xujs/anaconda/envs/python38/bin/python ${codePath}/merge_subbasin.py "$subbasintif" "$subbasinshp"
	echo "subbasin2shp2"
	/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools -r=RasterToVectorPolygons -v -i="$subbasintif" -o="$subbasinshp"


	# -------------------------- 7. 河道属性计算 --------------------------
	echo "河道属性计算..."

	# 复制必要栅格文件到工作空间
	spatialrasterfile="${workspaceFileSource}/spatial_raster"
	mkdir -p "$spatialrasterfile"
	cd "$spatialrasterfile" || exit
	cp "${projectSpatialFileSource}/basin_upa.tif" ./acc.tif
	cp "${projectSpatialFileSource}/basin_elv.tif" ./dem.tif
	cp "${projectSpatialFileSource}/basin_dir.tif" ./flow_dir.tif
	cp "${projectSpatialFileSource}/basin_Stream.tif" ./stream_link.tif
	cp "${projectSpatialFileSource}/subBasin.tif" ./subbasin.tif

	# 计算到河流距离
	echo "计算到河流距离..."
	/data/xujs/anaconda/envs/python38/bin/python "${codePath}/dist2stream.py" "$projectPath"

	# 输入河流参数
	echo "输入河流参数..."
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/input_stream_paras3.py" "$workspaceFileSource"

	# 复制预处理配置文件到工作空间
	preprocessConfig="${model_configsFileSource}/preprocess.ini"
	workspacepreprocessConfig="${workspaceFileSource}/preprocess.ini"
	cd "$workspaceFileSource" || exit
	cp "$preprocessConfig" ./

	# 复制查找表数据
	echo "复制查找表数据..."
	lookupfile="${projectRoot}/lookup"
	datapreparefile="${projectPath}/data_prepare"
	cd "$datapreparefile" || exit
	cp -r "$lookupfile" ./

	# 执行预处理1
	echo "执行预处理1..."
	/data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/test/demo_preprocess_web.py" "$workspacepreprocessConfig"

	# -------------------------- 8. 地形处理 --------------------------
	echo "地形处理..."

	# 复制子流域文件
	subbasinfile="${projectPath}/subbasinfile"
	mkdir -p "$subbasinfile"
	cd "$subbasinfile" || exit
	cp "${spatial_shpPath}/subbasin."{shp,dbf,prj,shx} ./

	# 计算坡度
	workspacedemtif="${spatialrasterfile}/dem.tif"
	slopetif="${spatialrasterfile}/slope.tif"
	/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
		-r=Slope -v --dem="$workspacedemtif" -o="$slopetif" --units="radians"
	cd "$spatialrasterfile" || exit
	cp "$slopetif" ./slope_dinf.tif

	# 计算坡向并重分类
	aspecttif="${spatialrasterfile}/aspect.tif"
	aspectreclasstif="${spatialrasterfile}/aspect_reclass.tif"
	/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
		-r=Aspect -v --dem="$workspacedemtif" -o="$aspecttif"
	/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
		-r=Reclass -v -i="$aspecttif" -o="$aspectreclasstif" --reclass_vals="2.0;0.0;45.0;4.0;45.0;135.0;3.0;135.0;225.0;4.0;225.0;315.0;2.0;315.0;361.0;1.0;-1;0"

	# -------------------------- 9. 土地利用处理 --------------------------
	echo "土地利用处理..."

	# 【可选2】重分类土地利用数据，若使用上传的土地利用请注释----
	landusetif_origin="${projectSpatialFileSource}/HLG_LU_WGS4_GP10m_noforest_origin.tif"
	landusetif="${projectSpatialFileSource}/HLG_LU_WGS84_GP10m_noforest.tif"
	[ -e "$landusetif_origin" ] || mv "$landusetif" "$landusetif_origin"

	/data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
		-r=Reclass -v -i="$landusetif_origin" -o="$landusetif" \
		--reclass_vals="8;0;3;7;3;5;6;5;6;16;6;8;192;8;10;15;10;11;11;11;12;1;12;13;400;13;14;2;14;15;98;16;17;3000;15;16;18;17;18"

	# 完成土地利用处理
	/data/xujs/anaconda/envs/python38/bin/python "${codePath}/completion_landuse.py" "$projectPath"


	# 复制土地利用数据到工作空间
	cd "$spatialrasterfile" || exit
	cp "$landusetif" ./landuse.tif
	# ------------------

	# -------------------------- 10. subarea/HRU划分 --------------------------
	echo "subarea/HRU划分..."
	basintif="${projectRoot}/${username}/$projectname/data_prepare/spatial/basin.tif"
	# # 【可选1，修改】上传landuse。用于subarea划分使用 "3" 叠加。需根据/data/xujs/code/SEIMS-hulugou/seims/preprocess/database/LanduseLookup.csv重分类。
	# upload_landuse="/data/xujs/srp/test/landuse_r.tif"
	# # 【可选1，修改】上传soiltype。用于subarea划分使用 "3" 叠加
	# upload_soiltype="/data/xujs/srp/test/soiltype_r.tif"

	# # 生成流域范围内土地利用和土壤类型tif
	# # 【需修改，可选1】若为上传landuse（和dem相同分辨率），需要打开。使用全球数据集时请注释。---------
	# origin_landusetif="${projectSpatialFileSource}/landuse.tif"
	# landusetif="${projectRoot}/${username}/$projectname/data_prepare/spatial/HLG_LU_WGS84_GP10m_noforest.tif";
	# temp="${projectSpatialFileSource}/landuse_temp.tif"
	# temp2="${projectSpatialFileSource}/landuse_temp2.tif"
	# cd $projectSpatialFileSource
	# cp $upload_landuse ./
	# # 注意修改文件名
	# mv landuse_r.tif landuse.tif
	# # 重采样
	# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
	# -r=Resample -v -i="$origin_landusetif" --base="$basintif" -o="$temp" --method="nn"
	# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
	# -r=SetNodataValue -v -i="$temp" -o="$origin_landusetif" --back_value="-9999"
	# mv landuse.tif HLG_LU_WGS84_GP10m_noforest.tif
	# # 裁剪上传的landuse
	# # /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/clipRaster3.py $basintif $origin_landusetif $temp $landusetif

	# # 根据流域范围补全土地利用缺值区域
	# echo "completion landuse"
	# /data/xujs/anaconda/envs/python38/bin/python /data/xujs/code/completion_landuse.py "$projectPath"

	# echo "copy landuse"
	# cd $spatialrasterfile
	# cp $landusetif ./landuse.tif
	# # --------------------------------------------------------------------------

	# # 【需修改，可选1】若为上传soiltype（和dem相同分辨率），需要打开。使用全球数据集时请注释。
	# origin_soiltypetif="${projectSpatialFileSource}/soiltype.tif"
	# soiltypetif="${projectRoot}/${username}/$projectname/data_prepare/spatial/soiltype.tif";
	# temp="${projectSpatialFileSource}/soiltype_temp.tif"
	# cd $projectSpatialFileSource
	# cp $upload_soiltype ./
	# # 注意修改文件名
	# mv soiltype_r.tif soiltype.tif
	# # 重采样
	# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
	# -r=Resample -v -i="$origin_soiltypetif" --base="$basintif" -o="$temp" --method="nn"
	# /data/xujs/anaconda/envs/python38/lib/python3.8/site-packages/WBT/whitebox_tools \
	# -r=SetNodataValue -v -i="$temp" -o="$origin_soiltypetif" --back_value="-9999"
	# # 裁剪上传的soiltype。不要crop
	# # /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/clipRaster3.py $basintif $origin_soiltypetif $temp $soiltypetif

	# # 没有上传soiltype请注释
	# echo "completion soiltype"
	# /data/xujs/anaconda/envs/python38/bin/python /data/xujs/code/completion_soiltype.py "$projectPath"
	# # --------------------------------------------------------------------------

	# 创建HRU目录
	HRUfile="${workspaceFileSource}/HRU_file"
	mkdir -p "$HRUfile"

	# 清理旧数据
	subbasins_path="${projectPath}/1"
	HillSlope_path="${projectPath}/HillSlope"
	if [ -d "$subbasins_path" ]; then
		echo "文件夹存在，正在删除: $subbasins_path"
		rm -rf "$subbasins_path"
	fi
	if [ -d "$HillSlope_path" ]; then
		echo "文件夹存在，正在删除: $HillSlope_path"
		rm -rf "$HillSlope_path"
	fi

	# 生成subarea划分必要文件
	echo "生成subarea划分必要文件..."
	/data/xujs/anaconda/envs/python38/bin/python "${codePath}/clipShpByShp2Raster2.py" "$projectPath"
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/clipShpByShp2Raster.py" "$projectPath"

	# # 【可选】在subarea划分时如不考虑坡面湖泊请注释 ----------------------------------------------------------------------------------------------------------------------------------
	# echo "clipGNWLShp1"
	# clipGNWIShpPath="${projectRoot}/${username}/$projectname/data_prepare/spatial/GNWI.shp"
	# # 【修改】 /data/xujs/data/GNWL/Arctic_level_11.shp 为北极板块数据，其他地区需要替换文件路径
	# /data/xujs/anaconda/envs/python36/bin/python ${codePath}/clipShp_by_shapely.py "$basinshp" "/data/xujs/data/GNWL/Arctic_level_11.shp" "$clipGNWIShpPath"

	# echo "clipGNWIShp2"
	# target_dir="${projectRoot}/${username}/$projectname/1"
	# out_GNWL_target_dir="${projectRoot}/${username}/$projectname/2"
	# find "$target_dir" -type f -name "*.shp" | while read -r shp_file; do
		# filename=$(basename "$shp_file")
		# name_without_ext="${filename%.*}"
		# # echo "完整路径: $shp_file"
		# # echo "完整路径: $out_GNWL_target_dir/${name_without_ext}.shp"
		# echo "/data/xujs/anaconda/envs/python36/bin/python ${codePath}/clipShp_by_shapely.py $shp_file $clipGNWIShpPath $out_GNWL_target_dir/${name_without_ext}.shp"
		# /data/xujs/anaconda/envs/python36/bin/python ${codePath}/clipShp_by_shapely.py "$shp_file" "$clipGNWIShpPath" "$out_GNWL_target_dir/${name_without_ext}.shp"
	# done

	# # 根据每个子流域的GNWL，生成坡面湖泊tif，湖泊坡面tif
	# echo "generateGNWLtif"
	# target_dir="${projectRoot}/${username}/$projectname/2"
	# slopelakePath="${projectRoot}/${username}/$projectname/HillSlope/SlopeLake"
	# lakeslopePath="${projectRoot}/${username}/$projectname/HillSlope/LakeSlope"
	# mkdir -p "$slopelakePath"
	# mkdir -p "$lakeslopePath"
	# find "$target_dir" -type f -name "*.shp" | while read -r shp_file; do
		# filename=$(basename "$shp_file")
		# name_without_ext="${filename%.*}"
		# slopelake="$slopelakePath/SlopeLake${name_without_ext}.tif"
		# lakeslope="$lakeslopePath/LakeSlope${name_without_ext}.tif"
		# temelete_tif="${projectRoot}/${username}/$projectname/HillSlope/Dir/Dir_${name_without_ext}.tif"
		# # echo "完整路径: $out_GNWL_target_dir/${name_without_ext}.shp"
		# echo "/data/xujs/anaconda/envs/python36/bin/python ${codePath}/generateGNWL_tif.py $shp_file $temelete_tif $slopelake $lakeslope"
		# /data/xujs/anaconda/envs/python36/bin/python ${codePath}/generateGNWL_tif.py "$shp_file" "$temelete_tif" "$slopelake" "$lakeslope"
	# done
	# # ------------------------------------------------------------------------------------------------------------------------------------------------------------

	# 执行subarea划分
	# 【修改】碎斑合并阈值(单位:cell)
	subarathreshold=50
	# 【修改】按需求修改第3个参数：  3：叠加土地利用、土壤属性； 7：考虑坡面湖泊叠加土地利用、土壤属性
	# /data/xujs/anaconda/envs/python36/bin/python "${codePath}/subArea/main.py" "$projectPath/" "5" "3" "$subarathreshold" "1"
	echo "执行subarea划分（新脚本）..."
    /data/user/longp/envs/lp_flood/bin/python /data/user/longp/CH4/HAND/Spatial-discretize-lp-V1/Spatial-discretize-lp_V1/main_1.16_xdw.py -u "${username}" -p "${projectname}"

	# 重命名并复制HRU文件到HRU_file目录
    echo "重命名并复制HRU文件..."
    HRU_source="${workspaceFileSource}/rundata/HRU.tif"
    confluence_source="${workspaceFileSource}/rundata/confluence.txt"
    HRU_target="${HRUfile}/ALL_HRU_final.tif"
    confluence_target="${HRUfile}/ALL_HRU_fields.txt"

    if [ -e "$HRU_source" ]; then
        cp "$HRU_source" "$HRU_target"
        echo "已复制 HRU.tif 到 ${HRU_target}"
    else
        echo "警告: 未找到 ${HRU_source}"
    fi

    if [ -e "$confluence_source" ]; then
        cp "$confluence_source" "$confluence_target"
        echo "已复制 confluence.txt 到 ${confluence_target}"
    else
        echo "警告: 未找到 ${confluence_source}"
    fi


	# -------------------------- 11. 参数处理 --------------------------
	echo "参数处理..."

	# 复制河流参数文件
	reach_paramcsv="${projectRoot}/other_files/reach_param.csv"
	cd "$HRUfile" || exit
	cp "$reach_paramcsv" ./

	# 复制HRU文件并重命名
	HRUtiffile="${HRUfile}/ALL_HRU_final.tif"
	cd "$projectSpatialFileSource" || exit
	cp "$HRUtiffile" ./
	mv ALL_HRU_final.tif HRU_soil_99.tif

	cd "$spatialrasterfile" || exit
	cp "$HRUtiffile" ./
	mv ALL_HRU_final.tif soiltype.tif

	# 编辑配置文件
	echo "编辑配置文件..."
	/data/xujs/anaconda/envs/python38/bin/python "${codePath}/edit_ini_replace.py" "$projectname" "${username}"
	# 【修改】第3、4参数：模拟起止时间 ；第5、6参数：率定起止时间。如：2018-2019两年。模拟时间要比率定时间长
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/edit_ini_runmodel_and_filein_tongyong.py" \
		"$projectname" "${username}" "${projectRoot}" "${WISE_Path}" "${port}" "2010" "2019" "2010" "2019"

	# 执行预处理2【此步开始导入部分数据到Mongodb数据库中】
	/data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/test/demo_preprocess_web2.py" \
		"$workspacepreprocessConfig" "arcgis"



	# -------------------------- 12. 土壤数据处理 --------------------------
	echo "土壤数据处理..."

	basin_shpPath="${projectSpatialFileSource}/basin.shp"
	soilPath="${projectPath}/data_prepare/soil"
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/mask_soil_properties.py" "$basin_shpPath" "$soilPath" "0" "None" "1"

	# 执行WBT_TP处理
	/data/xujs/anaconda/envs/python38/bin/python "/data/xujs/srp/wbt_TP.py" "$projectPath"

	# 读取HTML数据
	/data/xujs/anaconda/envs/python36/bin/python "${codePath}/readHTML.py" "$projectPath"

	# -------------------------- 13. 气象数据处理 --------------------------
	echo "气象数据处理..."

	climate_csv="${climateFileSource}/meteo_daily_CMFD.txt"
	# 若已经提取气象数据则不重复提取
	if [ ! -e "$climate_csv" ]; then
		echo "生成站点文件..."
		/data/xujs/anaconda/envs/python36_2/bin/python "${codePath}/Output_Sites_Files.py" "$projectPath"

		echo "生成气象数据..."
		# 【需修改】第2、3参数代表提取气象时间的起始年份，如：2018-2019两年
		/data/xujs/anaconda/envs/python36_2/bin/python "${codePath}/Generate_meteorological_data.py" "$projectPath" "2010" "2019"
	fi

	# 执行预处理3【导入数据到MongoDB】
	/data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/test/demo_preprocess_web3.py" \
		"$workspacepreprocessConfig" "arcgis"


	# -------------------------- 14. 观测数据处理 --------------------------
	echo "导入观测数据到MongoDB..."

	# echo "generate GRDC siteinfocsv"
	# /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/generate_GRDC_siteinfocsv.py "$projectPath" "/data/xujs/data/GRDC/Arctic/greatRiver/851/origindata" "$projectname"
	# echo "/data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/generate_GRDC_siteinfocsv.py $projectPath /data/xujs/data/GRDC/Arctic/greatRiver/851/origindata $projectname"

	# echo "Copy observation data"
	# /data/xujs/anaconda/envs/python36/bin/python /data/xujs/code/find_csv_by_station_id.py "$projectname" "$observedFileSource"


	# # 【可选】执行前，请手动复制观测数据到项目文件：项目文件目录/data_prepare/observed，并按格式整理：参考/data/xujs/WEB/basins/test/observed
	# echo "import observation data"
	# echo "/data/xujs/anaconda/envs/SEIMS/bin/python ${codePath}/SEIMS-hulugou/seims/preprocess/db_import_observed_local.py $projectname ${username}"
	# /data/xujs/anaconda/envs/SEIMS/bin/python ${codePath}/SEIMS-hulugou/seims/preprocess/db_import_observed_local.py "$projectname" "${username}"


	# -------------------------- 15. 导入土壤土地利用参数到MongoDB --------------------------
	echo "导入土壤土地利用参数到MongoDB..."

	mask_tifPath="${projectPath}/workspace/spatial_raster/mask.tif"
	databaseName=$(echo "$projectname" | tr '.' '_')
	longterm_model="${databaseName}_longterm_model"

	"${codePath}/SEIMS-hulugou/bin20241209/import_raster" "$mask_tifPath" "$spatialrasterfile" \
		"$longterm_model" "SPATIAL" "127.0.0.1" "${port}"

	# 执行预处理4
	/data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/test/demo_preprocess_web4.py" \
		"$workspacepreprocessConfig" "arcgis"




	# -------------------------- 16. subarea参数更新计算 --------------------------
	echo "subarea参数更新计算..."

	runmodel_iniPath1="${model_configsFileSource}/runmodel.ini"
	runmodel_iniPath="${workspaceFileSource}/runmodel.ini"
	cd "$workspaceFileSource" || exit
	cp "$runmodel_iniPath1" ./

	/data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/preprocess/HRU_compute_raster20251102.py" \
		"-ini" "$workspacepreprocessConfig"

	# -------------------------- 17. 分区率定参数文件处理 --------------------------
	echo "分区率定参数文件处理..."

	# 复制def参数文件
	# 【修改】选择一个参数文件，3选1
	# # 【可选】无湖泊参数非分区率定def文件，用于不想分区率定
	# def_file="${projectRoot}/other_files/cali_param_rng-Q_1.def"
	# cp "$def_file" "$longterm_modelFileSource"
	# mv cali_param_rng-Q_1.def cali_param_rng-Q.def

	# # 【可选】无湖泊参数def文件，用于没有湖泊的流域或者不想率定湖泊参数
	# def_file="${projectRoot}/other_files/cali_param_rng-Q.def"
	# cp "$def_file" "$longterm_modelFileSource"
	# cd $longterm_modelFileSource || exit

	# 【可选】有湖泊参数def文件，用于没有湖泊的流域或者不想率定湖泊参数  cali_param_rng-Q_buchong  cali_param_rng-Q是没有k_run_1d和P_max_1D
	def_file="${projectRoot}/other_files/cali_param_rng-Q_xdw.def"
	cp "$def_file" "$longterm_modelFileSource"
	cd $longterm_modelFileSource || exit
	mv cali_param_rng-Q_xdw.def cali_param_rng-Q.def

	# # 【可选】有湖泊参数def，用于有湖泊流域并想要率定湖泊参数
	# # def_file="${projectRoot}/other_files/cali_param_rng-Q_lake_arctic.def"
	# def_file="${projectRoot}/other_files/cali_param_rng-Q_slopelake_arctic.def"
	# cp "$def_file" "$longterm_modelFileSource"
	# cd $longterm_modelFileSource || exit
	# mv cali_param_rng-Q_slopelake_arctic.def cali_param_rng-Q.def


	# 【可选】若不需要分区率定请注释，在模型运行时使用通用版本。转换参数类型
	/data/xujs/anaconda/envs/python36/bin/python "${lp_codePath}/SEIMS-hulugou/seims/preprocess/trans_single2raster.py" "$projectname" "${username}"

	# 导入分区率定空间参数
	echo "import new parameter"
	echo "/data/xujs/anaconda/envs/SEIMS/bin/python ${codePath}/SEIMS-hulugou/seims/preprocess/import_param_by_csv.py -ini ${projectRoot}/${username}/${projectname}/workspace/preprocess.ini"
	/data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/preprocess/import_param_by_csv.py" \
		"-ini" "${projectRoot}/${username}/${projectname}/workspace/preprocess.ini"
	# --------------------------------------------

	# ====================== HAND 淹没信息预处理 ======================
    echo "生成 HAND 淹没查找表..."

    FloodStep_txt="${workspaceFileSource}/rundata/FloodStep.txt"
    HRU_shp="${workspaceFileSource}/HRU_file/HRU.shp"

    if [ -f "$FloodStep_txt" ] && [ -f "$HRU_shp" ]; then
        echo "执行 HAND 预处理脚本..."
        /data/user/longp/envs/lp_flood/bin/python \
            "${lp_codePath}/preprocess_hand.py" \
            --project-path "$projectPath" \
            --db "$longterm_model" \
            --mongo-host "127.0.0.1" \
            --mongo-port "${port}"

        if [ $? -eq 0 ]; then
            echo "HAND 预处理成功完成"
        else
            echo "WARNING: HAND 预处理失败，继续执行后续步骤"
        fi
    else
        echo "WARNING: 未找到 FloodStep.txt 或 HRU.shp，跳过 HAND 预处理"
        [ ! -f "$FloodStep_txt" ] && echo "  - 缺少: $FloodStep_txt"
        [ ! -f "$HRU_shp" ] && echo "  - 缺少: $HRU_shp"
    fi

    # ====================== HAND 淹没信息入库 ======================
    echo "导入 HAND 淹没等级信息到 MongoDB..."

    HAND_CSV="${projectPath}/workspace/rundata/InundationMap.csv"

    if [ -f "$HAND_CSV" ]; then
        /data/user/longp/envs/lp_flood/bin/python \
            "${lp_codePath}/db_import_hand_inundation_map.py" \
            --db "$longterm_model" \
            --csv "$HAND_CSV" \
            --host "127.0.0.1" \
            --port "${port}"

        if [ $? -eq 0 ]; then
            echo "HAND 淹没信息入库成功"
        else
            echo "WARNING: HAND 淹没信息入库失败"
        fi
    else
        echo "WARNING: 未找到 ${HAND_CSV}，跳过 HAND 入库步骤"
    fi

	# -------------------------- 18. 模型运行 --------------------------
	echo "模型运行..."
	# echo 'export LD_LIBRARY_PATH=/data/xujs/mongo-c-driver/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
	# source ~/.bashrc

	# 【修改】选择一个模型版本，2选1
	# 【可选】分区率定版本：
	LD_LIBRARY_PATH="/data/xujs/gdal/lib:/data/xujs/mongo-c-driver/lib:/usr/local/lib:$LD_LIBRARY_PATH" \
	/data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/test/demo_runmodel_web.py" "$workspacepreprocessConfig"
	# 【可选】通用版本：
	# /data/xujs/anaconda/envs/python36/bin/python "${codePath}/edit_ini2.py" "$projectname" "${username}" "${projectRoot}" "${WISE_Path}"
	# /data/xujs/anaconda/envs/SEIMS/bin/python "${codePath}/SEIMS-hulugou/seims/test/demo_runmodel_web.py" "$workspacepreprocessConfig"


done < "$csv_file"

# # -------------------------- 19. 计算耗时 --------------------------
end_time=$(date +%s.%N)
elapsed_time=$(echo "$end_time - $start_time" | bc)
echo "总耗时: $elapsed_time 秒"

# # 示例项目
# # ----默认全球数据，单流域建模（分区率定）示例项目
# # 项目根目录：/data/xujs/WEB/basins/test
# # 项目名称：MLX1
# # 用户名称：xujs
# # 项目文件目录：/data/xujs/WEB/basins/test/xujs/MLX1
# # 项目数据库(一个项目有三个库)：MLX1_longterm_model,MLX1_HydroClimate,MLX1_Scenario
# # 自动化脚本运行日志：/data/xujs/code_public/log/WISE_Workflow_V1.0_test1.log

# # ----上传未处理DEM，单流域建模（分区率定）示例项目
# # 项目根目录：/data/xujs/WEB/basins/test
# # 项目名称：MLX2
# # 用户名称：xujs
# # 项目文件目录：/data/xujs/WEB/basins/test/xujs/MLX2
# # 项目数据库(一个项目有三个库)：MLX2_longterm_model,MLX2_HydroClimate,MLX2_Scenario
# # 自动化脚本运行日志：/data/xujs/code_public/log/WISE_Workflow_V1.0_test2.log


# # 上传湖泊模式，马静在写


