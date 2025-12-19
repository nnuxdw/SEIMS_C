/*!
 * \brief A IO test demo of developing module for SEIMS.
 *
 * \author Liangjun Zhu
 * \date 2018-02-07
 */

#ifndef SEIMS_MODULE_OL_HAND_H
#define SEIMS_MODULE_OL_HAND_H

#include "SimulationModule.h"
#include "Scenario.h"
//#include "clsReach.h"
#define FLOOD_DEPTH_THRESH 0.1f

using namespace bmps;
using namespace std;

// ��ʾÿһ��� HAND ��Ϣ
struct Level {
	//vector<float> handHeights;   // index is hand id
	vector<int> handIds;   // index is layer 0,1,2,3..., value is hand id

	//float m_chOverHeadVol;      /// represents the physical space between the top of the channel banks and the upper boundary, index represents subbasin id for dim 1, index represents layer for dim 2 cooresponding to each HAND height
	/*float* m_handArea;					/// area of each hand
	float* m_handWtrDep;			    /// water depth of each hand, initialized by m_bankSto*/
	float m_levelDepth = 0.0f;                 /// depth of each level, eg. the level is from 0~5m, so the depth is 5-0=5m.
	double m_levelSumArea = 0.0f;   /// area of each hand level, contains all levels' area lower than this level
	float m_levelAvgDepth = 0.0; /// average depth of each layer's all hands, equals (channel's overhead area + lower level's sum area * this level's depth + SUM(this level's hand's area * dem's avg depth in hand))
	double m_levelSumVol = 0.0f;    /// area of each hand level, cooresponding to m_levelSumArea
	double m_levelAccVol = 0.0;              /// contains a level's vol and all lower level's vol, m3
	//vector<float> m_levelLowerAccDepth;   // m
	float* m_levelLowerAccDepth;   // m

	int m_levelHandNum = 0;          /// n layers of hand for each level
	float m_levelWtrDep = 0.0f;              /// water depth of each level,m. contains all water above  the level

};

// ��ʾÿ���������µ����� HAND ��
struct Hand {
	int n_levels = 0;
	int m_CurInundationLevel = 0;
	vector<Level> levels;   /// index represents subbasin id (or reach id)
	float excessWtrVol = 0.0f;     /// water excess subbasin's full volume

	// for test
	float volToAdd = 0.0f;
};

class OL_HAND : public SimulationModule {
public:
	OL_HAND();

    virtual ~OL_HAND();

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

	void SetReaches(clsReaches* reaches) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

	void SetValue(const char* key, const float value) OVERRIDE;

	void InitialOutputs() OVERRIDE;

private:


	bool HandInundation_BinarySearch(const int reachId, float sto);
	void LoadHandIdsToChHandLevels(const std::string& filename, vector<Hand>& m_Hands);

	void updateAllHandsWtrDep(const int reachId);
	void loadHandFromCSVIntoVector(const string& csvPath, vector<Hand>& m_Hands);
	vector<float> parseAccDepthArray(const std::string& str);

	void updateSbExcessWater(const int reachId, float* vol);

	int m_dt;            ///< time step (sec)
	int m_nCells;
	int m_nSubbsns;
	int m_inputSubbsnID; ///< current subbasin ID, 0 for the entire watershed


	map<int, int> m_idToIndex;   /// map from subbasin id(or reach id) to index of the array
	vector<Hand> m_Hands;  ///  subbasin (or reach)-- layers -- hands,  index represents subbasin id for dim 1, index represents layer for dim 2
	///


	float* m_subbsnID;       /// subbasin grid (subbasins ID)
	int m_nreach;                ///< reach number (= subbasin number)
	int m_outletID;    ///< outlet ID, also can be derived by m_reachLayers.rbegin()->second[0];
	float* m_chWth;           ///< channel top width (m)
	float* m_chDepth;        ///< channel depth (m)
	float* m_chLen;            ///< channel length (m)
	//float** m_chOverHeadVol;       /// represents the physical space between the top of the channel banks and the upper boundary, index represents subbasin id for dim 1, index represents layer for dim 2 cooresponding to each HAND height
	float* m_handArea;       /// area of each hand
	float* m_subbasinArea;
	float* m_handWtrDep;    /// water depth of each hand, initialized by m_bankSto,m
	//float** m_handLyrSumArea;   /// area of each hand layer, index represents subbasin id for dim 1, index represents layer for dim 2
	//float** m_handLyrDepth;        /// depth of each hand layer, index represents subbasin id for dim 1, index represents layer for dim 2
	//float* m_handNumLyrs;          /// n layers of hand for each subbasin ,index represents subbasin id

	//float* m_chArea;          ///< the reach area (m^2) at bankfull
	float* m_islake;
	float* m_isres;
	float* m_bankSto;   ///< bank storage (m^3), bankst in SWAT
	float* m_chSto;		///< reach storage (m^3), rchstor in SWAT
	float* m_bankStoLastStep;   ///< bank storage (m^3) of last time step, bankst in SWAT
	float* m_chStoLastStep;  ///< reach storage (m^3) of last time step, rchstor in SWAT
	float* m_chWtrDepth;  ///< channel water depth (m), rchdep in SWAT
	float* m_chWtrWth;    ///< channel water width (m), topw in SWAT
	float* m_chBedMeanElev;   /// channel bed elevation (m)
	//float* m_chOverHeadWth;    ///< channel top width
	map<int, vector<int> > m_reachLayers;   	/// channels

	// for CH4 module
	float* m_isHandFlooded;

	// for inundation area calibration
	float* m_subbasinInundationArea;   // km2
	float* m_subbasinWtrDep;
	float m_sumInundationArea;


	// test
	int curLev = 0;               // ��ʼ�ӵ�1�㿪ʼ
	int levCounter = 0;           // �����������ڿ���ÿ���ν�����һ��




};
#endif /* SEIMS_MODULE_OL_HAND_H */
