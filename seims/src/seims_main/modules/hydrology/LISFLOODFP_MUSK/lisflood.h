#pragma once
/*
#####################################################################################
LISFLOOD-FP flood inundation model
#####################################################################################

copyright Bristol University Hydrology Research Group 2008

webpage -	http://www.ggy.bris.ac.uk/research/hydrology/models/lisflood
contact -	Professor Paul Bates, email: paul.bates@Bristol.ac.uk,
Tel: +44-117-928-9108, Fax: +44-117-928-7878

*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
//#include <omp.h>
#include <vector> // CCS
#include <iostream> // CCS
//#include <netcdf.h> // JCN
#include <ctime>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "utility.h"
#include <unordered_set>
#include <unordered_map>
#include "./lisflood2/lisflood2.h"

#ifndef _SGM_BY_BLOCKS
// 0 SGC by row 
// 1 SGC by block
#define _SGM_BY_BLOCKS 0
#endif
#ifndef _BALANCE_TYPE
// 1 balance by wet, 
// 0 even fixed balance
#define _BALANCE_TYPE 0 
#endif

#ifndef _NUMERIC_MODE
#define _NUMERIC_MODE 1
#endif
#ifndef _PROFILE_MODE
#define _PROFILE_MODE 0
#endif
#ifndef _ONLY_RECT
#define _ONLY_RECT 1
#endif
#ifndef _DISABLE_WET_DRY
#define _DISABLE_WET_DRY 0
#endif
#ifndef _CALCULATE_Q_MODE
#define _CALCULATE_Q_MODE 1
#endif
#ifndef _NETCDF
#define _NETCDF 0
#endif
#ifndef _XDW_DEBUG
#define _XDW_DEBUG 0
#endif
//#define TESTING
#ifdef TESTING
void RunTests();
#endif
#ifdef _MSC_VER
//#define RESULT_CHECK 1
#endif

#if defined (_DEBUG) && _DEBUG > 1
//#define RESULT_CHECK 1
#endif

// older versions of visual studio does not contain cbrt function
#ifndef cbrt
	#define cbrt(x) pow(x,1/3.0);
#endif
#ifndef cbrtf
	#define cbrtf(x) powf(x,1/3.0f);
#endif

#if _NUMERIC_MODE == 1
#define NUMERIC_TYPE double
//#define NUMERIC_FLOAT float
//#define NUMERIC_INT int
#define NUMERIC_TYPE_NAME "double"
#define NUM_FMT "lf"
#define FMAX fmax
#define FMIN fmin
#define POW pow
#define FABS fabs
#define CBRT cbrt
#define SQRT sqrt
#define C(x) x
#else
#define NUMERIC_TYPE float
#define NUMERIC_TYPE_NAME "float"
#define NUM_FMT "f"
#define FMAX fmaxf
#define FMIN fminf
#define POW powf
#define FABS fabsf
#define CBRT cbrtf
#define SQRT sqrtf
#define C(x) x##f
#endif

#ifdef __unix__
#define FILE_SEP "/"
#define STRCMPi strcasecmp
#elif __APPLE__
#define FILE_SEP "/"
#define STRCMPi strcasecmp
#else
#define STRCMPi strcmpi
#define FILE_SEP "\\"
#endif

#if _XOPEN_SOURCE >= 600 || _ISOC99_SOURCE || _POSIX_C_SOURCE >= 200112L || _MSC_VER >= 1800
#define getmax(a, b) FMAX(a, b)
#else
#define getmax(a, b) (a>b?a:b)
#endif

#if _XOPEN_SOURCE >= 600 || _ISOC99_SOURCE || _POSIX_C_SOURCE >= 200112L || _MSC_VER >= 1800
#define getmin(a, b) FMIN(a, b)
#else
#define getmin(a, b) (a<b?a:b)
#endif

#define LINE_BUFFER_LEN 4096

// define basic constants
#define ON 1
#define OFF 0
#define CHKINTERVAL C(1.0) // default checkpoint interval in hours runtime
#define NULLVAL C(-9999.0) // MT: define ascii file NULL value as constant
//#define DEM_NO_DATA C(1e7) // new code should use Pars.nodata_elevation instead // decreased to 1e7 to support float
// xiaodw modify, for dembnk.asc, nodata is -9999
//#define DEM_NO_DATA 65535 
#define DEM_NO_DATA C(-9999.0)  
#define TIF_NO_DATA C(-9999.0) 
#define DEFAULT_PRECISION 6
#define ZERO 0

#define PARAM_FILE 0
#define CMD_LINE 1

#define TIME 1
#define TIME_SPACE 2

#define fix_small_negative(value) ((value < C(0.0) && value > -0.0001) ? C(0.0) : value)

#pragma warning( disable : 4996)  // MT: disable visual C++ depreciation warnings so we can see other warnings
// #pragma warning( disable : 1478)  // MT: disable intel depreciation warnings so we can see other warnings [CCS VS2010 throws warning C4616: #pragma warning : warning number '1478' not a valid compiler warning]
#define IS_DEBUG 1
// xdw modify, support green-ampt
#define VALUE_TYPE  1
#define FILE_TYPE  2
#define FILE_AND_VALUE_TYPE  3  // use a raster file and set value
// xiaodw,  .tif
#define TIF ".tif"
#define RAIN_FILE_PREFIX "avg_rainfall_" 
#define RAIN_TIME_STEP = 3600
#define MULTI_SOIL_POREINDEX_FILE "multi_soilPoreIndexFile"
#define MULTI_SOIL_POREINDEX_VALUE "multi_soilPoreIndexValue"
#define SOIL_POREINDEX_SCALE_FACTOR "soilPoreIndexScaleFactor"
#define MULTI_SOIL_FC_FILE  "multi_soilFcFile"
#define MULTI_SOIL_KS_FILE  "multi_soilKsFile"
#define MULTI_SOIL_THICKNESS_FILE  "multi_soilThicknessFile"
#define MULTI_SOIL_INIT_MOIS_FILE  "multi_soilInitMoistureFile"
#define MULTI_SOIL_POROSITY_FILE  "multi_soilPorosityFile"
#define MULTI_INTERFLOW_CS_VALUE "multi_interflowCsValueOfLyr"
#define MULTI_KSFACTOR_VALUE_V "multi_ksFactorVOfLyr"
#define MULTI_KSFACTOR_VALUE_H "multi_ksFactorHOfLyr"
#define MULTI_KSFACTOR_INFIL_VALUE "ksFactorValueInfil"

#define MULTI_INTERFLOW_SURLAG_VALUE "multi_interflowSurlag"
#define MULTI_INTERFLOW_TCONC_VALUE "multi_interflowTconc"
#define MULTI_INTERFLOW_LAGINDEX_VALUE "multi_interflowLagindex"

#define RUNOFF_CO_FILE  "runoff_coFile"
#define RUNOFF_CO_VAL  "runoff_coVal"
#define RUNOFF_CO_FACTOR  "runoff_coFactor"
#define ALPHA  "alpha"

// ********************* xiaodw add, for DHSVM**************************
#define SOIL_THICKNESS_FILE  "soilThicknessFile"

const int NNEIGHBORS =  8;
const int NDIRS = 8;
//const int NDIRS = 4;
const NUMERIC_TYPE OUTSIDEBASIN=-9999.0;
const NUMERIC_TYPE CP = 1013.0;		/* Specific heat of moist air at constant pressure (J/(kg*C)) */
const NUMERIC_TYPE WATER_DENSITY = 1000.;		/* Density of water in kg/m3 */
#ifndef ABSVAL
#define ABSVAL(x)  ( (x) < 0 ? -(x) : (x) )
#endif


/*! Return maximum value */
#ifndef Max
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#endif
/*! Return minimum value */
#ifndef Min
#define Min(a, b) ((a) >= (b) ? (b) : (a))
#endif
/*! Return absoulte value */
#ifndef Abs
#define Abs(x) ((x) >= 0 ? (x) : -(x))
#endif

using namespace std; // CCS
const NUMERIC_TYPE ZERO_LIMIT = 0.00001;

/*! A approximation of Zero */
#ifndef UTIL_ZERO
#define UTIL_ZERO       1.0e-6f
#endif /* UTIL_ZERO */
/*
*****************************************************************************

Define the structures
---------------------
Fnames - Contains all the filenames from the .par file
States - Contains all the state parameters for the simulation
Pars - Contains the parameter values specified in the .par file
Solver - Defines the solution settings
BoundaryValues - Used in the DG2 SWE solver
Arrays - Defines the global arrays
ChannelSegmentType - Defines the channel
Stage - Variables for outputting stage information at specified locations
Files - General output file pointers
BoundCs - Boundary conditions

*****************************************************************************
*/

/// time series loaded from .bdy file
struct TimeSeries{
	NUMERIC_TYPE *time;
	NUMERIC_TYPE *value;
	int count;
	int prev_index;

	// store the prev time queried
	NUMERIC_TYPE prev_time;
	// store the prev time queried result value
	NUMERIC_TYPE prev_value;
};

enum ESourceType {
	NONE0 = 0,
	FREE1 = 1,
	HFIX2 = 2,
	HVAR3 = 3,
	QFIX4 = 4,
	QVAR5 = 5,
	// FREE or Qout(rivers)
	FREE6 = 6,
	// rivers
	TRIB7 = 7,
	// rivers
	RATE8 = 8,
};

enum EWeirType
{
	EWeir_Weir = 0,
	EWeir_Bridge = 1
};

enum EDirection
{
	//N = 1, E = 2, S = 3, W = 4
	DirectionNA = 0,
	North = 1,
	East = 2,
	South = 3,
	West = 4
};

typedef struct
{
	NUMERIC_TYPE *DEM;
	NUMERIC_TYPE *H;
	NUMERIC_TYPE *HU;
	NUMERIC_TYPE *HV;
} BoundaryValues;

/*! \struct Arrays
Stores the pointers to arrays required globally in the computation. Defined as 1D
vectors but stores 2D data determined by the array subscripts.
*/
struct Arrays{
	/*! DEM, Water height, Flow in x-direction and Flow in y-direction */
	NUMERIC_TYPE *DEM; // Digital elevation model
	NUMERIC_TYPE *H;
	NUMERIC_TYPE *Qx;
	NUMERIC_TYPE *Qy;
	NUMERIC_TYPE *Qxold;
	NUMERIC_TYPE *Qyold;
	NUMERIC_TYPE *U;
	NUMERIC_TYPE *Rainmask;  //Distrubted rainfall AS
	NUMERIC_TYPE *DistributeRain;  // xiaodw add, support distribute rain
	/* Fields specific to Roe and SWE solvers  */
	NUMERIC_TYPE *HU;
	NUMERIC_TYPE *HV;
	NUMERIC_TYPE *FHx;
	NUMERIC_TYPE *FHUx;
	NUMERIC_TYPE *FHVx;
	NUMERIC_TYPE *FHy;
	NUMERIC_TYPE *FHUy;
	NUMERIC_TYPE *FHVy;

	/* Roe-specific fields */
	NUMERIC_TYPE *RSHU;
	NUMERIC_TYPE *LSHU;
	NUMERIC_TYPE *RSHV;
	NUMERIC_TYPE *LSHV;
	NUMERIC_TYPE *BSHU;
	NUMERIC_TYPE *TSHU;
	NUMERIC_TYPE *BSHV;
	NUMERIC_TYPE *TSHV;

	/* SWE-specific fields */
	NUMERIC_TYPE *Zstar_x;
	NUMERIC_TYPE *Zstar_y;
	NUMERIC_TYPE *Hstar_neg_x;
	NUMERIC_TYPE *Hstar_pos_x;
	NUMERIC_TYPE *Hstar_neg_y;
	NUMERIC_TYPE *Hstar_pos_y;

