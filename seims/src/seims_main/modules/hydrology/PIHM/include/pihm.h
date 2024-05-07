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

#endif

using namespace std;

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

	void Initialize();
	void ReadArgs();

private:
    int m_nCells; ///< valid cells number
	char            outputdir[MAXSTRING];
	pihm_struct     *pihm;
	ctrl_struct    *ctrl;
	N_Vector        CV_Y;
	void           *cvode_mem;
	SUNLinearSolver sun_ls;


};

#endif /* SEIMS_MODULE_TEMPLATE_H */
