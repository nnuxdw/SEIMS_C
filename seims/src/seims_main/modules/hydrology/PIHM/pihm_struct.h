#pragma once
#ifndef PIHM_STRUCT_HEADER
#define PIHM_STRUCT_HEADER
#include "pihm_tools_dev.h"
// Time structure
typedef struct pihm_t_struct
{
	int             t;                      // time since the epoch (s)
	int             year;                   // current year
	int             month;                  // current month
	int             day;                    // current day of month
	int             hour;                   // current hour
	int             minute;                 // current minute
	char            str[17];                // time string (yyyy-mm-dd HH:MM)
	char            strshort[13];           // time string (yyyymmddHHMM)
} pihm_t_struct;

// Site information structure
typedef struct siteinfo_struct
{
	double          longitude;              // (degree)
	double          latitude;               // (degree)
	double          zmax;                   // average surface elevation (m)
	double          zmin;                   // average soil bottom elevation (m)
	double          area;                   // total area (m2)
	double          tavg;                   // annual average air temperature (K)
} siteinfo_struct;

#if defined(_BGC_) || defined(_CYCLES_)
// A structure to hold information on the annual CO2 concentration
typedef struct co2control_struct
{
	int             varco2;                 // 0 = const 1 = use file
	double          co2ppm;                 // constant CO2 concentration (ppm)
} co2control_struct;
#endif

#if defined(_BGC_)
// A structure to hold annual nitrogen deposition data
typedef struct ndepcontrol_struct
{
	int             varndep;                // 0 = const 1 = use file
	double          ndep;                   // wet + dry atmospheric deposition of N (kgN m-2 yr-1) 大气干湿氮沉降
	double          nfix;                   // symbiotic + asymbiotic fixation of N (kgN m-2 yr-1) N的共生+非共生固定
} ndepcontrol_struct;

// Carbon and nitrogen state initialization structure
typedef struct cninit_struct
{
	double          max_leafc;              // first-year displayed + stored leafc (kgC m-2)
	double          max_stemc;              // first-year total stem carbon (kgC m-2)
	double          cwdc;                   // coarse woody debris C (kgC m-2)
	double          litr1c;                 // litter labile C (kgC m-2)
	double          litr2c;                 // litter unshielded cellulose C (kgC m-2)
	double          litr3c;                 // litter shielded cellulose C (kgC m-2)
	double          litr4c;                 // litter lignin C (kgC m-2)
	double          soil1c;                 // microbial recycling pool C (fast) (kgC m-2)
	double          soil2c;                 // microbial recycling pool C (medium) (kgC m-2)
	double          soil3c;                 // microbial recycling pool C (slow) (kgC m-2)
	double          soil4c;                 // recalcitrant SOM C (humus, slowest) (kgC m-2)
	double          litr1n;                 // litter labile N (kgN m-2)
	double          sminn;                  // soil mineral N (kgN m-2)
} cninit_struct;
#endif

// Global calibration coefficients
typedef struct calib_struct
{
	double          ksath;
	double          ksatv;
	double          kinfv;
	double          kmach;
	double          kmacv;
	double          rzd;
	double          dmac;
	double          porosity;
	double          alpha;
	double          beta;
	double          areafv;
	double          areafh;
	double          vegfrac;
	double          albedo;
	double          rough;
	double          rivrough;
	double          rivksath;
	double          rivdepth;
	double          rivshpcoeff;
	double          prcp;                   // multiplier of precipitation (-)
	double          sfctmp;                 // offset of surface air temperature (K)
#if defined(_BGC_)
	double          mortality;
	double          sla;
#endif
#if defined(_CYCLES_)
	double          fert;                   // fertilization rate
	double          soc_decomp_rate;        // soil organic carbon decomposition rate
	double          residue_decomp_rate;    // residue decomposition rate
	double          root_decomp_rate;       // root decomposition rate
	double          rhizo_decomp_rate;      // rhizome decomposition rate
	double          manure_decomp_rate;     // manure decomposition rate
	double          microb_decomp_rate;     // microbe decomposition rate
	double          soc_humif_power;        // soil organic carbon humification exponent
	double          nitrif_const;           // nitrification rate
	double          pot_denitrif;           // potential denitrification rate
	double          denitrif_half_rate;     // half saturation constant for denitrification
	double          decomp_half_resp;       // decomposition half response to saturation
	double          decomp_resp_power;      // decomposition exponential response to saturation
	double          kd_no3;                 // offset of adsorption coefficient for NO3 (cm3 g-1)
	double          kd_nh4;                 // offset of adsorption coefficient for NH4 (cm3 g-1)
#endif
#if defined(_DGW_)
	double          geol_ksath;
	double          geol_ksatv;
	double          geol_porosity;
	double          geol_alpha;
	double          geol_beta;
	double          geol_dmac;
	double          geol_kmach;
	double          geol_kmacv;
	double          geol_areafv;
	double          geol_areafh;
#endif
#if defined(_NOAH_)
	double          smcref;
	double          smcwlt;
	double          rsmin;
	double          drip;
	double          cmcmax;
	double          czil;
	double          fxexp;
	double          cfactr;
	double          rgl;
	double          hs;
#endif
#if defined(_RT_)
	double          rate;                   // rate constant
	double          ssa;                    // specific surface area
	double          Xsorption;              // DOC sorption
#endif
} calib_struct;