	/* DG2-specific fields */
	NUMERIC_TYPE *DEM1x;
	NUMERIC_TYPE *DEM1y;
	NUMERIC_TYPE *H1x;
	NUMERIC_TYPE *H1y;
	NUMERIC_TYPE *HU1x;
	NUMERIC_TYPE *HU1y;
	NUMERIC_TYPE *HV1x;
	NUMERIC_TYPE *HV1y;

	NUMERIC_TYPE *H_int;
	NUMERIC_TYPE *H1x_int;
	NUMERIC_TYPE *H1y_int;
	NUMERIC_TYPE *HU_int;
	NUMERIC_TYPE *HU1x_int;
	NUMERIC_TYPE *HU1y_int;
	NUMERIC_TYPE *HV_int;
	NUMERIC_TYPE *HV1x_int;
	NUMERIC_TYPE *HV1y_int;

	NUMERIC_TYPE *ETA1x_slopelim;
	NUMERIC_TYPE *ETA1y_slopelim;
	NUMERIC_TYPE *HU1x_slopelim;
	NUMERIC_TYPE *HU1y_slopelim;
	NUMERIC_TYPE *HV1x_slopelim;
	NUMERIC_TYPE *HV1y_slopelim;

	NUMERIC_TYPE *HUstar_neg_x;
	NUMERIC_TYPE *HUstar_pos_x;
	NUMERIC_TYPE *HUstar_neg_y;
	NUMERIC_TYPE *HUstar_pos_y;
	NUMERIC_TYPE *HVstar_neg_x;
	NUMERIC_TYPE *HVstar_pos_x;
	NUMERIC_TYPE *HVstar_neg_y;
	NUMERIC_TYPE *HVstar_pos_y;

	/* Flow direction map for Rainfall*/
	int *FlowDir; // CCS: added to hold DEM flow direction map for routing shallow rainfall flow 13/03/2012
	NUMERIC_TYPE *Route_dH;
	NUMERIC_TYPE *RouteInt; // CCS: added to record routing scheme dH and interval
	

	/* ---------------- */
	NUMERIC_TYPE *maxH;
	NUMERIC_TYPE *maxHtm;
	NUMERIC_TYPE *initHtm;
	NUMERIC_TYPE *totalHtm;
	NUMERIC_TYPE *Manningsn;
	NUMERIC_TYPE *SGCManningsn;
	NUMERIC_TYPE *paerial;
	NUMERIC_TYPE *pbound;

	int weir_count;
	//lists of weir data (each have 'weir_count' items)
	NUMERIC_TYPE *Weir_hc;
	NUMERIC_TYPE *Weir_Cd;
	NUMERIC_TYPE *Weir_m;
	NUMERIC_TYPE *Weir_w;
	EDirection *Weir_Fixdir;
	EWeirType *Weir_Typ;

	//grid for weirs updating Qx
	int *Weir_Identx;
	//grid for weirs updating Qy
	int *Weir_Identy;

	TimeSeries * evap;
	TimeSeries * rain;
	TimeSeries * temperature;

	int *ChanMask;
	int *SegMask;

	NUMERIC_TYPE *TRecx;
	NUMERIC_TYPE *TRecy; // MT: add to record TStep

	NUMERIC_TYPE *LimQx;
	NUMERIC_TYPE *LimQy; // MT: add to record Qlimits
	NUMERIC_TYPE *Vx;
	NUMERIC_TYPE *Vy;
	NUMERIC_TYPE *maxVx;
	NUMERIC_TYPE *maxVy;
	NUMERIC_TYPE *Vc; // JCN: added to record velocity
	NUMERIC_TYPE *maxVc;
	NUMERIC_TYPE *maxVcH;
	NUMERIC_TYPE *maxHaz; // JCN added to calculate hazard
	NUMERIC_TYPE *SGCwidth;
	NUMERIC_TYPE *SGCz;
	NUMERIC_TYPE *QxSGold;
	NUMERIC_TYPE *QySGold;
	NUMERIC_TYPE *SGCbfH;
	NUMERIC_TYPE *SGCVol;
	NUMERIC_TYPE *SGCdVol;
	NUMERIC_TYPE *SGCbfV;
	NUMERIC_TYPE *SGCc;
	NUMERIC_TYPE *SGCFlowWidth;
	NUMERIC_TYPE *SGCdx;
	NUMERIC_TYPE *SGCcat_area;// JCN added to store widths and depths
	NUMERIC_TYPE *dx;
	NUMERIC_TYPE *dy;
	NUMERIC_TYPE *dA; // CCS added for lat long data
	NUMERIC_TYPE *DamMask; // FEOL for Res..
	NUMERIC_TYPE *dist_infiltration; // JCN stores distributed infiltration rates
	
	int  *SGCgroup;
	int  *SGCdirn;  // PFU for prescribing sub grid channel flow directions
	BoundaryValues boundary;
};

//-------------------------------------------
// Files
struct Files{
	FILE *mass_fp;
	FILE *stage_fp;
	FILE *vel_fp;
	FILE *gau_fp;
	FILE *dam_fp;
	FILE **pois_fp;

};

//-------------------------------------------
// Fnames
struct Fnames{

	char resrootname[512]; // resrootname will be res_dirname + res_prefix
	char demfilename[256];
	char startfilename[256];
	char chanfilename[256];

	char res_prefix[256];
	char res_dirname[256];
	char qfilename[256];
	char nfilename[256];
	char SGCnfilename[256];
	char porfilename[256];
	char rivername[256];
	char bcifilename[256];
	//xdw add, poi file
	char poifilename[256];
	char bdyfilename[256];
	char weirfilename[256];
	char opfilename[256];
	char stagefilename[256];
	char ascheaderfilename[256];
	char multiriverfilename[256]; // CCS
	char checkpointfilename[256]; // used to write a checkpoint file (note it is placed in the input file dir not the results dir)
	char loadCheckpointFilename[256]; // explicit checkpoint file to start run (specify with -loadcheck option, defaults to checkpointfilename)
	char evapfilename[256];
	char rainfilename[256];
	char rain_csvfilename[256];
	char temperatureCsvFile[256];
	char rainmaskname[256]; //Distributed rainfall AS
	char rainTifFolder[256]; // xiaodw, distributed rain tif foler
	char logfilename[256];
	char SGCwidthfilename[256]; // JN sub grid channel widths
	char SGCbankfilename[256]; // JN sub grid channel bank elevations
	char SGCbedfilename[256];  // JN sub grid channel bed elevation
	char SGCleveefilename[256];  // NQ levee addition
	char SGCcat_areafilename[256]; // JN sub grid channel accumulation area
	char SGCchangroupfilename[256];
	char SGCchanpramsfilename[256];
	char gaugefilename[256];
	char DamMaskfilename[256]; // FEOL
	char Damparfilename[256]; // FEOL
	char ChanMaskfilename[256]; // JCN
	char LinkListfilename[256]; // JCN
	char SGCdirnfilename[256];  // PFU
	char infilfilename[256]; // JCN
    char dynamicrainfilename[256];
	// xdw modify, support green-ampt
	char ksFile[256];
	char initSoilMoistureFile[256];
	char porosityFile[256];
	char clayFile[256];
	char sandFile[256];
	char rootDepthFile[256];
	// xdw modify, support interflow
	char slopeFile[256];
	// xdw modify, support glacier and snow melt
	char glacierFile[256];
	char snowFile[256];
	// xdw modify, support time varying temperature, short radiation and albedo
	char temperatureFile[256];
	char shortRadiationFile[256];
	char albedoFile[256];
	// xdw modify, from input tif file to get metadata
	char tif_src_file[256];
	// xdw modify, support groundwater
	char fieldCapacityFile[256];
	char poreIndexFile[256];
	// xdw modify, support multilayer percolation and interflow
	char* multi_soilPoreIndexFile[256];
	char* multi_soilFcFile[256];
	char* multi_soilPorosityFile[256];
	char* multi_soilKsFile[256];
	char* multi_soilThicknessFile[256];
	char* multi_soilInitMoistureFile[256];
	// xdw modify, for dhsvm
	char soilThicknessAllLyrsFile[256];
	char ksLatFile[256];
	// xdw modify, for wetspa
	char runoffCoFile[256];
};
struct Pois {
	int *index;
	int* xpi; //used in legacy and read in
	int* ypi; //used in legacy and read in
	int num;
	NUMERIC_TYPE *Rain_Grid;  // mm
	NUMERIC_TYPE *Evap_Grid;  // mm
	NUMERIC_TYPE *Snow_Grid;
	NUMERIC_TYPE *Infilt_Grid;  // mm
	NUMERIC_TYPE *InfiltCh_Grid; //mm
	NUMERIC_TYPE *Freeze_Grid;
	NUMERIC_TYPE *SnowMelt_Grid;
	NUMERIC_TYPE *GlacierMelt_Grid;
	NUMERIC_TYPE *Qx_Grid;  // mm
	NUMERIC_TYPE *Qy_Grid;  // mm
	NUMERIC_TYPE *Q_Ch;    // mm
	NUMERIC_TYPE *Vol_Grid;
	NUMERIC_TYPE *surf_water_depth_Grid;  // 侧向流导致的地表水深 mm
	NUMERIC_TYPE **soil_water_depth_Grid;  // 每层土壤的水深
	NUMERIC_TYPE * soil_lat_flowin_Grid_allLyr; // 所有层土壤侧向流流入的流量 mm
	NUMERIC_TYPE * soil_lat_flowout_Grid_allLyr; // 所有层土壤侧向流流入的流量 mm
	NUMERIC_TYPE **soil_perc_Grid;   // 每层土壤纵向渗漏量  mm
	NUMERIC_TYPE **soil_lat_flowin_Grid;   // 每层土壤侧向壤中流流入量  mm/gwstep  
	NUMERIC_TYPE **soil_lat_flowout_Grid;   // 每层土壤侧向壤中流流出量  mm/gwstep  

	NUMERIC_TYPE *Rain_Grid_Last;  // mm
	NUMERIC_TYPE *Evap_Grid_Last;  // mm
	NUMERIC_TYPE *Infilt_Grid_Last;  // mm
	NUMERIC_TYPE *InfiltCh_Grid_Last;  // mm
	
	NUMERIC_TYPE *Qx_Grid_Last;  // mm
	NUMERIC_TYPE *Qy_Grid_Last;  // mm
	NUMERIC_TYPE *Q_Ch_Last;    // mm
	NUMERIC_TYPE *surf_water_depth_Grid_Last;  // 侧向流导致的地表水深 mm
	NUMERIC_TYPE **soil_water_depth_Grid_Last;  // 每层土壤的水深
	NUMERIC_TYPE * soil_lat_flowin_Grid_allLyr_Last; // 所有层土壤侧向流流入的流量 mm
	NUMERIC_TYPE * soil_lat_flowout_Grid_allLyr_Last; // 所有层土壤侧向流流入的流量 mm
	NUMERIC_TYPE **soil_perc_Grid_Last;   // 每层土壤纵向渗漏量  mm
	NUMERIC_TYPE **soil_lat_flowin_Grid_Last;   // 每层土壤侧向壤中流流入量  mm/gwstep  
	NUMERIC_TYPE **soil_lat_flowout_Grid_Last;   // 每层土壤侧向壤中流流出量  mm/gwstep  




	TimeSeries *Rain_TimeSeries;
	TimeSeries *Evap_TimeSeries;
	TimeSeries *Infilt_TimeSeries;
	TimeSeries *Q_TimeSeries;
	TimeSeries *Vol_TimeSeries;

};

//-------------------------------------------
// Boundary Conditions
struct BoundCs{
	int* xpi; //used in legacy and read in
	int* ypi; //used in legacy and read in

