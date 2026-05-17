#include "DWNEW_CH_HAND.h"
#include "text.h"
#include "utils_math.h"
#include "ChannelRoutingCommon.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <new>
#include <sstream>
#include <string>
#include <set>
#include <vector>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace utils_math;

namespace {
const int DW_SUBSTEPS = 48;
const float DW_LAKE_MIN_MANNING = 0.10f;
const bool DW_ENABLE_DEBUG_LOG = true;
const float DW_OVERBANK_WIDTH_FACTOR = 5.f;
const float DW_OVERBANK_PERIMETER_WIDTH_FACTOR = 4.f;
const float DW_OVERBANK_SIDE_SLOPE = 4.f;
const float DW_DRY_DEPTH = 0.005f;
const float DW_STAGE_DIFF_EPS = 1.e-4f;       // 0.1 mm head difference deadband.
const float DW_FLOW_LOG_Q_EPS = 1.e-4f;
const float DW_DEFAULT_OUTLET_BC_SLOPE = 1.e-5f;
const char* DW_KEY_OUTLET_BC_TYPE = "DW_OUTLET_BC_TYPE";
const char* DW_KEY_OUTLET_BC_STAGE = "DW_OUTLET_BC_STAGE";
const char* DW_KEY_OUTLET_BC_SLOPE = "DW_OUTLET_BC_SLOPE";
const char* DW_KEY_OUTLET_BC_ALLOW_BACKFLOW = "DW_OUTLET_BC_ALLOW_BACKFLOW";
const bool DW_DEBUG_LOG_ALL_SUBSTEPS = true;
const int DW_DEBUG_FIRST_N_DAYS = 0; // <= 0 means all simulation days
const int DW_DEBUG_ROOT_REACH = 81;
const int DW_DEBUG_MONTH = 0; // 1-12, 0 means all months
const int DW_DEBUG_FIRST_N_YEARS = 0; // <= 0 means all years
const char* DW_DEBUG_ROOT_LABEL = "reach81_only_fullperiod_allsubsteps";
const char* DW_DEBUG_LOG_FILE = "DWNEW_CH_HAND_debug_reach81_only_fullperiod_allsubsteps.csv";
const char* DW_DEBUG_TOPO_FILE = "DWNEW_CH_HAND_debug_reach81_only_topology.csv";

std::set<int> g_dwDebugReachCluster;
std::set<int> g_dwDebugFocusReachSet;
std::map<int, int> g_dwDebugReachDistance;
std::map<int, int> g_dwDebugReachRoot;
bool g_dwDebugLogHeaderWritten = false;
bool g_dwDebugClusterReady = false;
bool g_dwDebugTopologyWritten = false;
std::ofstream g_dwDebugLog;
std::vector<double> g_dwDebugHandInfilM3;
std::vector<double> g_dwDebugHandEvapM3;
std::vector<double> g_dwDebugHandDepM3;
std::vector<double> g_dwDebugHandBackFromGwM3;
std::vector<double> g_dwDebugHandNetSubtractM3;

void ResetDebugHandBudget(const int nreach) {
    g_dwDebugHandInfilM3.assign(nreach + 1, 0.0);
    g_dwDebugHandEvapM3.assign(nreach + 1, 0.0);
    g_dwDebugHandDepM3.assign(nreach + 1, 0.0);
    g_dwDebugHandBackFromGwM3.assign(nreach + 1, 0.0);
    g_dwDebugHandNetSubtractM3.assign(nreach + 1, 0.0);
}

std::string UpstreamIdsForLog(const int reachId, const std::vector<std::vector<int>>& reachUpStream) {
    if (reachId < 0 || reachId >= static_cast<int>(reachUpStream.size())) return "";
    const std::vector<int>& ups = reachUpStream[reachId];
    std::ostringstream ss;
    for (size_t i = 0; i < ups.size(); ++i) {
        if (i > 0) ss << '|';
        ss << ups[i];
    }
    return ss.str();
}

void InitDebugReachCluster(const int rootReach, const float* reachDownStream, const int nreach) {
    if (!DW_ENABLE_DEBUG_LOG) {
        g_dwDebugReachCluster.clear();
        g_dwDebugFocusReachSet.clear();
        g_dwDebugReachDistance.clear();
        g_dwDebugReachRoot.clear();
        g_dwDebugClusterReady = true;
        return;
    }
    if (g_dwDebugClusterReady) return;

    g_dwDebugReachCluster.clear();
    g_dwDebugFocusReachSet.clear();
    g_dwDebugReachDistance.clear();
    g_dwDebugReachRoot.clear();
    if (rootReach > 0 && rootReach <= nreach && nullptr != reachDownStream) {
        g_dwDebugFocusReachSet.insert(rootReach);
        g_dwDebugReachCluster.insert(rootReach);
        g_dwDebugReachDistance[rootReach] = 0;
        g_dwDebugReachRoot[rootReach] = rootReach;
    }

    g_dwDebugClusterReady = true;
}

bool IsDebugReach(const int reachId) {
    return DW_ENABLE_DEBUG_LOG && g_dwDebugReachCluster.find(reachId) != g_dwDebugReachCluster.end();
}

bool IsDebugFocusReach(const int reachId) {
    return DW_ENABLE_DEBUG_LOG && g_dwDebugFocusReachSet.find(reachId) != g_dwDebugFocusReachSet.end();
}

int DebugReachDistance(const int reachId) {
    std::map<int, int>::const_iterator it = g_dwDebugReachDistance.find(reachId);
    return it != g_dwDebugReachDistance.end() ? it->second : -1;
}

int DebugReachRoot(const int reachId) {
    std::map<int, int>::const_iterator it = g_dwDebugReachRoot.find(reachId);
    return it != g_dwDebugReachRoot.end() ? it->second : -1;
}

bool ShouldWriteDebugLog(const int month, const int dayOfYear,
    const int curSubStep, const int subStepsPerDay) {
    return DW_ENABLE_DEBUG_LOG
        && (DW_DEBUG_MONTH <= 0 || month == DW_DEBUG_MONTH)
        && (DW_DEBUG_FIRST_N_DAYS <= 0 || dayOfYear <= DW_DEBUG_FIRST_N_DAYS)
        && (DW_DEBUG_LOG_ALL_SUBSTEPS || curSubStep == subStepsPerDay);
}

void WriteDebugLogHeader(std::ofstream& dbg) {
    dbg << "date,year,month,day,doy,substep_idx,substeps_per_day,substep_dt_s,"
        << "root_reach,reach_id,debug_distance,is_focus,"
        << "is_lake,is_res,is_connector,connector_up_lake,downstream_id,has_downstream,"
        << "outlet_bc_type,outlet_bc_stage_m,outlet_bc_slope,allow_outlet_backflow,"
        << "sto_begin_m3,hand_infil_m3,hand_evap_m3,hand_dep_m3,hand_back_from_gw_m3,hand_net_subtract_m3,"
        << "sto_after_inflow_m3,sto_end_m3,storage_delta_m3,"
        << "qIn_total_cms,qs_local_cms,qi_local_cms,qg_local_cms,pt_local_cms,"
        << "qs_up_cms,qi_up_cms,qg_up_cms,prec_cms,"
        << "ch_len_m,hydraulic_depth_m,hydraulic_area_m2,eff_width_m,n_manning,dslp,"
        << "dout_raw_cms,max_vel_mps,max_dout_cms,dout_after_limit_cms,req_back_vol_m3,act_back_vol_m3,"
        << "lake_evap_m3,lake_seep_m3,river_seep_m3,river_evap_m3,bank_out_m3,bank_out_gw_m3,"
        << "ctrl_bed_elv_m,sfc_elv_m,dwn_elv_m,outlet_hand_depth_m,is_outlet_hand_wet,connector_stage_m,"
        << "ctrl_depth_m,ctrl_width_m,ctrl_area_m2,ctrl_velocity_mps,"
        << "subbasin_depth_m,hand_level,inundation_area_km2,lake_area_m2,"
        << "signed_q_cms,qout_cms,rteWtrIn_m3,rteWtrOut_m3,net_rte_exchange_m3,rrtime_h,outlet_hand_id"
        << '\n';
}
void AppendDebugLogRow(const std::string& row) {
    if (!DW_ENABLE_DEBUG_LOG) return;
    if (!g_dwDebugLog.is_open()) {
        g_dwDebugLog.open(DW_DEBUG_LOG_FILE,
            g_dwDebugLogHeaderWritten ? std::ios::app : std::ios::out);
        if (!g_dwDebugLog.is_open()) return;
    }
    if (!g_dwDebugLogHeaderWritten) {
        WriteDebugLogHeader(g_dwDebugLog);
        g_dwDebugLogHeaderWritten = true;
    }
    g_dwDebugLog << row;
}

void WriteDebugReachTopology(const std::vector<std::vector<int>>& reachUpStream,
    const float* reachDownStream, const float* isLake, const float* isRes,
    const int nreach) {
    if (!DW_ENABLE_DEBUG_LOG) return;
    if (g_dwDebugTopologyWritten || !g_dwDebugClusterReady) return;

    std::ofstream topo(DW_DEBUG_TOPO_FILE, std::ios::out);
    if (!topo.is_open()) return;

    topo << "root_reach,reach_id,debug_distance,is_focus,is_lake,is_res,downstream_id,upstream_count,upstream_ids\n";
    std::vector<int> orderedReaches(g_dwDebugReachCluster.begin(), g_dwDebugReachCluster.end());
    std::sort(orderedReaches.begin(), orderedReaches.end(), [](const int lhs, const int rhs) {
        const std::map<int, int>::const_iterator lhsIt = g_dwDebugReachDistance.find(lhs);
        const std::map<int, int>::const_iterator rhsIt = g_dwDebugReachDistance.find(rhs);
        const int lhsDist = lhsIt != g_dwDebugReachDistance.end()
            ? lhsIt->second : (std::numeric_limits<int>::max)();
        const int rhsDist = rhsIt != g_dwDebugReachDistance.end()
            ? rhsIt->second : (std::numeric_limits<int>::max)();
        if (lhsDist != rhsDist) return lhsDist < rhsDist;
        return lhs < rhs;
    });
    for (size_t idx = 0; idx < orderedReaches.size(); ++idx) {
        const int reachId = orderedReaches[idx];
        topo << DebugReachRoot(reachId) << ','
            << reachId << ','
            << DebugReachDistance(reachId) << ','
            << (IsDebugFocusReach(reachId) ? 1 : 0) << ','
            << ((nullptr != isLake && isLake[reachId] == 1.f) ? 1 : 0) << ','
            << ((nullptr != isRes && isRes[reachId] == 1.f) ? 1 : 0) << ','
            << static_cast<int>(reachDownStream[reachId]) << ',';

        if (reachId < static_cast<int>(reachUpStream.size())) {
            const std::vector<int>& ups = reachUpStream[reachId];
            topo << ups.size() << ',';
            for (size_t upIdx = 0; upIdx < ups.size(); ++upIdx) {
                if (upIdx > 0) topo << '|';
                topo << ups[upIdx];
            }
        }
        else {
            topo << "0,";
        }
        topo << '\n';
    }
    topo.flush();
    g_dwDebugTopologyWritten = true;
}

static inline bool IsFiniteF(const float v) { return std::isfinite(v); }
static inline bool IsNoData(const float v, const float nodata) {
    return std::isnan(v) || std::fabs(v - nodata) < 1.e-6f;
}

float SolveTrapezoidDepthFromArea(const float wettedArea, const float btmWth, const float sideSlope) {
    if (wettedArea <= 0.f) return 0.f;
    const float b = std::max(btmWth, 0.f);
    const float z = std::max(sideSlope, 0.f);
    if (z <= 1.e-6f) return wettedArea / std::max(b, 1.e-6f);
    const float disc = b * b + 4.f * z * wettedArea;
    return std::max((-b + sqrtf(std::max(disc, 0.f))) / (2.f * z), 0.f);
}

float ComputeCompoundAreaFromDepth(const float depth, const float chDepth,
    const float btmWth, const float sideSlope, const float bankfullWidth) {
    if (depth <= 0.f) return 0.f;
    const float dBkf = std::max(chDepth, 0.f);
    const float b = std::max(btmWth, 0.f);
    const float z = std::max(sideSlope, 0.f);
    if (depth <= dBkf + 1.e-6f) return b * depth + z * depth * depth;
    const float bankfullArea = b * dBkf + z * dBkf * dBkf;
    const float excessDepth = depth - dBkf;
    const float floodplainBaseWidth = std::max(bankfullWidth, 1.f) * DW_OVERBANK_WIDTH_FACTOR;
    return bankfullArea + floodplainBaseWidth * excessDepth
        + DW_OVERBANK_SIDE_SLOPE * excessDepth * excessDepth;
}

float ComputeCompoundTopWidthFromDepth(const float depth, const float chDepth,
    const float btmWth, const float sideSlope, const float bankfullWidth) {
    if (depth <= 0.f) return std::max(btmWth, 0.f);
    const float dBkf = std::max(chDepth, 0.f);
    if (depth <= dBkf + 1.e-6f) return std::max(btmWth, 0.f) + 2.f * std::max(sideSlope, 0.f) * depth;
    const float excessDepth = depth - dBkf;
    return std::max(bankfullWidth, 1.f) * DW_OVERBANK_WIDTH_FACTOR + 2.f * DW_OVERBANK_SIDE_SLOPE * excessDepth;
}

float ComputeCompoundWettedPerimeter(const float depth, const float chDepth,
    const float btmWth, const float sideSlope, const float bankfullWidth) {
    if (depth <= 0.f) return std::max(btmWth, 0.f);
    const float dBkf = std::max(chDepth, 0.f);
    const float z = std::max(sideSlope, 0.f);
    if (depth <= dBkf + 1.e-6f) return std::max(btmWth, 0.f) + 2.f * depth * sqrtf(1.f + z * z);
    const float excessDepth = depth - dBkf;
    const float bankfullPerimeter = std::max(btmWth, 0.f) + 2.f * dBkf * sqrtf(1.f + z * z);
    return bankfullPerimeter + std::max(bankfullWidth, 1.f) * DW_OVERBANK_PERIMETER_WIDTH_FACTOR
        + 2.f * excessDepth * sqrtf(1.f + DW_OVERBANK_SIDE_SLOPE * DW_OVERBANK_SIDE_SLOPE);
}
}


