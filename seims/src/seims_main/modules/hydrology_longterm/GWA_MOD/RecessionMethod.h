/*!
 * \file RecessionMethod.h
 * \brief Reservoir Method to calculate groundwater balance and baseflow.
 *
 * Changelog:
 *   - 1. 2011-01-24 - wh - Initial implementation.
 *   - 2. 2011-02-18 - zq -
 *        -# Add judgment to calculation of EG (Revap). The average percolation of
 *		       one subbasin is first calculated. If the percolation is less than 0.01,
 *		       EG is set to 0 directly. (in function setInputs of class subbasin)
 *	      -# Add member variable m_isRevapChanged to class subbasin. This variable
 *		       is the flag whether the Revap is changed by current time step. This flag
 *		       can avoid repeating setting values when converting subbasin average Revap
 *		       to cell Revap.(in function Execute of class ReservoirMethod)
 *   - 3. 2011-03-14 - zq - Add codes to process the groundwater which comes from bank storage in
 *		                      channel routing module. The water volume of this part of groundwater is
 *		                      added to the groundwater storage. The input variable "T_GWNEW" is used
 *		                      for this purpose. One additional parameter is added to function setInputs
 *		                      of class subbasin.
 *		                      See equation 8 in memo "Channel water balance" for detailed reference.
 *   - 4. 2016-07-27 - lj - Move subbasin class to base/data module for sharing with other modules.
 *	 - 5. 2018-06-28 - lj - Move SetSubbasinInfos() to dataCenter class.
 *	 - 6. 2018-07-18 - sf - revap should be calculated by cell first.
 *
 * \author Hui Wu, Zhiqiang Yu, Liangjun Zhu, Fang Shen
 */
#ifndef SEIMS_MODULE_GWA_MOD_H
#define SEIMS_MODULE_GWA_MOD_H

#include "SimulationModule.h"
#include "clsSubbasin.h"
# ifdef USE_PIHM
 // xiaodw, for pihm
#include "pihm_tools.h"
#ifndef MAXSTRING
#define MAXSTRING  1024
#endif
#endif
/** \defgroup GWA_MOD
 * \ingroup Hydrology_longterm
 * \brief Reservoir Method to calculate groundwater balance and baseflow of longterm model
 *
 */

/*!
 * \class RecessionMethod
 * \ingroup GWA_RE
 * \brief Reservoir Method to calculate groundwater balance and baseflow of longterm model
 *
 */
class ReservoirMethod: public SimulationModule {
public:
    ReservoirMethod();

    ~ReservoirMethod();

    void SetValue(const char* key, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, float** data) OVERRIDE;

    void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;

    void SetReaches(clsReaches* reaches) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* nrows, float** data) OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, float*** data) OVERRIDE;
	// xiaodw modify, 此模块应该是坡面模块，而非河道模块，因此注释掉此方法，ModelMain会默认判定为坡面模块
    /*TimeStepType GetTimeStepType() OVERRIDE{ return TIMESTEP_CHANNEL; }*/

# ifdef USE_PIHM
	// xiaodw, for pihm
	PIHM_TOOLS *pihm_tools = nullptr;
	vector<int> *hru_ids;
	char * pihm_dir;
	char * hru_ids_file;
	char * project;
#endif
private:
    //inputs

    //! time step, second
    int m_dt;
    //! Valid cells number
    int m_nCells;
    //! cell size of the grid (m)
    float m_cellWth;
    //! maximum soil layers number
    int m_maxSoilLyrs;
    //! soil layers number of each cell
    float* m_nSoilLyrs;
    //! soil thickness of each layer
    float** m_soilThk;

    //! groundwater Revap coefficient
    float m_dp_co;
    //! baseflow recession coefficient
    float m_Kg;
    //! baseflow recession exponent
    float m_Base_ex;
    //! the amount of water percolated from the soil water reservoir and input to the groundwater reservoir from the percolation module(mm)
    float** m_soilPerco;
    //! evaporation from interception storage (mm) from the interception module
    float* m_IntcpET;
    //! evaporation from the depression storage (mm) from the depression module
    float* m_deprStoET;
    //! evaporation from the soil water storage (mm) from the soil ET module
    float* m_soilET;
    //! actual amount of transpiration (mm H2O)
    float* m_actPltET;
    //! PET(mm) from the PET modules
    float* m_pet;
    //! revap needed of cell
    float* m_revap;
    //! initial ground water storage (or at time t-1)
    float m_GW0;
    //! maximum ground water storage
    float m_GWMAX;

    float* m_petSubbsn; ///< Average PET of each subbasin, mm
    float* m_gwSto;     ///<  Groundwater storage (mm) of the subbasin
	// xiaodw, output for pihm
# ifdef USE_PIHM
	float* m_subbasin_area;
#endif
    /// slope (percent, or drop/distance, or tan) of each cell
    float* m_slope;

    //! soil storage
    float** m_soilWtrSto;
    //! soil depth of each layer, the maximum soil depth is used here, i.e., m_soilDepth[i][(int)m_soilLayers[i]]
    float** m_soilDepth;
    //! ground water from bank storage, passed from channel routing module
    float* m_VgroundwaterFromBankStorage;

    //output
    //!
    float* m_T_Perco;
    //!
    float* m_T_PerDep;
    //!
    float* m_T_RG;
    //!
    float* m_T_QG;
    //!
    float* m_T_Revap;
    //! groundwater water balance statistics
    float** m_T_GWWB;

    //! subbasin number
    int m_nSubbsns;
    //! current subbasin ID, 0 for the entire watershed
    int m_inputSubbsnID;
    //! subbasin IDs
    vector<int> m_subbasinIDs;
    //! All subbasins information
    clsSubbasins* m_subbasinsInfo;

    //ljj++
    float m_GWMIN;
    //time required for water leaving the bottom of the root zone to reach the shallow aquifer
    float m_delay;
    //alpha factor for groundwater recession curve
    float m_alpha_bf;
    
    //specific yield for shallow aquifer
    float* m_gw_spyld;
    
    float* m_area;
    //time required for water leaving the bottom of the root zone to reach the shallow aquife
    float* gw_delaye;
    //groundwater height
    float* gw_height;

    float* m_gw_shallow;

    //groundwater contribution to streamflow
    float* m_gw_q;

    float** m_soilSat;

};
#endif /* SEIMS_MODULE_GWA_RE_H */