	char  *PS_Name;
	ESourceType   *PS_Ident;
	int   numPS;
	// PS_Val used in case of fixed e.g. HFIX or QFIX (otherwise set to -1)
	NUMERIC_TYPE *PS_Val;
	// time series indexed by psi (point source index) //TFD
	// PS_TimeSeries used in case of var e.e. HVAR or QVAR (otherwise set to NULL)
	TimeSeries **PS_TimeSeries;

	NUMERIC_TYPE *PS_Q_FP_old;
	NUMERIC_TYPE *PS_Q_SG_old;

	ESourceType   *BC_Ident;
	int numBCs;
	char  *BC_Name;
	// BC_Val used in case of fixed e.g. HFIX or QFIX (otherwise set to -1)
	NUMERIC_TYPE *BC_Val;
	// time series indexed by bci (boundary condition index) //TFD
	// BC_TimeSeries used in case of var e.e. HVAR or QVAR (otherwise set to NULL)
	TimeSeries **BC_TimeSeries;

	NUMERIC_TYPE Qpoint_pos; // replace Qpoint with positive and negative versions to keep track of input or output for point sources
	NUMERIC_TYPE Qpoint_neg;
	NUMERIC_TYPE Qin;
	NUMERIC_TYPE Qout;
	NUMERIC_TYPE QChanOut;
	NUMERIC_TYPE VolInMT; // added by JCN stores volume in over mass inteval
	NUMERIC_TYPE VolOutMT; // added by JCN stores volume out over mass inteval

	std::vector<TimeSeries> allTimeSeries;
};

//-------------------------------------------
// Stage
struct Stage{
	int Nstages, Ngauges;
	NUMERIC_TYPE *stage_loc_x, *stage_loc_y;
	NUMERIC_TYPE *gauge_loc_x, *gauge_loc_y, *gauge_dist;
	int *stage_grid_x, *stage_grid_y, *stage_check;
	int *gauge_grid_x, *gauge_grid_y;
	int *gauge_cells;
	EDirection *gauge_dir;
};

// SGC parameters
struct SGCprams{
	int NSGCprams;
	int *SGCchantype;
	NUMERIC_TYPE SGCbetahmin;
	NUMERIC_TYPE *SGCp, *SGCr, *SGCs, *SGCm, *SGCa;
	//mannings squared, indexed by channel group
	NUMERIC_TYPE *SGCn;
	NUMERIC_TYPE *SGCgamma, *SGCbeta1, *SGCbeta2, *SGCbeta3, *SGCbeta4, *SGCbeta5;
};
// 注意，竟然支持nc数据，后面看看是否支持读写
struct NetCDFVariable
{
  int ncid;
  size_t xlen;
  size_t ylen;
  size_t tlen;
  int varid;
  size_t time_idx;
  NUMERIC_TYPE dt;
  NUMERIC_TYPE* times;
  NUMERIC_TYPE* xs;
  NUMERIC_TYPE* ys;
  NUMERIC_TYPE* data;
};

struct NetCDFState
{
	int init_done;

	int ncid;
	int dimid_time;
	int dimid_x;
	int dimid_y;

	int dimid_x_edge; //x+1
	int dimid_y_edge; //y+1

	//int dimid_height;
	//int dimid_q;
	//int dim_id_Velocity;

	// one dimentional - record time at each write
	int varid_time;
	int varid_x;
	int varid_y;

	// time series variables
	int varid_depth;
	int varid_elevation;
	
	int varid_qx;
	int varid_qy;
	int varid_qcx;
	int varid_qcy;
	int varid_Vx;
	int varid_Vy;
	//int varid_Vc; // previously not written
	int varid_sgc_Vx;
	int varid_sgc_Vy;
	int varid_sgc_Vc;


	// single grid variables
	int varid_inittm;
	int varid_totaltm;
	int varid_max;
	int varid_mxe;
	int varid_maxtm;
	int varid_maxVx;
	int varid_maxVy;
	int varid_maxVc;
	int varid_maxVcd;
	int varid_maxHaz;

	

};


struct OutputParams
{
	int standard_extensions;
	int ascii_out;
	int binary_out;

	int call_gzip;

	int netcdf_out;
	NetCDFState netcdf_state;
};


//-------------------------------------------
// Simulation States
struct States{
	int ChannelPresent;
	int TribsPresent;
	int NCFS;
	int save_depth;
	int save_elev;
	int save_vtk;
	int save_snow_thickness;
	int save_glacier_thickness;
	int save_table_depth;
	int single_op;
	int multi_op;
	int calc_area;
	int calc_meandepth;
	int calc_volume;
	int save_stages;
	int adaptive_ts;
	int acceleration; // PB: Flag to switch to acceleration formulation
	int qlim; // TJF: Flag for qlim version
	int debugmode;
	int save_Qs;
	int calc_infiltration;
	int calc_distributed_infiltration; // uses a file to use spatially distributed infiltration rates
	int use_green_ampt_singlelayer; // 是否使用Green-ampt下渗算法
	int use_green_ampt_multilayer; // 是否使用Green-ampt下渗算法
	int use_wetspa_sur_mr;              // 是否使用wetspa SER_MR下渗算法
	int use_seims_aet;                      // 是否使用SEIMS AET蒸发算法
	int use_xaj_evap;                      // 是否使用新安江模型蒸发算法
	int use_interflow_singlelayer; // 是否使用壤中流
	int use_snow_glacier;// 是否使用冰川融雪模块
	int use_temperature;//  xdw add, use temperature series or not
	int use_percolation_singlelayer;
	int use_groundwater;
	int use_dhsvm;
	int use_change_acccum_depth;
	// multi-layers
	int use_interflow_multilayer;
	int use_percolation_multilayer;
	int multi_soilPoreIndexFile;
	int multi_soilPoreIndexOfLyr;
	int multi_soilFcFile;
	int multi_soilKsFile;
	int multi_soilThicknessFile;
	int multi_soilInitMoistureFile;
	int multi_soilPorosityFile;
	int multi_ksFactorValueV;
	int multi_ksFactorValueH;
	int multi_interflowCsValueOfLyr;
	//int use_multilayer_soil;
	//int use_distributed_rain; // xiaodw add, use distribute rainfall or not
	int call_gzip;
	int alt_ascheader;
	int checkpoint;
	int checkfile;
	int calc_evap;
	int rainfall; // TJF: added for time varying, spatially uniform rainfall
	int rainfallmask; //added for distributed rainfall
	int routing; // CCS: added for routing routine. 
	int routing_mass_check; // CCS: added for routing routine. 
	int diffusive_switch; //TFD switch for routing
	int reset_timeinit;
	int profileoutput;
	int porosity;
	int weirs;
	int save_Ts;   // MT: added flag to output adaptive timestep
	int save_QLs;  // MT: added flag to output Qlimits
	int diffusive; // MT: added flag to indicate wish to use diffusive channel solver instead of default kinematic
	int startq;    // MT: added flag to indicate wish to use start flow to calculate initial water depths throughout channel
	int logfile;   // MT: added flag to record logfile
	int startfile; // MT: added flag to note use of startfile
	int start_ch_h; // MT: added flag to note use of starting H in channel
	int comp_out; // TJF: added to make computational output information optional
	int chainagecalc; // MT: added so user can switch off grid independent river chainage calculation
	int mint_hk; // JN: added to request maxH, maxHtm totalHtm and initHtm be calulated at the mass interval
	int Roe; // JN/IV: added to use Roe solver
	int killsim; // MDW: added to flag kill of simulation after specified run time
	int dhoverw; // TJF: added as a switch for dhlin (ON - dhlin set by command line/parfile; OFF - dhlin prescribed by gradient C(0.0002) Cunge et al. 1980)
	int drychecking; //JN Option to turn DryCheck off
	int voutput; // exports velocity esimates based on Q's of Roe velocity (JCN)
	int maxdepthonly; // only export maxdepth file (AS)
	int voutput_max; // if max not required, v doesn't need to be calculated each time step
	int voutput_stage; // only if stages already enabled - can save velocity with stage, without saving velocity grids
	int steadycheck; // MDW: added flag to check for model steady state
	int hazard; // JN additional module for calculating hazards
	int startq2d; // JN: initalises inertial model with uniform flow for Qold
	int Roe_slow; // JN: ghost cell version of Roe solver
	int multiplerivers; // CCS multiple river switch
	int SGC; // JN sub gird channels r
	int SGCbed; // JN sub grid bed elevation file to override hydraulic geometry
	int SGClevee; // NQ levee (outflow only) addition
	int SGCcat_area; // JN sub grid channel accumulated area to override hydraulic geometry based on width
	int SGCchangroup; // turns on distributed channe groups
	int SGCchanprams; // parameters for distributed channel groups
	int binary_out; // JN binary raster output
	OutputParams output_params;
	int gsection; // JN virtual gauge sections
	int binarystartfile; // JN load a binary start file
	int startelev; // used to use an elevation file for the startfile
	int latlong; // CCS: added for lat-long coordinate systems
	int SGCbfh_mode; // JCN switches model to use parameter p as bank full depth
	int SGCA_mode; // JCN switches model to use parameter p as bank full Area
	int dist_routing; // JCN turnes on spatially distributed routing velocity
	int SGCvoutput; // JCN Turns on sub-grid channel velocity output
	int DamMode; // FEOL Turns on reservoir/dam
	int DammaskRead; // FEOL Turns on reservoir/dam
	int ChanMaskRead; // JCN Read channel mask
	int LinkListRead; // JCN Read channel mask
	int saveint_max; //instructs model to save max depth at each stage interval
	int maxint;
	int SGCd8; //PFU flag to choose d8 directions instead of d4 in the sub grid channels
	int cuda;
	int fv1;
	int fv2;
	int dg2;
	int dynamicrainfall;
	int acc_nugrid;
	// xdw add, for water depth output
	int output_icell;
	int counter;
	// xdw add, 支持poi输出
	int save_poi;
	// xdw add, 支持输出tif
	int save_in_tif;
	int save_in_jpg;

};