// Construct the module and set every pointer/state to a safe initial value.
DWNEW_CH_HAND::DWNEW_CH_HAND() :
    m_dt(-1), m_inputSubbsnID(-1), m_nreach(-1), m_outletID(-1),
    m_outletBcType(0), m_outletBcStage(0.f), m_outletBcSlope(DW_DEFAULT_OUTLET_BC_SLOPE),
    m_outletBcAllowBackflow(0.f),
    m_nCells(-1), m_maxSoilLyrs(-1), m_curSubStep(0), m_subStepsPerDay(0),
    m_gravity(9.81f),
    m_chMan(nullptr), m_chWth(nullptr), m_ChDepth(nullptr), m_chLen(nullptr),
    m_chArea(nullptr), m_chSideSlope(nullptr), m_Kchb(nullptr), m_Kbank(nullptr),
    m_reachDownStream(nullptr), m_chBedElev(nullptr), m_outletHandId(nullptr),
    m_sfcElv(nullptr), m_dwnElv(nullptr),
    m_rivOut(nullptr), m_rivVel(nullptr),
    m_Epch(NODATA_VALUE), m_Bnk0(NODATA_VALUE), m_Chs0_perc(NODATA_VALUE),
    m_aBank(NODATA_VALUE), m_bBank(NODATA_VALUE), m_Epch_1d(nullptr), m_subbsnID(nullptr),
    m_petSubbsn(nullptr), m_gwSto(nullptr), m_olQ2Rch(nullptr), m_ifluQ2Rch(nullptr),
    m_gndQ2Rch(nullptr), m_area(nullptr), m_netPcp(nullptr), m_PET(nullptr),
    m_HAND_Infil(nullptr), m_handEvap(nullptr), m_handDep(nullptr), m_HAND_BackFromGW(nullptr),
    m_ptSub(nullptr), m_qRchOut(nullptr), m_qsRchOut(nullptr), m_qiRchOut(nullptr), m_qgRchOut(nullptr),
    m_chSto(nullptr), m_chStoLastStep(nullptr), m_rteWtrIn(nullptr), m_rteWtrOut(nullptr),
    m_bankSto(nullptr), m_bankStoLastStep(nullptr), m_chWtrDepth(nullptr), m_chWtrWth(nullptr),
    m_chBtmWth(nullptr), m_chCrossArea(nullptr), m_seepage(nullptr), m_charge(nullptr),
    m_recharge(nullptr), m_Ch2GW(nullptr), m_aquifer(nullptr),
    m_GWMAX(NODATA_VALUE), m_GWMIN(NODATA_VALUE), m_Kg(NODATA_VALUE), m_Base_ex(NODATA_VALUE),
    m_evlake(NODATA_VALUE), m_lakeseep(NODATA_VALUE), m_petFactor(NODATA_VALUE),
    m_minvol(NODATA_VALUE), m_lakeb(NODATA_VALUE), m_minvol_1d(nullptr), m_lakeb_1d(nullptr),
    m_islake(nullptr), m_lakevol(nullptr), m_isres(nullptr), m_ResLc(nullptr), m_ResLn(nullptr),
    m_ResLf(nullptr), m_ResAdjust(nullptr), m_resndq(nullptr), m_resminq(nullptr), m_resnormq(nullptr),
    m_res_normMult(nullptr), m_prec(nullptr), m_pet(nullptr), m_lakepcp(nullptr), m_lakeperc(nullptr),
    m_lakedp(nullptr), m_lakedpini(nullptr), m_lakearea(nullptr), m_T_LKWB(nullptr), m_rrtime(nullptr),
    m_lakeHandLevelini(nullptr), m_handLevels(nullptr), m_handArea(nullptr), m_handWtrDep(nullptr),
    m_isHandFlooded(nullptr), m_subbasinWtrDep(nullptr), m_subbasinInundationArea(nullptr), m_subbasinArea(nullptr),
    m_HAND_Subbasin(nullptr), m_HAND_Flood_Level(nullptr), m_HAND_LevelDepth(nullptr), m_HAND_SumArea(nullptr),
    m_HAND_SumVolume(nullptr), m_HAND_AvgDepth(nullptr), m_HAND_AccVolume(nullptr),
    m_HAND_LowerAccDepthFlat(nullptr), m_HAND_LowerAccDepthLen(nullptr),
    m_subbasinsInfo(nullptr), m_petFactor_1d(nullptr) {
}

// Release arrays owned by this module.
DWNEW_CH_HAND::~DWNEW_CH_HAND() {
    if (m_sfcElv) Release1DArray(m_sfcElv);
    if (m_dwnElv) Release1DArray(m_dwnElv);
    if (m_rivOut) Release1DArray(m_rivOut);
    if (m_rivVel) Release1DArray(m_rivVel);
    if (m_ptSub) Release1DArray(m_ptSub);
    if (m_qRchOut) Release1DArray(m_qRchOut);
    if (m_qsRchOut) Release1DArray(m_qsRchOut);
    if (m_qiRchOut) Release1DArray(m_qiRchOut);
    if (m_qgRchOut) Release1DArray(m_qgRchOut);
    if (m_chSto) Release1DArray(m_chSto);
    if (m_chStoLastStep) Release1DArray(m_chStoLastStep);
    if (m_rteWtrIn) Release1DArray(m_rteWtrIn);
    if (m_rteWtrOut) Release1DArray(m_rteWtrOut);
    if (m_bankSto) Release1DArray(m_bankSto);
    if (m_bankStoLastStep) Release1DArray(m_bankStoLastStep);
    if (m_chWtrDepth) Release1DArray(m_chWtrDepth);
    if (m_chWtrWth) Release1DArray(m_chWtrWth);
    if (m_chBtmWth) Release1DArray(m_chBtmWth);
    if (m_chCrossArea) Release1DArray(m_chCrossArea);
    if (m_seepage) Release1DArray(m_seepage);
    if (m_charge) Release1DArray(m_charge);
    if (m_recharge) Release1DArray(m_recharge);
    if (m_Ch2GW) Release1DArray(m_Ch2GW);
    if (m_aquifer) Release1DArray(m_aquifer);
    if (m_prec) Release1DArray(m_prec);
    if (m_pet) Release1DArray(m_pet);
    if (m_lakepcp) Release1DArray(m_lakepcp);
    if (m_lakeperc) Release1DArray(m_lakeperc);
    if (m_lakedp) Release1DArray(m_lakedp);
    if (m_lakedpini) Release1DArray(m_lakedpini);
    if (m_lakearea) Release1DArray(m_lakearea);
    if (m_rrtime) Release1DArray(m_rrtime);
    if (m_handLevels) Release1DArray(m_handLevels);
    if (m_handWtrDep) Release1DArray(m_handWtrDep);
    if (m_isHandFlooded) Release1DArray(m_isHandFlooded);
    if (m_subbasinWtrDep) Release1DArray(m_subbasinWtrDep);
    if (m_subbasinInundationArea) Release1DArray(m_subbasinInundationArea);
    if (m_subbasinArea) Release1DArray(m_subbasinArea);
    if (m_T_LKWB) Release2DArray(m_nreach + 1, m_T_LKWB);
}

// Set scalar parameters passed by the model framework.
void DWNEW_CH_HAND::SetValue(const char* key, float value) {
    string sk(key);
    if (StringMatch(sk, Tag_ChannelTimeStep)) m_dt = CVT_INT(value);
    else if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);
    else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
    else if (StringMatch(sk, VAR_OUTLETID)) m_outletID = CVT_INT(value);
    else if (StringMatch(sk, VAR_EP_CH)) m_Epch = value;
    else if (StringMatch(sk, VAR_BNK0)) m_Bnk0 = value;
    else if (StringMatch(sk, VAR_CHS0_PERC)) m_Chs0_perc = value;
    else if (StringMatch(sk, VAR_A_BNK)) m_aBank = value;
    else if (StringMatch(sk, VAR_B_BNK)) m_bBank = value;
    else if (StringMatch(sk, VAR_GWMAX)) m_GWMAX = value;
    else if (StringMatch(sk, VAR_KG)) m_Kg = value;
    else if (StringMatch(sk, VAR_GWMIN)) m_GWMIN = value;
    else if (StringMatch(sk, VAR_Base_ex)) m_Base_ex = value;
    else if (StringMatch(sk, VAR_LAKE_EVP)) m_evlake = value;
    else if (StringMatch(sk, VAR_LAKE_SEEP)) m_lakeseep = value;
    else if (StringMatch(sk, VAR_K_PET)) m_petFactor = value;
    else if (StringMatch(sk, VAR_LAKE_MNVOL)) m_minvol = value;
    else if (StringMatch(sk, "LAKEB")) m_lakeb = value;
    else if (StringMatch(sk, DW_KEY_OUTLET_BC_TYPE)) m_outletBcType = CVT_INT(value);
    else if (StringMatch(sk, DW_KEY_OUTLET_BC_STAGE)) m_outletBcStage = value;
    else if (StringMatch(sk, DW_KEY_OUTLET_BC_SLOPE)) m_outletBcSlope = std::max(value, 0.f);
    else if (StringMatch(sk, DW_KEY_OUTLET_BC_ALLOW_BACKFLOW)) m_outletBcAllowBackflow = value;
    else throw ModelException(MID_DWNEW_CH_HAND, "SetValue", "Parameter " + sk + " does not exist.");
}

// Override one reach output when the model runs a selected subbasin.
void DWNEW_CH_HAND::SetValueByIndex(const char* key, int index, float value) {
    if (m_inputSubbsnID == 0) return;
    if (index <= 0 || index > m_nreach) return;
    if (nullptr == m_qRchOut) InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_QRECH)) m_qRchOut[index] = value;
    else if (StringMatch(sk, VAR_QS)) m_qsRchOut[index] = value;
    else if (StringMatch(sk, VAR_QI)) m_qiRchOut[index] = value;
    else if (StringMatch(sk, VAR_QG)) m_qgRchOut[index] = value;
    else throw ModelException(MID_DWNEW_CH_HAND, "SetValueByIndex", "Parameter " + sk + " does not exist.");
}

