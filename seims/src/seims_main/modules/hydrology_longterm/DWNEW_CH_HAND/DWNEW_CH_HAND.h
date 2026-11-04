/*! 
 * \file DWNEW_CH_HAND.h
 * \brief Pure diffusive-wave channel routing with HAND-based inundation states.
 * \author lp
 * \date 2026-03-20
 */
#ifndef SEIMS_MODULE_DWNEW_CH_HAND_H
#define SEIMS_MODULE_DWNEW_CH_HAND_H

#define FLOOD_DEPTH_THRESH 0.1f
#ifndef MID_DWNEW_CH_HAND
#define MID_DWNEW_CH_HAND "DWNEW_CH_HAND"
#endif


#include "SimulationModule.h"
#include "Scenario.h"

#include <map>
#include <vector>

using namespace std;
using namespace bmps;

// HAND level information for one reach.
struct Level {
    int* handIds = nullptr;
    float m_levelDepth = 0.f;
    double m_levelSumArea = 0.0;
    float m_levelAvgDepth = 0.f;
    double m_levelSumVol = 0.0;
    double m_levelAccVol = 0.0;
    float* m_levelLowerAccDepth = nullptr;
    int m_levelHandNum = 0;
    float m_levelWtrDep = 0.f;
};

// HAND layered storage-space information for one reach.
struct Hand {
    int n_levels = 0;
    int m_CurInundationLevel = 0;
    vector<Level> levels;
    float excessWtrVol = 0.f;
    float volToAdd = 0.f;
};

class DWNEW_CH_HAND : public SimulationModule {
public:
    DWNEW_CH_HAND();
    virtual ~DWNEW_CH_HAND();

    void SetValue(const char* key, float value) OVERRIDE;
    void SetValueByIndex(const char* key, int index, float value) OVERRIDE;
    void Set1DData(const char* key, int n, float* data) OVERRIDE;
    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;
    void SetScenario(Scenario* sce) OVERRIDE;
    void SetReaches(clsReaches* rches) OVERRIDE;
    void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;

    bool CheckInputData() OVERRIDE;
    void InitialOutputs() OVERRIDE;
    int Execute() OVERRIDE;

    TimeStepType GetTimeStepType() OVERRIDE;
    void GetValue(const char* key, float* value) OVERRIDE;
    void Get1DData(const char* key, int* n, float** data) OVERRIDE;
    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

    void LoadHandLevelsFromArrays(int cellsNum, int flatLen,
        std::vector<Hand>& hands, float nodata, bool buildHandIds);

private:
    void PointSourceLoading();
    void RefreshReachHydraulicState(int i);
    bool ChannelFlow_DiffusiveWave(int i, float sub_dt);

    float ComputeRiverChannelStorageCap(int reachId) const;
    float ComputeRiverChannelOutletDepth(int reachId, float sto) const;
    void SetRiverChannelOnlyOutletHandDepth(int reachId, float depth);
    bool IsLakeLikeReach(int reachId) const;
    int GetLakeConnectorUpstreamLake(int reachId) const;
    bool IsLakeConnectorReach(int reachId) const;
    float ComputeLakeConnectorStage(int reachId) const;
    float ComputeDiffusiveManningDischarge(int reachId, float waterSurfaceSlope,
        float hydraulicDepth, bool isLakeLike) const;

    bool HandInundation_BinarySearch(int reachId, float sto);
    void updateAllHandsWtrDep(int reachId);
    void ClearHandStateForReach(int reachId);

    float ComputeResScheduledOutflow(int i, float curSto, float qIn, float sub_dt);

private:
    // ===== Basic control =====
    int m_dt;
    int m_inputSubbsnID;
    int m_nreach;
    int m_outletID;
    // ===== External outlet boundary control =====
    // 0 = closed/endoreic outlet (default, no outflow when there is no downstream reach).
    // 1 = free/normal-depth boundary, using m_outletBcSlope as the external water-surface slope.
    // 2 = fixed-stage boundary, using m_outletBcStage as a downstream ghost water level.
    int m_outletBcType;
    float m_outletBcStage;
    float m_outletBcSlope;
    float m_outletBcAllowBackflow;

    int m_nCells;
    int m_maxSoilLyrs;
    int m_curSubStep;
    int m_subStepsPerDay;

