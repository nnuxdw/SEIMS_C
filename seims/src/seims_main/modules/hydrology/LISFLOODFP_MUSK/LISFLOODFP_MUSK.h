/*!
 * \brief A IO test demo of developing module for SEIMS.
 *
 * \author Dawei Xiao
 * \date 2018-02-07
 */

#ifndef SEIMS_MODULE_LISFLOODFP_MUSK_H
#define SEIMS_MODULE_LISFLOODFP_MUSK_H

#include "SimulationModule.h"
#include "Scenario.h"
#include "lisflood.h"
#include <unordered_set>
#include <unordered_map>
//#include "./lisflood2/DataTypes.h"

using namespace bmps;
using namespace std;



class LISFLOODFP_MUSK : public SimulationModule {
public:

	LISFLOODFP_MUSK();

    virtual ~LISFLOODFP_MUSK();

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

	void Set2DData(const char* key, int nrows, int ncols, float** data) OVERRIDE;

	void SetScenario(Scenario* sce) OVERRIDE;

	void SetReaches(clsReaches* reaches) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

	void Get2DData(const char* key, int* nrows, int* ncols, float*** data) OVERRIDE;

	void GetValue(const char* key, float* value) OVERRIDE;

	void SetValue(const char* key, const float value) OVERRIDE;

	//ljj++
	void SetSubbasins(clsSubbasins* subbasins) OVERRIDE;

	void InitialOutputs() OVERRIDE;




private:


	int m_dt;            ///< time step (sec)
	int m_nCells;
	int m_nSubbsns;
	//int m_inputSubbsnID; ///< current subbasin ID, 0 for the entire watershed
	
	float* m_subbsnID;/// subbasin grid (subbasins ID)
	int counter;
	string seims_start_time;

	// Instances of Structures
	Arrays Raster;
	Files Fps;
	Fnames ParFp;
	States SimStates;
	Pars Params;
	Solver ParSolver;
	Pois PoisHandler;
	BoundCs Bounds;
	Stage OutLocs;
	SGCprams SGCchanprams;
	DamData DamDataprams;
	vector<ChannelSegmentType> ChannelSegments;
	LISFLOODFPContext LFPContext;
	LfpCouplingInfo LfpCoupleInfo;
	//int verbosemode;
	

	struct Arrays *Arrptr;
	struct Files* FpsPtr;
	struct Fnames *Fnameptr;
	struct States *Statesptr;
	struct Pars *Parptr;
	struct Solver *Solverptr;
	struct Pois *Poisptr;
	struct BoundCs *BCptr;
	struct Stage *Stageptr;
	struct SGCprams *SGCptr;
	struct DamData *Damptr;
	struct Stage *Locptr;
	char* tmpFileNamePtr;
	char* tmpSysCmdPtr;
	vector<ChannelSegmentType> *ChannelSegmentsVecPtr;
	struct SuperGridLinksList *Super_linksptr;
	struct LISFLOODFPContext* LFPContextPtr;
	struct LfpCouplingInfo * LfpCouplingInfoPtr;

	unordered_set<int> lfpSetFirst;  // lfp首个子流域
	unordered_set<int> lfpSetOther;  // lfp剩余子流域
	unordered_map<int, LfpCouplingInfo> coupling_map; // key is the first id of lfp's subbasin


	void RunCalculation();

	void InitializeLisfloodFP();

	void updateCurrentTimestamp(int * current_timestamp, LISFLOODFPContext* LFPContextPtr);

	void parseCouplingFile(const string& filepath,unordered_set<int>& lfpSetFirst,unordered_set<int>& lfpSetOther,unordered_map<int, LfpCouplingInfo>& coupling_map);


	/************************************MUSK_CH**********************************/
	void PointSourceLoading();

	bool ChannelFlow(int i);

	//ljj++
	bool LakeBudget(int i);

	bool ResBudget(int i);

	void SetValueByIndex(const char* key, const int index, const float value);

	int m_inputSubbsnID; ///< current subbasin ID, 0 for the entire watershed

	int m_nreach;      ///< reach number (= subbasin number)
	int m_outletID;    ///< outlet ID, also can be derived by m_reachLayers.rbegin()->second[0];
	float m_Epch;      ///< reach evaporation adjustment factor, evrch in SWAT.
	float m_Bnk0;      ///< initial bank storage per meter of reach length (m^3/m)
	float m_Chs0_perc; ///< initial percentage of channel water volume
	float m_aBank;     ///< bank flow recession constant
	float m_bBank;     ///< bank storage loss coefficient

	/// Muskingum input parameters

	// Weighting factor controlling relative importance of inflow rate and outflow rate in determining water storage in reach segment
	float m_mskX;
	// Calibration coefficient used to control impact of the storage time constant for normal flow
	float m_mskCoef1;
	// Calibration coefficient used to control impact of the storage time constant fro low flow
	float m_mskCoef2;

	/// Reach information

