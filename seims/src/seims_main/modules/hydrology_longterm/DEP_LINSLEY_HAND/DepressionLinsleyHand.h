/*!
 * \file DepressionLinsley.h
 * \brief Linsley(1982) method to calculate depression storage.
 *
 * Changelog:
 *   - 1. 2011-02-14 - jz - Initial implementation.
 *   - 2. 2011-02-15 - zq -
 *        -# Modify the name of some parameters, input and output variables.
 *		       Please see the metadata rules for the names.
 *        -# Depre_in would be DT_Single. Add function SetSingleData() to set its value.
 *        -# This module will be called by infiltration module to get the
 *		       depression storage. And this module will also use the outputs
 *		       of infiltration module. The sequence of this two modules is
 *		       infiltration->depression. When infiltration first calls the
 *		       depression module, the execute function of depression module
 *		       is not executed before getting the outputs. So, the output
 *		       variables should be initial in the Get1DData function. This
 *		       initialization is realized by function initalOutputs.
 *        -# Delete input D_INFIL and add input D_EXCP.
 *   - 3. 2016-07-14 - lj - Code review and reformat.
 *
 * \author Junzhi Liu, Zhiqiang Yu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_DEP_LINSLEY_H
#define SEIMS_MODULE_DEP_LINSLEY_H

#include "SimulationModule.h"

/** \defgroup DEP_LINSLEY
 * \ingroup Hydrology_longterm
 * \brief A simple fill and spill method method to calculate depression storage
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
/*!
 * \class DepressionFSDaily
 * \ingroup DEP_LINSLEY
 * \brief A simple fill and spill method method to calculate depression storage
 *
 */
class DepressionLinsleyHand: public SimulationModule {
public:
	DepressionLinsleyHand();

    ~DepressionLinsleyHand();

    int Execute() OVERRIDE;

    void SetValue(const char* key, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    bool CheckInputData() OVERRIDE;
    /*!
     * \brief Initialize output variables
     * This module will be called by infiltration module to get the
     *		depression storage. And this module will also use the outputs
     *		of infiltration module. The sequence of this two modules is
     *		infiltration->depression. When infiltration first calls the
     *		depression module, the execute function of depression module
     *		is not executed before getting the outputs. So, the output
     *		variables should be initial in the Get1DData function. This
     *		initialization is realized by function initalOutputs.
     */
    void InitialOutputs() OVERRIDE;

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
    /// valid cells number
    int m_nCells;
    /// impound/release
    float* m_impoundTriger;
    /// pothole volume, mm
    float* m_potVol;
    /// initial depression storage coefficient
    float m_depCo;
    /// depression storage capacity (mm)
    float* m_depCap;

    /// pet
    float* m_pet;
    /// evaporation from the interception storage
    float* m_ei;

    /// excess precipitation calculated in the infiltration module
    float* m_pe;

    // state variables (output)

    /// depression storage
    float* m_sd;
    /// evaporation from depression storage
    float* m_ed;
    /// surface runoff
    float* m_sr;
	/// water depth of each hand, initialized by m_bankSto,m
	float* m_handWtrDep;
	/// subbasin grid (ID of subbasin)
	float *m_subbsnID;
	float* m_chSto;		///< reach storage (m^3), rchstor in SWAT
	int m_outletID;    ///< outlet ID, also can be derived by m_reachLayers.rbegin()->second[0];
	float* m_handArea;       /// area of each hand
	int m_nreach;      ///< reach number (= subbasin number)
	float*  m_hand_dep;
	float*  m_hand_eavp;
	// xiaodw add
		// xiaodw add
	float* m_HAND_Subbasin;
	float* m_HAND_Flood_Level;
	float* m_HAND_LevelDepth;
	float* m_HAND_SumArea;
	float* m_HAND_SumVolume;
	float* m_HAND_AvgDepth;
	float* m_HAND_AccVolume;
	float* m_HAND_LowerAccDepthFlat;
	float* m_HAND_LowerAccDepthLen;
	vector<Hand> m_Hands;  ///  subbasin (or reach)-- layers -- hands,  index represents subbasin id for dim 1, index represents layer for dim 2
	// debug
	float* handWtrDepAftDep;
};
#endif /* SEIMS_MODULE_DEP_LINSLEY_H */