// Attach 1-D input arrays from upstream modules, reach data, climate, and HAND rasters.
void DWNEW_CH_HAND::Set1DData(const char* key, int n, float* data) {
    string sk(key);
    if (StringMatch(sk, VAR_SUBBSN)) m_subbsnID = data;
    else if (StringMatch(sk, VAR_SBPET)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n - 1, m_nreach); m_petSubbsn = data; }
    else if (StringMatch(sk, VAR_SBGS)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n - 1, m_nreach); m_gwSto = data; }
    else if (StringMatch(sk, VAR_SBOF)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n - 1, m_nreach); m_olQ2Rch = data; }
    else if (StringMatch(sk, VAR_SBIF)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n - 1, m_nreach); m_ifluQ2Rch = data; }
    else if (StringMatch(sk, VAR_SBQG)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n - 1, m_nreach); m_gndQ2Rch = data; }
    else if (StringMatch(sk, VAR_PCP)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_netPcp = data; }
    else if (StringMatch(sk, VAR_PET)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_PET = data; }
    else if (StringMatch(sk, VAR_OL_HAND_INFIL)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_Infil = data; }
    else if (StringMatch(sk, VAR_HAND_EVAP)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_handEvap = data; }
    else if (StringMatch(sk, VAR_HAND_DEP)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_handDep = data; }
    else if (StringMatch(sk, VAR_OL_HAND_BACK_FROM_GW)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_BackFromGW = data; }
    else if (StringMatch(sk, VAR_AHRU)) {
        CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells);
        m_area = data;
        m_handArea = data;
    }
    else if (StringMatch(sk, "ep_ch_1d")) m_Epch_1d = data;
    else if (StringMatch(sk, VAR_K_PET_1D)) m_petFactor_1d = data;
    else if (StringMatch(sk, VAR_SUBBASIN_WTR_DEPTH)) m_subbasinWtrDep = data;
    else if (StringMatch(sk, VAR_SUBBASIN_FLOODED_AREA)) m_subbasinInundationArea = data;
    else if (StringMatch(sk, VAR_HAND_Subbasin)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_Subbasin = data; }
    else if (StringMatch(sk, VAR_HAND_Flood_Level)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_Flood_Level = data; }
    else if (StringMatch(sk, VAR_HAND_LevelDepth)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_LevelDepth = data; }
    else if (StringMatch(sk, VAR_HAND_SumArea)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_SumArea = data; }
    else if (StringMatch(sk, VAR_HAND_SumVolume)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_SumVolume = data; }
    else if (StringMatch(sk, VAR_HAND_AvgDepth)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_AvgDepth = data; }
    else if (StringMatch(sk, VAR_HAND_AccVolume)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_AccVolume = data; }
    else if (StringMatch(sk, VAR_HAND_LowerAccDepthFlat)) m_HAND_LowerAccDepthFlat = data;
    else if (StringMatch(sk, VAR_HAND_LowerAccDepthLen)) { CheckInputSize(MID_DWNEW_CH_HAND, key, n, m_nCells); m_HAND_LowerAccDepthLen = data; }
    else if (StringMatch(sk, VAR_DEM) || StringMatch(sk, VAR_RUNOFF_CO) || StringMatch(sk, VAR_SLOPE)) return;
    else throw ModelException(MID_DWNEW_CH_HAND, "Set1DData", "Parameter " + sk + " does not exist.");
}

// Keep the 2-D input hook for framework compatibility.
void DWNEW_CH_HAND::Set2DData(const char* key, int n, int col, float** data) {
    string sk(key);
    if (StringMatch(sk, VAR_SOILT)) return;
    throw ModelException(MID_DWNEW_CH_HAND, "Set2DData", "Parameter " + sk + " does not exist.");
}

// Load reach geometry, routing topology, lake/reservoir flags, and control data.
void DWNEW_CH_HAND::SetReaches(clsReaches* rches) {
    if (nullptr == rches) throw ModelException(MID_DWNEW_CH_HAND, "SetReaches", "The reaches input can not to be NULL.");
    m_nreach = rches->GetReachNumber();
    if (nullptr == m_chWth) rches->GetReachesSingleProperty(REACH_WIDTH, &m_chWth);
    if (nullptr == m_ChDepth) rches->GetReachesSingleProperty(REACH_DEPTH, &m_ChDepth);
    if (nullptr == m_chLen) rches->GetReachesSingleProperty(REACH_LENGTH, &m_chLen);
    if (nullptr == m_chArea) rches->GetReachesSingleProperty(REACH_AREA, &m_chArea);
    if (nullptr == m_chSideSlope) rches->GetReachesSingleProperty(REACH_SIDESLP, &m_chSideSlope);
    if (nullptr == m_chMan) rches->GetReachesSingleProperty(REACH_MANNING, &m_chMan);
    if (nullptr == m_Kbank) rches->GetReachesSingleProperty(REACH_BNKK, &m_Kbank);
    if (nullptr == m_Kchb) rches->GetReachesSingleProperty(REACH_BEDK, &m_Kchb);
    if (nullptr == m_reachDownStream) rches->GetReachesSingleProperty(REACH_DOWNSTREAM, &m_reachDownStream);
    if (nullptr == m_chBedElev) rches->GetReachesSingleProperty(REACH_CH_BED_ELEV, &m_chBedElev);
    if (nullptr == m_outletHandId) rches->GetReachesSingleProperty(REACH_CH_OUTLET_HANDID, &m_outletHandId);
    if (nullptr == m_islake) rches->GetReachesSingleProperty(REACH_ISLAKE, &m_islake);
    if (nullptr == m_lakevol) rches->GetReachesSingleProperty(REACH_LAKEVOL, &m_lakevol);
    if (nullptr == m_isres) rches->GetReachesSingleProperty(REACH_ISRES, &m_isres);
    if (nullptr == m_resminq) rches->GetReachesSingleProperty("RES_minq", &m_resminq);
    if (nullptr == m_resnormq) rches->GetReachesSingleProperty("RES_normq", &m_resnormq);
    if (nullptr == m_resndq) rches->GetReachesSingleProperty("RES_ndq", &m_resndq);
    if (nullptr == m_res_normMult) rches->GetReachesSingleProperty("RES_normMult", &m_res_normMult);
    if (nullptr == m_ResLc) rches->GetReachesSingleProperty(REACH_RES_LC, &m_ResLc);
    if (nullptr == m_ResLn) rches->GetReachesSingleProperty(REACH_RES_LN, &m_ResLn);
    if (nullptr == m_ResLf) rches->GetReachesSingleProperty(REACH_RES_LF, &m_ResLf);
    if (nullptr == m_ResAdjust) rches->GetReachesSingleProperty(REACH_RES_ADJUST, &m_ResAdjust);
    if (nullptr == m_lakeb_1d) rches->GetReachesSingleProperty(REACH_LAKEB_1D, &m_lakeb_1d);
    if (nullptr == m_minvol_1d) rches->GetReachesSingleProperty(VAR_LAKE_MNVOL_1D, &m_minvol_1d);
    if (nullptr == m_lakeHandLevelini) rches->GetReachesSingleProperty(REACH_LAKE_HAND_LEVEL_INI, &m_lakeHandLevelini);
    m_reachUpStream = rches->GetUpStreamIDs();
    m_rteLyrs = rches->GetReachLayers();
}

// Store subbasin metadata for lake precipitation and PET aggregation.
void DWNEW_CH_HAND::SetSubbasins(clsSubbasins* subbsns) {
    if (nullptr == m_subbasinsInfo) {
        m_subbasinsInfo = subbsns;
        if (nullptr != m_subbasinsInfo) {
            m_subbasinIDs = m_subbasinsInfo->GetSubbasinIDs();
        }
    }
}

// Load point-source BMP factories from the current scenario.
void DWNEW_CH_HAND::SetScenario(Scenario* sce) {
    if (nullptr == sce) {
        throw ModelException(MID_DWNEW_CH_HAND, "SetScenario", "The scenario can not to be NULL.");
    }
    map<int, BMPFactory*>& tmpBMPFactories = sce->GetBMPFactories();
    for (auto it = tmpBMPFactories.begin(); it != tmpBMPFactories.end(); ++it) {
        if (it->first / 100000 == BMP_TYPE_POINTSOURCE) {
#ifdef HAS_VARIADIC_TEMPLATES
            m_ptSrcFactory.emplace(it->first, static_cast<BMPPointSrcFactory*>(it->second));
#else
            m_ptSrcFactory.insert(make_pair(it->first, static_cast<BMPPointSrcFactory*>(it->second)));
#endif
        }
    }
}

// Validate scalar settings, required arrays, and reach-to-HAND outlet ids.
bool DWNEW_CH_HAND::CheckInputData() {
    CHECK_POSITIVE(MID_DWNEW_CH_HAND, m_dt);
    CHECK_NONNEGATIVE(MID_DWNEW_CH_HAND, m_inputSubbsnID);
    CHECK_POSITIVE(MID_DWNEW_CH_HAND, m_nreach);
    CHECK_POSITIVE(MID_DWNEW_CH_HAND, m_outletID);
    CHECK_NODATA(MID_DWNEW_CH_HAND, m_Epch);
    CHECK_NODATA(MID_DWNEW_CH_HAND, m_Bnk0);
    CHECK_NODATA(MID_DWNEW_CH_HAND, m_Chs0_perc);
    CHECK_NODATA(MID_DWNEW_CH_HAND, m_aBank);
    CHECK_NODATA(MID_DWNEW_CH_HAND, m_bBank);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_subbsnID);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_petSubbsn);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_gwSto);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_olQ2Rch);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_ifluQ2Rch);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_gndQ2Rch);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_chBedElev);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_ChDepth);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_outletHandId);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_Subbasin);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_Flood_Level);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_LevelDepth);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_SumArea);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_SumVolume);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_AvgDepth);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_AccVolume);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_LowerAccDepthFlat);
    CHECK_POINTER(MID_DWNEW_CH_HAND, m_HAND_LowerAccDepthLen);

    int invalidBedCount = 0;
    int invalidHandCount = 0;
    std::ostringstream handMsg;
    int handMsgCount = 0;
    for (int i = 1; i <= m_nreach; ++i) {
        if (!IsFiniteF(m_chBedElev[i]) || m_chBedElev[i] <= -9998.f) ++invalidBedCount;
        const int outletHandId = static_cast<int>(std::lround(m_outletHandId[i]));
        if (outletHandId < 0 || outletHandId >= m_nCells) {
            ++invalidHandCount;
            if (handMsgCount < 10) {
                handMsg << " reach=" << i
                    << " handId=" << m_outletHandId[i]
                    << " rounded=" << outletHandId
                    << " nCells=" << m_nCells
                    << " bed=" << m_chBedElev[i];
                ++handMsgCount;
            }
        }
    }
    if (invalidBedCount > 0) {
        throw ModelException(MID_DWNEW_CH_HAND, "CheckInputData", "CH_BED_ELEV data is missing or invalid.");
    }
    if (invalidHandCount > 0) {
        throw ModelException(MID_DWNEW_CH_HAND, "CheckInputData",
            "CH_OUTLET_HANDID data is invalid for " + ValueToString(invalidHandCount)
            + " reaches." + handMsg.str());
    }
    return true;
}