//-------------------------------------------
// Model Parameters
struct Pars{
	int xsz, ysz;
	NUMERIC_TYPE dx, dx_sqrt;
	NUMERIC_TYPE dy, dA;
	// friction flood plain (when per cell mannings disabled) - (not squared)
	NUMERIC_TYPE FPn;
	NUMERIC_TYPE tlx, tly, blx, bly;
	// xdw add，支持POI输出
	NUMERIC_TYPE SaveInt, MassInt, PoiSaveInt;
	NUMERIC_TYPE SaveTotal, MassTotal, PoiSaveTotal;
	int SaveNo;
	int op_multinum;
	NUMERIC_TYPE *op_multisteps;
	int *op_multiswitch;
	NUMERIC_TYPE op;
	NUMERIC_TYPE InfilRate;
	NUMERIC_TYPE InfilLoss, EvapLoss, RainLoss; // previous mass interval loss
	NUMERIC_TYPE InfilTotalLoss, EvapTotalLoss, RainTotalLoss; // cumulative loss
	// xiaodw, for interflow
	NUMERIC_TYPE Interflow2Ch, InterflowGen, InterflowRunoff;
	NUMERIC_TYPE Interflow2ChTotal, InterflowGenTotal, InterflowRunoffTotal;
	// xiaodw, for groundwater
	//NUMERIC_TYPE Percolation, GroundWaterQ2RiverTotal,GroundWaterStorageTotal;
	// xiaodw, for greenampt
	NUMERIC_TYPE InfilRateGA, AccumuDepthGA;
	NUMERIC_TYPE checkfreq, nextcheck;
	NUMERIC_TYPE reset_timeinit_time;
	NUMERIC_TYPE maxelev, zlev; // Water depth dependent porosity
	int zsz; // Water depth dependent porosity
	int Por_Ident;
	NUMERIC_TYPE dAPor;
	char **ascheader;
	NUMERIC_TYPE ch_start_h; // starting depth of channel flow. default to 2m or read from par file.
	NUMERIC_TYPE killsim_time; // time to kill simulation
	NUMERIC_TYPE steadyQdiff, steadyQtol, steadyInt, steadyTotal; // used for checking steady-state
	NUMERIC_TYPE SGC_p; // sub grid channel width depth exponent
	NUMERIC_TYPE SGC_r; // sub grid channel width depth mutiplier
	int SGCchan_type; // JCN bank slop for trapazoidal channel
	NUMERIC_TYPE SGC_s, SGC_2, SGC_n; // JCN trapazodal channel slope dependent constant
	NUMERIC_TYPE *SGCprams; // pointer to table of SGC parameters
	NUMERIC_TYPE Routing_Speed, RouteInt; // CCS variables controlling routing speed in rainfall routing routine
	NUMERIC_TYPE RouteSfThresh; // CCS water surface slope at which routing scheme takes over from shallow water eqn is SGC mode (when routing==ON).
	NUMERIC_TYPE DiffusiveFroudeThresh;
	NUMERIC_TYPE SGC_m, SGC_a; // allows a meander coefficient to be set for the sub-grid model, default 1, allows channel upstream area to be set, defaul -1;
	NUMERIC_TYPE min_dx, min_dy, min_dx_dy; // CCS added to hold minimum values of dx and dy when using lat-long projected grids.
	NUMERIC_TYPE max_Froude; // maximum Froude for sub grid solver (needs CALCULATE_Q_MODE = 1)
	NUMERIC_TYPE maxint; // writes and resets maximum depth over interval 
	NUMERIC_TYPE maxintTotal; // writes and resets maximum depth over interval 
	int maxintcount; // counts number of maxint saves
	int output_precision;
    NUMERIC_TYPE nodata_elevation; // DEM elevation used for NODATA values
    int drain_nodata; // remove water from DEM NODATA cells
    int limit_slopes; /**< DG2 slope limiter enabled when limit_slopes = ON */
	//***********************xdw add, for subgrid initial Q****************************
	NUMERIC_TYPE sgcStartH = 2;
	//***********************xdw add, for green-ampt****************************
	/// Soil Capillary Suction Head (m)
	NUMERIC_TYPE *capillarySuction;
	/// cumulative infiltration depth (m)
	NUMERIC_TYPE *accumuDepth;
	/// 每个时间步长的下渗深度
	//float *s
	//NUMERIC_TYPE *soilMoisture;
	NUMERIC_TYPE *infil;
	NUMERIC_TYPE *infilCapacitySurplus;
	/// saturated hydraulic conductivity from parameter database (m/s)
	NUMERIC_TYPE *ks;
	/// initail soil moisture
	NUMERIC_TYPE *initSoilMoisture;
	NUMERIC_TYPE *porosity;
	NUMERIC_TYPE *clay;
	NUMERIC_TYPE *sand;
	NUMERIC_TYPE *rootDepth;
	/// user specify value
	NUMERIC_TYPE ksValue;
	NUMERIC_TYPE initSoilMoistureValue;
	NUMERIC_TYPE porosityValue;
	NUMERIC_TYPE clayValue;
	NUMERIC_TYPE sandValue;
	NUMERIC_TYPE rootDepthValue;
	/// -----------------with padding-------------------
	NUMERIC_TYPE *capillarySuctionPD;
	NUMERIC_TYPE *accumuDepthPD;
	/// 每个时间步长的下渗深度
	//float *s
	NUMERIC_TYPE *soilMoisturePD;
	NUMERIC_TYPE *infilPD;
	NUMERIC_TYPE *infilChPD;
	NUMERIC_TYPE *infilCapacitySurplusPD;
	/// saturated hydraulic conductivity from parameter database (mm/h)
	NUMERIC_TYPE *ksPD;
	/// initail soil moisture
	NUMERIC_TYPE *initSoilMoisturePD;
	NUMERIC_TYPE *porosityPD;
	NUMERIC_TYPE *clayPD;
	NUMERIC_TYPE *sandPD;
	NUMERIC_TYPE *rootDepthPD;
	NUMERIC_TYPE ks_factor;
	//***********************xdw add, for constant infiltration****************************
	NUMERIC_TYPE saturation_value;
	NUMERIC_TYPE * soilWaterDepth;
	NUMERIC_TYPE * soilWaterDepthPD;
	//NUMERIC_TYPE soilWaterDepthAvgPercell;
	//***********************xdw add, for interflow****************************
	NUMERIC_TYPE * interflowGenVolPD;
	NUMERIC_TYPE * interflow2ChVolPD;
	NUMERIC_TYPE * interflowRunoffVolPD;
	NUMERIC_TYPE * slopePD;
	NUMERIC_TYPE * slope;
	NUMERIC_TYPE interflow_cs;
	NUMERIC_TYPE interflow_surlag;
	NUMERIC_TYPE interflow_t_conc;
	NUMERIC_TYPE interflow_lagindex;
	NUMERIC_TYPE interflowTstep;
	int interflowSlopeType = 2;
	//***********************xdw add, for wetspa SUR_MR****************************
	NUMERIC_TYPE * rainExcessPD;
	NUMERIC_TYPE * rainExcessPD_Last;
	// 当 m_kRunoff 较大时，alpha 较大，径流比例增长更陡峭（对土壤湿度更敏感）;当 m_kRunoff 接近 1 时，alpha 几乎为常数，径流响应更平缓
	// 典型取值范围:1.0 – 3.0（有时 1.0 – 5.0）
	NUMERIC_TYPE kRun;     ///  Rainfall intensity corresponding to a surface runoff exponent(m_rfExp) of 1, [1,10]，当kRun取1时，alpha=1，径流变得急而陡峭；当kRun取10时，响应越平滑、平缓，更适合“积雪融化、地下水贡献大”的流域
	NUMERIC_TYPE pMax;      	 /// maximum P corresponding to runoffCo, mm/day[10~1000] 一天的净雨量超过该阈值，则径流响应指数 α 收敛为 1.0
	NUMERIC_TYPE* runoffCo;
	NUMERIC_TYPE* runoffCoPD;
	NUMERIC_TYPE alpha = -1;                   // 直接使用alpha
	NUMERIC_TYPE runoffCoFactor = 1.0;   // 径流系数的调节因子
	

	int useRunoffCoType = 2;
	int useAlphaType = 1;


	//***********************xdw add, for percolation and groundwater****************************
	// variables which read from files should have original version and padding version, while other variables calculated by progarm just need padding version
	NUMERIC_TYPE * fieldCapacity;
	NUMERIC_TYPE * poreIndex;  // pore size distribution index
	NUMERIC_TYPE * fieldCapacityPD;
	NUMERIC_TYPE * poreIndexPD;
	NUMERIC_TYPE   poreIndexValue;
	NUMERIC_TYPE   poreIndexScaleFactor;
	NUMERIC_TYPE * rechargePD;

	NUMERIC_TYPE * percolationPD;
	NUMERIC_TYPE * gwStoragePD;
	NUMERIC_TYPE * gndQ2RchPD;    // m_gndQ2Rch, groundwater flow out of the subbasin
	NUMERIC_TYPE recessionCoefficient;
	NUMERIC_TYPE recessionExponent;
	NUMERIC_TYPE deepCoefficient;
	NUMERIC_TYPE sumGndQ2Rch;
	NUMERIC_TYPE sumInterflowQ2Rch;
	NUMERIC_TYPE GwStorageDepth;         // mm
	NUMERIC_TYPE GwStorageDepthMax;  // mm
	NUMERIC_TYPE GwStorageVol;
	NUMERIC_TYPE PercolationDepth;
	NUMERIC_TYPE PercolationVol;
	NUMERIC_TYPE initGwStorageDepth;
	NUMERIC_TYPE gwTstep;
	int sumNCells;
	int sumNSgcCells;
	NUMERIC_TYPE avgCellArea;
	NUMERIC_TYPE gwQPerSgcCell;
	NUMERIC_TYPE interflow2ChVolPerSgcCell;
	NUMERIC_TYPE InterflowRunoffPerSgcCell;
	NUMERIC_TYPE soilMoisAvgPerCell;
	NUMERIC_TYPE soilWaterDepthAvgPerCell;
	NUMERIC_TYPE soilFCAvgPerCell;
	NUMERIC_TYPE soilProsityAvgPerCell;

	//***********************xdw add, for multilayer percolation****************************
	int multi_nSoilLyrs;
	int multi_curSoilLyr;
	//int multi_lyr_soilPerco;
	//int multi_lyr_soilPorosity;
	//int multi_lyr_soilKs;
	//int multi_lyr_soilThickness;
	//int multi_lyr_soilInitMoisture;
	//int multi_lyr_soilPoreIndex;
	NUMERIC_TYPE** multi_soilPoreIndexPD; 
	NUMERIC_TYPE** multi_soilFcPD;
	NUMERIC_TYPE** multi_soilPorosityPD;
	NUMERIC_TYPE** multi_soilKsPD;	
	NUMERIC_TYPE** multi_soilThicknessPD;  // m
	NUMERIC_TYPE** multi_soilDepthPD;       // m
	NUMERIC_TYPE** multi_soilInitMoisturePD;

	NUMERIC_TYPE** multi_soilPercoPD;      // m
	NUMERIC_TYPE** multi_soilWaterDepthPD;   // mm
	NUMERIC_TYPE** multi_soilMoisturePD;
	NUMERIC_TYPE*  multi_ksFactorVOfLyr;
	NUMERIC_TYPE*  multi_soilWtrStoPrfl;
	NUMERIC_TYPE*  multi_soilPoreIndexOfLyr;
	NUMERIC_TYPE ksFactorInfil;

	//NUMERIC_TYPE*  multi_soilMoistureOfLyr;

	NUMERIC_TYPE** multi_soilPoreIndex;
	NUMERIC_TYPE** multi_soilFc;
	NUMERIC_TYPE** multi_soilPorosity;
	NUMERIC_TYPE** multi_soilKs;
	NUMERIC_TYPE** multi_soilThickness;
	NUMERIC_TYPE** multi_soilInitMoisture;

	NUMERIC_TYPE*  multi_soilMoistureOfLyr;
	NUMERIC_TYPE*  multi_soilPercoVolOfLyr;
	NUMERIC_TYPE*  multi_soilPercoDepOfLyr;
	NUMERIC_TYPE*  multi_soilWaterDepthOfLyr;

	NUMERIC_TYPE*  multi_soilMoistureOfLyr_Last;
	NUMERIC_TYPE*  multi_soilPercoVolOfLyr_Last;
	NUMERIC_TYPE*  multi_soilPercoDepOfLyr_Last;
	NUMERIC_TYPE*  multi_soilWaterDepthOfLyr_Last;

	NUMERIC_TYPE* multi_soilPercoDepOfLyr_rate;

	NUMERIC_TYPE* multi_soilFcOfLyr;
	NUMERIC_TYPE* multi_soilProsityOfLyr;

	//***********************xdw add, for multilayer interflow****************************
	NUMERIC_TYPE*    multi_interflowCsValueOfLyr;
	NUMERIC_TYPE**  multi_interflowGenVolPD;
	NUMERIC_TYPE**  multi_interflow2ChVolPD;
	NUMERIC_TYPE**  multi_interflowRunoffVolPD;

	NUMERIC_TYPE*  multi_interflowGenVolOfLyr;
	NUMERIC_TYPE*  multi_interflow2ChVolOfLyr;
	NUMERIC_TYPE*  multi_interflowRunoffVolOfLyr;

	NUMERIC_TYPE*  multi_interflowGenVolOfLyr_Last;
	NUMERIC_TYPE*  multi_interflow2ChVolOfLyr_Last;
	NUMERIC_TYPE*  multi_interflowRunoffVolOfLyr_Last;