    // ===== Main hydraulic parameters =====
    float m_gravity;
    float* m_chMan;          ///< Manning n at control section.
    float* m_chWth;          ///< Bankfull top width at control section (m).
    float* m_ChDepth;        ///< Channel/control-section depth below ground (m).
    float* m_chLen;          ///< Effective exchange length to downstream (m).
    float* m_chArea;         ///< Reach plan area (m2).
    float* m_chSideSlope;    ///< Trapezoid side slope.
    float* m_Kchb;           ///< Channel bed hydraulic conductivity.
    float* m_Kbank;          ///< Bank hydraulic conductivity.
    float* m_reachDownStream;
    float* m_chBedElev;      ///< Control-section bed elevation (m).
    float* m_outletHandId;   ///< HAND unit id of the control section.

    vector<vector<int>> m_reachUpStream;
    map<int, vector<int>> m_rteLyrs;

    // ===== Current control-section state =====
    float* m_sfcElv;
    float* m_dwnElv;
    float* m_rivOut;
    float* m_rivVel;

    // ===== Shared routing parameters =====
    float m_Epch;
    float m_Bnk0;
    float m_Chs0_perc;
    float m_aBank;
    float m_bBank;
    float* m_Epch_1d;
    float* m_subbsnID;

    // ===== Inputs from other modules =====
    float* m_petSubbsn;
    float* m_gwSto;
    float* m_olQ2Rch;
    float* m_ifluQ2Rch;
    float* m_gndQ2Rch;
    float* m_area;
    float* m_netPcp;
    float* m_PET;
    float* m_HAND_Infil;
    float* m_handEvap;
    float* m_handDep;
    float* m_HAND_BackFromGW;
    map<int, BMPPointSrcFactory*> m_ptSrcFactory;
    float* m_ptSub;

    // ===== Main outputs / states =====
    float* m_qRchOut;
    float* m_qsRchOut;
    float* m_qiRchOut;
    float* m_qgRchOut;
    float* m_chSto;
    float* m_chStoLastStep;
    float* m_rteWtrIn;
    float* m_rteWtrOut;
    float* m_bankSto;
    float* m_bankStoLastStep;
    float* m_chWtrDepth;     ///< Water depth above control-section bed.
    float* m_chWtrWth;
    float* m_chBtmWth;
    float* m_chCrossArea;
    float* m_seepage;
    float* m_charge;
    float* m_recharge;
    float* m_Ch2GW;
    float* m_aquifer;

    // ===== Lake / reservoir related =====
    float m_GWMAX;
    float m_GWMIN;
    float m_Kg;
    float m_Base_ex;
    float m_evlake;
    float m_lakeseep;
    float m_petFactor;
    float m_minvol;
    float m_lakeb;
    float* m_minvol_1d;
    float* m_lakeb_1d;
    float* m_islake;
    float* m_lakevol;
    float* m_isres;
    float* m_ResLc;
    float* m_ResLn;
    float* m_ResLf;
    float* m_ResAdjust;
    float* m_resndq;
    float* m_resminq;
    float* m_resnormq;
    float* m_res_normMult;
    float* m_prec;
    float* m_pet;
    float* m_lakepcp;
    float* m_lakeperc;
    float* m_lakedp;
    float* m_lakedpini;
    float* m_lakearea;
    float** m_T_LKWB;
    float* m_rrtime;

    // ===== HAND states =====
    float* m_lakeHandLevelini;
    float* m_handLevels;
    float* m_handArea;
    float* m_handWtrDep;
    float* m_isHandFlooded;
    float* m_subbasinWtrDep;
    float* m_subbasinInundationArea;
    float* m_subbasinArea;
    vector<Hand> m_Hands;

    // ===== HAND static arrays =====
    float* m_HAND_Subbasin;
    float* m_HAND_Flood_Level;
    float* m_HAND_LevelDepth;
    float* m_HAND_SumArea;
    float* m_HAND_SumVolume;
    float* m_HAND_AvgDepth;
    float* m_HAND_AccVolume;
    float* m_HAND_LowerAccDepthFlat;
    float* m_HAND_LowerAccDepthLen;

    // ===== Misc / subbasin info =====
    vector<int> m_subbasinIDs;
    clsSubbasins* m_subbasinsInfo;
    float* m_petFactor_1d;
};

#endif /* SEIMS_MODULE_DWNEW_CH_HAND_H */