// Allocate outputs and initialize channel, lake, HAND, and debug states.
void DWNEW_CH_HAND::InitialOutputs() {
    CHECK_POSITIVE(MID_DWNEW_CH_HAND, m_nreach);
    CHECK_POSITIVE(MID_DWNEW_CH_HAND, m_nCells);
    if (nullptr != m_qRchOut) return;

    const int sz = m_nreach + 1;
    if (nullptr == m_handLevels) Initialize1DArray(sz, m_handLevels, 0.f);
    if (nullptr == m_handWtrDep) Initialize1DArray(m_nCells, m_handWtrDep, 0.f);
    if (nullptr == m_isHandFlooded) Initialize1DArray(m_nCells, m_isHandFlooded, 0.f);
    if (nullptr == m_subbasinArea) Initialize1DArray(sz, m_subbasinArea, 0.f);
    if (nullptr == m_subbasinInundationArea) Initialize1DArray(sz, m_subbasinInundationArea, 0.f);
    if (nullptr == m_subbasinWtrDep) Initialize1DArray(sz, m_subbasinWtrDep, 0.f);

    m_sfcElv = new(nothrow) float[sz]();
    m_dwnElv = new(nothrow) float[sz]();
    m_rivOut = new(nothrow) float[sz]();
    m_rivVel = new(nothrow) float[sz]();
    m_ptSub = new(nothrow) float[sz]();
    m_qRchOut = new(nothrow) float[sz]();
    m_qsRchOut = new(nothrow) float[sz]();
    m_qiRchOut = new(nothrow) float[sz]();
    m_qgRchOut = new(nothrow) float[sz]();
    m_chSto = new(nothrow) float[sz]();
    m_chStoLastStep = new(nothrow) float[sz]();
    m_rteWtrIn = new(nothrow) float[sz]();
    m_rteWtrOut = new(nothrow) float[sz]();
    m_bankSto = new(nothrow) float[sz]();
    m_bankStoLastStep = new(nothrow) float[sz]();
    m_chWtrDepth = new(nothrow) float[sz]();
    m_chWtrWth = new(nothrow) float[sz]();
    m_chBtmWth = new(nothrow) float[sz]();
    m_chCrossArea = new(nothrow) float[sz]();
    m_seepage = new(nothrow) float[sz]();
    m_charge = new(nothrow) float[sz]();
    m_recharge = new(nothrow) float[sz]();
    m_Ch2GW = new(nothrow) float[sz]();
    m_aquifer = new(nothrow) float[sz]();
    m_prec = new(nothrow) float[sz]();
    m_pet = new(nothrow) float[sz]();
    m_lakepcp = new(nothrow) float[sz]();
    m_lakeperc = new(nothrow) float[sz]();
    m_lakedp = new(nothrow) float[sz]();
    m_lakedpini = new(nothrow) float[sz]();
    m_lakearea = new(nothrow) float[sz]();
    m_rrtime = new(nothrow) float[sz]();
    Initialize2DArray(sz, 7, m_T_LKWB, 0.f);

    int lowerFlatLen = 0;
    for (int i = 0; i < m_nCells; ++i) lowerFlatLen += static_cast<int>(m_HAND_LowerAccDepthLen[i]);
    LoadHandLevelsFromArrays(m_nCells, lowerFlatLen, m_Hands, NODATA_VALUE, TRUE);

    for (int i = 0; i <= m_nreach; ++i) {
        m_ptSub[i] = 0.f;
        m_prec[i] = 0.f;
        m_pet[i] = 0.f;
        m_lakepcp[i] = 0.f;
        m_lakeperc[i] = 0.f;
        m_lakedp[i] = 0.f;
        m_lakedpini[i] = 0.f;
        m_lakearea[i] = 0.f;
        m_rrtime[i] = 0.f;
        m_seepage[i] = 0.f;
        m_charge[i] = 0.f;
        m_recharge[i] = 0.f;
        m_Ch2GW[i] = 0.f;
        m_aquifer[i] = 0.f;
    }

    for (int i = 1; i <= m_nreach; ++i) {
        m_qRchOut[i] = std::max(m_olQ2Rch[i], 0.f);
        m_qsRchOut[i] = std::max(m_olQ2Rch[i], 0.f);
        m_qiRchOut[i] = (nullptr != m_ifluQ2Rch) ? std::max(m_ifluQ2Rch[i], 0.f) : 0.f;
        m_qgRchOut[i] = (nullptr != m_gndQ2Rch) ? std::max(m_gndQ2Rch[i], 0.f) : 0.f;
        m_qRchOut[i] += m_qiRchOut[i] + m_qgRchOut[i];
        m_bankSto[i] = std::max(m_Bnk0, 0.f) * std::max(m_chLen[i], 0.f);
        m_bankStoLastStep[i] = m_bankSto[i];
        m_chBtmWth[i] = ChannleBottomWidth(m_chWth[i], m_chSideSlope[i], m_ChDepth[i]);

        // Align DWNEW initial storage with MUSK_CH_HAND.
        // Do not initialize ordinary river storage from m_Chs0_perc * channel depth.
        // All reaches use the HAND initial level table.
        // If m_lakeHandLevelini[i] is zero or invalid, initial storage is zero.
        const int initLevel =
            (nullptr != m_lakeHandLevelini)
                ? static_cast<int>(m_lakeHandLevelini[i])
                : 0;

        const bool validLevel =
            (i > 0
                && i < static_cast<int>(m_Hands.size())
                && initLevel > 0
                && initLevel < static_cast<int>(m_Hands[i].levels.size()));

        float initSto = 0.f;

        if (validLevel) {
            initSto = static_cast<float>(m_Hands[i].levels[initLevel].m_levelAccVol);
        }

        m_chSto[i] = std::max(initSto, 0.f);
        m_chStoLastStep[i] = m_chSto[i];
        m_rteWtrIn[i] = 0.f;
        m_rteWtrOut[i] = 0.f;

        // Rebuild water depth, width, area, surface elevation, and HAND state from storage.
        RefreshReachHydraulicState(i);

        if (m_islake[i] == 1.f || m_isres[i] == 1.f) {
            m_lakedpini[i] = m_lakedp[i];
        }
    }

    InitDebugReachCluster(DW_DEBUG_ROOT_REACH, m_reachDownStream, m_nreach);
    WriteDebugReachTopology(m_reachUpStream, m_reachDownStream, m_islake, m_isres, m_nreach);
}

// Rebuild hydraulic state from current storage for one reach.
void DWNEW_CH_HAND::RefreshReachHydraulicState(int i) {
    if (i <= 0 || i > m_nreach) return;
    const bool isLakeLike = (m_islake[i] == 1.f || m_isres[i] == 1.f);
    const bool isConnector = IsLakeConnectorReach(i);
    const float channelStoCap = ComputeRiverChannelStorageCap(i);
    float inBankDepth = 0.f;
    // Recover the inundation state from the current storage after the channel prism
    // is stripped from river reaches. Below bankfull, only the outlet HAND cell keeps
    // a channel-only depth so inundation mapping excludes the channel volume.
    if (isLakeLike) {
        // Lakes and reservoirs use their full storage to recover HAND inundation.
        if (m_chSto[i] > UTIL_ZERO) HandInundation_BinarySearch(i, m_chSto[i]);
        else ClearHandStateForReach(i);
    }
    else if (isConnector) {
        // Connector reaches keep water in the channel prism and avoid floodplain pooling.
        ClearHandStateForReach(i);
        const float connectorSto = std::min(std::max(m_chSto[i], 0.f), channelStoCap);
        if (connectorSto > UTIL_ZERO) {
            inBankDepth = std::min(
                SolveTrapezoidDepthFromArea(connectorSto / std::max(m_chLen[i], 1.f),
                    m_chBtmWth[i], m_chSideSlope[i]),
                std::max(m_ChDepth[i], 0.f));
            SetRiverChannelOnlyOutletHandDepth(i, inBankDepth);
        }
    }
    else if (m_chSto[i] > channelStoCap + UTIL_ZERO) {
        // Only the storage above bankfull is mapped to HAND floodplain inundation.
        const float bandSto = m_chSto[i] - channelStoCap;
        HandInundation_BinarySearch(i, bandSto);
        // Channel water is at bankfull; overbank depth is added later through outletHandDepth.
        inBankDepth = std::max(m_ChDepth[i], 0.f);
    }
    else if (m_chSto[i] > UTIL_ZERO) {
        // In-bank river storage is converted to a channel-only outlet HAND depth.
        SetRiverChannelOnlyOutletHandDepth(i, ComputeRiverChannelOutletDepth(i, m_chSto[i]));
        const float inBankArea = m_chSto[i] / std::max(m_chLen[i], 1.f);
        inBankDepth = std::min(
            SolveTrapezoidDepthFromArea(inBankArea, m_chBtmWth[i], m_chSideSlope[i]),
            std::max(m_ChDepth[i], 0.f));
    }
    else {
        ClearHandStateForReach(i);
    }

    // Keep the basin-scale inundation diagnostics for reporting and dynamic lake area.
    if (isLakeLike) {
        m_lakearea[i] = std::max(m_subbasinInundationArea[i], 0.f) * 1.e6f;
        m_lakedp[i] = std::max(m_subbasinWtrDep[i], 0.f);
    }
    else {
        m_lakearea[i] = 0.f;
        m_lakedp[i] = 0.f;
    }
    m_handLevels[i] = (i < static_cast<int>(m_Hands.size()))
        ? static_cast<float>(m_Hands[i].m_CurInundationLevel) : 0.f;

    // Build the control-section hydraulic state used by the diffusive-wave flux.
    // The outlet HAND cell controls the exchange head after the section becomes flooded.
    // If it is still dry, a river reach can still store water inside the channel prism.
    const int handId = static_cast<int>(std::lround(m_outletHandId[i]));
    const bool validHand = (handId >= 0 && handId < m_nCells);
    const float outletHandDepth = (validHand && nullptr != m_handWtrDep)
        ? std::max(m_handWtrDep[handId], 0.f) : 0.f;
    const float ctrlGroundElev = m_chBedElev[i] + std::max(m_ChDepth[i], 0.f);
    const float bulkLakeDepth = std::max(m_subbasinWtrDep[i], 0.f);
    const float refLakeDepth = std::max(m_lakedpini[i], 0.f);
    const float lakeBedElevApprox = ctrlGroundElev - refLakeDepth;
    const float lakeStageFromBulkDepth = lakeBedElevApprox + bulkLakeDepth;

    float sfcElv = m_chBedElev[i];
    if (isConnector) {
        // Connector stage is controlled by its channel depth and neighboring lake stages.
        sfcElv = std::max(m_chBedElev[i] + inBankDepth, ComputeLakeConnectorStage(i));
    }
    else if (!isLakeLike && m_chSto[i] > channelStoCap + UTIL_ZERO) {
        // Overbank river depth equals bankfull depth plus outlet HAND water depth.
        sfcElv = ctrlGroundElev + outletHandDepth;
    }
    else if (!isLakeLike && m_chSto[i] > UTIL_ZERO) {
        sfcElv = m_chBedElev[i] + inBankDepth;
    }
    else if (isLakeLike) {
        if (outletHandDepth > UTIL_ZERO) {
            sfcElv = ctrlGroundElev + outletHandDepth;
        }
        else if (bulkLakeDepth > UTIL_ZERO && refLakeDepth > UTIL_ZERO) {
            // When the outlet HAND cell dries, keep a storage-consistent lake stage
            // instead of collapsing the routing head to the outlet-channel bed.
            sfcElv = lakeStageFromBulkDepth;
        }
    }

    m_sfcElv[i] = std::max(sfcElv, m_chBedElev[i]);
    m_chWtrDepth[i] = std::max(m_sfcElv[i] - m_chBedElev[i], 0.f);
    m_chWtrWth[i] = ComputeCompoundTopWidthFromDepth(m_chWtrDepth[i], m_ChDepth[i],
        m_chBtmWth[i], m_chSideSlope[i], m_chWth[i]);
    m_chCrossArea[i] = ComputeCompoundAreaFromDepth(m_chWtrDepth[i], m_ChDepth[i],
        m_chBtmWth[i], m_chSideSlope[i], m_chWth[i]);
}
/**
 * @brief Run one channel time step.
 * @return 0 when the routing step succeeds.
 *
 * The step validates inputs, initializes state, loads point sources, splits the
 * channel time step into substeps, routes reaches from upstream to downstream,
 * and then aggregates substep outputs back to the channel time step.
 */
