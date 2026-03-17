/*!
 * \file ReservoirMethod.h
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
#ifndef SEIMS_MODULE_GWA_RE_H
#define SEIMS_MODULE_GWA_RE_H

#include "SimulationModule.h"
#include "clsSubbasin.h"

/** \defgroup GWA_RE
 * \ingroup Hydrology_longterm
 * \brief Reservoir Method to calculate groundwater balance and baseflow of longterm model
 *
 */

/*!
 * \class ReservoirMethod
 * \ingroup GWA_RENEW
 * \brief Reservoir Method to calculate groundwater balance and baseflow of longterm model
 *
 */
class ReservoirMethodNEW: public SimulationModule {
public:
    ReservoirMethodNEW();

    ~ReservoirMethodNEW();

    void SetValue(const char* key, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, float** data) OVERRIDE;

    void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;

    void SetScenario(Scenario* sce) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* nrows, float** data) OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, float*** data) OVERRIDE;


    //TimeStepType GetTimeStepType() OVERRIDE { return TIMESTEP_CHANNEL; }

private:
    //whc++
    //float* wascobRaster;
    //map<int, vector<BMPWascob*>> wascobSubarea;
    //map<int, vector<BMPWascob*>> wascob;
    ///// OutletFactory of BMP Wascob and TileDrain
    //clsTileDrainData* soilPar;

    float* gwSub;
    //vector< BMPFactory*> m_OutletFactory;
    //// tiledrain map
    //map<int, vector<BMPTileDrain*>> tiledrain;
    //// tiledrain map of reach i
    //map<int, vector<BMPTileDrain*>> tiledrainReach;
    //map<int, vector<BMPTileDrain*>> tiledrainSubarea;//按subarea组织的tiledrain
    float* m_rootDepth;
    //float** m_tileDrainOutputs;
    float* QGSub;//QG organized by subarea
    float** m_soilFC;//AWC
    float** m_soilPor;
    float** m_fieldCap;
    float** m_soilSat;
    float* m_potVol;
    float* m_surfRf;
    float* m_impoundTrig;
    float* m_infil;
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
    //! initial ground water Table (or at time t-1)
    float m_GWT0;
    //! maximum ground water storage
    float m_GWMAX;
	float *m_GWMAX_1d;

    float* m_petSubbsn; ///< Average PET of each subbasin, mm
    float* m_gwSto;     ///<  Groundwater storage (mm) of the subbasin
    float* m_gwTab;
    /// slope (percent, or drop/distance, or tan) of each cell
    float* m_slope;
    /// soil water storage in soil profile (mm)
    float* m_soilWtrStoPrfl;
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

    float* m_area;
    float* curBasinArea;
	float* gw_delay_1d;
	float* m_Kg_1d;
	float* m_Base_ex_1d;
	float*  m_hand_eavp;
	float* m_handWtrDep;
	float* m_chSto;		///< reach storage (m^3), rchstor in SWAT
	float** m_soilWtrStoBfe;
	float** m_soilMoistBfe;
	float* perco_200;

};

#endif /* SEIMS_MODULE_GWA_RE_NEW_H */