// Model control parameters
typedef struct ctrl_struct
{
	int             ascii;                  // flag to turn on ascii output
	int             waterbal;               // flag to turn on water balance diagnostic output
	int             write_ic;               // flag to write model output as initial conditions
	int             nstep;                  // number of external time steps (when results can be printed) for the whole 总共多少个用户指定的步长
											// simulation
	int             cstep;                  // current model step (from 0)  0,1,2,3对应60s,120s,
	int             prtvrbl[MAXPRINT];      // number of output
	int             init_type;              // initialization mode: 0 = relaxed mode, 1 = use .ic file
	int             etstep;                 // land surface (ET) time step (s)
	int             starttime;              // start time of simulation (ctime)
	int             endtime;                // end time of simulation (ctime)
	int             stepsize;               // model step size (s)
	int            *tout;                   // model output times (ctime)
	double          abstol;                 // absolute solver tolerance (m)
	double          reltol;                 // relative solver tolerance (-)
	double          initstep;               // initial step size (s)
	double          maxstep;                // CVode maximum step size (s)
	double          stmin;                  // minimum allowed CVode max step size (s)
	double          nncfn;                  // number of non-convergence failures tolerance
	double          nnimax;                 // maximum number of non-linear iterations
	double          nnimin;                 // minimum number of non-linear iterations
	double          decr;                   // decrease factor (-)
	double          incr;                   // increase factor (-)
	int             maxspinyears;           // maximum number of years for spinup run
#if defined(_BGC_)
	int             read_bgc_restart;       // flag to read BGC restart file
	int             write_bgc_restart;      // flag to write BGC restart file
#endif
#if defined(_CYCLES_)
	int             read_cycles_restart;    // flag to read Cycles restart file
	int             write_cycles_restart;   // flag to write Cycles restart file
#endif
#if defined(_NOAH_)
	int             nlayers;                // number of standard soil layers
	double          soil_depth[MAXLYR];     // thickness of soil layer (m)
	int             rad_mode;               // radiation forcing mode: 0 = uniform, 1 = topographic
#endif
#if defined(_RT_)
	int             read_rt_restart;        // flag to read chemistry restart file
	int             write_rt_restart;       // flag to write chemistry restart file
	int             AvgScl;                 // reaction time step (s)
#endif

} ctrl_struct;

// Print variable control structure
typedef struct varctrl_struct
{
	char            name[MAXSTRING];        // name of output file
	int             intvl;                  // output interval (s)
	int             intr;                   // output type flag
	int             upd_intvl;              // 0: hydrology step, 1: land surface step, 2: CN step
	int             nvar;                   // number of variables for print
	const double  **var;                    // pointers to model variables
	double         *buffer;                 // buffer for averaging variables
	int             counter;                // counter for averaging variables
	FILE           *txtfile;                // pointer to txt file
	FILE           *datfile;                // pointer to binary file
} varctrl_struct;

typedef struct time_struct
{
	double init_time;
	clock_t t1;
	double spinup_time;
	clock_t t2;
	double applybc_time;
	clock_t t3;
	double landsurface_time;
	clock_t t4;
	double reaction_time;
	clock_t t5;
	double solvecvode_time;
	clock_t t5_1;
	double solvecvode_hydro_time;
	clock_t t5_1_1;
	double solvecvode_hydro_surfh_time;
	clock_t t5_1_2;
	double solvecvode_hydro_et_time;
	clock_t t5_1_3;
	double solvecvode_hydro_lateralflow_time;
	clock_t t5_1_4;
	double solvecvode_hydro_verticalflow_time;
	clock_t t5_1_5;
	double solvecvode_hydro_riverflow_time;
	clock_t t5_2;
	double solvecvode_bgc_time;
	clock_t t6;
	double noahhydro_time;
	clock_t t7;
	double noahhydro_watertable_time;
	clock_t t7_1;
	double noahhydro_smflx_time;
	clock_t t7_2;
	double chemical_time;
	clock_t t8;
	double dailybgc_time;
	clock_t t9;
	double other_time;
	clock_t t10;

};
// Print structure
typedef struct print_struct
{
	varctrl_struct  varctrl[MAXPRINT];
	int             nprint;                 // number of output variables
	FILE           *watbal_file;            // pointer to water balance file
	FILE           *cvodeperf_file;         // pointer to CVode performance file
} print_struct;

// 定义存储Down_ID映射的结构体
//typedef struct DownstreamDetail {
//	int id;
//	double proportion;
//};

//// 定义HRU结构体
//typedef struct hru_struct {
//	int key;
//	int down_type;
//	int down_id;
//	map<int, float>  down_ids; // 用于存储复杂的Down_ID数据
//} hru_struct;
//
//typedef struct arg_struct {
//	char** argv;
//	int argc;
//} arg_struct;