int DWNEW_CH_HAND::Execute() {
    CheckInputData();
    InitialOutputs();
    PointSourceLoading();
    ResetDebugHandBudget(m_nreach);

    // Split the channel step into shorter hydraulic substeps for stability.
    const int subSteps = DW_SUBSTEPS;
    const float sub_dt = static_cast<float>(m_dt) / static_cast<float>(subSteps);
    m_subStepsPerDay = subSteps;

    // Match MUSK_CH_HAND by removing daily HAND net losses from channel storage
    // before substep routing begins. For ordinary rivers, remove overbank water
    // first and only then consume in-bank channel storage.
    if (nullptr != m_subbasinsInfo && nullptr != m_area) {
        for (auto id = m_subbasinIDs.begin(); id != m_subbasinIDs.end(); ++id) {
            const int reachId = *id;
            if (reachId <= 0 || reachId > m_nreach) continue;
            Subbasin* sub = m_subbasinsInfo->GetSubbasinByID(reachId);
            if (nullptr == sub) continue;
            const int curCellsNum = sub->GetCellCount();
            int* curCells = sub->GetCells();
            double handInfilM3 = 0.0;
            double handEvapM3 = 0.0;
            double handDepM3 = 0.0;
            double handBackFromGwM3 = 0.0;
            for (int ii = 0; ii < curCellsNum; ++ii) {
                const int cell = curCells[ii];
                if (cell < 0 || cell >= m_nCells) continue;
                const double area = std::max(static_cast<double>(m_area[cell]), 0.0);
                if (nullptr != m_HAND_Infil) handInfilM3 += static_cast<double>(m_HAND_Infil[cell]) * 0.001 * area;
                if (nullptr != m_handEvap) handEvapM3 += static_cast<double>(m_handEvap[cell]) * 0.001 * area;
                if (nullptr != m_handDep) handDepM3 += static_cast<double>(m_handDep[cell]) * 0.001 * area;
                if (nullptr != m_HAND_BackFromGW) handBackFromGwM3 += static_cast<double>(m_HAND_BackFromGW[cell]) * 0.001 * area;
            }
            const double handNetSubtractM3 = handInfilM3 + handEvapM3 + handDepM3 - handBackFromGwM3;
            g_dwDebugHandInfilM3[reachId] = handInfilM3;
            g_dwDebugHandEvapM3[reachId] = handEvapM3;
            g_dwDebugHandDepM3[reachId] = handDepM3;
            g_dwDebugHandBackFromGwM3[reachId] = handBackFromGwM3;
            g_dwDebugHandNetSubtractM3[reachId] = handNetSubtractM3;

            if (handNetSubtractM3 > 0.0) {
                float remainingSubtract = static_cast<float>(handNetSubtractM3);
                const bool isLakeLike = (m_islake[reachId] == 1.f || m_isres[reachId] == 1.f);
                const bool isConnector = IsLakeConnectorReach(reachId);
                if (!isLakeLike && !isConnector) {
                    const float channelStoCap = ComputeRiverChannelStorageCap(reachId);
                    const float overbankSto = std::max(m_chSto[reachId] - channelStoCap, 0.f);
                    const float removeOverbank = std::min(remainingSubtract, overbankSto);
                    m_chSto[reachId] -= removeOverbank;
                    remainingSubtract -= removeOverbank;
                }
                if (remainingSubtract > 0.f) {
                    const float removeInBank = std::min(remainingSubtract, m_chSto[reachId]);
                    m_chSto[reachId] -= removeInBank;
                }
                if (m_chSto[reachId] < 0.f) m_chSto[reachId] = 0.f;
            }
        }
    }

    // Convert cell precipitation and PET to reach-level depth rates for lakes.
    vector<float> pcpDepthRate(m_nreach + 1, 0.f);
    vector<float> petDepthRate(m_nreach + 1, 0.f);
    if (nullptr != m_subbasinsInfo && nullptr != m_area && nullptr != m_netPcp && nullptr != m_PET) {
        for (auto id = m_subbasinIDs.begin(); id != m_subbasinIDs.end(); ++id) {
            Subbasin* sub = m_subbasinsInfo->GetSubbasinByID(*id);
            if (nullptr == sub) continue;
            const int curCellsNum = sub->GetCellCount();
            int* curCells = sub->GetCells();
            float totalArea = 0.f;
            float pcpVolDay = 0.f;
            float petVolDay = 0.f;
            if (m_islake[*id] == 1.f || m_isres[*id] == 1.f) {
                // Aggregate cell depths as volumes, then convert back to basin mean depth.
                for (int ii = 0; ii < curCellsNum; ++ii) {
                    const int index = curCells[ii];
                    totalArea += m_area[index];
                    pcpVolDay += (m_netPcp[index] / 1000.f) * m_area[index];
                    petVolDay += (m_PET[index] / 1000.f) * m_area[index];
                }
                if (totalArea > 0.f) {
                    pcpDepthRate[*id] = (pcpVolDay / totalArea) / static_cast<float>(m_dt);
                    petDepthRate[*id] = (petVolDay / totalArea) / static_cast<float>(m_dt);
                }
            }
        }
    }

    // Accumulate substep flow, lake, and travel-time outputs for this channel step.
    vector<float> sumQRchOut(m_nreach + 1, 0.f);
    vector<float> sumQsRchOut(m_nreach + 1, 0.f);
    vector<float> sumQiRchOut(m_nreach + 1, 0.f);
    vector<float> sumQgRchOut(m_nreach + 1, 0.f);
    vector<float> sumRteWtrIn(m_nreach + 1, 0.f);
    vector<float> sumRteWtrOut(m_nreach + 1, 0.f);
    vector<float> sumLakePcp(m_nreach + 1, 0.f);
    vector<float> sumLakePerc(m_nreach + 1, 0.f);
    vector<float> sumRrtime(m_nreach + 1, 0.f);

    // Reset lake/reservoir water balance terms for the current channel step.
    for (int i = 0; i <= m_nreach; ++i) {
        for (int j = 0; j < 7; ++j) m_T_LKWB[i][j] = 0.f;
    }

    for (int step = 0; step < subSteps; ++step) {
        m_curSubStep = step + 1;

        // First recover HAND inundation and control-section geometry from storage.
        for (int i = 1; i <= m_nreach; ++i) {
            RefreshReachHydraulicState(i);
            if (m_islake[i] == 1.f || m_isres[i] == 1.f) {
                // Lake rain and PET depend on the updated dynamic water surface area.
                m_prec[i] = pcpDepthRate[i] * std::max(m_lakearea[i], 0.f);
                m_pet[i] = petDepthRate[i] * std::max(m_lakearea[i], 0.f);
            }
            else {
                m_prec[i] = 0.f;
                m_pet[i] = 0.f;
            }
        }

        // Route one hydraulic substep by reach layers, upstream before downstream.
        for (auto it = m_rteLyrs.begin(); it != m_rteLyrs.end(); ++it) {
            const int reachNum = static_cast<int>(it->second.size());
            size_t errCount = 0;
            for (int idx = 0; idx < reachNum; ++idx) {
                const int reachIndex = it->second[idx];
                if (m_inputSubbsnID == 0 || m_inputSubbsnID == reachIndex) {
                    if (!ChannelFlow_DiffusiveWave(reachIndex, sub_dt)) ++errCount;
                }
            }
            if (errCount > 0) {
                throw ModelException(MID_DWNEW_CH_HAND, "Execute",
                    "Error in diffusive-wave routing substep.");
            }
        }

        // Store substep outputs for later time-step aggregation.
        for (int i = 1; i <= m_nreach; ++i) {
            sumQRchOut[i] += m_qRchOut[i];
            sumQsRchOut[i] += m_qsRchOut[i];
            sumQiRchOut[i] += m_qiRchOut[i];
            sumQgRchOut[i] += m_qgRchOut[i];
            sumRteWtrIn[i] += m_rteWtrIn[i];
            sumRteWtrOut[i] += m_rteWtrOut[i];
            sumLakePcp[i] += m_lakepcp[i];
            sumLakePerc[i] += m_lakeperc[i];
            sumRrtime[i] += m_rrtime[i];
        }
    }

    m_curSubStep = 0;
    // Average flow rates over substeps and keep routed volumes as time-step sums.
    for (int i = 1; i <= m_nreach; ++i) {
        m_qRchOut[i] = sumQRchOut[i] / static_cast<float>(subSteps);
        m_qsRchOut[i] = sumQsRchOut[i] / static_cast<float>(subSteps);
        m_qiRchOut[i] = sumQiRchOut[i] / static_cast<float>(subSteps);
        m_qgRchOut[i] = sumQgRchOut[i] / static_cast<float>(subSteps);
        m_rteWtrIn[i] = sumRteWtrIn[i];
        m_rteWtrOut[i] = sumRteWtrOut[i];
        m_lakepcp[i] = sumLakePcp[i];
        m_lakeperc[i] = sumLakePerc[i];
        m_rrtime[i] = sumRrtime[i] / static_cast<float>(subSteps);
    }
    return 0;
}
// This module advances with the channel-routing time step.
TimeStepType DWNEW_CH_HAND::GetTimeStepType() {
    return TIMESTEP_CHANNEL;
}

// Return a scalar output for the selected reach.
void DWNEW_CH_HAND::GetValue(const char* key, float* value) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_QRECH) && m_inputSubbsnID > 0) *value = m_qRchOut[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_QS) && m_inputSubbsnID > 0) *value = m_qsRchOut[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_QI) && m_inputSubbsnID > 0) *value = m_qiRchOut[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_QG) && m_inputSubbsnID > 0) *value = m_qgRchOut[m_inputSubbsnID];
    else throw ModelException(MID_DWNEW_CH_HAND, "GetValue", "Parameter " + sk + " does not exist.");
}

// Return reach-indexed 1-D outputs.
void DWNEW_CH_HAND::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    *n = m_nreach + 1;
    if (StringMatch(sk, VAR_QRECH)) { m_qRchOut[0] = m_qRchOut[m_outletID]; *data = m_qRchOut; }
    else if (StringMatch(sk, VAR_QS)) { m_qsRchOut[0] = m_qsRchOut[m_outletID]; *data = m_qsRchOut; }
    else if (StringMatch(sk, VAR_QI)) { m_qiRchOut[0] = m_qiRchOut[m_outletID]; *data = m_qiRchOut; }
    else if (StringMatch(sk, VAR_QG)) { m_qgRchOut[0] = m_qgRchOut[m_outletID]; *data = m_qgRchOut; }
    else if (StringMatch(sk, VAR_CHST)) { m_chSto[0] = m_chSto[m_outletID]; *data = m_chSto; }
    else if (StringMatch(sk, VAR_CHST_LAST_STEP)) { m_chStoLastStep[0] = m_chStoLastStep[m_outletID]; *data = m_chStoLastStep; }
    else if (StringMatch(sk, VAR_RTE_WTRIN)) { m_rteWtrIn[0] = m_rteWtrIn[m_outletID]; *data = m_rteWtrIn; }
    else if (StringMatch(sk, VAR_RTE_WTROUT)) { m_rteWtrOut[0] = m_rteWtrOut[m_outletID]; *data = m_rteWtrOut; }
    else if (StringMatch(sk, VAR_BKST)) { m_bankSto[0] = m_bankSto[m_outletID]; *data = m_bankSto; }
    else if (StringMatch(sk, VAR_BKST_LAST_STEP)) { m_bankStoLastStep[0] = m_bankStoLastStep[m_outletID]; *data = m_bankStoLastStep; }
    else if (StringMatch(sk, VAR_CHWTRDEPTH)) { m_chWtrDepth[0] = m_chWtrDepth[m_outletID]; *data = m_chWtrDepth; }
    else if (StringMatch(sk, VAR_CHWTRWIDTH)) { m_chWtrWth[0] = m_chWtrWth[m_outletID]; *data = m_chWtrWth; }
    else if (StringMatch(sk, VAR_CHBTMWIDTH)) { m_chBtmWth[0] = m_chBtmWth[m_outletID]; *data = m_chBtmWth; }
    else if (StringMatch(sk, VAR_CHCROSSAREA)) { m_chCrossArea[0] = m_chCrossArea[m_outletID]; *data = m_chCrossArea; }
    else if (StringMatch(sk, VAR_qout)) { *data = m_qRchOut; }
    else if (StringMatch(sk, VAR_qsurf)) { *data = m_qsRchOut; }
    else if (StringMatch(sk, "LAKE_P")) { *data = m_lakepcp; }
    else if (StringMatch(sk, "LAKE_E")) { *data = m_lakeperc; }
    else if (StringMatch(sk, "rrtime")) { *data = m_rrtime; }
    else if (StringMatch(sk, "Qout")) { *data = m_qRchOut; }
    else throw ModelException(MID_DWNEW_CH_HAND, "Get1DData", "Output " + sk + " does not exist.");
}

// Return 2-D outputs, currently lake/reservoir water balance only.
void DWNEW_CH_HAND::Get2DData(const char* key, int* n, int* col, float*** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, "lake_wb")) {
        *data = m_T_LKWB;
        *n = m_nreach + 1;
        *col = 7;
    }
    else {
        throw ModelException(MID_DWNEW_CH_HAND, "Get2DData", "Parameter " + sk + " does not exist.");
    }
}

// Convert point-source management records to reach inflow rates.
void DWNEW_CH_HAND::PointSourceLoading() {
    for (auto it = m_ptSrcFactory.begin(); it != m_ptSrcFactory.end(); ++it) {
        for (int i = 0; i <= m_nreach; ++i) m_ptSub[i] = 0.f;
        vector<int>& ptSrcMgtSeqs = it->second->GetPointSrcMgtSeqs();
        map<int, PointSourceMgtParams*>& pointSrcMgtMap = it->second->GetPointSrcMgtMap();
        vector<int>& ptSrcIDs = it->second->GetPointSrcIDs();
        map<int, PointSourceLocations*>& pointSrcLocsMap = it->second->GetPointSrcLocsMap();
        for (auto seqIter = ptSrcMgtSeqs.begin(); seqIter != ptSrcMgtSeqs.end(); ++seqIter) {
            PointSourceMgtParams* curPtMgt = pointSrcMgtMap.at(*seqIter);
            if (curPtMgt->GetStartDate() != 0 && curPtMgt->GetEndDate() != 0) {
                if (m_date < curPtMgt->GetStartDate() || m_date > curPtMgt->GetEndDate()) continue;
            }
            const float perWtrVol = curPtMgt->GetWaterVolume();
            for (auto locIter = ptSrcIDs.begin(); locIter != ptSrcIDs.end(); ++locIter) {
                if (pointSrcLocsMap.find(*locIter) != pointSrcLocsMap.end()) {
                    PointSourceLocations* curPtLoc = pointSrcLocsMap.at(*locIter);
                    const int curSubID = curPtLoc->GetSubbasinID();
                    m_ptSub[curSubID] += perWtrVol * curPtLoc->GetSize() / 86400.f;
                }
            }
        }
    }
}