	float* m_chWth;           ///< channel width (m)
	float* m_chDepth;         ///< channel depth (m)
	float* m_chLen;           ///< channel length (m)
	float* m_chArea;          ///< the reach area (m^2) at bankfull
	float* m_chSideSlope;     ///< inverse of the channel side slope, by default is 2. chside in SWAT.
	float* m_chSlope;         ///< average slope of main channel
	float* m_chMan;           ///< Manning's "n" value for the main channel
	float* m_Kchb;            ///< hydraulic conductivity of the channel bed (mm/h)
	float* m_Kbank;           ///< hydraulic conductivity of the channel bank (mm/h)
	float* m_reachDownStream; ///< downstream id (The value is -1 if there if no downstream reach)
	/*!
	 * Index of upstream Ids (The value is -1 if there if no upstream reach)
	 * m_reachUpStream.size() = N+1
	 * m_reachUpStream[1] = [2, 3] means Reach 2 and Reach 3 flow into Reach 1.
	 */
	vector<vector<int> > m_reachUpStream;
	/*!
	 * reach layers
	 * key: computing order, \sa LayeringMethod
	 * value: reach ID
	 */
	map<int, vector<int> > m_rteLyrs;

	/// scenario data

	/*!
	 * point source operations
	 * key: unique index, BMPID * 100000 + subScenarioID
	 * value: point source management factory instance
	 */
	map<int, BMPPointSrcFactory *> m_ptSrcFactory;


	// Inputs from other modules

	float* m_petSubbsn; ///< Average PET of each subbasin, mm
	float* m_gwSto;     ///< Groundwater storage (mm) of the subbasin
	float* m_olQ2Rch;   ///< overland flow to streams from each subbasin (m^3/s)
	float* m_ifluQ2Rch; ///< interflow to streams from each subbasin (m^3/s)
	float* m_gndQ2Rch;  ///< groundwater flow out of the subbasin (m^3/s)

	// Temporary variables

	float* m_ptSub;   ///< The point source discharge (m^3/s) load from m_ptSrcFactory
	float* m_flowIn;  ///< flow into reach for routing iteration, m^3
	float* m_flowOut; ///< flow out of reach for routing iteration, m^3
	float* m_seepage; ///< seepage to deep aquifer

	// Ouputs

	float* m_qRchOut;  ///< reach outflow (m^3/s), sdti in SWAT
	float* m_qsRchOut; ///< surface part of channel outflow
	float* m_qiRchOut; ///< subsurface part of channel outflow
	float* m_qgRchOut; ///< groundwater part of channel outflow

	float* m_chSto;     ///< reach storage (m^3), rchstor in SWAT
	float* m_chStoLastStep;   ///< reach storage (m^3) of last step, rchstor in SWAT(xiaodw add, for calculating hand water level change)
	float* m_rteWtrIn;  ///< Water flowing in reach on day before channel routing, m^3
	float* m_rteWtrOut; ///< Water leaving reach on day after channel routing, m^3, rtwtr in SWAT
	float* m_bankSto;   ///< bank storage (m^3), bankst in SWAT
	float* m_bankStoLastStep;   ///< bank storage (m^3) of last step, bankst in SWAT(xiaodw add, for calculating hand water level change)

	float* m_chWtrDepth;  ///< channel water depth (m), rchdep in SWAT
	float* m_chWtrWth;    ///< channel water width (m), topw in SWAT
	float* m_chBtmWth;    ///< bottom width of channel (m), phi(6,:) in SWAT
	float* m_chCrossArea; ///< cross-sectional area (m^2), rcharea in SWAT
	float* m_chBedMeanElev;   /// channel bed mean elevation (m)
	float* m_chBedStartElev;   /// channel bed start point elevation (m)
	float* m_chBedEndElev;   /// channel bed end point elevation (m)

	//ljj++
	int m_maxSoilLyrs;

	//! maximum ground water storage
	float m_GWMAX;
	float m_GWMIN;
	float m_Kg;
	float m_Base_ex;
	float m_evlake; //lake evaporation coefficient
	float m_lakeseep; //m/day; hydraulic conductivity of the lake bottom
	float m_petFactor;
	float m_minvol;
	float m_lakeb;

	float* m_ispermafrost;
	float* m_islake;
	float* m_lakearea;
	float* curBasinArea;
	float* m_area;
	float* m_lakevol;
	float* m_lakedpini;
	float* m_lakealpha;
	float* m_isres;
	float* m_natural_flow; //naturalized daily streamflow
	float* m_ResLc;
	float* m_ResLn;
	float* m_ResLf;
	float* m_ResAdjust;
	float* flowoutlength;

	float* m_A_Va;
	float* m_A_Vb;
	float* m_A_a;
	float* m_A_b;

	float* m_netPcp;
	float* m_PET;
	float* m_prec;
	float* m_pet;
	float* m_lakepcp;
	float* m_lakeperc;

	float* m_qin1;
	float* m_qout1;

	float* m_resndq;
	float* m_resminq;
	float* m_resnormq;
	float* m_res_normMult;

	float* m_lakedp;

	float* m_Ch2GW;
	float* m_aquifer;
	float* m_charge;
	float* m_recharge;
	float* m_qin;

	float* m_temp1;
	float* m_temp2;
	float* m_dem;
	float* m_slope;
	float* m_potRfCoef;
	float* curBasinDem;
	float** m_soilTempprofile;
	float** m_T_LKWB;

	float* m_rrtime;

	// subbasin IDs
	vector<int> m_subbasinIDs;
	// All subbasins information
	clsSubbasins* m_subbasinsInfo;

	/************************************End MUSK_CH**********************************/
};
#endif 