	NUMERIC_TYPE* multi_interflowSurlag;
	NUMERIC_TYPE* multi_interflowTconc;
	NUMERIC_TYPE* multi_interflowLagindex;

	//***********************xdw add, for glacier and snow melt****************************
	/// 冰川厚度，同时也反映冰川范围
	NUMERIC_TYPE *glacier;
	NUMERIC_TYPE *snow;
	/// 融化温度阈值
	NUMERIC_TYPE melt_temperature;
	NUMERIC_TYPE glacierValue;
	NUMERIC_TYPE snowValue;
	NUMERIC_TYPE FddSnow; // 积雪度日因子
	NUMERIC_TYPE FddGlacier; // 冰川度日因子
	NUMERIC_TYPE *Ca; // 坡向
	NUMERIC_TYPE Frr;  // 积雪度日因子再冻结修正参数
	// 0 neither, 1 use value, 2 use file 
	int useKsType = 2;
	int useInitSoilMoistureType = 2;
	int usePorosityType = 2;
	int useClayType = 2;
	int useSandType = 2;
	int useRootDepthType = 2;
	int useGlacierType = 2;
	int useSnowType = 2;
	// groundwater
	int useFieldCapacityType = 2;
	int usePoreIndexType = 2;
	// ********************* xiaodw add, for DHSVM**************************
	
	NUMERIC_TYPE* waterLevelPD;     // 地下水位的绝对高程。。。需要初始化，不用读取
	NUMERIC_TYPE* tableDepthPD;    // 地下水距离地表的距离（埋深）。。。
	NUMERIC_TYPE* subFlowGradPD;  /* Magnitude of subsurface flow gradient slope * width */
	unsigned char **subDirPD;             /* Fraction of flux moving in each direction*/
	unsigned int *subTotalDirPD;         /* Sum of subDirPD array */
	NUMERIC_TYPE **multi_adjustPD;        // 由于河道的存在，需要从土壤层扣除河道占据的体积，multi_adjustPD就是调整体积的系数，在没有河道存在时就等于1。。根据每一层的土壤厚度、河道宽度、河道深度数据计算
	NUMERIC_TYPE *soilThicknessAllLyrs;
	NUMERIC_TYPE *soilThicknessAllLyrsPD;   // 土壤总厚度，各层之和.....
	//NUMERIC_TYPE **rootLyrDepthPD;  // 每层的根系层厚度
	NUMERIC_TYPE ksLatValue;
	NUMERIC_TYPE*  multi_ksFactorHOfLyr;
	NUMERIC_TYPE *ksLat;
	NUMERIC_TYPE *ksLatPD;                // 侧向壤中流的分布式ks......  mm/h
	//NUMERIC_TYPE ksLat;                   // 侧向壤中流的ks
	//NUMERIC_TYPE *KsLatExpPD;       // 侧向壤中流的指数
	NUMERIC_TYPE KsLatExpValue = 0.0;       // 侧向壤中流的指数, 不需要了，每个格子都在预处理时根据土壤属性加权平均计算ks
	NUMERIC_TYPE *satFlowPD;            // 侧向壤中流。。。
	NUMERIC_TYPE *satFlow2ChPD;          // 补给河道的侧向饱和壤中流
	NUMERIC_TYPE *satFlow2ChLyr;          // 补给河道的侧向饱和壤中流 每层总量
	NUMERIC_TYPE *satFlow2ChLyr_Last;
	NUMERIC_TYPE *satFlow2ChLyr_Rate;
	NUMERIC_TYPE *satFlow2NeiborPD;   // 流向相邻栅格的侧向饱和壤中流
	NUMERIC_TYPE *satFlow2SurfPD;       // 溢流到地表的壤中流
	NUMERIC_TYPE *PercExcess2SurfPD;
	NUMERIC_TYPE *delta_volume_grid_ch; //河道内的水量变化
	NUMERIC_TYPE soilWaterDepthThresh = 0.0;  
	int neighbor_ref[NNEIGHBORS];
	int *neighbor_col_ref;
	int *neighbor_row_ref;
	int multi_nRootLyrs;                         // 根系层的层数，等于multi_nSoilLyrs-1
	NUMERIC_TYPE subSurfaceLatFlow2Channel_rate;
	NUMERIC_TYPE subSurfaceLatFlow2ChTotal;
	NUMERIC_TYPE subSurfaceLatFlowTotal2Ch_Last;
	NUMERIC_TYPE *subSurfaceWaterDepth;

	NUMERIC_TYPE subSurfaceLatFlow2Surf_rate;
	NUMERIC_TYPE subSurfaceLatFlow2SurfTotal;
	NUMERIC_TYPE subSurfaceLatFlowTotal2Surf_Last;

	NUMERIC_TYPE subSurfacePerc2Surf_rate;
	NUMERIC_TYPE subSurfacePerc2SurfTotal;
	NUMERIC_TYPE subSurfacePercTotal2Surf_Last;
	NUMERIC_TYPE evapTstep;

	NUMERIC_TYPE surfaceFlow2Ch_rate;
	NUMERIC_TYPE surfaceFlow2ChTotal;
	NUMERIC_TYPE surfaceFlow2ChTotal_Last;

	NUMERIC_TYPE surfaceHydro2Ch_rate;
	NUMERIC_TYPE surfaceHydro2ChTotal;
	NUMERIC_TYPE surfaceHydro2ChTotal_Last;
	
	int useSoilThicknessAllLyrsType = 2;
	int useKsLatType = 2;


	// ********************* xiaodw add, for AET PriestleyTaylor Hargreaves**************************
	NUMERIC_TYPE esco;  // 土壤蒸发补偿系数
	int multi_nSoilEvapLyrs;
	NUMERIC_TYPE etco;   // evaporation threshold coefficient（蒸发限制系数）
	NUMERIC_TYPE runoffCoVal;
	// ********************* xiaodw add, for XAJ evap**************************
	NUMERIC_TYPE* es;         // 输出：地表水蒸发,mm
	NUMERIC_TYPE* eu;         // 输出：上层蒸发,mm
	NUMERIC_TYPE* el;          // 输出：中层蒸发,mm
	NUMERIC_TYPE* ed;         // 输出：深层蒸发,mm
	NUMERIC_TYPE c;             // 深层蒸发系数 无量纲, 在 南方多林地区可达0.18，而对北方半湿润地区约为0.08, ref. 新安江模型参数的分析.赵人俊 1988
	// ********************* xiaodw add, for sgc bed evap and infil**************************
	//NUMERIC_TYPE* sgcBedSoilThicknessPD;  // 河床底部土壤厚度，等于土壤总厚度-河堤高度，mm
	//NUMERIC_TYPE* sgcBedSoilMoisturePD;   // 河床底部土壤湿度
	//NUMERIC_TYPE* sgcBedSoilPorosityPD;    //  河床底部土壤孔隙度
	NUMERIC_TYPE* sgcBedSoilLyrPD;             // 河床底部土壤对应其栅格土壤的第几层

	// ********************* xiaodw add, modify up layer interflow for DHSVM**************************
	//NUMERIC_TYPE** tableDepthUpLyrPD;
	NUMERIC_TYPE *satFlowUpPD;            // 侧向壤中流
	NUMERIC_TYPE *satFlowUp2ChPD;          // 补给河道的侧向饱和壤中流
	NUMERIC_TYPE* tableDepthUpLyrPD;
	NUMERIC_TYPE* waterLevelUpLyrPD;
	int * lyrOfWaterTableUpLayer;
	NUMERIC_TYPE* subFlowGradUpLyrPD;
	unsigned char **subDirUpLyrPD;             /* Fraction of flux moving in each direction*/
	//unsigned char ***subDirUpLyrPD;
	unsigned int *subTotalDirUpLyrPD;         /* Sum of subDirPD array */
	//NUMERIC_TYPE** curLyrWaterTable;
	NUMERIC_TYPE* surflow2ChPD;    // 地表汇入河道的流量
	NUMERIC_TYPE* hydro2ChPD;     // 降雨-入渗后进入河道像元的流量
	NUMERIC_TYPE* delta_volumn_dhsvm_PD;

	// adaptation. to merge?
//	NUMERIC_TYPE       xmin;
//	NUMERIC_TYPE       xmax;
//	NUMERIC_TYPE       ymin;
//	NUMERIC_TYPE       ymax;
//	Coordinate xsz;
//	Coordinate ysz;
//	NUMERIC_TYPE       g;
//	NUMERIC_TYPE       time;
//	NUMERIC_TYPE       manning;

};

// Solver settings
struct Solver{
	NUMERIC_TYPE t;
	NUMERIC_TYPE g;
	NUMERIC_TYPE divg;
	NUMERIC_TYPE cfl;
	int    ts_multiple; // channel timestep multiple for running 1D decoupled from 2D
	long   Nit, itCount;
	NUMERIC_TYPE Sim_Time;
	NUMERIC_TYPE InitTstep; // Maximum timestep
	NUMERIC_TYPE Tstep;  // Adapting timestep
	NUMERIC_TYPE MinTstep;  // Stores minimum timestep during simulation
	NUMERIC_TYPE SolverAccuracy;
	int dynsw; // Switch for full dynamic steady state or diffusive steady state
	NUMERIC_TYPE Impfactor;
	NUMERIC_TYPE Hds;
	NUMERIC_TYPE vol1, vol2;
	NUMERIC_TYPE Qerror;
	NUMERIC_TYPE Verror;
	NUMERIC_TYPE FArea; // Store flooded area
	NUMERIC_TYPE DepthThresh, MomentumThresh, MaxHflow;
	NUMERIC_TYPE dhlin;
	NUMERIC_TYPE htol;
	NUMERIC_TYPE Qlimfact; // MT added to allow user to relax Qlimit
	NUMERIC_TYPE itrn_time;
	NUMERIC_TYPE itrn_time_now;
	NUMERIC_TYPE SGCtmpTstep; // JCN added to enable time step calculatin in UpdateH for SGC method
	time_t time_start;
	time_t time_finish;
	time_t time_check;
	NUMERIC_TYPE theta; //GAMA added for q-centred scheme
	int fricSolver2D; //GAMA: Solves the friction term using the vectorial (2D) scheme
	NUMERIC_TYPE maxH; /**< maximum H in the domain at the current time */
	NUMERIC_TYPE krivodonova_threshold; /**< DG2 slope detector */
	NUMERIC_TYPE SpeedThresh; /**< FV1/DG2 threshold for friction application */
    NUMERIC_TYPE DG2DepthThresh; /**< Threshold above which DG2 L1 operator is activated */
    NUMERIC_TYPE DG2ThinDepthTstep; /**< Tstep assigned to cells with thin depths */

	// adaptation
	NUMERIC_TYPE epsilon; // error threshold for adaptation
	int L; // max resolution level for adaptation

	// xdw add, support distributed rainfall
	char rain_begin_time[256];
	char rain_end_time[256];

	char seims_begin_time[256];
	char seims_end_time[256];
	int rain_time_step;   // 降雨间隔时间 s
};