// Route one reach through one pure diffusive-wave substep.
bool DWNEW_CH_HAND::ChannelFlow_DiffusiveWave(int i, float sub_dt) {
    const float dt = sub_dt;
    const float decayScale = dt / static_cast<float>(m_dt);

    // Classify the reach so lake, reservoir, and connector rules can diverge.
    const bool isLake = (m_islake[i] == 1.f);
    const bool isRes = (m_isres[i] == 1.f);
    const bool isLakeLike = (isLake || isRes);
    const bool isConnector = IsLakeConnectorReach(i);
    const int connectorUpLake = isConnector ? GetLakeConnectorUpstreamLake(i) : 0;

    // Keep storage checkpoints for water balance and debug output.
    const float stoBegin = m_chSto[i];
    float stoAfterInflow = stoBegin;
    float reqBackVol = 0.f;
    float actBackVol = 0.f;


    // Start with local surface, interflow, groundwater, and point-source inflows.
    float qIn = std::max(m_olQ2Rch[i], 0.f);
    float qiSub = 0.f, qgSub = 0.f, ptSub = 0.f;
    if (nullptr != m_ifluQ2Rch && m_ifluQ2Rch[i] > 0.f) { qiSub = m_ifluQ2Rch[i]; qIn += qiSub; }
    if (nullptr != m_gndQ2Rch && m_gndQ2Rch[i] > 0.f) { qgSub = m_gndQ2Rch[i]; qIn += qgSub; }
    if (nullptr != m_ptSub && m_ptSub[i] > 0.f) { ptSub = m_ptSub[i]; qIn += ptSub; }

    // Add routed flow from all upstream reaches.
    float qsUp = 0.f, qiUp = 0.f, qgUp = 0.f;
    for (auto upRchID = m_reachUpStream.at(i).begin(); upRchID != m_reachUpStream.at(i).end(); ++upRchID) {
        const int up = *upRchID;
        if (!IsFiniteF(m_qsRchOut[up]) || !IsFiniteF(m_qiRchOut[up]) || !IsFiniteF(m_qgRchOut[up])) {
            cout << "NaN detected in upstream routing result, current reach = " << i
                << ", upstream reach = " << up << endl;
            return false;
        }
        if (m_qsRchOut[up] > 0.f) qsUp += m_qsRchOut[up];
        if (m_qiRchOut[up] > 0.f) qiUp += m_qiRchOut[up];
        if (m_qgRchOut[up] > 0.f) qgUp += m_qgRchOut[up];
    }
    qIn += qsUp + qiUp + qgUp;

    // Lake and reservoir precipitation is applied as a direct storage inflow.
    if (isLakeLike) {
        qIn += m_prec[i];
        m_lakepcp[i] = m_prec[i] * dt;
    }
    else {
        m_lakepcp[i] = 0.f;
    }

    // River bank storage releases water to the channel and groundwater.
    float bankOut = 0.f, bankOutGw = 0.f;
    if (!isLakeLike) {
        bankOut = m_bankSto[i] * (1.f - expf(-m_aBank * decayScale));
        m_bankSto[i] -= bankOut;
        qIn += bankOut / dt;
        bankOutGw = m_bankSto[i] * (1.f - expf(-m_bBank * decayScale));
        m_bankSto[i] -= bankOutGw;
        if (nullptr != m_gwSto && m_chArea[i] > 0.f) {
            m_gwSto[i] += bankOutGw / m_chArea[i] * 1000.f;
        }
    }

    m_chStoLastStep[i] = m_chSto[i];
    m_rteWtrIn[i] = qIn * dt;
    m_chSto[i] += qIn * dt;
    // Refresh the control-section state after local inflow has been added.
    RefreshReachHydraulicState(i);
    stoAfterInflow = m_chSto[i];

    // Define the topological downstream reach once.  A missing downstream
    // reach is handled as an explicit outlet boundary below.
    const int jseq = static_cast<int>(m_reachDownStream[i]);
    const bool hasDownstream = (jseq > 0 && jseq <= m_nreach);

    // Pure diffusive-wave flux.  This module deliberately does not use
    // any previous-step discharge or depth.  Backwater is represented only through
    // the current water-surface gradient between this reach and its downstream
    // reach (or an external ghost boundary at the outlet).
    bool usesOutletBoundary = false;
    float downstreamStage = hasDownstream ? m_sfcElv[jseq] : m_sfcElv[i];
    float rawSlope = 0.f;
    if (isConnector && connectorUpLake > 0 && hasDownstream) {
        // For a lake connector, the energy gradient is controlled by the two
        // adjacent lake/reservoir stages, not by transient connector storage.
        downstreamStage = m_sfcElv[jseq];
        rawSlope = (m_sfcElv[connectorUpLake] - downstreamStage) / std::max(m_chLen[i], 1.f);
    }
    else if (hasDownstream) {
        rawSlope = (m_sfcElv[i] - downstreamStage) / std::max(m_chLen[i], 1.f);
    }
    else if (i == m_outletID && m_outletBcType == 1) {
        // Free / normal-depth outlet: use a prescribed minimum external slope.
        // This is the recommended external-basin setting when no downstream
        // water level is known.
        usesOutletBoundary = true;
        downstreamStage = m_sfcElv[i] - std::max(m_outletBcSlope, 0.f) * std::max(m_chLen[i], 1.f);
        rawSlope = std::max(m_outletBcSlope, 0.f);
    }
    else if (i == m_outletID && m_outletBcType == 2) {
        // Fixed-stage ghost boundary, similar in spirit to CaMa-Flood river
        // mouth / downstream water-level boundary handling.  If the boundary
        // stage is higher than the outlet stage, backwater is produced.  Actual
        // inflow from the external boundary is only allowed when
        // DW_OUTLET_BC_ALLOW_BACKFLOW > 0.
        usesOutletBoundary = true;
        downstreamStage = m_outletBcStage;
        rawSlope = (m_sfcElv[i] - downstreamStage) / std::max(m_chLen[i], 1.f);
    }
    else {
        // Closed endorheic boundary, used by default for internal drainage
        // basins.  No water leaves the terminal outlet.
        rawSlope = 0.f;
    }
    if (std::fabs(rawSlope) * std::max(m_chLen[i], 1.f) < DW_STAGE_DIFF_EPS) {
        rawSlope = 0.f;
    }
    const float dslp = rawSlope;
    m_dwnElv[i] = downstreamStage;

    float hydraulicDepth = std::max(m_chWtrDepth[i], 0.f);
    if (isConnector && connectorUpLake > 0 && hasDownstream) {
        const float conveyanceStage = std::max(m_sfcElv[connectorUpLake], m_sfcElv[jseq]);
        hydraulicDepth = std::max(conveyanceStage - m_chBedElev[i], 0.f);
    }
    const float hydraulicArea = ComputeCompoundAreaFromDepth(hydraulicDepth, m_ChDepth[i],
        m_chBtmWth[i], m_chSideSlope[i], m_chWth[i]);
    const float effWidth = ComputeCompoundTopWidthFromDepth(hydraulicDepth, m_ChDepth[i],
        m_chBtmWth[i], m_chSideSlope[i], m_chWth[i]);
    const float nMain = isLakeLike ? std::max(m_chMan[i], DW_LAKE_MIN_MANNING)
        : std::max(m_chMan[i], 1.e-4f);

    float dout = ComputeDiffusiveManningDischarge(i, dslp, hydraulicDepth, isLakeLike);
    const float doutRaw = dout;

    // Limit velocity so the explicit storage update remains bounded.
    const float maxVel = isLakeLike ? 2.f : 10.f;
    const float maxDout = std::max(hydraulicArea, 0.f) * maxVel;
    dout = std::max(-maxDout, std::min(maxDout, dout));
    if ((!hasDownstream && !usesOutletBoundary) || hydraulicArea <= 1.e-8f) dout = 0.f;
    if (usesOutletBoundary && m_outletBcAllowBackflow <= 0.f && dout < 0.f) dout = 0.f;
    const float doutAfterLimit = dout;

    m_rivOut[i] = dout;
    m_rivVel[i] = (hydraulicArea > 1.e-8f) ? (dout / hydraulicArea) : 0.f;
    if (m_rivOut[i] < 0.f && hasDownstream) {
        // Reverse flow can only use water actually stored in the downstream reach.
        reqBackVol = -m_rivOut[i] * dt;
        actBackVol = std::min(reqBackVol, std::max(m_chSto[jseq], 0.f));
        m_chSto[jseq] -= actBackVol;

        if (reqBackVol > UTIL_ZERO && actBackVol > 0.f) m_rivOut[i] *= actBackVol / reqBackVol;
        else m_rivOut[i] = 0.f;

        if (isConnector && connectorUpLake > 0 && actBackVol > UTIL_ZERO) {
            // A lake-connector reach should not trap reverse flow as an independent
            // floodplain pool; send it back to the upstream lake storage directly.
            m_chSto[connectorUpLake] += actBackVol;
            m_rivOut[i] = 0.f;
            m_rivVel[i] = 0.f;
        }
    }
    float lakeEvap = 0.f;
    float lakeSeep = 0.f;
    if (isLakeLike) {
        // Lake / reservoir evaporation is removed directly from storage after inflow
        // and before routing outflow is finalized.
        lakeEvap = m_evlake * m_pet[i] * dt;
        lakeEvap = std::min(lakeEvap, m_chSto[i]);
        m_chSto[i] -= lakeEvap;
        m_lakeperc[i] = lakeEvap;

        // LAKE_SEEP is treated as mm/day and converted to a storage volume.
        if (m_lakeseep > 0.f && m_lakearea[i] > 0.f && m_chSto[i] > UTIL_ZERO) {
            lakeSeep = m_lakeseep * 0.001f * decayScale * m_lakearea[i];
            lakeSeep = std::min(lakeSeep, m_chSto[i]);
            lakeSeep = std::max(lakeSeep, 0.f);
            m_chSto[i] -= lakeSeep;

            if (nullptr != m_gwSto && m_lakearea[i] > 0.f) {
                m_gwSto[i] += lakeSeep / m_lakearea[i] * 1000.f;
            }
            m_Ch2GW[i] = lakeSeep;
        }
        else {
            m_Ch2GW[i] = 0.f;
        }
    }
    else {
        m_lakeperc[i] = 0.f;
    }

    // Reservoir outflow is capped by the scheduled operating rule.
    if (isRes && m_rivOut[i] > 0.f) {
        const float scheduledMaxQ = ComputeResScheduledOutflow(i, m_chSto[i], qIn, dt);
        m_rivOut[i] = std::min(m_rivOut[i], scheduledMaxQ);
        if (hasDownstream && m_rivOut[i] < m_resminq[i] && m_chSto[i] > m_resminq[i] * dt) {
            m_rivOut[i] = m_resminq[i];
        }
    }

    // Prevent positive outflow from removing more water than current storage.
    const float stoOut = std::max(m_rivOut[i], 0.f) * dt;
    if (stoOut > m_chSto[i] && m_chSto[i] > 0.f) {
        const float rate = m_chSto[i] / stoOut;
        m_rivOut[i] *= rate;
    }
    m_chSto[i] -= m_rivOut[i] * dt;
    if (m_chSto[i] < 0.f) m_chSto[i] = 0.f;
    float routedOut = std::max(m_rivOut[i], 0.f) * dt;
    float rttlc = 0.f;
    float riverEvapLoss = 0.f;
    float floodplainEvap = 0.f;
    float floodplainSeep = 0.f;
    // Keep storage-only losses active whenever the river still holds water,
    // even if no positive routed outflow occurs in this substep.
    if (!isLakeLike && (m_chSto[i] + routedOut > UTIL_ZERO)) {
        const float wettedPerimeter = ComputeCompoundWettedPerimeter(m_chWtrDepth[i], m_ChDepth[i],
            m_chBtmWth[i], m_chSideSlope[i], m_chWth[i]);
        // Transmission loss is split between remaining storage and outgoing routed volume.
        rttlc = 24.f * m_Kchb[i] * 0.001f * m_chLen[i] * wettedPerimeter * decayScale;
        float lossFromSto = (m_chSto[i] + routedOut > 0.f) ? rttlc * m_chSto[i] / (m_chSto[i] + routedOut) : 0.f;
        lossFromSto = std::min(lossFromSto, m_chSto[i]);
        m_chSto[i] -= lossFromSto;
        float lossFromOut = std::min(rttlc - lossFromSto, routedOut);
        routedOut -= lossFromOut;
        rttlc = lossFromSto + lossFromOut;
        const float rivEvapDepth = (nullptr != m_Epch_1d && nullptr != m_petSubbsn)
            ? m_Epch_1d[i] * m_petSubbsn[i] * 0.001f * decayScale : 0.f;
        // River evaporation is also shared by storage and outgoing water.
        const float rivEvapVol = rivEvapDepth * m_chLen[i] * std::max(m_chWtrWth[i], m_chWth[i]);
        float evapFromSto = (m_chSto[i] + routedOut > 0.f) ? rivEvapVol * m_chSto[i] / (m_chSto[i] + routedOut) : 0.f;
        evapFromSto = std::min(evapFromSto, m_chSto[i]);
        m_chSto[i] -= evapFromSto;
        float evapFromOut = std::min(rivEvapVol - evapFromSto, routedOut);
        routedOut -= evapFromOut;
        riverEvapLoss = evapFromSto + evapFromOut;

        if (wettedPerimeter > UTIL_ZERO) {
            // Bed losses recharge groundwater; bank-side losses return to bank storage.
            const float toGwFrac = std::min(std::max(m_chBtmWth[i] / wettedPerimeter, 0.f), 1.f);
            const float seepToGw = rttlc * toGwFrac;
            const float seepToBank = rttlc - seepToGw;
            m_bankSto[i] += seepToBank;
            if (nullptr != m_gwSto && m_chArea[i] > 0.f) m_gwSto[i] += seepToGw / m_chArea[i] * 1000.f;
            m_Ch2GW[i] = seepToGw;
        }
    }
    else {
        m_Ch2GW[i] = 0.f;
    }

    // Connector reaches cannot keep storage above their in-bank capacity.
    if (isConnector) {
        const float connectorCap = ComputeRiverChannelStorageCap(i);
        if (m_chSto[i] > connectorCap + UTIL_ZERO) {
            const float excessSto = m_chSto[i] - connectorCap;
            const bool dischargeToDownstream = hasDownstream && connectorUpLake > 0
                && m_sfcElv[connectorUpLake] >= m_sfcElv[jseq];
            if (dischargeToDownstream) {
                // Send excess forward when the upstream lake is not lower.
                routedOut += excessSto;
            }
            else if (connectorUpLake > 0) {
                // Otherwise return excess water to the upstream lake.
                m_chSto[connectorUpLake] += excessSto;
            }
            m_chSto[i] = connectorCap;
        }
    }

    if (!isLakeLike && !isConnector && m_chSto[i] > 0.f && m_chSto[i] < 10.f) {
        routedOut += m_chSto[i];
        m_chSto[i] = 0.f;
    }

    // m_seepage stores the total vertical loss from the current reach/lake in this substep.
    m_seepage[i] = isLakeLike ? lakeSeep : (rttlc + floodplainSeep);
    m_charge[i] = bankOut;
    m_recharge[i] = bankOutGw;

    // Refresh again after routing, seepage and evaporation so the saved state is consistent.
    RefreshReachHydraulicState(i);
    m_rteWtrOut[i] = routedOut;
    m_qRchOut[i] = routedOut / dt;
    m_rrtime[i] = m_chLen[i] / (3600.f * std::max(fabsf(m_rivVel[i]), 0.001f));

    // Split outgoing flow into surface, interflow, and groundwater source fractions.
    const float srcQs = std::max(m_olQ2Rch[i], 0.f) + ptSub + qsUp;
    const float srcQi = qiSub + qiUp;
    const float srcQg = qgSub + qgUp;
    const float srcTotal = srcQs + srcQi + srcQg;
    if (srcTotal > UTIL_ZERO && m_qRchOut[i] > 0.f) {
        m_qsRchOut[i] = m_qRchOut[i] * srcQs / srcTotal;
        m_qiRchOut[i] = m_qRchOut[i] * srcQi / srcTotal;
        m_qgRchOut[i] = m_qRchOut[i] * srcQg / srcTotal;
    }
    else {
        m_qsRchOut[i] = std::max(m_qRchOut[i], 0.f);
        m_qiRchOut[i] = 0.f;
        m_qgRchOut[i] = 0.f;
    }

    // Record lake/reservoir water balance terms for this channel time step.
    if (isLakeLike) {
        m_T_LKWB[i][0] += (qsUp + qiUp + qgUp) * dt;
        m_T_LKWB[i][1] += (std::max(m_olQ2Rch[i], 0.f) + qiSub + qgSub + ptSub) * dt;
        m_T_LKWB[i][2] += m_lakepcp[i];
        m_T_LKWB[i][3] += lakeEvap;
        m_T_LKWB[i][4] += lakeSeep;
        m_T_LKWB[i][5] += m_rteWtrOut[i];
        m_T_LKWB[i][6] = m_chSto[i];
    }

    if (IsDebugReach(i) && ShouldWriteDebugLog(m_month, m_dayOfYear, m_curSubStep, m_subStepsPerDay)) {
        static bool fileAnnounced = false;
        if (!fileAnnounced) {
            const std::string substepWindow = DW_DEBUG_LOG_ALL_SUBSTEPS
                ? ("all " + std::to_string(m_subStepsPerDay) + " substeps per day")
                : "last substep of each day";
            cout << "[DWNEW_CH_HAND] Focused pure diffusive-wave debug log for reach "
                << DW_DEBUG_ROOT_REACH << ": " << DW_DEBUG_ROOT_LABEL
                << " (" << substepWindow << ") -> " << DW_DEBUG_LOG_FILE << endl;
            cout << "[DWNEW_CH_HAND] Debug topology for reach " << DW_DEBUG_ROOT_REACH
                << " -> " << DW_DEBUG_TOPO_FILE << endl;
            fileAnnounced = true;
        }

        const int outletHandIdx = static_cast<int>(std::lround(m_outletHandId[i]));
        const bool validOutletHand = outletHandIdx >= 0 && outletHandIdx < m_nCells && nullptr != m_handWtrDep;
        const float outletHandDepthForDebug = validOutletHand ? std::max(m_handWtrDep[outletHandIdx], 0.f) : 0.f;
        const int isOutletHandWet = (outletHandDepthForDebug > UTIL_ZERO) ? 1 : 0;
        const float connectorStageForDebug = isConnector ? ComputeLakeConnectorStage(i) : m_sfcElv[i];
        std::ostringstream row;
        row << std::fixed << std::setprecision(6)
            << m_date << ','
            << m_year << ','
            << m_month << ','
            << m_day << ','
            << m_dayOfYear << ','
            << m_curSubStep << ','
            << m_subStepsPerDay << ','
            << dt << ','
            << DebugReachRoot(i) << ','
            << i << ','
            << DebugReachDistance(i) << ','
            << (IsDebugFocusReach(i) ? 1 : 0) << ','
            << (isLake ? 1 : 0) << ','
            << (isRes ? 1 : 0) << ','
            << (isConnector ? 1 : 0) << ','
            << connectorUpLake << ','
            << jseq << ','
            << (hasDownstream ? 1 : 0) << ','
            << m_outletBcType << ','
            << m_outletBcStage << ','
            << m_outletBcSlope << ','
            << m_outletBcAllowBackflow << ','
            << stoBegin << ','
            << g_dwDebugHandInfilM3[i] << ','
            << g_dwDebugHandEvapM3[i] << ','
            << g_dwDebugHandDepM3[i] << ','
            << g_dwDebugHandBackFromGwM3[i] << ','
            << g_dwDebugHandNetSubtractM3[i] << ','
            << stoAfterInflow << ','
            << m_chSto[i] << ','
            << (m_chSto[i] - stoBegin) << ','
            << qIn << ','
            << std::max(m_olQ2Rch[i], 0.f) << ','
            << qiSub << ','
            << qgSub << ','
            << ptSub << ','
            << qsUp << ','
            << qiUp << ','
            << qgUp << ','
            << m_prec[i] << ','
            << m_chLen[i] << ','
            << hydraulicDepth << ','
            << hydraulicArea << ','
            << effWidth << ','
            << nMain << ','
            << dslp << ','
            << doutRaw << ','
            << maxVel << ','
            << maxDout << ','
            << doutAfterLimit << ','
            << reqBackVol << ','
            << actBackVol << ','
            << lakeEvap << ','
            << lakeSeep << ','
            << rttlc << ','
            << riverEvapLoss << ','
            << bankOut << ','
            << bankOutGw << ','
            << m_chBedElev[i] << ','
            << m_sfcElv[i] << ','
            << m_dwnElv[i] << ','
            << outletHandDepthForDebug << ','
            << isOutletHandWet << ','
            << connectorStageForDebug << ','
            << m_chWtrDepth[i] << ','
            << m_chWtrWth[i] << ','
            << m_chCrossArea[i] << ','
            << m_rivVel[i] << ','
            << m_subbasinWtrDep[i] << ','
            << m_handLevels[i] << ','
            << m_subbasinInundationArea[i] << ','
            << m_lakearea[i] << ','
            << m_rivOut[i] << ','
            << m_qRchOut[i] << ','
            << m_rteWtrIn[i] << ','
            << m_rteWtrOut[i] << ','
            << (m_rteWtrIn[i] - m_rteWtrOut[i]) << ','
            << m_rrtime[i] << ','
            << static_cast<int>(std::lround(m_outletHandId[i]))
            << '\n';
        AppendDebugLogRow(row.str());
    }

    return true;
}

