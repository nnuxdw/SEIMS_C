/*!
 * \file template.h
 * \brief Brief description of this module
 *        Detail description about the implementation.
 * \author Liangjun Zhu
 * \date 2018-02-07
 *
 * Changelog:
 *   - 1. 2018-02-07 - lj - Initial implementaition
 *   - 2. 2019-01-30 - lj - Add (or update) all available APIs
 */

#ifndef SEIMS_MODULE_TEMPLATE_H
#define SEIMS_MODULE_TEMPLATE_H

#include "SimulationModule.h"
#pragma once
#ifndef PIHM_HEADER
#define PIHM_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <float.h>
#include <sys/stat.h>
#if defined(_WIN32) || defined(_WIN64)
# include <windows.h>
# include <direct.h>
# include <io.h>
#else
# include <unistd.h>
#endif
#if defined(unix) || defined(__unix__) || defined(__unix)
# include <fenv.h>
#endif
#if defined(_OPENMP)
# include <omp.h>
#endif
#define VERSION             "1.0.0.post"

 // SUNDIAL Header Files
#include "cvode/cvode.h"    // Prototypes for CVODE fcts., consts.
#include "sunlinsol/sunlinsol_spgmr.h"  // Access to SPGMR SUNLinearSolver
#if defined(_CVODE_OMP)
# include "nvector/nvector_openmp.h"    // Access to N_Vector
#else
# include "nvector/nvector_serial.h"
#endif
#include "sundials/sundials_math.h"     // Definition of macros SUNSQR and EXP
#include "sundials/sundials_dense.h"    // Prototypes for small dense fcts.

#if defined(_NOAH_)
# include "spa.h"
#endif

#include "custom_io.h"

#include "pihm_const.h"
#include "pihm_input_struct.h"
#include "elem_struct.h"
#include "river_struct.h"
#include "pihm_struct.h"
#include "pihm_func.h"
#include "pihm_errors.h"
//#include "pihm_tools_dev.h"
#include <unordered_set>
#endif

//using namespace std;


class PIHM: public SimulationModule {
public:
    PIHM(); //! Constructor

    ~PIHM(); //! Destructor

    ///////////// SetData series functions /////////////

    void SetValue(const char* key, float value) OVERRIDE;

    void SetValueByIndex(const char* key, int index, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

    void SetReaches(clsReaches* rches) OVERRIDE;

    void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;

    void SetScenario(Scenario* sce) OVERRIDE;

    ///////////// CheckInputData and InitialOutputs /////////////

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    ///////////// Main control structure of execution code /////////////

    int Execute() OVERRIDE;

    ///////////// GetData series functions /////////////

    TimeStepType GetTimeStepType() OVERRIDE;

    void GetValue(const char* key, float* value) OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

	void PostExcute();

public:
	// PIHM variables

	pihm_struct     *pihm_strc;
	ctrl_struct    *ctrl;
	N_Vector        CV_Y;
	void           *cvode_mem;
	SUNLinearSolver sun_ls;

	PIHM_TOOLS_DEV *pihm_tools;

	
	SeimsMeteoStruct * seims_meteo;

	// xiaodw, other pihm variables 
	double      cputime, cputime_dt;    // Time cpu duration

	// xiaodw
// 记录模型运行时间
	clock_t start_time, end_time;
	double elapsed_time;
	// 计数器，用于控制输出
	int counter;
	int* cur_sim_time_ptr;
	int* last_sim_time_ptr;
	int finish_times;
	bool initial_flag;

	//vector<hru_struct> *hrus;
	//arg_struct *args;
	//vector<int> *hru_ids;
	//map<int, int*> *hru_tri_id_map;
	
	char * pihm_dir;
	char * outputdir;
	char * final_downstream_file;
	char * args_file;          // args.txt file,xiaodw
	char * hru_ids_file;
	char * hru_tri_map_file;

	/*
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

	/// IUH of each grid cell (1/s)
	float** m_iuhCell;
	/// the number of columns of Ol_iuh
	int m_iuhCols;
	/// surface runoff from depression module
	float* m_surfRf;
	// Precipitation
	//For STROM_MODE model, the unit is rainfall intensity mm/h
	//For LONGTERM_MODE model, the unit is mm
	float* m_pcp;

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

	float** m_rteLyrs;
	float** m_flowInIdxD8;
	float** m_ks;

	float** m_Qtrans;

	*/
};

#endif /* SEIMS_MODULE_TEMPLATE_H */