//-------------------------------------------
// ChannelSegmentType
struct ChannelSegmentType{
	NUMERIC_TYPE *Chandx;
	NUMERIC_TYPE *Shalf;
	NUMERIC_TYPE *Chainage;
	NUMERIC_TYPE *ChanQ; // only for recording Q for output in profile
	NUMERIC_TYPE *A;
	NUMERIC_TYPE *NewA;
	NUMERIC_TYPE *ChanWidth;
	NUMERIC_TYPE *ChanN;
	int   *ChanX;
	int 	*ChanY;
	// BC_Val used in case of fixed flow (otherwise set to 0)
	NUMERIC_TYPE *Q_Val;
	// time series indexed by bci (boundary condition index) //TFD
	// Q_TimeSeries used in case of var e.e. HVAR or QVAR (otherwise set to NULL)
	TimeSeries **Q_TimeSeries;
	ESourceType *Q_Ident;
	char  *Q_Name;
	NUMERIC_TYPE *BankZ;
	int chsz;
	int Next_Segment;
	int Next_Segment_Loc;
	int N_Channel_Segments;
	NUMERIC_TYPE JunctionH; // allows recording of H data for dummy junction point of tributary, without overwriting main channel info
	NUMERIC_TYPE JunctionDEM; // allows recording of DEM data for dummy junction point of tributary, without overwriting main channel info
};

// DamType //FEOL
// structure is needed for DynamicEdges as list per dam is needed
struct DamEdge {
	int *EdgeCell;
};
struct DamData {
	NUMERIC_TYPE *DynamicEdgeData;
	int *DynamicEdge;
	//NUMERIC_TYPE *Outputcell; Not needed
	NUMERIC_TYPE *Volmax;
	NUMERIC_TYPE *DamArea;
	NUMERIC_TYPE *InitialHeight; //Initial Height of Dam
	NUMERIC_TYPE *DamHeight; // Internal Depth of Dam for Vol calc
	NUMERIC_TYPE *SpillWidth;
	NUMERIC_TYPE *Spill_Cd;
	NUMERIC_TYPE *SpillHeight; // Crest Height of Spill
	NUMERIC_TYPE *DamOperationQ;
	int *DamOperationCode; //1 is default and removes MeanQ for Dam as Operation Rule
	NUMERIC_TYPE *DamMeanQ;
	NUMERIC_TYPE *DamVin;
	NUMERIC_TYPE *SpillQ;
	NUMERIC_TYPE *DamTotalQ;
	NUMERIC_TYPE *DamVol;
	NUMERIC_TYPE DamLoss;
	NUMERIC_TYPE *AnnualRelease;
	NUMERIC_TYPE *OP7_Kappa;
	NUMERIC_TYPE *OutputCellX;
	NUMERIC_TYPE *OutputCellY;
	int NumDams;
	int *Edgenos;
	int TotalEdge;
	int DamMaxH;//Check FEOL 21July
	int *DamYear;
	};



//-------------------------------------------
/* QID7_Store // CCS for temp storage of trib boundary condition info when (Q_Ident_tmp[i]==7) in LoadRiver. A vector containing these
structures is built in LoadRiver function and used in UpdateChannelsVector function. */
struct QID7_Store{
	int trib;
	int Next_Segment_Loc;
	int chseg;
	int RiverID;
};
//---------------------DataTypes.h----------------------
struct IndexRange
{
	int start;
	int end;
};

struct WetDryRowBound
{
	// track start and end of the inundation boundary 淹没范围的起止号
	IndexRange * fp_h;

	// track previous start and end of the inundation boundary
	// used to zero any flows that are outside the normal processing bounds
	IndexRange * fp_h_prev;

	// any update to volume should set the volume bounds, to ensure update h is processed 淹没范围的起止号
	IndexRange * fp_vol;

	// first cell where not nodata
	IndexRange * dem_data;

	int block_count;
	/// list of row indexes (j's)
	IndexRange * block_row_bounds;
};

void AllocateWetDryRowBound(int row_count, int block_count, WetDryRowBound * wet_dry_bound);

// each cell has a list of flow_indexes that flow in/out of a cell
// indexes point to the SubGridFlowInfo.sg_flow_Q 
struct SubGridFlowLookup
{
	//index 0 : dx
	//index 1 : dy
	//index 2 : d 45 degrees right (d8)
	//index 3 : d 45 degrees left (d8)
	int flow_add[4]; // in d8 this must be 4 (PFU changed from 2 to 4)
	int flow_subtract[4]; // in d8 this must be 4 (PFU changed from 2 to 4)
};


struct SubGridState
{
	// SubGridFlowInfo: flow_count number of items stored
	NUMERIC_TYPE * sg_flow_Q;
	// SubGridFlowInfo: flow_count number of items stored
	//NUMERIC_TYPE * Flow_CurrentChannelWidth;

	NUMERIC_TYPE * sg_velocity;

};



/// info - for a sub grid cell
/// all lists are 0 to cell_count
struct SubGridCellInfo
{
	// total number of memory space allocated
	int cell_count;

	int *sg_cell_x;
	int *sg_cell_y;
	int *sg_cell_grid_index_lookup;

	NUMERIC_TYPE *sg_cell_cell_area; // surface area - from above.
	NUMERIC_TYPE *sg_cell_dem;
	NUMERIC_TYPE *sg_cell_cell_infil_rate; // for distrubuted infiltration rates

	NUMERIC_TYPE *sg_cell_SGC_width; // channel width constant
	NUMERIC_TYPE *sg_cell_SGC_BankFullHeight;
	NUMERIC_TYPE *sg_cell_SGC_BankFullVolume;
	NUMERIC_TYPE *sg_cell_SGC_c;

	int * sg_cell_SGC_group;
	int * sg_cell_SGC_is_large; //if SGC_width > C(0.5)*(row_cell_dx + row_cell_dy), then there is no flood plain cell calc for evap
};

void AllocateSubGridCellInfo(int cell_count, SubGridCellInfo * sub_grid_cell_info);
void ZeroSubGridCellInfo(SubGridCellInfo * sub_grid_cell_info, int cell_index);

/// used to store point source info and boundary condition info
struct WaterSource
{
	// total number of memory space allocated
	int count;

	//char  *Name;
	ESourceType   *Ident;
	// PS_Val used in case of fixed e.g. HFIX or QFIX (otherwise set to -1)
	NUMERIC_TYPE *Val;
	// time series indexed by psi (point source index) //TFD
	// PS_TimeSeries used in case of var e.e. HVAR or QVAR (otherwise set to NULL)
	TimeSeries **timeSeries;

	SubGridCellInfo ws_cell;

	NUMERIC_TYPE *Q_FP_old;
	NUMERIC_TYPE *Q_SG_old;

	NUMERIC_TYPE *g_friction_squared_FP; // friction for this point source cell (pre-calculated from mannings)
	NUMERIC_TYPE *g_friction_squared_SG; // friction for this point source cell (pre-calculated from mannings)
};

void AllocateWaterSource(int count, WaterSource * waterSource);

///
/// arrays prefixed with Flow_ have flow_count items
/// arrays prefixed with Cell_ have flow_count*2 items
/// this is because flow_pair are stored for source and destination
/// flow_pair.xxx[i*2] is source
/// flow_pair.xxx[i*2+1] is dest
struct SubGridFlowInfo
{
	/// (dx or dy) * meander
	NUMERIC_TYPE *sg_flow_effective_distance;
	NUMERIC_TYPE *sg_flow_g_friction_sq;

	SubGridCellInfo flow_pair;
	// index of the cell state Cell_h
	// i.e. when looping over flows, the cell_h can be retreived
	int * sg_pair_cell_index_lookup;

	// indexed by cell_count
	// each cell has list of indexes into the sg_flow_Q array
	SubGridFlowLookup * sg_cell_flow_lookup;

	NUMERIC_TYPE * sg_flow_ChannelRatio;

};

// supergrid channels structure and data setup function
struct SuperGridLinksList
{
	int num_links;
	int *link_index_SGC_i, *link_index_2D_i, *link_index_SGC_j, *link_index_2D_j, *link_index_SGC, *link_index_2D;
	NUMERIC_TYPE *SGC_z, *DEM_z, *gn2, *w, *Qold, *dx, *SGC_bfH;
};
void InitSuperLinksStructure(const int grid_rows, const int grid_cols, const int grid_cols_padded, SuperGridLinksList * super_linksptr, States *Statesptr, Pars *Parptr, Arrays *Arrptr, SGCprams *SGCptr, Solver *Solverptr, Fnames * Fnameptr, int verbose);
void AllocateSuperLinksMemory(int n_links, SuperGridLinksList * Super_linksptr);

struct SubGridRowList
{
	int row_cols_padded;

	// grid_rows items
	int * flow_row_count;
	// grid_rows items
	int * cell_row_count;

	// each row has flow_row_count items
	// in memory each row is padded to row_cols_padded
	// indexed from j * row_cols_padded + 0 
	// to j * row_cols_padded + cell_row_count[j]
	// 
	SubGridFlowInfo flow_info;

	// each row has cell_row_count items
	// in memory each row is padded to row_cols_padded
	// indexed from j * row_cols_padded + 0 
	// to j * row_cols_padded + cell_row_count[j]
	//
	// e.g. (* means cell has info, 0 means cell info not present)
	//    row_cols_padded = 8
	//    row_count  j,  row_count
	//               0,  3
	//               1,  4
	//               2,  0
	//               3,  1
	//
	// 0  |*|*|*|0|0|0|0|0|
	// 1  |*|*|*|*|0|0|0|0|
	// 2  |0|0|0|0|0|0|0|0|
	// 3  |*|0|0|0|0|0|0|0|
	//
	SubGridCellInfo cell_info;
};


struct PointSourceRowList
{
	int row_cols_padded;
	int * ps_row_count;

	WaterSource ps_info;

	NUMERIC_TYPE Qpoint_pos; // Vol per sec // replace Qpoint with positive and negative versions to keep track of input or output for point sources
	NUMERIC_TYPE Qpoint_neg; // Vol per sec 
};

struct BoundaryCondition
{
	WaterSource bc_info;

	NUMERIC_TYPE Qin;
	NUMERIC_TYPE Qout;
	NUMERIC_TYPE QChanOut;
	NUMERIC_TYPE VolInMT; // added by JCN stores volume in over mass inteval
	NUMERIC_TYPE VolOutMT; // added by JCN stores volume out over mass inteval
};


struct WeirLayout
{
	int row_cols_padded;
	// count of weirs per row in qx direction
	int * weir_Qx_row_count;
	// count of weirs per row in qy direction
	int * weir_Qy_row_count;

	// weir_index_qx contains the index of the weir, in the weir lists (below)
	// indexed by j*row_cols_padded + 0 to j*row_cols_padded + row_weir_count
	int *weir_index_qx;
	// weir_index_qy contains the index of the weir, in the weir lists (below)
	// indexed by j*row_cols_padded + 0 to j*row_cols_padded + row_weir_count
	int *weir_index_qy;

	// weirs are not stored by row.
	// just using the old list of weirs.
	// find the indexes into this list: row weir_index_qx and weir_index_qy
	int weir_count;

	int *Weir_grid_index;
	NUMERIC_TYPE *Weir_hc;
	NUMERIC_TYPE *Weir_Cd;
	NUMERIC_TYPE *Weir_m;
	NUMERIC_TYPE *Weir_w;
	NUMERIC_TYPE *Weir_g_friction_sq;
	EDirection *Weir_Fixdir;
	EWeirType *Weir_Typ;

	NUMERIC_TYPE *Weir_Q_old_SG;
	// 2 * weir_count items stored
	// 2 * weir_id = flow to the north or west
	// 2 * weir_id + 1 = flow to the south or east
	int * Weir_pair_stream_flow_index;
	SubGridCellInfo cell_pair;
};

void AllocateWeir(int count, WeirLayout * waterSource);

struct RouteDynamicList
{
	// row_cols_padded not uses as it is set to maximum possible i.e. grid_cols_padded
	// although a full grid of data is used, it will not be a drain on memory bandwidth, since only a few will be generally accesses.

	// count of weirs per row in y
	int * row_route_qx_count;
	int * row_route_qy_count;

	// x coordinate of the qx route (y coordinate is row)
	int * route_list_i_lookup_qx;
	// x coordinate of the qy route (y coordinate is row)
	int * route_list_i_lookup_qy;

};
struct LISFLOODFPContext {
	int grid_cols;
	int grid_rows;
	int grid_cols_padded;