typedef struct SeimsMeteoStruct {
	double *pihm_pcp;
	double *pihm_tmean;
	double *pihm_ws ;
	double *pihm_rhd;
	double *pihm_sr;
	//double *pihm_pa;

}SeimsMeteoStruct;



typedef struct SeimsVariablesStruct {
	/// time step (sec)
	int m_TimeStep;
	/// validate cells number
	int m_nCells;
	/// cell width of the grid (m)
	float m_CellWth;
	/// cell area, BE CAUTION, the unit is m^2, NOT ha!!!
	float m_cellArea;
	/// the total number of subbasins
	int m_nSubbsns;
	/// current subbasin ID, 0 for the entire watershed
	int m_inputSubbsnID;
	/// subbasin grid (subbasins ID)
	float* m_subbsnID;

	/// surface runoff from depression module
	float* m_surfRf;
	/*! Precipitation
 * For STROM_MODE model, the unit is rainfall intensity mm/h
 * For LONGTERM_MODE model, the unit is mm
 */
	float* m_pcp;

	float* m_meanTemp;
	float* m_maxTemp; ///< maximum air temperature for a given day (deg C)
	float* m_minTemp; ///< minimum air temperature for a given day (deg C)
	float* m_rhd; ///< relative humidity (%)
	float *m_WindSpeed;
	float* m_SR;

	//temporary

	/// store the flow of each cell in each day between min time and max time
	float** m_cellFlow;
	/// the maximum of second column of OL_IUH plus 1.
	int m_cellFlowCols;

	//output

	/// overland flow to streams for each subbasin (m3/s)
	float* m_Q_SBOF;
	// overland flow in each cell (mm) //added by Gao, as intermediate variable, 29 Jul 2016
	float* m_OL_Flow;
	//ljj
	float* m_area;
	float* total_area;
	int m_nRteLyrs;
	int m_maxSoilLyrs;
	float* m_rchID;
	float* m_landCover;
	float* m_slope;
	float* m_chWidth;
	float* m_flowout_length;
	float* m_flowOutIdxD8;
	float* m_surfRftotal;
	float **m_subSurfRfVol;     /// subsurface runoff volume (m3), VAR_SSRUVOL
	//float *m_gwStorage;          ///  Groundwater storage (mm) of the subbasin
	float *m_gwQ;                    /// groundwater discharge (m3/s)
	float** m_rteLyrs;
	float *m_nSoilLyrs;            	/// number of soil layers of each cell
	float** m_flowInIdxD8;
	float** m_ks;
	float* subbasin_area;          /// subbasin area without downstream triangle hrus

	float** m_Qtrans;
}SeimsVariablesStruct;

typedef struct exchange_struct {
	// 上游向下游的地表流量输入
	double* elem_upstream_surfq;
	double* elem_upstream_subsurvol;
	double* elem_upstream_gwQ;
}exchange_struct;


typedef struct PIHMDataStruct {
	// 记录每个时步的变量，时间序列
	double** elem_upstream_surfq; //接收上游地表来水
	double** elem_upstream_subsurq; //接收上游土壤水来水
	double** elem_upstream_gwq;//接收上游地下水来水
	double** elem_sufh; // 地表水深
	double** elem_gwh; // 地下水深
	double** elem_pcp;
	int * timeseries;
}PIHMDataStruct;

typedef struct pihm_struct
{
	siteinfo_struct siteinfo;
	filename_struct filename;
	meshtbl_struct  meshtbl;
	atttbl_struct   atttbl;
	soiltbl_struct  soiltbl;
	geoltbl_struct  geoltbl;
	lctbl_struct    lctbl;
	rivtbl_struct   rivtbl;
	shptbl_struct   shptbl;
	matltbl_struct  matltbl;
	PIHMToolDataStruct* PIHMToolData;
	SeimsVariablesStruct * SeimsVariables;
	SeimsMeteoStruct * SeimsMetros;
	exchange_struct * ExchangeData;
	PIHMDataStruct* PIHMData;

#if defined(_NOAH_)
	noahtbl_struct  noahtbl;
#endif
#if defined(_CYCLES_)
	agtbl_struct    agtbl;
	crop_struct     croptbl[MAXCROP];
	mgmt_struct     mgmttbl[MAXOP];
#endif
#if defined(_BGC_) || defined(_CYCLES_)
	co2control_struct co2ctrl;
#endif
#if defined(_BGC_)
	ndepcontrol_struct ndepctrl;
	epctbl_struct   epctbl;
	cninit_struct   cninit;
#endif
	forc_struct     forc;
	elem_struct    *elem;
	river_struct   *river;
	calib_struct    calib;
	ctrl_struct     ctrl;
	print_struct    print;

#if defined(_RT_)
	chemtbl_struct  chemtbl[MAXSPS];
	kintbl_struct   kintbl[MAXSPS];
	rttbl_struct    rttbl;
	chmictbl_struct chmictbl;
#endif
#if defined(_STATISTIC_TIME_)
	struct time_struct* ptime_calculator;
#endif
} pihm_struct;
#endif



