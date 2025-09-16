#pragma once
//
//#include "../lisflood.h"
#include "../utility.h"
#include "lisflood2.h"
//
//struct IndexRange
//{
//	int start;
//	int end;
//};
//
//struct WetDryRowBound
//{
//	// track start and end of the inundation boundary ÑÍÃ»·¶Î§µÄÆðÖ¹ºÅ
//	IndexRange * fp_h;
//
//	// track previous start and end of the inundation boundary
//	// used to zero any flows that are outside the normal processing bounds
//	IndexRange * fp_h_prev;
//
//	// any update to volume should set the volume bounds, to ensure update h is processed ÑÍÃ»·¶Î§µÄÆðÖ¹ºÅ
//	IndexRange * fp_vol;
//
//	// first cell where not nodata
//	IndexRange * dem_data;
//
//	int block_count;
//	/// list of row indexes (j's)
//	IndexRange * block_row_bounds;
//};
//
//void AllocateWetDryRowBound(int row_count, int block_count, WetDryRowBound * wet_dry_bound);
//
//// each cell has a list of flow_indexes that flow in/out of a cell
//// indexes point to the SubGridFlowInfo.sg_flow_Q 
//struct SubGridFlowLookup
//{
//	//index 0 : dx
//	//index 1 : dy
//	//index 2 : d 45 degrees right (d8)
//	//index 3 : d 45 degrees left (d8)
//	int flow_add[4]; // in d8 this must be 4 (PFU changed from 2 to 4)
//	int flow_subtract[4]; // in d8 this must be 4 (PFU changed from 2 to 4)
//};
//
//
//struct SubGridState
//{
//	// SubGridFlowInfo: flow_count number of items stored
//	NUMERIC_TYPE * sg_flow_Q;
//	// SubGridFlowInfo: flow_count number of items stored
//	//NUMERIC_TYPE * Flow_CurrentChannelWidth;
//
//	NUMERIC_TYPE * sg_velocity;
//
//};
//
//
//
///// info - for a sub grid cell
///// all lists are 0 to cell_count
//struct SubGridCellInfo
//{
//	// total number of memory space allocated
//	int cell_count;
//
//	int *sg_cell_x;
//	int *sg_cell_y;
//	int *sg_cell_grid_index_lookup;
//
// 	NUMERIC_TYPE *sg_cell_cell_area; // surface area - from above.
//	NUMERIC_TYPE *sg_cell_dem;
//	NUMERIC_TYPE *sg_cell_cell_infil_rate; // for distrubuted infiltration rates
//
//	NUMERIC_TYPE *sg_cell_SGC_width; // channel width constant
//	NUMERIC_TYPE *sg_cell_SGC_BankFullHeight;
//	NUMERIC_TYPE *sg_cell_SGC_BankFullVolume;
//	NUMERIC_TYPE *sg_cell_SGC_c;
//
//	int * sg_cell_SGC_group;
//	int * sg_cell_SGC_is_large; //if SGC_width > C(0.5)*(row_cell_dx + row_cell_dy), then there is no flood plain cell calc for evap
//};
//
//void AllocateSubGridCellInfo(int cell_count, SubGridCellInfo * sub_grid_cell_info);
//void ZeroSubGridCellInfo(SubGridCellInfo * sub_grid_cell_info, int cell_index);
//
///// used to store point source info and boundary condition info
//struct WaterSource
//{
//	// total number of memory space allocated
//	int count;
//
//	//char  *Name;
//	ESourceType   *Ident;
//	// PS_Val used in case of fixed e.g. HFIX or QFIX (otherwise set to -1)
//	NUMERIC_TYPE *Val;
//	// time series indexed by psi (point source index) //TFD
//	// PS_TimeSeries used in case of var e.e. HVAR or QVAR (otherwise set to NULL)
//	TimeSeries **timeSeries;
//
//	SubGridCellInfo ws_cell;
//
//	NUMERIC_TYPE *Q_FP_old;
//	NUMERIC_TYPE *Q_SG_old;
//
//	NUMERIC_TYPE *g_friction_squared_FP; // friction for this point source cell (pre-calculated from mannings)
//	NUMERIC_TYPE *g_friction_squared_SG; // friction for this point source cell (pre-calculated from mannings)
//};
//
//void AllocateWaterSource(int count, WaterSource * waterSource);
//
/////
///// arrays prefixed with Flow_ have flow_count items
///// arrays prefixed with Cell_ have flow_count*2 items
///// this is because flow_pair are stored for source and destination
///// flow_pair.xxx[i*2] is source
///// flow_pair.xxx[i*2+1] is dest
//struct SubGridFlowInfo
//{
//	/// (dx or dy) * meander
//	NUMERIC_TYPE *sg_flow_effective_distance;
//	NUMERIC_TYPE *sg_flow_g_friction_sq;
//
//	SubGridCellInfo flow_pair;
//	// index of the cell state Cell_h
//	// i.e. when looping over flows, the cell_h can be retreived
//	int * sg_pair_cell_index_lookup;
//
//	// indexed by cell_count
//	// each cell has list of indexes into the sg_flow_Q array
//	SubGridFlowLookup * sg_cell_flow_lookup;
//
//	NUMERIC_TYPE * sg_flow_ChannelRatio;
//
//};
//
//// supergrid channels structure and data setup function
//struct SuperGridLinksList
//{
//	int num_links;
//	int *link_index_SGC_i, *link_index_2D_i, *link_index_SGC_j, *link_index_2D_j, *link_index_SGC, *link_index_2D;
//	NUMERIC_TYPE *SGC_z, *DEM_z, *gn2, *w, *Qold, *dx, *SGC_bfH;
//};
//void InitSuperLinksStructure(const int grid_rows, const int grid_cols, const int grid_cols_padded, SuperGridLinksList * super_linksptr, States *Statesptr, Pars *Parptr, Arrays *Arrptr, SGCprams *SGCptr, Solver *Solverptr, Fnames * Fnameptr, int verbose);
//void AllocateSuperLinksMemory(int n_links, SuperGridLinksList * Super_linksptr);
//
//struct SubGridRowList
//{
//	int row_cols_padded;
//
//	// grid_rows items
//	int * flow_row_count;
//	// grid_rows items
//	int * cell_row_count;
//
//	// each row has flow_row_count items
//	// in memory each row is padded to row_cols_padded
//	// indexed from j * row_cols_padded + 0 
//	// to j * row_cols_padded + cell_row_count[j]
//	// 
//	SubGridFlowInfo flow_info;
//
//	// each row has cell_row_count items
//	// in memory each row is padded to row_cols_padded
//	// indexed from j * row_cols_padded + 0 
//	// to j * row_cols_padded + cell_row_count[j]
//	//
//	// e.g. (* means cell has info, 0 means cell info not present)
//	//    row_cols_padded = 8
//	//    row_count  j,  row_count
//	//               0,  3
//	//               1,  4
//	//               2,  0
//	//               3,  1
//	//
//	// 0  |*|*|*|0|0|0|0|0|
//	// 1  |*|*|*|*|0|0|0|0|
//	// 2  |0|0|0|0|0|0|0|0|
//	// 3  |*|0|0|0|0|0|0|0|
//	//
//	SubGridCellInfo cell_info;
//};
//
//
//struct PointSourceRowList
//{
//	int row_cols_padded;
//	int * ps_row_count;
//
//	WaterSource ps_info;
//
//	NUMERIC_TYPE Qpoint_pos; // Vol per sec // replace Qpoint with positive and negative versions to keep track of input or output for point sources
//	NUMERIC_TYPE Qpoint_neg; // Vol per sec 
//};
//
//struct BoundaryCondition
//{
//	WaterSource bc_info;
//
//	NUMERIC_TYPE Qin;
//	NUMERIC_TYPE Qout;
//	NUMERIC_TYPE QChanOut;
//	NUMERIC_TYPE VolInMT; // added by JCN stores volume in over mass inteval
//	NUMERIC_TYPE VolOutMT; // added by JCN stores volume out over mass inteval
//};
//
//
//struct WeirLayout
//{
//	int row_cols_padded;
//	// count of weirs per row in qx direction
//	int * weir_Qx_row_count;
//	// count of weirs per row in qy direction
//	int * weir_Qy_row_count;
//	
//	// weir_index_qx contains the index of the weir, in the weir lists (below)
//	// indexed by j*row_cols_padded + 0 to j*row_cols_padded + row_weir_count
//	int *weir_index_qx;
//	// weir_index_qy contains the index of the weir, in the weir lists (below)
//	// indexed by j*row_cols_padded + 0 to j*row_cols_padded + row_weir_count
//	int *weir_index_qy;
//
//	// weirs are not stored by row.
//	// just using the old list of weirs.
//	// find the indexes into this list: row weir_index_qx and weir_index_qy
//	int weir_count;
//
//	int *Weir_grid_index;
//	NUMERIC_TYPE *Weir_hc;
//	NUMERIC_TYPE *Weir_Cd;
//	NUMERIC_TYPE *Weir_m;
//	NUMERIC_TYPE *Weir_w;
//	NUMERIC_TYPE *Weir_g_friction_sq;
//	EDirection *Weir_Fixdir;
//	EWeirType *Weir_Typ;
//
//	NUMERIC_TYPE *Weir_Q_old_SG;
//	// 2 * weir_count items stored
//	// 2 * weir_id = flow to the north or west
//	// 2 * weir_id + 1 = flow to the south or east
//	int * Weir_pair_stream_flow_index;
//	SubGridCellInfo cell_pair;
//};
//
//void AllocateWeir(int count, WeirLayout * waterSource);
//
//struct RouteDynamicList
//{
//	// row_cols_padded not uses as it is set to maximum possible i.e. grid_cols_padded
//	// although a full grid of data is used, it will not be a drain on memory bandwidth, since only a few will be generally accesses.
//
//	// count of weirs per row in y
//	int * row_route_qx_count;
//	int * row_route_qy_count;
//
//	// x coordinate of the qx route (y coordinate is row)
//	int * route_list_i_lookup_qx;
//	// x coordinate of the qy route (y coordinate is row)
//	int * route_list_i_lookup_qy;
//
//};
//struct LISFLOODFPContext {
//	int grid_cols;
//	int grid_rows;
//	int grid_cols_padded;
//
//	NUMERIC_TYPE* h_grid;
//	NUMERIC_TYPE* volume_grid;
//	NUMERIC_TYPE* Qx_grid;
//	NUMERIC_TYPE* Qy_grid;
//	NUMERIC_TYPE* Qx_old_grid;
//	NUMERIC_TYPE* Qy_old_grid;
//	NUMERIC_TYPE* maxH_grid;
//	NUMERIC_TYPE* maxHtm_grid;
//	NUMERIC_TYPE* initHtm_grid;
//	NUMERIC_TYPE* totalHtm_grid;
//	NUMERIC_TYPE* maxVc_grid;
//	NUMERIC_TYPE* maxVc_height_grid;
//	NUMERIC_TYPE* maxHazard_grid;
//	NUMERIC_TYPE* Vx_grid;
//	NUMERIC_TYPE* Vy_grid;
//	NUMERIC_TYPE* Vx_max_grid;
//	NUMERIC_TYPE* Vy_max_grid;
//
//	NUMERIC_TYPE* dem_grid;
//	NUMERIC_TYPE* g_friction_sq_x_grid;
//	NUMERIC_TYPE* g_friction_sq_y_grid;
//	NUMERIC_TYPE* friction_x_grid;
//	NUMERIC_TYPE* friction_y_grid;
//	NUMERIC_TYPE* dx_col;
//	NUMERIC_TYPE* dy_col;
//	NUMERIC_TYPE* cell_area_col;
//	NUMERIC_TYPE* Fp_xwidth;
//	NUMERIC_TYPE* Fp_ywidth;
//
//	SubGridRowList* sub_grid_layout_rows;
//	SubGridState* sub_grid_state_rows;
//	SubGridRowList* sub_grid_layout_blocks;
//	SubGridState* sub_grid_state_blocks;
//	NUMERIC_TYPE* SGC_BankFullHeight_grid;
//
//	TimeSeries* evap_time_series;
//	NetCDFVariable* evap_grid;
//	TimeSeries* rain_time_series;
//	TimeSeries* temperature_time_series;
//	NUMERIC_TYPE* rain_grid;
//	NUMERIC_TYPE* dist_infil_grid;
//
//	WetDryRowBound* wet_dry_bounds;
//	PointSourceRowList* ps_layout;
//	BoundaryCondition* boundary_cond;
//	WeirLayout* weirs_weirs;
//	WeirLayout* weirs_bridges;
//	RouteDynamicList* route_dynamic_list;
//	NUMERIC_TYPE* route_V_ratio_per_sec_qx;
//	NUMERIC_TYPE* route_V_ratio_per_sec_qy;
//
//	timeval timstr;
//	double processing_start_time;
//	double processing_end_time;
//	double total_write_time;
//	double loop_start;
//
//	NUMERIC_TYPE curr_time;
//	time_t rain_begin_timestamp;
//	time_t rain_end_timestamp;
//	int last_rain_time;
//	char * nextRainTifName;
//	char * nextRainTifPath;
//	vector<string> tifFileTimes;
//	NUMERIC_TYPE* rainfall_no_padding;
//	NUMERIC_TYPE depth_thresh;
//	NUMERIC_TYPE last_gw_time;
//	NUMERIC_TYPE * tmp_grid1;
//	NUMERIC_TYPE * tmp_grid2;
//	NUMERIC_TYPE * tmp_grid3;
//	int* steadyCount;
//	int stop_loop;
//	int verbose;
//
//	NUMERIC_TYPE ** tmp_thread_data;
//	NUMERIC_TYPE ** tmp_thread_data_ch;
//};
//void AllocateRoutingDynamicList(int rows, int grid_cols_padded, RouteDynamicList * route_dynamic_list);
//
//int LisFloodFP_Initilize(int argc, char *argv[], Arrays *Arrptr, Files* FpsPtr, Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr, Stage *Stageptr, SGCprams *SGCptr, DamData *Damptr,
//	vector<ChannelSegmentType> *ChannelSegmentsVecPtr, LISFLOODFPContext* LFPContextPtr, SuperGridLinksList *Super_linksptr, char* tmpFileNamePtr, char* tmpSysCmdPtr);
//
//int LisFloodFP_Finilize(Solver *Solverptr, Arrays *Arrptr, Fnames *Fnameptr, Files* FpsPtr, States *Statesptr, Pars *Parptr, LISFLOODFPContext *LFPContext, char* tmpFileNamePtr);
//void Fast_RunStep(Arrays *Arrptr, Files *Fptr, Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, SGCprams * SGCptr, DamData *Damptr, Stage *Locptr, LISFLOODFPContext *LFPContext, SuperGridLinksList *Super_linksptr);