	NUMERIC_TYPE* h_grid;
	NUMERIC_TYPE* volume_grid;
	NUMERIC_TYPE* Qx_grid;
	NUMERIC_TYPE* Qy_grid;
	NUMERIC_TYPE* Qx_old_grid;
	NUMERIC_TYPE* Qy_old_grid;
	NUMERIC_TYPE* maxH_grid;
	NUMERIC_TYPE* maxHtm_grid;
	NUMERIC_TYPE* initHtm_grid;
	NUMERIC_TYPE* totalHtm_grid;
	NUMERIC_TYPE* maxVc_grid;
	NUMERIC_TYPE* maxVc_height_grid;
	NUMERIC_TYPE* maxHazard_grid;
	NUMERIC_TYPE* Vx_grid;
	NUMERIC_TYPE* Vy_grid;
	NUMERIC_TYPE* Vx_max_grid;
	NUMERIC_TYPE* Vy_max_grid;

	NUMERIC_TYPE* dem_grid;
	NUMERIC_TYPE* g_friction_sq_x_grid;
	NUMERIC_TYPE* g_friction_sq_y_grid;
	NUMERIC_TYPE* friction_x_grid;
	NUMERIC_TYPE* friction_y_grid;
	NUMERIC_TYPE* dx_col;
	NUMERIC_TYPE* dy_col;
	NUMERIC_TYPE* cell_area_col;
	NUMERIC_TYPE* Fp_xwidth;
	NUMERIC_TYPE* Fp_ywidth;

	SubGridRowList* sub_grid_layout_rows;
	SubGridState* sub_grid_state_rows;
	SubGridRowList* sub_grid_layout_blocks;
	SubGridState* sub_grid_state_blocks;
	NUMERIC_TYPE* SGC_BankFullHeight_grid;

	TimeSeries* evap_time_series;
	NetCDFVariable* evap_grid;
	TimeSeries* rain_time_series;
	TimeSeries* temperature_time_series;
	NUMERIC_TYPE* rain_grid;
	NUMERIC_TYPE* dist_infil_grid;

	WetDryRowBound* wet_dry_bounds;
	PointSourceRowList* ps_layout;
	BoundaryCondition* boundary_cond;
	WeirLayout* weirs_weirs;
	WeirLayout* weirs_bridges;
	RouteDynamicList* route_dynamic_list;
	NUMERIC_TYPE* route_V_ratio_per_sec_qx;
	NUMERIC_TYPE* route_V_ratio_per_sec_qy;

	timeval timstr;
	double processing_start_time;
	double processing_end_time;
	double total_write_time;
	double loop_start;
	time_t seims_begin_timestamp;
	time_t seims_end_timestamp;

	NUMERIC_TYPE curr_time;
	time_t rain_begin_timestamp;
	time_t rain_end_timestamp;
	int last_rain_time;
	char * nextRainTifName;
	char * nextRainTifPath;
	vector<string> tifFileTimes;
	NUMERIC_TYPE* rainfall_no_padding;
	NUMERIC_TYPE depth_thresh;
	NUMERIC_TYPE last_gw_time;
	NUMERIC_TYPE * tmp_grid1;
	NUMERIC_TYPE * tmp_grid2;
	NUMERIC_TYPE * tmp_grid3;
	int* steadyCount;
	int stop_loop;
	int verbose;

	NUMERIC_TYPE ** tmp_thread_data;
	NUMERIC_TYPE ** tmp_thread_data_ch;

	// for coupling

};

struct SeimsUpstream {
	int seims_id;
	char* inflow_pt_name;
	NUMERIC_TYPE qIn;

	// to find location in lfp
	int ws_index;
	int ps_index;
	int ps_x;
	int ps_y;
	int grid_index;
};

struct LfpCouplingInfo {
	vector<SeimsUpstream> seims_up_list;
	int seims_down_id;
	vector<NUMERIC_TYPE> qOutList;
	NUMERIC_TYPE qOutOneSeimsStep;
};

void AllocateRoutingDynamicList(int rows, int grid_cols_padded, RouteDynamicList * route_dynamic_list);

int LisFloodFP_Initilize(int argc, char *argv[], Arrays *Arrptr, Files* FpsPtr, Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr, Stage *Stageptr, SGCprams *SGCptr, DamData *Damptr,
	vector<ChannelSegmentType> *ChannelSegmentsVecPtr, LISFLOODFPContext* LFPContextPtr, LfpCouplingInfo * LfpCouplingInfoPtr, SuperGridLinksList *Super_linksptr, char* tmpFileNamePtr, char* tmpSysCmdPtr);

int LisFloodFP_Finilize(Solver *Solverptr, Arrays *Arrptr, Fnames *Fnameptr, Files* FpsPtr, States *Statesptr, Pars *Parptr, LISFLOODFPContext *LFPContext, char* tmpFileNamePtr);
void Fast_RunStep(Arrays *Arrptr, Files *Fptr, Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, SGCprams * SGCptr, DamData *Damptr, Stage *Locptr, LISFLOODFPContext *LFPContext, SuperGridLinksList *Super_linksptr, LfpCouplingInfo * LfpCouplingInfoPtr);

//---------------------utility.h----------------------
void *memory_allocate(size_t size);
void* memory_allocate_aligned(size_t size, size_t alignment = 64);

NUMERIC_TYPE*memory_allocate_zero_numeric_legacy(size_t size);
NUMERIC_TYPE*memory_allocate_numeric_legacy(size_t size);
void memory_free_legacy(void** memory);
void memory_free_legacy(int** memory);
void memory_free_legacy(NUMERIC_TYPE** memory);

void memory_free(void** memory);
void memory_free(int** memory);
void memory_free(NUMERIC_TYPE** memory);


void memory_free(void** memory, size_t size);

char *trimwhitespace(char *str);

void SetArrayValue(int* arr, int value, int length);
//-------------------------------------------
/*


*****************************************************************************

Define the function prototypes
---------------------
Prototypes split into approximate groups that correspond to file locations
for easier editing.

*****************************************************************************
*/

// Input prototypes - input.cpp
void ReadConfiguration
(
	int argc,
	char *argv[],
	Fnames *Fnameptr,
	States *Statesptr,
	Pars *Parptr,
	Solver *Solverptr,
	const int verbose
);

int ReadVerboseMode(int argc, char *argv[]);
void ReadCommandLine(int argc, char *argv[], Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, int verbose);
void ReadParamFile(char *, Fnames *, States *, Pars *, Solver*, int);
void CheckParams(Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, int verbose);
void LoadDEM(Fnames *, States *, Pars *, Arrays *, const int verbose);
FILE* LoadDomainGeometry(const char* filename, Pars *Parptr, const int verbose, NUMERIC_TYPE& no_data_value);
void LoadDEMData(Pars*, NUMERIC_TYPE *DEM, FILE *fp, NUMERIC_TYPE file_nodata_value);
void LoadManningsn(Fnames *, Pars *, Arrays *, const int verbose);
void LoadDistInfil(Fnames *Fnameptr, Pars *Parptr, Arrays *Arrptr, const int verbose);
void LoadSGCManningsn(Fnames *, Pars *, Arrays *, const int verbose);
void LoadSGCdirn(Fnames *, Pars *, Arrays *, const int verbose);
void LoadRiverNetwork(Fnames *, States *, Pars *, vector<ChannelSegmentType> *, Arrays *, vector<QID7_Store> *, vector<int> *, const int verbose); // CCS
void LoadRiver(Fnames *, States *, Pars *, vector<ChannelSegmentType> *, Arrays *, vector<QID7_Store> *, vector<int> *, const int verbose);
void UpdateChannelsVector(States *, ChannelSegmentType *, vector<QID7_Store> *, QID7_Store *, int *); // CCS
void LoadStart(Fnames *, States *, Pars *, Arrays *, SGCprams *, const int verbose);
void LoadStartQ2D(Fnames*, Pars*, Arrays*, const int verbose);
void LoadBCs(Fnames *Fnameptr, States *Statesptr, Pars *Parptr, BoundCs *BCptr, LfpCouplingInfo * LfpCouplingInfoPtr, const int verbose);
// xdw add, 加载兴趣点，以输出降雨、入渗、蒸发、水深、流量变化
void LoadPOIs(Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Pois *Poisptr, const int verbose);
void LoadBCVar(Fnames *, States *, Pars *, BoundCs *, ChannelSegmentType *, Arrays *, vector<ChannelSegmentType> *, const int verbose);
void LoadWeir(Fnames *, States *, Pars *, Arrays *, const int verbose);
void LoadStages(Fnames *, States *, Pars *, Stage *, const int verbose);
void LoadGauges(Fnames *, States *, Pars *, Stage *, const int verbose);
void LoadPor(Fnames *, States *, Pars *, Arrays *, const int verbose);
void LoadEvap(Fnames *, Arrays *, const int verbose);
void LoadRain(Fnames *, Arrays *, const int verbose);
void LoadDataByCSV(char *filename, TimeSeries ** dataSeries, const int verbose, int col_index);
void LoadRainmask(Fnames *Fnameptr, Pars *Parptr, Arrays *Arrptr, States *Statesptr, const int verbose);
void LoadSGC(Fnames *Fnameptr, Pars *Parptr, Arrays *Arrptr, States *Statesptr, const int verbose);
void LoadBinaryStart(Fnames *, States *, Pars *, Arrays *, SGCprams *SGCptr, const int verbose);
void LoadSGCChanPrams(Fnames *, States *, Pars *, SGCprams *, const int verbose);
void LoadDamPrams(Fnames *Fnameptr, States *Statesptr, Pars *Parptr, DamData *Damptr, const int verbose); //FEOL
void LoadDamMask(Fnames *Fnameptr,Pars *Parptr, Arrays *Arrptr, DamData *Damptr, const int verbose);  //FEOL
void loadSoilPropertiesGASinglelayer(Fnames *Fnameptr,Pars *Parptr,int m_nCells);
void loadSoilPropertiesGAMultilayer(Fnames *Fnameptr, Pars *Parptr, int m_nCells);
void loadSoilPropertiesInterflow(Fnames *Fnameptr, Pars *Parptr, int m_nCells);
void loadSoilPropertiesPerco_Multilayer(Fnames *Fnameptr, Pars *Parptr, States *Statesptr, int m_nCells);
void loadSoilPropertiesGW(Fnames *Fnameptr, Pars *Parptr, int m_nCells);
void loadSoilPropertiesDHSVM(Fnames *Fnameptr, Pars *Parptr, int m_nCells);
void loadSoilPropertiesWetspa(Fnames *Fnameptr, Pars *Parptr, int m_nCells);
void LoadProperty(int type, string paramName, NUMERIC_TYPE * paramPtr, char * filename, NUMERIC_TYPE value, int m_nCells);
void loadGlacierSnowProperties(Fnames *Fnameptr, Pars *Parptr, int m_nCells);
void LoadTimeVaringTemperature(Fnames *Fnameptr, Arrays *Arrptr, const int verbose);
void initialize_array(float * arr, const int grid_rows, const int grid_cols, const int grid_cols_padded);
FILE* fopen_or_die(const char * filename, const char* mode, const char* message = "", const int verbose = OFF);

// LISFLOOD Solution prototypes - iterateq.cpp
void IterateQ(Fnames *, Files *, States *, Pars *, Solver*, BoundCs *, Stage *, ChannelSegmentType *, Arrays *, SGCprams *, vector<int> *, int *, vector<ChannelSegmentType> *, const int verbose);
void UpdateH(States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *);

