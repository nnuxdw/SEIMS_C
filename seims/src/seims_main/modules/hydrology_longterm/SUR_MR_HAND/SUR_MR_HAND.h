/*!
 * \file SUR_MR_HAND.h
 * \brief Modified Rational Method to calculate infiltration and excess precipitation.
 *
 * Changelog:
 *   - 1. 2011-01-19 - jz - Initial implementation.
 *   - 2. 2011-02-15 - zq -
 *        -# Parameter S_M_frozen would be s_frozen and DT_Single.
 *        -# Parameter sfrozen would be t_soil and in WaterBalance table.
 *        -# Delete parameter Moist_in.
 *        -# Rename the input and output variables. See metadata rules for names.
 *        -# In function execute, do not change m_pNet[i] directly. This will have influence
 *             on another modules who will use net precipitation. Use local variable to replace it.
 *        -# Add API function GetValue.
 *   - 3. 2011-02-19 - jz - Take snowmelt into consideration when calculating PE, PE=P_NET+Snowmelt-F.
 *   - 4. 2013-10-28 - jz - Add multi-layers support for soil parameters.
 *   - 5. 2016-05-27 - lj - Update the support for multi-layers soil parameters.
 *   - 6. 2016-07-14 - lj -
 *        -# Remove snowmelt as AddInput, because snowmelt is considered into net precipitation in SnowMelt moudule,
 *             by the meantime, this can avoid runtime error when SnowMelt module is not configured.
 *        -# Change the unit of soil moisture from mm H2O/mm Soil to mm H2O, which is more rational.
 *        -# Change soil moisture to soil storage which is coincident with SWAT, and do not include wilting point.
 *
 * \author Junzhi Liu, Zhiqiang Yu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_SUR_MR_HAND_H
#define SEIMS_MODULE_SUR_MR_HAND_H

#include "SimulationModule.h"

/** \defgroup SUR_MR_HAND
 * \ingroup Hydrology_longterm
 * \brief Modified Rational Method to calculate infiltration and excess precipitation
 *
 */

 // 表示每一层的 HAND 信息
struct Level {
	//vector<float> handHeights;   // index is hand id
	int* handIds;   // index is layer 0,1,2,3..., value is hand id

	//float m_chOverHeadVol;      /// represents the physical space between the top of the channel banks and the upper boundary, index represents subbasin id for dim 1, index represents layer for dim 2 cooresponding to each HAND height
	/*float* m_handArea;					/// area of each hand
	float* m_handWtrDep;			    /// water depth of each hand, initialized by m_bankSto*/
	float m_levelDepth;                 /// depth of each level, eg. the level is from 0~5m, so the depth is 5-0=5m.
	double m_levelSumArea;   /// area of each hand level, contains all levels' area lower than this level
	float m_levelAvgDepth; /// average depth of each layer's all hands, equals (channel's overhead area + lower level's sum area * this level's depth + SUM(this level's hand's area * dem's avg depth in hand))
	double m_levelSumVol;    /// area of each hand level, cooresponding to m_levelSumArea
	double m_levelAccVol;              /// contains a level's vol and all lower level's vol, m3
	float* m_levelLowerAccDepth;   // m

	int m_levelHandNum;          /// n layers of hand for each level
	float m_levelWtrDep;              /// water depth of each level,m. contains all water above  the level

};

// 表示每个子流域下的所有 HAND 层
struct Hand {
	int n_levels;
	int m_CurInundationLevel;
	vector<Level> levels;   /// index represents subbasin id (or reach id)
	float excessWtrVol;     /// water excess subbasin's full volume

	// for test
	float volToAdd;
};
class SUR_MR_HAND: public SimulationModule {
public:
    SUR_MR_HAND();

    ~SUR_MR_HAND();

    void SetValue(const char* key, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int nrows, int ncols, float** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, float*** data) OVERRIDE;

	void SetReaches(clsReaches* reaches) OVERRIDE;