// Return true when a reach is modeled as a lake or reservoir.
bool DWNEW_CH_HAND::IsLakeLikeReach(int reachId) const {
    return reachId > 0 && reachId <= m_nreach
        && (m_islake[reachId] == 1.f || m_isres[reachId] == 1.f);
}

// Find the single upstream lake/reservoir feeding a connector reach.
int DWNEW_CH_HAND::GetLakeConnectorUpstreamLake(int reachId) const {
    if (reachId <= 0 || reachId > m_nreach) return 0;
    if (reachId >= static_cast<int>(m_reachUpStream.size())) return 0;

    int lakeUp = 0;
    for (auto upIter = m_reachUpStream.at(reachId).begin(); upIter != m_reachUpStream.at(reachId).end(); ++upIter) {
        const int upId = *upIter;
        if (!IsLakeLikeReach(upId)) continue;
        if (lakeUp != 0) return 0;
        lakeUp = upId;
    }
    return lakeUp;
}

// A connector is a non-lake reach between one upstream lake and a downstream lake.
bool DWNEW_CH_HAND::IsLakeConnectorReach(int reachId) const {
    if (reachId <= 0 || reachId > m_nreach) return false;
    if (IsLakeLikeReach(reachId)) return false;

    const int downId = static_cast<int>(m_reachDownStream[reachId]);
    if (!IsLakeLikeReach(downId)) return false;
    return GetLakeConnectorUpstreamLake(reachId) > 0;
}

// Estimate connector stage from adjacent lake/reservoir stages and the channel bed.
float DWNEW_CH_HAND::ComputeLakeConnectorStage(int reachId) const {
    if (reachId <= 0 || reachId > m_nreach) return 0.f;

    const int upLake = GetLakeConnectorUpstreamLake(reachId);
    const int downId = static_cast<int>(m_reachDownStream[reachId]);
    const float bedElev = m_chBedElev[reachId];
    if (upLake <= 0 || downId <= 0 || downId > m_nreach || nullptr == m_sfcElv) return bedElev;

    const float upStage = std::max(m_sfcElv[upLake], bedElev);
    const float downStage = std::max(m_sfcElv[downId], bedElev);
    return std::max(std::min(upStage, downStage), bedElev);
}

// Compute signed discharge from the current water-surface slope using a
// non-inertial Manning diffusive-wave relation. Positive discharge follows the
// reach downstream direction; negative discharge represents backwater/reverse
// flow from the downstream reach or outlet ghost boundary.
float DWNEW_CH_HAND::ComputeDiffusiveManningDischarge(int reachId, float waterSurfaceSlope,
    float hydraulicDepth, bool isLakeLike) const {
    if (reachId <= 0 || reachId > m_nreach) return 0.f;
    if (std::fabs(waterSurfaceSlope) <= 1.e-12f) return 0.f;
    if (std::fabs(waterSurfaceSlope) * std::max(m_chLen[reachId], 1.f) < DW_STAGE_DIFF_EPS) return 0.f;
    if (hydraulicDepth <= DW_DRY_DEPTH) return 0.f;

    const float area = ComputeCompoundAreaFromDepth(hydraulicDepth, m_ChDepth[reachId],
        m_chBtmWth[reachId], m_chSideSlope[reachId], m_chWth[reachId]);
    const float perimeter = ComputeCompoundWettedPerimeter(hydraulicDepth, m_ChDepth[reachId],
        m_chBtmWth[reachId], m_chSideSlope[reachId], m_chWth[reachId]);
    if (area <= 1.e-8f || perimeter <= UTIL_ZERO) return 0.f;

    const float radius = area / perimeter;
    const float n = isLakeLike ? std::max(m_chMan[reachId], DW_LAKE_MIN_MANNING)
        : std::max(m_chMan[reachId], 1.e-4f);
    const float qAbs = (1.f / n) * area * powf(radius, 2.f / 3.f)
        * sqrtf(std::fabs(waterSurfaceSlope));
    return (waterSurfaceSlope >= 0.f) ? qAbs : -qAbs;
}

// Estimate in-bank channel storage capacity for a reach.
float DWNEW_CH_HAND::ComputeRiverChannelStorageCap(int reachId) const {
    if (reachId <= 0 || reachId > m_nreach) return 0.f;
    const float bankfullDepth = std::max(m_ChDepth[reachId], 0.f);
    const float bankfullArea = std::max(m_chBtmWth[reachId], 0.f) * bankfullDepth
        + std::max(m_chSideSlope[reachId], 0.f) * bankfullDepth * bankfullDepth;
    return std::max(m_chLen[reachId], 1.f) * std::max(bankfullArea, 0.f);
}