// Floodplain prototypes - fp_flow.cpp
void FloodplainQ(States *, Pars *, Solver *, Arrays *, SGCprams *);
NUMERIC_TYPE CalcFPQx(int i, int j, States *, Pars *, Solver *, Arrays *, NUMERIC_TYPE * TSptr);
NUMERIC_TYPE CalcFPQy(int i, int j, States *, Pars *, Solver *, Arrays *, NUMERIC_TYPE * TSptr);
int MaskTest(int m1, int m2);
int MaskTestAcc(int m1);

// Channel prototypes - ch_flow.cpp
void SetChannelStartH(States *Statesptr, Pars *Parptr, Arrays *Arrptr, ChannelSegmentType *ChannelSegments, vector<int> *, int *);
void SetChannelStartHfromQ(States *Statesptr, Pars *Parptr, Arrays *Arrptr, ChannelSegmentType *ChannelSegments, Solver *, vector<int> *, int *);
void CalcChannelStartQ(States *Statesptr, Pars *Parptr, Arrays *Arrptr, ChannelSegmentType *ChannelSegments, vector<int> *, int *);
void ChannelQ(NUMERIC_TYPE deltaT, States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *, vector<int> *, int *);
NUMERIC_TYPE CalcA(NUMERIC_TYPE n, NUMERIC_TYPE s, NUMERIC_TYPE w, NUMERIC_TYPE Q);
NUMERIC_TYPE BankQ(int chani, ChannelSegmentType *, Pars *, Arrays *);
NUMERIC_TYPE ChannelVol(States *, Pars *, ChannelSegmentType *, Arrays *);
NUMERIC_TYPE CalcQ(NUMERIC_TYPE n, NUMERIC_TYPE s, NUMERIC_TYPE w, NUMERIC_TYPE h);
NUMERIC_TYPE Newton_Raphson(NUMERIC_TYPE Ai, NUMERIC_TYPE dx, NUMERIC_TYPE a0, NUMERIC_TYPE a1, NUMERIC_TYPE c, Solver *);

// Diffusive channel solver specific functions
void ChannelQ_Diff(NUMERIC_TYPE deltaT, States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *, vector<int> *, int *);
void bandec(NUMERIC_TYPE **a, int n, int m1, int m2, NUMERIC_TYPE **al, int indx[], NUMERIC_TYPE &d);
void banbks(NUMERIC_TYPE **a, int n, int m1, int m2, NUMERIC_TYPE **al, int indx[], NUMERIC_TYPE b[]);
void SWAP(NUMERIC_TYPE &a, NUMERIC_TYPE &b);
void calcF(NUMERIC_TYPE *x, NUMERIC_TYPE *xn, NUMERIC_TYPE *f, NUMERIC_TYPE dt, ChannelSegmentType *csp, Pars *Parptr, Arrays *Arrptr, NUMERIC_TYPE Qin, int chseg, NUMERIC_TYPE WSout, int HoutFREE, Solver *Solverptr, int low);
void calcJ(NUMERIC_TYPE *x, NUMERIC_TYPE *xn, NUMERIC_TYPE **J, NUMERIC_TYPE dt, ChannelSegmentType *csp, int chseg, int HoutFREE);
NUMERIC_TYPE norm(NUMERIC_TYPE *x, int n);
NUMERIC_TYPE CalcEnergySlope(NUMERIC_TYPE n, NUMERIC_TYPE w, NUMERIC_TYPE h, NUMERIC_TYPE Q);
//void precond(NUMERIC_TYPE **a, int n);

// Acceleration floodplain solver
NUMERIC_TYPE CalcFPQxAcc(int i, int j, States *, Pars *, Solver *, Arrays *);
NUMERIC_TYPE CalcFPQyAcc(int i, int j, States *, Pars *, Solver *, Arrays *);
NUMERIC_TYPE CalcMaxH(Pars *, Arrays *);
void CalcT(Pars *, Solver *, Arrays *);
void UpdateQs(Pars *, Arrays *);




// Boundary prototypes - boundary.cpp
void BCs(States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *);
void BoundaryFlux(States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *, vector<ChannelSegmentType> *);
NUMERIC_TYPE InterpolateTimeSeries(TimeSeries *timeSeries, NUMERIC_TYPE t);

/*
 * Implements a free boundary condition for irregular domains by zeroing
 * water depths over cells with DEM value nodata_elevation
 */
void drain_nodata_water(Pars*, Solver*, BoundCs*, Arrays*);

NUMERIC_TYPE RoeBCy(int edge, int p0, int p1, int pq0, NUMERIC_TYPE z0, NUMERIC_TYPE z1, NUMERIC_TYPE hl, NUMERIC_TYPE hr, NUMERIC_TYPE hul, NUMERIC_TYPE hur, NUMERIC_TYPE hvl, NUMERIC_TYPE hvr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Arrays *Arrptr);
NUMERIC_TYPE RoeBCx(int edge, int p0, int p1, int pq0, NUMERIC_TYPE z0, NUMERIC_TYPE z1, NUMERIC_TYPE hl, NUMERIC_TYPE hr, NUMERIC_TYPE hul, NUMERIC_TYPE hur, NUMERIC_TYPE hvl, NUMERIC_TYPE hvr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Arrays *Arrptr);


// Optional addon protoypes
// chkpnt.cpp
void ReadCheckpoint(Fnames *, States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *, const int verbose);
void WriteCheckpoint(Fnames *, States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *, const int verbose);
// infevap.cpp
void FPInfiltration(Pars *, Solver *, Arrays *);
void Evaporation(Pars *, Solver *, Arrays *);
void Rainfall(Pars *, Solver *, Arrays *);
void FlowDirDEM(Pars *, Arrays *, States *, BoundCs *); // Calculate routing intervals and flow directions from DEM for rainfall component CCS 13/03/2012
void Routing(States *, Pars *, Solver *, Arrays *); // Route shallow flows from rainfall CCS 14/03/2012
// por_flow.cpp
NUMERIC_TYPE CalcFPQxPor(int i, int j, States *, Pars *, Solver *, Arrays *);
NUMERIC_TYPE CalcFPQyPor(int i, int j, States *, Pars *, Solver *, Arrays *);
NUMERIC_TYPE PorArea(int t, int j, Pars *, Arrays *);
// weir_flow.cpp
NUMERIC_TYPE CalcWeirQx(int i, int j, const Pars *, const Arrays *, const Solver *, const States *, const SGCprams *);
NUMERIC_TYPE CalcWeirQy(int i, int j, const Pars *, const Arrays *, const Solver *, const States *, const SGCprams *);

// Utility prototypes - util.cpp
NUMERIC_TYPE DomainVol(States *, Pars *, ChannelSegmentType *, Arrays *, vector<ChannelSegmentType> *);
void SmoothBanks(Pars *, Solver *, ChannelSegmentType *, Arrays *, vector<ChannelSegmentType> *, const int verbose);
NUMERIC_TYPE x_centre(Pars *Parptr, const int i);
NUMERIC_TYPE y_centre(Pars *Parptr, const int j);
NUMERIC_TYPE x_vertex(Pars *Parptr, const int i);
NUMERIC_TYPE y_vertex(Pars *Parptr, const int j);
void DryCheck(Pars *, Solver *, Arrays *);
int signR(NUMERIC_TYPE a);
void UpdateV(States *, Pars *, Solver *, BoundCs *, ChannelSegmentType *, Arrays *);
void InitFloodplainQ(States *, Pars *, Solver *, Arrays *);
//NUMERIC_TYPE CalcVirtualGauge(int i, Pars *, Arrays *, Stage *);
NUMERIC_TYPE CalcVirtualGauge(const int gauge_i, const int grid_cols_padded,
	const NUMERIC_TYPE * qx_grid, const NUMERIC_TYPE * qy_grid,
	Stage *Locptr);

void CalcArrayDims(States *, Pars *, Arrays *);

// Ouput prototypes - output.cpp
void fileoutput(Fnames *, States *, Pars *, Arrays *);
void write_regular_output(Fnames *, Solver *, States *, Pars *, Arrays *, SGCprams *);

// MT new general purpose functions
void write_ascfile(const char *root, int SaveNumber, const char *extension, NUMERIC_TYPE *data, NUMERIC_TYPE *dem, int outflag, States *, Pars *); // general purpose ascii write routine
void write_ascfile(const char *root, int SaveNumber, const char *extension, NUMERIC_TYPE *data, NUMERIC_TYPE *dem, int outflag, States *Statesptr, Pars *Parptr, NUMERIC_TYPE depth_thresh);
void write_ascfile(const char *root, int SaveNumber, const char *extension, NUMERIC_TYPE *data, NUMERIC_TYPE *dem, int outflag, States *Statesptr, Pars *Parptr, NUMERIC_TYPE depth_thresh, const char* format_specifier);
void write_ascfileDScaled(const char* root, int SaveNumber, const char* extension, NUMERIC_TYPE* data, NUMERIC_TYPE* dem, int outflag, States* Statesptr, Pars* Parptr, NUMERIC_TYPE depth_thresh, const char* format_specifier);
void write_binrasterfile(const char *root, int SaveNumber, const char *extension, NUMERIC_TYPE *data, NUMERIC_TYPE *dem, int outflag, States *, Pars *); // general purpose binary write routine
void write_binrasterfile(const char *root, int SaveNumber, const char *extension, NUMERIC_TYPE *data, NUMERIC_TYPE *dem, int outflag, States *, Pars *, NUMERIC_TYPE depth_thresh); // general purpose binary write routine
void write_ascfile_SGCf(char *root, int SaveNumber, char *extension, NUMERIC_TYPE *data, NUMERIC_TYPE *SGC_BankFullHeight_grid, States *, Pars *); // specific routine for SGC floodplain depth export ascii
void write_binrasterfile_SGCf(char *root, int SaveNumber, char *extension, NUMERIC_TYPE *data, NUMERIC_TYPE *SGC_BankFullHeight_grid, States *, Pars *); // specific routine for SGC floodplain depth export binary
void write_profile(char *root, int SaveNumber, char *extension, States *Statesptr, ChannelSegmentType *ChannelSegments, Arrays *Arrptr, Pars *Parptr, vector<int> *, int *RiversIndexPtr); // write river channel profiles 
void debugfileoutput(Fnames *, States *, Pars *, Arrays *); // Debug option file output (currently the modified DEM and the channel and trib masks
void printversion(int verbose); // output program version header
int fexist(char *filename); // check if file exists

// TRENT functions
NUMERIC_TYPE maximum(NUMERIC_TYPE a, NUMERIC_TYPE b, NUMERIC_TYPE c);
NUMERIC_TYPE CalcFPQxRoe(int i, int j, States *, Pars *, Solver *, Arrays *);
NUMERIC_TYPE CalcFPQyRoe(int i, int j, States *, Pars *, Solver *, Arrays *);
void UpdateQsRoe(Pars *, Solver *, Arrays *);
//void UpdateHRoe(States *, Pars *, Solver *,BoundCs *,ChannelSegmentType *,Arrays *);
void CalcTRoe(Pars *, Solver *, Arrays *);
void ReadMultilayerValues(char* param_name, char* param_value_ptr, int line_number, NUMERIC_TYPE*  multi_Values,
	string param_prefix, Pars *Parptr, int verbose, int mode);
void ReadMultilayerFiles(char* param_name, char* param_value_ptr, int line_number, char*  multi_Files[],
	string param_prefix, Pars *Parptr, int verbose, int mode);
//*********************xiaodw, DHSVM****************************
//void RouteSubSurface(const int row_start, int row_end,
//	Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
//	const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area, const int grid_cols_padded, NUMERIC_TYPE * volume_grid);
float WaterTableDepth(Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, Pois*Poisptr, const NUMERIC_TYPE row_cell_area, int NSoilLayers, int NRootLayers, int index);
float* getLatLongByIndex(int index, int grid_cols_padded, Pars *Parptr);

bool IsNumber(float x);