	void LoadHandLevelsFromArrays(
		int cellsNum,
		int flatLen,
		std::vector<Hand>& m_Hands,
		float nodata /*= -9999.0f*/,
		bool buildHandIds /*= false*/
	);
	bool HandInundation_BinarySearch(const int reachId, float sto);
	void updateAllHandsWtrDep(const int reachId);

private:
    /// Hillslope time step (second)
    float m_dt;
    /// count of valid cells
    int m_nCells;
    /// net precipitation of each cell (mm)
    float* m_netPcp;
    /// potential runoff coefficient
    float* m_potRfCoef;

    /// number of soil layers, i.e., the maximum soil layers of all soil types
    int m_maxSoilLyrs;
    /// soil layers number of each cell
    float* m_nSoilLyrs;

    /// mm H2O: (sol_fc) amount of water available to plants in soil layer at field capacity (fc - wp)
    float** m_soilFC;
    float** m_soilAWC;
    /// mm H2O: (sol_ul) amount of water held in the soil layer at saturation (sat - wp water)
    float** m_soilSat;
    /// amount of water held in the soil layer at saturation (sat - wp water), mm H2O, sol_sumul of SWAT
    float* m_soilSumSat;
    /// initial soil water storage fraction related to field capacity (FC-WP)
    float* m_initSoilWtrStoRatio;

    /// Runoff exponent for a near zero rainfall intensity
    float m_rfExp;
    /// Rainfall intensity corresponding to a surface runoff exponent (m_rfExp) of 1
    float m_maxPcpRf;
    /// depression storage (mm)
    float* m_deprSto; // SD(t-1) from the depression storage module

    /// mean air temperature (deg C)
    float* m_meanTemp;

    /// threshold soil freezing temperature (deg C)
    float m_soilFrozenTemp;
    float* m_soilFrozenTemp_1d;
    /// frozen soil moisture relative to saturation above which no infiltration occur
    /// (m3/m3 or mm H2O/ mm Soil)
    float m_soilFrozenWtrRatio;
    /// soil temperature obtained from the soil temperature module (deg C)
    float* m_soilTemp;

    /// pothole volume, mm
    float* m_potVol;
    /// impound trigger
    float* m_impndTrig;
    // output
    /// the excess precipitation (mm) of the total nCells, which could be depressed or generated surface runoff
    float* m_exsPcp;
    /// infiltration map of watershed (mm) of the total nCells
    float* m_infil;
    /// soil water storage (mm)
    float** m_soilWtrSto;
    /// soil water storage in soil profile (mm)
    float* m_soilWtrStoPrfl;

    //ljj++
    float** m_soilIceSto;
    float** m_soilPor;
    float** m_soilThk;
    float* m_dem;
    float* m_soilIceStoPrfl;
    float* m_landUse;
    float* m_rchID;
    float* m_pcp;
    float* m_lakesto;
    float* m_pet;


	//xdw++
	/// m_soilPor * m_soilThk
	float** m_soilPorDepth;
	/// m_soilFC * m_soilThk
	float** m_soilFCDepth;
	/// water depth of each hand, initialized by m_bankSto,m
	float* m_handWtrDep;   

	int m_nSubbsns;
	/// subbasin grid (ID of subbasin)
	float *m_subbsnID;
	float* m_chSto;		///< reach storage (m^3), rchstor in SWAT
	int m_outletID;    ///< outlet ID, also can be derived by m_reachLayers.rbegin()->second[0];
	float* m_handArea;       /// area of each hand
	int m_nreach;      ///< reach number (= subbasin number)

	// xiaodw add
	vector<Hand> m_Hands;  ///  subbasin (or reach)-- layers -- hands,  index represents subbasin id for dim 1, index represents layer for dim 2
	float* m_HAND_Subbasin;
	float* m_HAND_Flood_Level;
	float* m_HAND_LevelDepth;
	float* m_HAND_SumArea;
	float* m_HAND_SumVolume;
	float* m_HAND_AvgDepth;
	float* m_HAND_AccVolume;
	float* m_HAND_LowerAccDepthFlat;
	float* m_HAND_LowerAccDepthLen;

	// debug
	float* m_HAND_Infil;
	float* handWtrDepAftInfil;
	float** m_soilWtrStoBfe;
	float* m_HAND_Runoff_Perc;
	float* m_alpha;

};
#endif /* SEIMS_MODULE_SUR_MR_HAND_H */