// Convert channel storage to an outlet depth without triggering floodplain mapping.
float DWNEW_CH_HAND::ComputeRiverChannelOutletDepth(int reachId, float sto) const {
    if (reachId <= 0 || reachId > m_nreach || sto <= 0.f) return 0.f;
    const float maxDepth = std::max(m_ChDepth[reachId], 0.f);
    const float inBankArea = sto / std::max(m_chLen[reachId], 1.f);
    return std::min(
        SolveTrapezoidDepthFromArea(inBankArea, m_chBtmWth[reachId], m_chSideSlope[reachId]),
        maxDepth);
}

// Set water only on the outlet HAND cell to represent in-bank channel water.
void DWNEW_CH_HAND::SetRiverChannelOnlyOutletHandDepth(int reachId, float depth) {
    ClearHandStateForReach(reachId);
    if (depth <= 0.f) return;
    if (nullptr != m_subbasinWtrDep) m_subbasinWtrDep[reachId] = depth;

    if (nullptr == m_outletHandId || nullptr == m_handWtrDep) return;
    const int handId = static_cast<int>(std::lround(m_outletHandId[reachId]));
    if (handId >= 0 && handId < m_nCells) {
        m_handWtrDep[handId] = depth;
    }
}

// Find the HAND inundation level whose accumulated volume contains the storage.
bool DWNEW_CH_HAND::HandInundation_BinarySearch(int reachId, float sto) {
    if (sto <= 0.f) return false;
    if (reachId <= 0 || reachId >= static_cast<int>(m_Hands.size())) return false;

    Hand& hand = m_Hands[reachId];
    const int n = hand.n_levels;
    if (n <= 0 || static_cast<int>(hand.levels.size()) <= n) return false;

    vector<Level>& levels = hand.levels;
    int left = 1, right = n, targetLevel = n;
    while (left <= right) {
        const int mid = (left + right) / 2;
        if (sto <= levels[mid].m_levelAccVol) {
            targetLevel = mid;
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }

    hand.m_CurInundationLevel = targetLevel;
    hand.excessWtrVol = 0.f;
    float remaining = sto;
    if (targetLevel > 1) remaining = static_cast<float>(sto - levels[targetLevel - 1].m_levelAccVol);
    if (remaining < 0.f) remaining = 0.f;

    const float partialDepth = (levels[targetLevel].m_levelSumArea > 0.0)
        ? static_cast<float>(remaining / levels[targetLevel].m_levelSumArea) : 0.f;
    for (int i = 1; i <= targetLevel; ++i) {
        if (nullptr != levels[i].m_levelLowerAccDepth) levels[i].m_levelWtrDep = levels[i].m_levelLowerAccDepth[targetLevel] + partialDepth;
        else levels[i].m_levelWtrDep = partialDepth;
    }
    for (int i = targetLevel + 1; i <= n; ++i) levels[i].m_levelWtrDep = 0.f;

    const float maxVolume = static_cast<float>(levels[n].m_levelAccVol);
    if (sto > maxVolume) hand.excessWtrVol = sto - maxVolume;
    updateAllHandsWtrDep(reachId);
    return true;
}

// Push level water depths down to HAND cells and update basin diagnostics.
void DWNEW_CH_HAND::updateAllHandsWtrDep(int reachId) {
    float inundationArea = 0.f, subbasinArea = 0.f;
    if (reachId <= 0 || reachId >= static_cast<int>(m_Hands.size())) return;
    for (int ll = 1; ll <= m_Hands[reachId].n_levels; ++ll) {
        for (int idx = 0; idx < m_Hands[reachId].levels[ll].m_levelHandNum; ++idx) {
            const int handId = m_Hands[reachId].levels[ll].handIds[idx];
            m_handWtrDep[handId] = m_Hands[reachId].levels[ll].m_levelWtrDep;
            if (m_handWtrDep[handId] > FLOOD_DEPTH_THRESH) {
                m_isHandFlooded[handId] = 1.f;
                inundationArea += m_handArea[handId];
            }
            else {
                m_isHandFlooded[handId] = 0.f;
            }
            subbasinArea += m_handArea[handId];
        }
    }
    m_subbasinInundationArea[reachId] = inundationArea * 1.e-6f;
    m_subbasinArea[reachId] = subbasinArea * 1.e-6f;
    m_subbasinWtrDep[reachId] = (m_Hands[reachId].n_levels >= 1) ? m_Hands[reachId].levels[1].m_levelWtrDep : 0.f;
}

// Clear HAND water depths, flooded flags, and summary diagnostics for one reach.
void DWNEW_CH_HAND::ClearHandStateForReach(int reachId) {
    if (reachId <= 0) return;
    if (nullptr != m_subbasinInundationArea) m_subbasinInundationArea[reachId] = 0.f;
    if (nullptr != m_subbasinWtrDep) m_subbasinWtrDep[reachId] = 0.f;
    if (nullptr != m_subbasinArea) m_subbasinArea[reachId] = 0.f;
    if (nullptr != m_handLevels) m_handLevels[reachId] = 0.f;

    if (reachId >= static_cast<int>(m_Hands.size())) return;
    Hand& hand = m_Hands[reachId];
    hand.m_CurInundationLevel = 0;
    hand.excessWtrVol = 0.f;
    for (int ll = 1; ll <= hand.n_levels && ll < static_cast<int>(hand.levels.size()); ++ll) {
        Level& level = hand.levels[ll];
        level.m_levelWtrDep = 0.f;
        for (int idx = 0; idx < level.m_levelHandNum; ++idx) {
            const int handId = level.handIds[idx];
            if (nullptr != m_handWtrDep) m_handWtrDep[handId] = 0.f;
            if (nullptr != m_isHandFlooded) m_isHandFlooded[handId] = 0.f;
        }
    }
}

// Rebuild per-reach HAND levels from flat raster arrays.
void DWNEW_CH_HAND::LoadHandLevelsFromArrays(int cellsNum, int flatLen,
    std::vector<Hand>& hands, float nodata, bool buildHandIds) {
    if (!m_HAND_Subbasin || !m_HAND_Flood_Level || !m_HAND_LevelDepth
        || !m_HAND_SumArea || !m_HAND_SumVolume || !m_HAND_AvgDepth
        || !m_HAND_AccVolume || !m_HAND_LowerAccDepthFlat || !m_HAND_LowerAccDepthLen) {
        std::cerr << "[ERROR] HAND arrays not loaded (one or more pointers are null)." << std::endl;
        return;
    }
    if (cellsNum <= 0) {
        std::cerr << "[ERROR] cellsNum <= 0" << std::endl;
        return;
    }

    int maxSbid = -1;
    std::map<int, std::set<int>> subbasinLevels;
    for (int i = 0; i < cellsNum; ++i) {
        const float sbv = m_HAND_Subbasin[i];
        const float levv = m_HAND_Flood_Level[i];
        if (IsNoData(sbv, nodata) || IsNoData(levv, nodata)) continue;
        const int sbid = static_cast<int>(sbv);
        const int lev = static_cast<int>(levv);
        if (sbid < 0 || lev < 0) continue;
        maxSbid = MAX(maxSbid, sbid);
        subbasinLevels[sbid].insert(lev);
    }

    if (maxSbid < 0) {
        std::cerr << "[WARN] No valid HAND records found in arrays." << std::endl;
        return;
    }
    if (static_cast<int>(hands.size()) <= maxSbid) hands.resize(maxSbid + 1);

    for (const auto& kv : subbasinLevels) {
        const int sbid = kv.first;
        const auto& levSet = kv.second;
        hands[sbid].n_levels = static_cast<int>(levSet.size());
        const int maxLevInSb = levSet.empty() ? -1 : *levSet.rbegin();
        if (maxLevInSb >= 0 && static_cast<int>(hands[sbid].levels.size()) <= maxLevInSb) {
            hands[sbid].levels.resize(maxLevInSb + 1);
        }
    }

    int flatPos = 0;
    std::map<std::pair<int, int>, std::vector<int>> idsTmp;
    for (int i = 0; i < cellsNum; ++i) {
        const float sbv = m_HAND_Subbasin[i];
        const float levv = m_HAND_Flood_Level[i];
        if (IsNoData(sbv, nodata) || IsNoData(levv, nodata)) continue;
        const int sbid = static_cast<int>(sbv);
        const int lev = static_cast<int>(levv);
        if (sbid < 0 || lev < 0) continue;

        Hand& hand = hands[sbid];
        if (lev >= static_cast<int>(hand.levels.size())) hand.levels.resize(lev + 1);
        Level& level = hand.levels[lev];
        if (!IsNoData(m_HAND_LevelDepth[i], nodata)) level.m_levelDepth = m_HAND_LevelDepth[i];
        if (!IsNoData(m_HAND_SumArea[i], nodata)) level.m_levelSumArea = m_HAND_SumArea[i];
        if (!IsNoData(m_HAND_SumVolume[i], nodata)) level.m_levelSumVol = static_cast<double>(m_HAND_SumVolume[i]);
        if (!IsNoData(m_HAND_AvgDepth[i], nodata)) level.m_levelAvgDepth = m_HAND_AvgDepth[i];
        if (!IsNoData(m_HAND_AccVolume[i], nodata)) level.m_levelAccVol = static_cast<double>(m_HAND_AccVolume[i]);

        int L = 0;
        const float lenf = m_HAND_LowerAccDepthLen[i];
        if (!IsNoData(lenf, nodata) && lenf > 0.f) L = static_cast<int>(std::round(lenf));
        if (L > 0) {
            if (flatPos + L > flatLen) {
                std::cerr << "[ERROR] LowerAccDepthFlat overflow." << std::endl;
                return;
            }
            if (nullptr != level.m_levelLowerAccDepth) {
                delete[] level.m_levelLowerAccDepth;
                level.m_levelLowerAccDepth = nullptr;
            }
            level.m_levelLowerAccDepth = new(nothrow) float[L];
            if (nullptr == level.m_levelLowerAccDepth) {
                std::cerr << "[ERROR] Allocation failed for m_levelLowerAccDepth." << std::endl;
                return;
            }
            for (int k = 0; k < L; ++k) level.m_levelLowerAccDepth[k] = m_HAND_LowerAccDepthFlat[flatPos + k];
            flatPos += L;
        }
        if (buildHandIds) idsTmp[std::make_pair(sbid, lev)].push_back(i);
    }

    if (buildHandIds) {
        for (auto& kv : idsTmp) {
            const int sbid = kv.first.first;
            const int lev = kv.first.second;
            std::vector<int>& ids = kv.second;
            Level& level = hands[sbid].levels[lev];
            if (nullptr != level.handIds) {
                delete[] level.handIds;
                level.handIds = nullptr;
            }
            level.m_levelHandNum = static_cast<int>(ids.size());
            if (level.m_levelHandNum > 0) {
                level.handIds = new(nothrow) int[level.m_levelHandNum];
                if (nullptr == level.handIds) {
                    std::cerr << "[ERROR] Allocation failed for handIds." << std::endl;
                    return;
                }
                for (int j = 0; j < level.m_levelHandNum; ++j) level.handIds[j] = ids[j];
            }
        }
    }
}

// Compute reservoir release from storage fill and rule-curve thresholds.
float DWNEW_CH_HAND::ComputeResScheduledOutflow(int i, float curSto, float qIn, float sub_dt) {
    const float totalVol = m_lakevol[i];
    if (totalVol <= 0.f) return 0.f;
    const float fill = curSto / totalVol;
    const float dt = sub_dt;
    const float Lc = std::min(m_ResLc[i], 0.5f);
    const float Ln = std::min(m_ResLn[i], 0.99f);
    const float Lf = std::min(m_ResLf[i], 1.2f);
    const float adj = std::min(m_ResAdjust[i], 1.f);
    const float Lnf = Ln + adj * (Lf - Ln);
    const float Qmin = m_resminq[i];
    float Qnorm = m_resnormq[i] * m_res_normMult[i];
    const float Qnd = m_resndq[i];
    Qnorm = std::max(Qnorm, Qmin + 0.01f);
    Qnorm = std::min(Qnorm, Qnd - 0.01f);

    float Q = std::min(Qmin, curSto / std::max(dt, 1.e-6f));
    if (fill > 2.f * Lc) {
        const float dO = Qnorm - Qmin;
        const float dL = std::max(Ln - 2.f * Lc, 1.e-6f);
        Q = Qmin + dO * (fill - 2.f * Lc) / dL;
    }
    if (fill > Ln) Q = Qnorm;
    if (fill > Lnf) {
        const float dNF = std::max(Lf - Lnf, 1.e-6f);
        Q = Qnorm + (fill - Lnf) / dNF * (Qnd - Qnorm);
    }
    if (fill > Lf) Q = std::min(Qnd, std::max(qIn * 1.2f, Qnorm));
    if (Q > 1.2f * qIn && Q > Qnorm && fill < Lf) Q = std::min(Q, std::max(qIn, Qnorm));
    return std::max(Q, 0.f);
}
