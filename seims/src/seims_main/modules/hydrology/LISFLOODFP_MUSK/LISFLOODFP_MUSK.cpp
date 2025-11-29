#include "LISFLOODFP_MUSK.h"
#include "ChannelRoutingCommon.h"
#include "utils_math.h"
#include <numeric>  // std::accumulate

#include <map>
#include <set> 
#include "text.h"
#include <string>

LISFLOODFP_MUSK::LISFLOODFP_MUSK() :
	m_dt(-1), m_subbsnID(nullptr),
	m_inputSubbsnID(-1), m_nreach(-1), m_outletID(-1),
	m_Epch(NODATA_VALUE), m_Bnk0(NODATA_VALUE), m_Chs0_perc(NODATA_VALUE),
	m_aBank(NODATA_VALUE), m_bBank(NODATA_VALUE),
	m_mskX(NODATA_VALUE), m_mskCoef1(NODATA_VALUE), m_mskCoef2(NODATA_VALUE),
	m_chWth(nullptr), m_chDepth(nullptr), m_chLen(nullptr), m_chArea(nullptr),
	m_chSideSlope(nullptr), m_chSlope(nullptr), m_chMan(nullptr),
	m_Kchb(nullptr), m_Kbank(nullptr), m_reachDownStream(nullptr),
	// Inputs from other modules
	m_petSubbsn(nullptr), m_gwSto(nullptr),
	m_olQ2Rch(nullptr), m_ifluQ2Rch(nullptr), m_gndQ2Rch(nullptr),
	// Temporary variables
	m_ptSub(nullptr), m_flowIn(nullptr), m_flowOut(nullptr), m_seepage(nullptr),
	// Outputs
	m_qRchOut(nullptr), m_qsRchOut(nullptr), m_qiRchOut(nullptr), m_qgRchOut(nullptr),
	m_chSto(nullptr), m_rteWtrIn(nullptr), m_rteWtrOut(nullptr), m_bankSto(nullptr),
	m_chWtrDepth(nullptr), m_chWtrWth(nullptr), m_chBtmWth(nullptr), m_chCrossArea(nullptr),
	//ljj++
	m_GWMAX(NODATA_VALUE), m_Kg(NODATA_VALUE), m_Base_ex(NODATA_VALUE), m_ispermafrost(nullptr),
	m_nCells(-1), m_subbasinsInfo(nullptr), m_prec(nullptr), curBasinArea(nullptr), m_area(nullptr), m_netPcp(nullptr),
	m_islake(nullptr), m_lakearea(nullptr), m_evlake(NODATA_VALUE), m_lakeseep(NODATA_VALUE), m_petFactor(NODATA_VALUE),
	m_minvol(NODATA_VALUE), m_lakedpini(nullptr), m_lakedp(nullptr), m_lakevol(nullptr), m_lakeb(NODATA_VALUE),
	m_A_Va(nullptr), m_A_Vb(nullptr), m_A_a(nullptr), m_A_b(nullptr), m_pet(nullptr), m_PET(nullptr),
	m_qin1(nullptr), m_qout1(nullptr), m_lakealpha(nullptr), m_natural_flow(nullptr),
	m_isres(nullptr), m_ResLc(nullptr), m_ResLn(nullptr), m_ResLf(nullptr), m_ResAdjust(nullptr),
	m_Ch2GW(nullptr), m_GWMIN(NODATA_VALUE), m_aquifer(nullptr), m_maxSoilLyrs(-1), m_temp1(nullptr), m_temp2(nullptr),
	m_dem(nullptr), curBasinDem(nullptr), m_charge(nullptr), m_qin(nullptr), m_recharge(nullptr),
	m_potRfCoef(nullptr), m_slope(nullptr), flowoutlength(nullptr), m_T_LKWB(nullptr),
	m_resminq(nullptr), m_resndq(nullptr), m_resnormq(nullptr), m_res_normMult(nullptr), m_rrtime(nullptr),
	m_lakeperc(nullptr), m_lakepcp(nullptr), m_chBedMeanElev(nullptr), m_chBedStartElev(nullptr), m_chBedEndElev(nullptr) {

}

LISFLOODFP_MUSK::~LISFLOODFP_MUSK() {
	if (!ptrs_map.empty())
	{
		for (const auto& pair : ptrs_map) {
			int subbasin_id = pair.first;
			LISFLOODFPPtrs LFPPtrs = pair.second;
			LisFloodFP_Finilize(&LFPPtrs.Solverptr, &LFPPtrs.Arrptr, &LFPPtrs.Fnameptr, &LFPPtrs.FpsPtr, &LFPPtrs.Statesptr, &LFPPtrs.Parptr, &LFPPtrs.LFPContextPtr, LFPPtrs.tmpFileNamePtr);
		}
	}


	/************************************MUSK_CH**********************************/
	if (nullptr != m_ptSub) Release1DArray(m_ptSub);
	if (nullptr != m_flowIn) Release1DArray(m_flowIn);
	if (nullptr != m_flowOut) Release1DArray(m_flowOut);
	if (nullptr != m_seepage) Release1DArray(m_seepage);
	if (nullptr != m_charge) Release1DArray(m_charge);
	if (nullptr != m_recharge) Release1DArray(m_recharge);


	if (nullptr != m_qRchOut) Release1DArray(m_qRchOut);
	if (nullptr != m_qsRchOut) Release1DArray(m_qsRchOut);
	if (nullptr != m_qiRchOut) Release1DArray(m_qiRchOut);
	if (nullptr != m_qgRchOut) Release1DArray(m_qgRchOut);

	if (nullptr != m_chSto) Release1DArray(m_chSto);
	if (nullptr != m_rteWtrIn) Release1DArray(m_rteWtrIn);
	if (nullptr != m_rteWtrOut) Release1DArray(m_rteWtrOut);
	if (nullptr != m_bankSto) Release1DArray(m_bankSto);
	if (nullptr != m_Ch2GW) Release1DArray(m_Ch2GW);
	if (nullptr != m_aquifer) Release1DArray(m_aquifer);

	if (nullptr != m_chWtrDepth) Release1DArray(m_chWtrDepth);
	if (nullptr != m_chWtrWth) Release1DArray(m_chWtrWth);
	if (nullptr != m_chBtmWth) Release1DArray(m_chBtmWth);
	if (nullptr != m_chCrossArea) Release1DArray(m_chCrossArea);

	//ljj++
	if (nullptr != curBasinArea) Release1DArray(curBasinArea);
	if (nullptr != curBasinDem) Release1DArray(curBasinDem);
	if (nullptr != m_lakedp) Release1DArray(m_lakedp);
	if (nullptr != m_qin1) Release1DArray(m_qin1);
	if (nullptr != m_qout1) Release1DArray(m_qout1);
	if (nullptr != m_temp1) Release1DArray(m_temp1);
	if (nullptr != m_temp2) Release1DArray(m_temp2);
	if (nullptr != m_qin) Release1DArray(m_qin);

	if (m_T_LKWB != nullptr) Release2DArray(m_nreach + 1, m_T_LKWB);
	if (nullptr != m_lakepcp) Release1DArray(m_lakepcp);
	if (nullptr != m_lakeperc) Release1DArray(m_lakeperc);

	// xiaodw
	if (nullptr != m_chBedMeanElev) Release1DArray(m_chBedMeanElev);
	if (nullptr != m_chBedStartElev) Release1DArray(m_chBedStartElev);
	if (nullptr != m_chBedEndElev) Release1DArray(m_chBedEndElev);
}

void LISFLOODFP_MUSK::SetValue(const char* key, const float value) {
	string sk(key);
	if (StringMatch(sk, Tag_ChannelTimeStep)) m_dt = CVT_INT(value);
	//else if (StringMatch(sk, Tag_StartTime)) seims_start_time = CVT_INT(value);
	else if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);
	//else if (StringMatch(sk, Tag_CellWidth)) m_CellWth = value;
	else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
	//else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
	/************************************MUSK_CH**********************************/
	else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
	else if (StringMatch(sk, VAR_OUTLETID)) m_outletID = CVT_INT(value);
	else if (StringMatch(sk, VAR_EP_CH)) m_Epch = value;
	else if (StringMatch(sk, VAR_BNK0)) m_Bnk0 = value;
	else if (StringMatch(sk, VAR_CHS0_PERC)) m_Chs0_perc = value;
	else if (StringMatch(sk, VAR_A_BNK)) m_aBank = value;
	else if (StringMatch(sk, VAR_B_BNK)) m_bBank = value;
	else if (StringMatch(sk, VAR_MSK_X)) m_mskX = value;
	else if (StringMatch(sk, VAR_MSK_CO1)) m_mskCoef1 = value;
	//ljj++
	else if (StringMatch(sk, VAR_GWMAX)) m_GWMAX = value;
	else if (StringMatch(sk, VAR_KG)) m_Kg = value;
	else if (StringMatch(sk, VAR_GWMIN)) m_GWMIN = value;
	else if (StringMatch(sk, VAR_Base_ex)) m_Base_ex = value;
	else if (StringMatch(sk, VAR_LAKE_EVP)) m_evlake = value;
	else if (StringMatch(sk, VAR_LAKE_SEEP)) m_lakeseep = value;
	else if (StringMatch(sk, VAR_K_PET)) m_petFactor = value;
	else if (StringMatch(sk, VAR_LAKE_MNVOL))  m_minvol = value;
	else if (StringMatch(sk, "LAKEB"))  m_lakeb = value;
	else {
		throw ModelException(MID_LISFLOODFP_MUSK, "SetValue", "Parameter " + sk + " does not exist.");
	}
}



void LISFLOODFP_MUSK::Set1DData(const char* key, const int n, float* data) {

	string sk(key);

	//if (StringMatch(sk, VAR_AHRU)) {
	//	CheckInputSize(MID_LISFLOODFP_MUSK, key, n, m_nCells);
	//	//m_handArea = data;
	//}
	if (StringMatch(sk, VAR_SUBBSN)) m_subbsnID = data;
	else if (StringMatch(sk, VAR_BKST)) m_bankSto = data;
	else if (StringMatch(sk, VAR_SUBBSN)) {
		m_subbsnID = data;
	}
	else if (StringMatch(sk, VAR_SBPET)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n - 1, m_nreach);
		m_petSubbsn = data;
	}
	else if (StringMatch(sk, VAR_SBGS)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n - 1, m_nreach);
		m_gwSto = data;
	}
	else if (StringMatch(sk, VAR_SBOF)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n - 1, m_nreach);
		m_olQ2Rch = data;
	}
	else if (StringMatch(sk, VAR_SBIF)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n - 1, m_nreach);
		m_ifluQ2Rch = data;
	}
	else if (StringMatch(sk, VAR_SBQG)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n - 1, m_nreach);
		m_gndQ2Rch = data;
	}
	//ljj++
	else if (StringMatch(sk, VAR_PCP)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n, m_nCells);
		m_netPcp = data;
	}
	else if (StringMatch(sk, VAR_PET)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n, m_nCells);
		m_PET = data;
	}
	else if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n, m_nCells);
		m_area = data;
	}
	else if (StringMatch(sk, VAR_DEM)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n, m_nCells);
		m_dem = data;
	}
	else if (StringMatch(sk, VAR_RUNOFF_CO)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n, m_nCells);
		m_potRfCoef = data;
	}
	else if (StringMatch(sk, VAR_SLOPE)) {
		CheckInputSize(MID_LISFLOODFP_MUSK, key, n, m_nCells);
		m_slope = data;
	}
	else {
		throw ModelException(MID_LISFLOODFP_MUSK, "Set1DData", "Parameter " + sk + " does not exist.");
	}
}

void LISFLOODFP_MUSK::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
	string sk(key);
	//if (!CheckInputSize2D("IO_TEST", key, n, col, m_nCells, m_maxSoilLyrs)) return;
	//if (StringMatch(sk, VAR_CONDUCT)) {
	//    m_raster2D = data;
	//}
	CheckInputSize2D(MID_NUTR_TF, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
	if (StringMatch(sk, VAR_SOILT)) {
		m_soilTempprofile = data;
	}
}


void LISFLOODFP_MUSK::SetScenario(Scenario* sce) {
	if (nullptr != sce) {
		map<int, BMPFactory *>& tmpBMPFactories = sce->GetBMPFactories();
		for (auto it = tmpBMPFactories.begin(); it != tmpBMPFactories.end(); ++it) {
			/// Key is uniqueBMPID, which is calculated by BMP_ID * 100000 + subScenario;
			if (it->first / 100000 == BMP_TYPE_POINTSOURCE) {
#ifdef HAS_VARIADIC_TEMPLATES
				m_ptSrcFactory.emplace(it->first, static_cast<BMPPointSrcFactory*>(it->second));
#else
				m_ptSrcFactory.insert(make_pair(it->first, static_cast<BMPPointSrcFactory*>(it->second)));
#endif
			}
		}
	}
	else {
		throw ModelException(MID_MUSK_CH, "SetScenario", "The scenario can not to be NULL.");
	}
}

void LISFLOODFP_MUSK::GetValue(const char* key, float* value) {
	InitialOutputs();
	string sk(key);
	/// IN/OUTPUT variables
	if (StringMatch(sk, VAR_QRECH) && m_inputSubbsnID > 0) *value = m_qRchOut[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_QS) && m_inputSubbsnID > 0) *value = m_qsRchOut[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_QI) && m_inputSubbsnID > 0) *value = m_qiRchOut[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_QG) && m_inputSubbsnID > 0) *value = m_qgRchOut[m_inputSubbsnID];
	else {
		throw ModelException(MID_MUSK_CH, "GetValue", "Parameter " + sk + " does not exist.");
	}
}

void LISFLOODFP_MUSK::Get1DData(const char* key, int* n, float** data) {
	InitialOutputs();
	string sk(key);
	*n = m_nreach + 1;
	if (StringMatch(sk, VAR_QRECH)) {
		m_qRchOut[0] = m_qRchOut[m_outletID];
		*data = m_qRchOut;
	}
	else if (StringMatch(sk, VAR_QS)) {
		m_qsRchOut[0] = m_qsRchOut[m_outletID];
		*data = m_qsRchOut;
	}
	else if (StringMatch(sk, VAR_QI)) {
		m_qiRchOut[0] = m_qiRchOut[m_outletID];
		*data = m_qiRchOut;
	}
	else if (StringMatch(sk, VAR_QG)) {
		m_qgRchOut[0] = m_qgRchOut[m_outletID];
		*data = m_qgRchOut;
	}
	else if (StringMatch(sk, VAR_CHST)) {
		m_chSto[0] = m_chSto[m_outletID];
		*data = m_chSto;
	}
	else if (StringMatch(sk, VAR_CHST_LAST_STEP)) {
		m_chStoLastStep[0] = m_chStoLastStep[m_outletID];
		*data = m_chStoLastStep;
	}
	else if (StringMatch(sk, VAR_RTE_WTRIN)) {
		m_rteWtrIn[0] = m_rteWtrIn[m_outletID];
		*data = m_rteWtrIn;
	}
	else if (StringMatch(sk, VAR_RTE_WTROUT)) {
		m_rteWtrOut[0] = m_rteWtrOut[m_outletID];
		*data = m_rteWtrOut;
	}
	else if (StringMatch(sk, VAR_BKST)) {
		m_bankSto[0] = m_bankSto[m_outletID];
		*data = m_bankSto;
	}
	else if (StringMatch(sk, VAR_BKST_LAST_STEP)) {
		m_bankStoLastStep[0] = m_bankStoLastStep[m_outletID];
		*data = m_bankStoLastStep;
	}
	else if (StringMatch(sk, VAR_CHWTRDEPTH)) {
		m_chWtrDepth[0] = m_chWtrDepth[m_outletID];
		*data = m_chWtrDepth;
	}
	else if (StringMatch(sk, VAR_CHWTRWIDTH)) {
		m_chWtrWth[0] = m_chWtrWth[m_outletID];
		*data = m_chWtrWth;
	}
	else if (StringMatch(sk, VAR_CHBTMWIDTH)) {
		m_chBtmWth[0] = m_chBtmWth[m_outletID];
		*data = m_chBtmWth;
	}
	else if (StringMatch(sk, VAR_CHCROSSAREA)) {
		m_chCrossArea[0] = m_chCrossArea[m_outletID];
		*data = m_chCrossArea;
	}
	else if (StringMatch(sk, VAR_qout)) {
		*data = m_qRchOut;
	}
	else if (StringMatch(sk, VAR_qsurf)) {
		*data = m_qsRchOut;
	}
	else if (StringMatch(sk, "LAKE_P")) {
		*data = m_lakepcp;
	}
	else if (StringMatch(sk, "LAKE_E")) {
		*data = m_lakeperc;
	}
	else if (StringMatch(sk, "rrtime")) {
		*data = m_rrtime;
	}
	else {
		throw ModelException(MID_MUSK_CH, "Get1DData", "Output " + sk + " does not exist.");
	}
}

void LISFLOODFP_MUSK::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, "lake_wb")) {
		*data = m_T_LKWB;
		*nrows = m_nreach + 1;
		*ncols = 7;
	}
	else {
		throw ModelException(MID_GWA_RE, "Get2DData", "Parameter " + sk + " does not exist in current module.");
	}
}


void LISFLOODFP_MUSK::SetReaches(clsReaches* reaches) {

	if (nullptr == reaches) {
		throw ModelException(MID_LISFLOODFP_MUSK, "SetReaches", "The reaches input can not to be NULL.");
	}

	m_nreach = reaches->GetReachNumber();

	if (nullptr == m_chWth) reaches->GetReachesSingleProperty(REACH_WIDTH, &m_chWth);
	if (nullptr == m_chDepth) reaches->GetReachesSingleProperty(REACH_DEPTH, &m_chDepth);
	if (nullptr == m_chLen) reaches->GetReachesSingleProperty(REACH_LENGTH, &m_chLen);
	if (nullptr == m_chArea) reaches->GetReachesSingleProperty(REACH_AREA, &m_chArea);
	if (nullptr == m_chSideSlope) reaches->GetReachesSingleProperty(REACH_SIDESLP, &m_chSideSlope);
	if (nullptr == m_chSlope) reaches->GetReachesSingleProperty(REACH_SLOPE, &m_chSlope);
	if (nullptr == m_chMan) reaches->GetReachesSingleProperty(REACH_MANNING, &m_chMan);
	if (nullptr == m_Kbank) reaches->GetReachesSingleProperty(REACH_BNKK, &m_Kbank);
	if (nullptr == m_Kchb) reaches->GetReachesSingleProperty(REACH_BEDK, &m_Kchb);
	if (nullptr == m_reachDownStream) reaches->GetReachesSingleProperty(REACH_DOWNSTREAM, &m_reachDownStream);
	if (nullptr == m_ispermafrost) reaches->GetReachesSingleProperty(REACH_PERMAFORST, &m_ispermafrost);
	//ljj++
	if (nullptr == m_islake) reaches->GetReachesSingleProperty(REACH_ISLAKE, &m_islake);
	if (nullptr == m_lakearea) reaches->GetReachesSingleProperty(REACH_LAKEAREA, &m_lakearea);
	if (nullptr == m_lakevol) reaches->GetReachesSingleProperty(REACH_LAKEVOL, &m_lakevol);
	if (nullptr == m_lakedpini) reaches->GetReachesSingleProperty(REACH_LAKEDPINI, &m_lakedpini);
	if (nullptr == m_lakealpha) reaches->GetReachesSingleProperty(REACH_LAKEALPHA, &m_lakealpha);
	if (nullptr == m_A_Va) reaches->GetReachesSingleProperty("A_Va", &m_A_Va);
	if (nullptr == m_A_Vb) reaches->GetReachesSingleProperty("A_Vb", &m_A_Vb);
	if (nullptr == m_A_a) reaches->GetReachesSingleProperty("A_a", &m_A_a);
	if (nullptr == m_A_b) reaches->GetReachesSingleProperty("A_b", &m_A_b);

	if (nullptr == m_isres) reaches->GetReachesSingleProperty(REACH_ISRES, &m_isres);
	if (nullptr == m_natural_flow) reaches->GetReachesSingleProperty(REACH_NATURAL_FLOW, &m_natural_flow);
	if (nullptr == m_resminq) reaches->GetReachesSingleProperty("RES_minq", &m_resminq);
	if (nullptr == m_resnormq) reaches->GetReachesSingleProperty("RES_normq", &m_resnormq);
	if (nullptr == m_resndq) reaches->GetReachesSingleProperty("RES_ndq", &m_resndq);
	if (nullptr == m_res_normMult) reaches->GetReachesSingleProperty("RES_normMult", &m_res_normMult);
	if (nullptr == m_ResLc) reaches->GetReachesSingleProperty(REACH_RES_LC, &m_ResLc);
	if (nullptr == m_ResLn) reaches->GetReachesSingleProperty(REACH_RES_LN, &m_ResLn);
	if (nullptr == m_ResLf) reaches->GetReachesSingleProperty(REACH_RES_LF, &m_ResLf);
	if (nullptr == m_ResAdjust) reaches->GetReachesSingleProperty(REACH_RES_ADJUST, &m_ResAdjust);
	if (nullptr == m_chBedMeanElev) reaches->GetReachesSingleProperty(REACH_BED_MEAN_ELEV, &m_chBedMeanElev);
	if (nullptr == m_chBedStartElev) reaches->GetReachesSingleProperty(REACH_BED_START_ELEV, &m_chBedStartElev);
	if (nullptr == m_chBedEndElev) reaches->GetReachesSingleProperty(REACH_BED_END_ELEV, &m_chBedEndElev);

	m_reachUpStream = reaches->GetUpStreamIDs();
	m_rteLyrs = reaches->GetReachLayers();
}

bool LISFLOODFP_MUSK::ChannelFlow(const int i) {
	// 1. first add all the inflow water
	float qIn = 0.f; /// Water entering reach on current day from both current subbasin and upstreams
	// 1.1. water from this subbasin
	qIn += m_olQ2Rch[i]; /// surface flow
	float qiSub = 0.f;   /// interflow flow
	if (nullptr != m_ifluQ2Rch && m_ifluQ2Rch[i] >= 0.f) {
		qiSub = m_ifluQ2Rch[i];
		qIn += qiSub;
	}
	float qgSub = 0.f; /// groundwater flow
	if (nullptr != m_gndQ2Rch && m_gndQ2Rch[i] >= 0.f && (m_isres[i] != 1 && m_isres[i] != 1)) {
		qgSub = m_gndQ2Rch[i];
		qIn += qgSub;
	}
	float ptSub = 0.f; /// point sources flow
	if (nullptr != m_ptSub && m_ptSub[i] >= 0.f) {
		ptSub = m_ptSub[i];
		qIn += ptSub;
	}
	// 1.2. water from upstream reaches
	float qsUp = 0.f;
	float qiUp = 0.f;
	float qgUp = 0.f;
	for (auto upRchID = m_reachUpStream.at(i).begin(); upRchID != m_reachUpStream.at(i).end(); ++upRchID) {
		if (m_qsRchOut[*upRchID] != m_qsRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", surface part illegal!" << endl;
			return false;
		}
		if (m_qiRchOut[*upRchID] != m_qiRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", subsurface part illegal!" << endl;
			return false;
		}
		if (m_qgRchOut[*upRchID] != m_qgRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", groundwater part illegal!" << endl;
			return false;
		}
		//ljj++
		if (m_isres[*upRchID] == 1 || m_isres[*upRchID] == 1) {
			qgUp += m_gndQ2Rch[*upRchID];
		}
		if (m_qsRchOut[*upRchID] > 0.f) qsUp += m_qsRchOut[*upRchID];
		// xiaodw,  qin is allowed when upstream channel's water elevation is higher than lake's water elevation
		//if (m_qiRchOut[*upRchID] > 0.f && (m_chBedMeanElev[*upRchID] + m_chWtrDepth[*upRchID]) ) {

		//	qiUp += m_qiRchOut[*upRchID];
		//}
		if (m_qgRchOut[*upRchID] > 0.f) qgUp += m_qgRchOut[*upRchID];
		//cout<<i<<"   "<<*upRchID<<"   "<< m_Ch2GW[*upRchID]<<endl;
	}
	qIn += qsUp + qiUp + qgUp;
#ifdef PRINT_DEBUG
	cout << "ID: " << i << ", surfaceQ: " << m_qsSub[i] << ", subsurfaceQ: " << qiSub <<
		", groundQ: " << qgSub << ", pointQ: " << ptSub <<
		", UPsurfaceQ: " << qsUp << ", UPsubsurface: " << qiUp << ", UPground: " << qgUp << endl;
#endif
	// 1.3. water from bank storage
	// xiaodw add, for calculating hand water lavel, with no effect on other modules
	m_bankStoLastStep[i] = m_bankSto[i];
	float bankOut = m_bankSto[i] * (1.f - exp(-m_aBank));
	m_bankSto[i] -= bankOut;
	qIn += bankOut / m_dt;

	// loss the water from bank storage to the adjacent unsaturated zone and groundwater storage
	float bankOutGw = m_bankSto[i] * (1.f - exp(-m_bBank));
	m_bankSto[i] -= bankOutGw;
	if (nullptr != m_gwSto) {
		m_gwSto[i] += bankOutGw / m_chArea[i] * 1000.f; // updated groundwater storage
	}

	// Compute storage time constant (ratio of storage to discharge) for reach
	// Wetting perimeter at bankfull
	float wet_perimeter = ChannelWettingPerimeter(m_chBtmWth[i], m_chDepth[i], m_chSideSlope[i]);
	// Cross-sectional area at bankfull
	float cross_area = ChannelCrossSectionalArea(m_chBtmWth[i], m_chDepth[i], m_chSideSlope[i]);
	// Hydraulic radius
	float radius = cross_area / wet_perimeter;
	// The storage time constant calculated for the reach segment with bankfull flows.
	float k_bankfull = StorageTimeConstant(m_chMan[i], m_chSlope[i], m_chLen[i], radius); // Hour

	// The storage time constant calculated for the reach segment with one-tenth of the bankfull flows.
	float wet_perimeter2 = ChannelWettingPerimeter(m_chBtmWth[i], 0.1f * m_chDepth[i], m_chSideSlope[i]);
	float cross_area2 = ChannelCrossSectionalArea(m_chBtmWth[i], 0.1f * m_chDepth[i], m_chSideSlope[i]);
	float radius2 = cross_area2 / wet_perimeter2;
	float k_bankfull2 = StorageTimeConstant(m_chMan[i], m_chSlope[i], m_chLen[i], radius2); // Hour

	float xkm = k_bankfull * m_mskCoef1 + k_bankfull2 * m_mskCoef2;
	// Eq. 7:1.4.9 in SWAT Theory 2009.
	// Check Muskingum numerical stability
	float detmax = 2.f * xkm * (1.f - m_mskX);
	float detmin = 2.f * xkm * m_mskX;
	// Discretize time interval to meet the stability criterion
	float det = 24.f; // hours, time step
	int nn = 0;       // number of subdaily computation points for stable routing
	if (det > detmax) {
		if (det / 2.f <= detmax) {
			det = 12.f;
			nn = 2;
		}
		else if (det / 4.f <= detmax) {
			det = 6.f;
			nn = 4;
		}
		else {
			det = 1;
			nn = 24;
		}
	}
	else {
		det = 24;
		nn = 1;
	}
	// get coefficients of Muskingum
	float temp = detmax + det;
	float c1 = (det - detmin) / temp;
	float c2 = (det + detmin) / temp;
	float c3 = (detmax - det) / temp;
	// make sure any coefficient is positive. Not sure whether this is needed.
	//if (c1 < 0) {
	//    c2 += c1;
	//    c1 = 0.f;
	//}
	//if (c3 < 0) {
	//    c2 += c1;
	//    c3 = 0.f;
	//}

#ifdef PRINT_DEBUG
	cout << " chStorage before routing " << m_chStorage[i] << endl;
#endif
	m_rteWtrOut[i] = qIn * m_dt;   // m^3
	float wtrin = qIn * m_dt / nn; // Inflow during a sub time interval, m^3
	float vol = 0.f;               // volume of water in reach, m^3
	float volrt = 0.f;             // flow rate, m^3/s
	float max_rate = 0.f;          // maximum flow capacity of the channel at bank full (m^3/s)
	float sdti = 0.f;              // average flow on day in reach, m^3/s, i.e., m_qRchOut
	float vc = 0.f;                // average flow velocity in channel, m/s
	float rchp = 0.f;              // wet perimeter, m
	float rcharea = 0.f;           // cross-sectional area, m^2
	float rchradius = 0.f;         // hydraulic radius
	float rtwtr = 0.f;             // water leaving reach on day, m^3, i.e., m_rteWtrOut
	float rttlc = 0.f;             // transmission losses from reach on day, m^3
	float qinday = 0.f;            // m^3
	float qoutday = 0.f;           // m^3

	m_rteWtrOut[i] = qIn * m_dt;
	wtrin = qIn * m_dt / nn;
	// Iterate for the day
	m_rrtime[i] = 0.f;
	for (int ii = 0; ii < nn; ii++) {
		// Calculate volume of water in reach
		vol = m_chSto[i] + wtrin; // m^3
		// Find average flowrate in a sub time interval, m^3/s
		volrt = vol / (86400.f / nn);
		// Find maximum flow capacity of the channel at bank full, m^3/s
		max_rate = manningQ(cross_area, radius, m_chMan[i], m_chSlope[i]);
		sdti = 0.f;
		m_chWtrDepth[i] = 0.f;
		// If average flowrate is greater than the channel capacity at bank full,
		//   then simulate flood plain flow, else simulate the regular channel flow.
		if (volrt > max_rate) {
			m_chWtrDepth[i] = m_chDepth[i];
			sdti = max_rate;
			//a = b * d + chsslope * d * d
			rcharea = m_chBtmWth[i] * m_chDepth[i] + m_chSideSlope[i] * m_chDepth[i] * m_chDepth[i];
			rchp = m_chDepth[i];
			float p = m_chBtmWth[i] + 2. * m_chDepth[i] * sqrt(1. + m_chSideSlope[i] * m_chSideSlope[i]);
			// Find the cross-sectional area and depth for volrt by iteration method at 1cm interval depth.
			// Find the depth until the discharge rate is equal to volrt
			while (sdti < volrt) {
				m_chWtrDepth[i] += 0.01f; // Increase 1cm at each interation
				float addarea = rcharea + ((m_chWth[i] * 5) + 4 * m_chWtrDepth[i]) * m_chWtrDepth[i];
				float addp = p + (m_chWth[i] * 4) + 2. * m_chWtrDepth[i] * sqrt(1. + 4 * 4);
				radius = addarea / addp;
				rcharea = addarea;
				rchp = addp;
				// rcharea = ChannelCrossSectionalArea(m_chBtmWth[i], m_chDepth[i], m_chWtrDepth[i],
				//                                     m_chSideSlope[i], m_chWth[i], 4.f);
				// rchp = ChannelWettingPerimeter(m_chBtmWth[i], m_chDepth[i], m_chWtrDepth[i],
				//                                m_chSideSlope[i], m_chWth[i], 4.f);
				// radius = rcharea / rchp;
				sdti = manningQ(rcharea, radius, m_chMan[i], m_chSlope[i]);
			}
			sdti = volrt;
		}
		else {
			// Find the cross-sectional area and depth for volrt by iteration method at 1cm interval depth
			// Find the depth until the discharge rate is equal to volrt.
			while (sdti < volrt) {
				m_chWtrDepth[i] += 0.01f;
				rcharea = (m_chBtmWth[i] + m_chSideSlope[i] * m_chWtrDepth[i]) * m_chWtrDepth[i];
				rchp = m_chBtmWth[i] + 2. * m_chWtrDepth[i] * sqrt(1. + m_chSideSlope[i] * m_chSideSlope[i]);
				// rcharea = ChannelCrossSectionalArea(m_chBtmWth[i], m_chWtrDepth[i], m_chSideSlope[i]);
				// rchp = ChannelWettingPerimeter(m_chBtmWth[i], m_chWtrDepth[i], m_chSideSlope[i]);
				rchradius = rcharea / rchp;
				sdti = manningQ(rcharea, rchradius, m_chMan[i], m_chSlope[i]);
			}
			sdti = volrt;
		}
		// Calculate top width of channel at water level, topw in SWAT
		if (m_chWtrDepth[i] <= m_chDepth[i]) {
			m_chWtrWth[i] = m_chBtmWth[i] + 2.f * m_chWtrDepth[i] * m_chSideSlope[i];
		}
		else {
			m_chWtrWth[i] = 5.f * m_chWth[i] + 2.f * (m_chWtrDepth[i] - m_chDepth[i]) * 4.f;
		}
		if (sdti > 0.f) {
			// Calculate velocity and travel time
			vc = sdti / rcharea;                       // vel_chan(:) in SWAT
			float rttime = m_chLen[i] / (3600.f * vc); // reach travel time, hr
			m_rrtime[i] += rttime;
			// Compute water leaving reach on day
			rtwtr = c1 * wtrin + c2 * m_flowIn[i] + c3 * m_flowOut[i];
			if (rtwtr < 0.f) rtwtr = 0.f;
			rtwtr = Min(rtwtr, wtrin + m_chSto[i]);
			// Calculate amount of water in channel at end of day
			m_chSto[i] += wtrin - rtwtr;
			// Add if statement to keep m_chStorage from becoming negative
			if (m_chSto[i] < 0.f) m_chSto[i] = 0.f;

			// Transmission and evaporation losses are proportionally taken from the channel storage
			//   and from volume flowing out
			if (rtwtr > 0.f) {
				// Total time in hours to clear the water
				rttlc = det * m_Kchb[i] * 0.001f * m_chLen[i] * rchp; // m^3
				float rttlc2 = rttlc * m_chSto[i] / (rtwtr + m_chSto[i]);
				float rttlc1 = 0.f;
				if (m_chSto[i] <= rttlc2) {
					rttlc2 = Min(rttlc2, m_chSto[i]);
				}
				m_chSto[i] -= rttlc2;
				rttlc1 = rttlc - rttlc2;
				if (rtwtr <= rttlc1) {
					rttlc1 = Min(rttlc1, rtwtr);
				}
				rtwtr -= rttlc1;
				rttlc = rttlc1 + rttlc2; // Total water loss by transmission
			}
			// Calculate evaporation
			float rtevp = 0.f;
			float rtevp1 = 0.f;
			float rtevp2 = 0.f;
			//ljj++ for hulugou rep will increase DOC concentration 
			//assuming the transport is very fast, rep is small???
			if (rtwtr > 0.f) {
				/// In SWAT source code, line 306 of rtmusk.f, I think aaa should be divided by nn! By lj.
				float aaa = m_Epch * m_petSubbsn[i] * 0.001f / nn; // m
				if (m_chWtrDepth[i] <= m_chDepth[i]) {
					rtevp = aaa * m_chLen[i] * m_chWtrWth[i]; // m^3
				}
				else {
					if (aaa <= m_chWtrDepth[i] - m_chDepth[i]) {
						rtevp = aaa * m_chLen[i] * m_chWtrWth[i];
					}
					else {
						rtevp = aaa;
						m_chWtrWth[i] = m_chBtmWth[i] + 2.f * m_chDepth[i] * m_chSideSlope[i];
						rtevp *= m_chLen[i] * m_chWtrWth[i]; // m^3
					}
				}
				rtevp2 = rtevp * m_chSto[i] / (rtwtr + m_chSto[i]);
				if (m_chSto[i] <= rtevp2) {
					rtevp2 = Min(rtevp2, m_chSto[i]);
				}
				m_chSto[i] -= rtevp2;
				rtevp1 = rtevp - rtevp2;
				if (rtwtr <= rtevp1) {
					rtevp1 = Min(rtevp1, rtwtr);
				}
				rtwtr -= rtevp1;
				rtevp = rtevp1 + rtevp2; // Total water loss by evaporation
			}

			// Define flow parameters for current iteration
			m_flowIn[i] = wtrin;
			m_flowOut[i] = rtwtr;
			// Define flow parameters for current day
			qinday += wtrin;
			qoutday += rtwtr;
			// Total outflow for the day
			rtwtr = qoutday;
		}
		else {
			rtwtr = 0.f;
			sdti = 0.f;
			m_chSto[i] = 0.f;
			m_flowIn[i] = 0.f;
			m_flowOut[i] = 0.f;
		}
	} /* Iterate for the day */
	if (rtwtr < 0.f) rtwtr = 0.f;
	if (m_chSto[i] < 0.f) m_chSto[i] = 0.f;
	if (m_chSto[i] < 10.f) {
		rtwtr += m_chSto[i];
		m_chSto[i] = 0.f;
	}
	m_qRchOut[i] = sdti;
	m_rteWtrOut[i] = rtwtr;
	m_chCrossArea[i] = rcharea;
	m_qRchOut[i] = rtwtr / m_dt;
	if (m_qRchOut[i] <= UTIL_ZERO) m_rteWtrOut[i] = 0.f;

	float qInSum = m_olQ2Rch[i] + qiSub + qgSub + qsUp + qiUp + qgUp;
	if (qInSum < UTIL_ZERO) {
		// In case of divided by zero.
		m_qsRchOut[i] = 0.f;
		m_qiRchOut[i] = 0.f;
		m_qgRchOut[i] = 0.f;
		m_qRchOut[i] = 0.f;
	}
	else {
		// In my opinion, these lines should use `qIn` instead of `qInSum`. By lj.
		m_qsRchOut[i] = m_qRchOut[i] * (m_olQ2Rch[i] + qsUp) / (qIn);
		m_qiRchOut[i] = m_qRchOut[i] * (qiSub + qiUp) / (qIn);
		m_qgRchOut[i] = m_qRchOut[i] * (qgSub + qgUp) / (qIn);
	}

	// Add transmission losses to bank storage/deep aquifer (i.e., groundwater in current version)
	if (rttlc > 0.f) {
		float trnsrch = 0.5f;
		if (rchp > 0.f) {
			trnsrch = m_chBtmWth[i] / rchp; // Use bottom width / wetting perimeter to estimate.
		}
		m_bankSto[i] += rttlc * (1.f - trnsrch); // m^3
		if (nullptr != m_gwSto) {
			m_gwSto[i] += rttlc * trnsrch / m_chArea[i] * 1000.f; // mm
		}
	}

	// todo, compute revap from bank storage. In SWAT, revap coefficient is equal to gw_revap.

#ifdef PRINT_DEBUG
	cout << " chStorage after routing " << m_chStorage[i] << endl;
	cout << " surfq: " << m_qsCh[i] << ", ifluq: " << m_qiCh[i] << ", groudq: " << m_qgCh[i] << endl;
#endif
	return true;
}
//ljj++
bool LISFLOODFP_MUSK::LakeBudget(const int i) {
	m_chWtrDepth[i] = 0.f;

	//! 1. add all the inflow water
	float qIn = 0.f; /// Water entering reach on current day from both current subbasin and upstreams
	// 1.1. water from this subbasin
	qIn += m_olQ2Rch[i]; /// surface flow
	float qiSub = 0.f;   /// interflow flow
	if (nullptr != m_ifluQ2Rch && m_ifluQ2Rch[i] >= 0.f) {
		qiSub = m_ifluQ2Rch[i];
		qIn += qiSub;
	}
	float qgSub = 0.f; /// groundwater flow
	// if (nullptr != m_gndQ2Rch && m_gndQ2Rch[i] >= 0.f) {
	//     qgSub = m_gndQ2Rch[i];
	//     qIn += qgSub;
	// }

	// 1.2. water from upstream reaches
	float qsUp = 0.f;
	float qiUp = 0.f;
	float qgUp = 0.f;
	for (auto upRchID = m_reachUpStream.at(i).begin(); upRchID != m_reachUpStream.at(i).end(); ++upRchID) {
		if (m_qsRchOut[*upRchID] != m_qsRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", surface part illegal!" << endl;
			return false;
		}
		if (m_qiRchOut[*upRchID] != m_qiRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", subsurface part illegal!" << endl;
			return false;
		}
		if (m_qgRchOut[*upRchID] != m_qgRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", groundwater part illegal!" << endl;
			return false;
		}
		if (m_qgRchOut[*upRchID] != m_qgRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", groundwater part illegal!" << endl;
			return false;
		}
		if (m_qsRchOut[*upRchID] > 0.f) qsUp += m_qsRchOut[*upRchID];
		if (m_qiRchOut[*upRchID] > 0.f) qiUp += m_qiRchOut[*upRchID];
		if (m_qgRchOut[*upRchID] > 0.f) qgUp += m_qgRchOut[*upRchID];
	}
	qIn += qsUp + qiUp + qgUp;
	m_chStoLastStep[i] = m_chSto[i];
	float pre_Sto = m_chSto[i];
	//add precipitation
	qIn += m_prec[i];
	m_lakepcp[i] = m_prec[i] * m_dt;
	// add qIn
	//m_chSto[i] += qIn * m_dt;
	float rtwtr = 0.f;    //flow out
	//!!! important: for lake unit, chSto == lakeSto
	float A1 = m_A_a[i] * pow(m_lakedp[i], m_A_b[i]);  //km2
	float aa = 0.f;
	aa = Max(1.f, A1*1.e6f);

	//! 2. minus all the outflow water
	// float h0 = 0.f;
	// h0 = m_minvol;
	// float thwl = m_lakedpini[i] * h0; 
	// float max_outflow = Max(0.f, (m_lakedp[i] - thwl) * aa);

	// lake evaporation
	float rtevp = 0.f;

	rtevp = m_evlake * m_pet[i] * m_dt; //m3
	//rtevp = m_evlake * m_petSubbsn[i] * 0.001f * m_lakearea[i]; //m3
	rtevp = Min(rtevp, m_chSto[i]);
	//m_chSto[i] -= rtevp;

	// lake groundwater
	float LakeOutGw = m_chSto[i] * m_lakeseep *0.01f;
	LakeOutGw = Min(LakeOutGw, m_chSto[i]);
	LakeOutGw = Max(LakeOutGw, 0.f);
	m_lakeperc[i] = LakeOutGw;
	if (nullptr != m_gwSto) {
		m_gwSto[i] += LakeOutGw / m_lakearea[i] * 1000.f; // updated groundwater storage
		m_chSto[i] -= LakeOutGw;
	}
	m_chSto[i] -= rtevp;

	//float SI = (m_chSto[i] / m_dt) -m_qout1[i]*0.5 + (qIn+m_qin1[i])*0.5;
	float SI = (m_chSto[i] / m_dt) + qIn;
	SI = Max(SI, 0.f);
	// Lake parameter A (suggested  value equal to outflow width in [m])
	// float lakefactor = m_lakearea[i] / m_dt / sqrt(alpha[i]);
	float lakefactor = aa / m_dt / sqrt(m_chWth[i] * m_lakealpha[i]);
	// if (m_chSto[i]>0.f){
		 //rtwtr = pow(sqrt((lakefactor*lakefactor) + 2*SI)-lakefactor,2);
		 //rtwtr = Min(rtwtr, 0.5f*m_chSto[i]/m_dt);
   // }
	 // else{
	 //     rtwtr = 0.f;
	 // }
	float thwl = m_lakedpini[i] * m_minvol;
	float runoff = 0.f;
	float max_outflow = Max(0.f, pow((m_lakedp[i] - thwl), m_A_Vb[i])*m_A_Va[i] * 1.e9f);
	if (m_lakedp[i] > thwl) {
		runoff = m_lakealpha[i] * pow((m_lakedp[i] - thwl), m_lakeb);
		runoff = Max(runoff, 0.f);
	}
	if (runoff*m_dt > max_outflow) runoff = max_outflow / m_dt;
	rtwtr = runoff;
	m_chSto[i] = (SI - rtwtr)*m_dt;
	m_chSto[i] = Max(m_chSto[i], 0.f);
	// update lake water level
	m_qRchOut[i] = rtwtr;
	float h1 = pow(m_chSto[i] * 1.e-9f / (m_A_Va[i]), 1.0 / (m_A_Vb[i]));
	m_lakedp[i] = Max(0.f, h1);
	m_rteWtrOut[i] = m_qRchOut[i] * m_dt;   // m^3
	m_qin1[i] = qIn;
	m_qout1[i] = rtwtr;
	m_chWtrDepth[i] = m_lakedp[i];

	float qInSum = m_olQ2Rch[i] + qiSub + qgSub + qsUp + qiUp + qgUp;
	if (qInSum < UTIL_ZERO) {
		// In case of divided by zero.
		// m_qsRchOut[i] = 0.f;
		// m_qiRchOut[i] = 0.f;
		// m_qgRchOut[i] = 0.f;
		// m_qgsRchOut[i] = 0.f;
		// m_qRchOut[i] = 0.f;
	}
	else {
		// In my opinion, these lines should use `qIn` instead of `qInSum`. By lj.
		// m_qsRchOut[i] = m_qRchOut[i] * (m_olQ2Rch[i] + qsUp) / qIn;
		// m_qiRchOut[i] = m_qRchOut[i] * (qiSub + qiUp) / qIn;
		// m_qgRchOut[i] = m_qRchOut[i] * (qgSub + qgUp) / qIn;
		m_qsRchOut[i] = m_qRchOut[i];
		m_qiRchOut[i] = 0.f;
		m_qgRchOut[i] = 0.f;
	}

	m_T_LKWB[i][0] = (qsUp + qiUp + qgUp) * m_dt;
	m_T_LKWB[i][1] = (m_olQ2Rch[i] + m_ifluQ2Rch[i]) * m_dt;
	m_T_LKWB[i][2] = (m_prec[i]) * m_dt;
	m_T_LKWB[i][3] = rtevp;
	m_T_LKWB[i][4] = LakeOutGw;
	m_T_LKWB[i][5] = m_qRchOut[i] * m_dt;
	m_T_LKWB[i][6] = m_chSto[i];
	return true;
}

bool LISFLOODFP_MUSK::ResBudget(const int i) {
	//! 1. add all the inflow water
	float qIn = 0.f; /// Water entering reach on current day from both current subbasin and upstreams
	// 1.1. water from this subbasin
	qIn += m_olQ2Rch[i]; /// surface flow
	float qiSub = 0.f;   /// interflow flow
	if (nullptr != m_ifluQ2Rch && m_ifluQ2Rch[i] >= 0.f) {
		qiSub = m_ifluQ2Rch[i];
		qIn += qiSub;
	}
	float qgSub = 0.f; /// groundwater flow
	if (nullptr != m_gndQ2Rch && m_gndQ2Rch[i] >= 0.f) {
		qgSub = m_gndQ2Rch[i];
		qIn += qgSub;
	}
	// 1.2. water from upstream reaches
	float qsUp = 0.f;
	float qiUp = 0.f;
	float qgUp = 0.f;
	for (auto upRchID = m_reachUpStream.at(i).begin(); upRchID != m_reachUpStream.at(i).end(); ++upRchID) {
		if (m_qsRchOut[*upRchID] != m_qsRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", surface part illegal!" << endl;
			return false;
		}
		if (m_qiRchOut[*upRchID] != m_qiRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", subsurface part illegal!" << endl;
			return false;
		}
		if (m_qgRchOut[*upRchID] != m_qgRchOut[*upRchID]) {
			cout << "DayOfYear: " << m_dayOfYear << ", rchID: " << i << ", upRchID: " << *upRchID <<
				", groundwater part illegal!" << endl;
			return false;
		}
		if (m_qsRchOut[*upRchID] > 0.f) qsUp += m_qsRchOut[*upRchID];
		if (m_qiRchOut[*upRchID] > 0.f) qiUp += m_qiRchOut[*upRchID];
		if (m_qgRchOut[*upRchID] > 0.f) qgUp += m_qgRchOut[*upRchID];
	}
	qIn += qsUp + qiUp + qgUp;

	float pre_Sto = m_chSto[i];

	//add precipitation
	qIn += m_prec[i];

	float wtrin = qIn * m_dt;  //m3  
	float rtwtr = 0.f;    //flow out

	// evaporation
	float rtevp = 0.f;
	rtevp = m_evlake * m_petSubbsn[i] / m_petFactor * 0.001f * m_lakearea[i]; //m3
	rtevp = Min(rtevp, m_chSto[i]);
	//  groundwater
	float ResOutGw = m_chSto[i] * m_lakeseep *0.01f;  //ljj++todo add new param resseep
	ResOutGw = Min(ResOutGw, m_chSto[i]);
	if (nullptr != m_gwSto) {
		m_gwSto[i] += ResOutGw / m_lakearea[i] * 1000.f; // updated groundwater storage
		m_chSto[i] -= ResOutGw;
	}
	m_chSto[i] -= rtevp;

	// add qIn
	m_chSto[i] += qIn * m_dt;

	//initial 
	float TotalReservoirStorageM3CC = m_lakevol[i];
	float ConservativeStorageLimitCC = m_ResLc[i]; //minimum storage, 0.1 as defult
	ConservativeStorageLimitCC = Min(ConservativeStorageLimitCC, 0.5f);
	float NormalStorageLimitCC = m_ResLn[i]; //normal storage, 0.3 as defult
	NormalStorageLimitCC = Min(NormalStorageLimitCC, 0.99f);
	float FloodStorageLimitCC = m_ResLf[i]; //maximum storage, 0.97 as defult
	FloodStorageLimitCC = Min(FloodStorageLimitCC, 1.2f);
	float adjust_Normal_FloodCC = m_ResAdjust[i]; //adjust parameter, 0.01~0.99, 0.7 as defult
	adjust_Normal_FloodCC = Min(adjust_Normal_FloodCC, 1.f);
	float Normal_FloodStorageLimitCC = NormalStorageLimitCC + adjust_Normal_FloodCC * (FloodStorageLimitCC - NormalStorageLimitCC);

	float InvDtSecDay = m_dt;
	float MinReservoirOutflowCC = m_resminq[i];  //minimum outflow 
	float NormalReservoirOutflowCC = m_resnormq[i]; //normal outflow 
	float NonDamagingReservoirOutflowCC = m_resndq[i]; //non-damaging outflow 
	float ReservoirRnormqMultCC = m_res_normMult[i];  //!!todo
	NormalReservoirOutflowCC = NormalReservoirOutflowCC * ReservoirRnormqMultCC;
	if (NormalReservoirOutflowCC > MinReservoirOutflowCC) {
		NormalReservoirOutflowCC = NormalReservoirOutflowCC;
	}
	else {
		NormalReservoirOutflowCC = MinReservoirOutflowCC + 0.01;
	}
	if (NormalReservoirOutflowCC < NonDamagingReservoirOutflowCC) {
		NormalReservoirOutflowCC = NormalReservoirOutflowCC;
	}
	else {
		NormalReservoirOutflowCC = NonDamagingReservoirOutflowCC - 0.01;
	}

	//New reservoir fill (fraction)
	float ReservoirFillCC = m_chSto[i] / TotalReservoirStorageM3CC;

	//below 2Lc
	float ReservoirOutflow1 = Min(MinReservoirOutflowCC, m_chSto[i] / InvDtSecDay);

	//2Lc<F<=Ln
	float DeltaO = NormalReservoirOutflowCC - MinReservoirOutflowCC;
	float DeltaLN = NormalStorageLimitCC - 2 * ConservativeStorageLimitCC;
	float ReservoirOutflow2 = MinReservoirOutflowCC + DeltaO * (ReservoirFillCC - 2 * ConservativeStorageLimitCC) / DeltaLN;

	//Ln<F<Lf
	float DeltaNFL = FloodStorageLimitCC - Normal_FloodStorageLimitCC;
	float ReservoirOutflow3a = NormalReservoirOutflowCC;
	float ReservoirOutflow3b = NormalReservoirOutflowCC + ((ReservoirFillCC - Normal_FloodStorageLimitCC) / DeltaNFL)
		* (NonDamagingReservoirOutflowCC - NormalReservoirOutflowCC);

	//F>Lf
	float temp = Min(NonDamagingReservoirOutflowCC, Max(qIn * 1.2, NormalReservoirOutflowCC));
	float ReservoirOutflow4 = Min(Max((ReservoirFillCC - FloodStorageLimitCC - 0.01) *
		TotalReservoirStorageM3CC / InvDtSecDay, NonDamagingReservoirOutflowCC), temp);

	//Reservoir outflow [m3/s] 
	rtwtr = 0.f;
	rtwtr = ReservoirOutflow1;
	if (ReservoirFillCC > 2 * ConservativeStorageLimitCC) rtwtr = ReservoirOutflow2;
	if (ReservoirFillCC > NormalStorageLimitCC) rtwtr = ReservoirOutflow3a;
	if (ReservoirFillCC > Normal_FloodStorageLimitCC) rtwtr = ReservoirOutflow3b;
	if (ReservoirFillCC > FloodStorageLimitCC) rtwtr = ReservoirOutflow4;


	temp = Min(rtwtr, Max(qIn, NormalReservoirOutflowCC));

	if ((rtwtr > 1.2 * qIn) & (rtwtr > NormalReservoirOutflowCC) & (ReservoirFillCC < FloodStorageLimitCC)) {
		rtwtr = temp;
	}
	m_qRchOut[i] = rtwtr;
	m_rteWtrOut[i] = m_qRchOut[i] * m_dt;   // m^3
	m_rteWtrOut[i] = Min(m_rteWtrOut[i], m_chSto[i]);
	m_rteWtrOut[i] = Max(m_rteWtrOut[i], m_chSto[i] - TotalReservoirStorageM3CC);
	m_qRchOut[i] = m_rteWtrOut[i] / m_dt;   //m3 s-1
	m_chSto[i] = m_chSto[i] - m_rteWtrOut[i];
	ReservoirFillCC = m_chSto[i] / TotalReservoirStorageM3CC;
	if (ReservoirFillCC <= 0.f) ReservoirFillCC = 0.f;
	m_chWtrDepth[i] = ReservoirFillCC;

	float qInSum = m_olQ2Rch[i] + qiSub + qgSub + qsUp + qiUp + qgUp;
	if (qInSum < UTIL_ZERO) {
		// In case of divided by zero.
		// m_qsRchOut[i] = 0.f;
		// m_qiRchOut[i] = 0.f;
		// m_qgRchOut[i] = 0.f;
		// m_qRchOut[i] = 0.f;
	}
	else {
		// In my opinion, these lines should use `qIn` instead of `qInSum`. By lj.
		// m_qsRchOut[i] = m_qRchOut[i] * (m_olQ2Rch[i] + qsUp) / qIn;
		// m_qiRchOut[i] = m_qRchOut[i] * (qiSub + qiUp) / qIn;
		// m_qgRchOut[i] = m_qRchOut[i] * (qgSub + qgUp) / qIn;
		m_qsRchOut[i] = m_qRchOut[i];
		m_qiRchOut[i] = 0.f;
		m_qgRchOut[i] = 0.f;
	}

	return true;
}

void LISFLOODFP_MUSK::SetSubbasins(clsSubbasins* subbsns) {
	if (m_subbasinsInfo == nullptr) {
		m_subbasinsInfo = subbsns;
		// m_nSubbasins = m_subbasinsInfo->GetSubbasinNumber(); // Set in SetValue()! lj
		m_subbasinIDs = m_subbasinsInfo->GetSubbasinIDs();
	}
}

bool LISFLOODFP_MUSK::CheckInputData() {
	/// m_date is protected variable member in base class SimulationModule.
	//CHECK_POSITIVE("IO_TEST", m_date);
	//CHECK_POSITIVE("IO_TEST", m_nCells);
	//CHECK_POINTER("IO_TEST", m_raster1D);
	//CHECK_POINTER("IO_TEST", m_raster2D);
	//CHECK_POINTER("IO_TEST", m_nSoilLyrs);

	/************************************MUSK_CH**********************************/
	CHECK_POSITIVE(MID_LISFLOODFP_MUSK, m_dt);
	CHECK_NONNEGATIVE(MID_LISFLOODFP_MUSK, m_inputSubbsnID);
	CHECK_POSITIVE(MID_LISFLOODFP_MUSK, m_nreach);
	CHECK_POSITIVE(MID_LISFLOODFP_MUSK, m_outletID);
	CHECK_NODATA(MID_LISFLOODFP_MUSK, m_Epch);
	CHECK_NODATA(MID_LISFLOODFP_MUSK, m_Bnk0);
	CHECK_NODATA(MID_LISFLOODFP_MUSK, m_Chs0_perc);
	CHECK_NODATA(MID_LISFLOODFP_MUSK, m_aBank);
	CHECK_NODATA(MID_LISFLOODFP_MUSK, m_bBank);
	CHECK_POINTER(MID_LISFLOODFP_MUSK, m_subbsnID);

	CHECK_POINTER(MID_LISFLOODFP_MUSK, m_petSubbsn);
	CHECK_POINTER(MID_LISFLOODFP_MUSK, m_gwSto);
	CHECK_POINTER(MID_LISFLOODFP_MUSK, m_olQ2Rch);
	CHECK_POINTER(MID_LISFLOODFP_MUSK, m_ifluQ2Rch);
	CHECK_POINTER(MID_LISFLOODFP_MUSK, m_gndQ2Rch);
	return true;
}

void LISFLOODFP_MUSK::InitialOutputs() {
	//CHECK_POSITIVE(MID_LISFLOODFP_MUSK, m_nreach);
	//CHECK_POSITIVE(MID_LISFLOODFP_MUSK, m_nCells);



	/************************************MUSK_CH**********************************/
	CHECK_POSITIVE(MID_LISFLOODFP_MUSK, m_nreach);
	if (nullptr != m_qRchOut) return; // DO NOT Initial Outputs repeatedly.
	if (m_mskX < 0.f) m_mskX = 0.2f;
	if (m_mskCoef1 < 0.f || m_mskCoef1 > 1.f) {
		m_mskCoef1 = 0.75f;
		m_mskCoef2 = 0.25f;
	}
	else {
		// There is no need to use mskCoef2 as input parameter.
		// Make sure m_mskCoef1 + m_mskCoef2 = 1.
		//float msk1 = m_mskCoef1 / (m_mskCoef1 + m_mskCoef2);
		//float msk2 = m_mskCoef2 / (m_mskCoef1 + m_mskCoef2);
		//m_mskCoef1 = msk1;
		//m_mskCoef2 = msk2;
	}
	m_mskCoef2 = 1.f - m_mskCoef1;

	m_flowIn = new(nothrow) float[m_nreach + 1];
	m_flowOut = new(nothrow) float[m_nreach + 1];
	m_seepage = new(nothrow) float[m_nreach + 1];
	m_charge = new(nothrow) float[m_nreach + 1];
	m_recharge = new(nothrow) float[m_nreach + 1];

	m_qRchOut = new(nothrow) float[m_nreach + 1];
	m_qsRchOut = new(nothrow) float[m_nreach + 1];
	m_qiRchOut = new(nothrow) float[m_nreach + 1];
	m_qgRchOut = new(nothrow) float[m_nreach + 1];

	m_chSto = new(nothrow) float[m_nreach + 1];
	m_chStoLastStep = new(nothrow) float[m_nreach + 1];
	m_rteWtrIn = new(nothrow) float[m_nreach + 1];
	m_rteWtrOut = new(nothrow) float[m_nreach + 1];
	m_bankSto = new(nothrow) float[m_nreach + 1];
	m_bankStoLastStep = new(nothrow) float[m_nreach + 1];
	m_Ch2GW = new(nothrow) float[m_nreach + 1];
	m_aquifer = new(nothrow) float[m_nreach + 1];

	m_chWtrDepth = new(nothrow) float[m_nreach + 1];
	m_chWtrWth = new(nothrow) float[m_nreach + 1];
	m_chBtmWth = new(nothrow) float[m_nreach + 1];
	m_chCrossArea = new(nothrow) float[m_nreach + 1];

	//ljj++
	curBasinArea = new(nothrow) float[m_nreach + 1];
	curBasinDem = new(nothrow) float[m_nreach + 1];
	m_prec = new(nothrow) float[m_nreach + 1];
	m_rrtime = new(nothrow) float[m_nreach + 1];
	m_pet = new(nothrow) float[m_nreach + 1];
	m_lakedp = new(nothrow) float[m_nreach + 1];
	m_qin1 = new(nothrow) float[m_nreach + 1];
	m_qout1 = new(nothrow) float[m_nreach + 1];
	m_temp1 = new(nothrow) float[m_nreach + 1];
	m_temp2 = new(nothrow) float[m_nreach + 1];
	m_qin = new(nothrow) float[m_nreach + 1];
	for (int i = 1; i <= m_nreach; i++) {
		m_qRchOut[i] = m_olQ2Rch[i];
		m_qsRchOut[i] = m_olQ2Rch[i];
		if (nullptr != m_ifluQ2Rch) {
			m_qRchOut[i] += m_ifluQ2Rch[i];
			m_qiRchOut[i] = m_ifluQ2Rch[i];
		}
		else {
			m_qiRchOut[i] = 0.f;
		}
		if (nullptr != m_gndQ2Rch) {
			m_qRchOut[i] += m_gndQ2Rch[i];
			m_qgRchOut[i] = m_gndQ2Rch[i];
		}
		else {
			m_qgRchOut[i] = 0.f;
		}
		m_seepage[i] = 0.f;
		m_charge[i] = 0.f;
		m_recharge[i] = 0.f;
		m_bankSto[i] = m_Bnk0 * m_chLen[i];
		m_bankStoLastStep[i] = m_bankSto[i];
		m_chBtmWth[i] = ChannleBottomWidth(m_chWth[i], m_chSideSlope[i], m_chDepth[i]);
		m_chCrossArea[i] = ChannelCrossSectionalArea(m_chBtmWth[i], m_chDepth[i], m_chSideSlope[i]);
		m_chWtrDepth[i] = m_chDepth[i] * m_Chs0_perc;
		m_chWtrWth[i] = m_chBtmWth[i] + 2.f * m_chSideSlope[i] * m_chWtrDepth[i];
		m_chSto[i] = m_chLen[i] * m_chWtrDepth[i] * (m_chBtmWth[i] + m_chSideSlope[i] * m_chWtrDepth[i]);
		m_flowIn[i] = m_chSto[i];
		m_flowOut[i] = m_chSto[i];
		m_rteWtrIn[i] = 0.f;
		m_rteWtrOut[i] = 0.f;
		//ljj++
		if (m_islake[i] == 1) {
			m_lakedpini[i] = pow(m_lakevol[i] * 1.e-9f / (m_A_Va[i]), 1.0 / (m_A_Vb[i])); //初值改了
			m_lakedp[i] = m_lakedpini[i]; //初值改了
		}
		curBasinArea[i] = 0.f;
		curBasinDem[i] = 0.f;
		m_prec[i] = 0.f;
		m_rrtime[i] = 0.f;
		m_pet[i] = 0.f;
		m_qin1[i] = 0.f;
		m_qout1[i] = 0.f;
		m_temp1[i] = 0.f;
		m_temp2[i] = 0.f;
		m_Ch2GW[i] = 0.f;
		m_aquifer[i] = 0.f;
		m_qin[i] = 0.f;
		if (m_islake[i] == 1) m_chSto[i] = m_lakevol[i];
		if (m_isres[i] == 1) m_chSto[i] = m_lakevol[i];
	}
	/// initialize point source loadings
	if (nullptr == m_ptSub) {
		Initialize1DArray(m_nreach + 1, m_ptSub, 0.f);
	}
	if (m_T_LKWB == nullptr) Initialize2DArray(m_nreach + 1, 7, m_T_LKWB, 0.f);
	if (nullptr == m_lakepcp) {
		Initialize1DArray(m_nreach + 1, m_lakepcp, 0.f);
		Initialize1DArray(m_nreach + 1, m_lakeperc, 0.f);
	}
	/************************************LISFLOODFP**********************************/
	//char couplingConfigFile[255] = "F:\\BasinFloodData\\BasinFloodData1726898305348_250m\\prepdata\\Basin\\lisfloodfp\\cali_test\\couplingConfig.txt";
	char couplingConfigFile[255] = "F:\\BasinFloodData\\BasinFloodData1729687482509\\prepdata\\Basin\\lisfloodfp\\couplingConfig.txt";
	parseCouplingFile(couplingConfigFile, lfpSetFirst, lfpSetOther, coupling_map, ptrs_map);
	for (auto it = m_rteLyrs.begin(); it != m_rteLyrs.end(); ++it) {
		int reachNum = CVT_INT(it->second.size());
		size_t errCount = 0;
		// the size of m_rteLyrs (map) is equal to the maximum stream order
	//#pragma omp parallel for reduction(+:errCount)
		for (int i = 0; i < reachNum; i++) {
			int reachIndex = it->second[i]; // index in the array, i.e., subbasinID
			if (m_inputSubbsnID == 0 || m_inputSubbsnID == reachIndex) {
				// LFP subbasin
				if (lfpSetFirst.count(reachIndex))
				{
					LISFLOODFPPtrs& LFPPtrs = ptrs_map[reachIndex];
					LisFloodFP_Initilize(LFPPtrs.argc, LFPPtrs.argv, &LFPPtrs.Arrptr, &LFPPtrs.FpsPtr, &LFPPtrs.Fnameptr, &LFPPtrs.Statesptr, &LFPPtrs.Parptr, &LFPPtrs.Solverptr, &LFPPtrs.Poisptr, &LFPPtrs.BCptr, &LFPPtrs.Stageptr,
						&LFPPtrs.SGCptr, &LFPPtrs.Damptr, &LFPPtrs.ChannelSegmentsVecPtr, &LFPPtrs.LFPContextPtr, &(coupling_map[reachIndex]), &LFPPtrs.Super_linksptr, LFPPtrs.tmpFileNamePtr, LFPPtrs.tmpSysCmdPtr);


				}
			}

		}
	}
	cout << "Finish Lisfloodfp initilization." << endl;


}

int LISFLOODFP_MUSK::Execute() {


	//check the data
	CheckInputData();
	if (ptrs_map.empty())
	{
		InitialOutputs();
	}
	/************************************MUSK_CH**********************************/

	/// load point source water volume from m_ptSrcFactory
	PointSourceLoading();
	//ljj++
	for (auto id = m_subbasinIDs.begin(); id != m_subbasinIDs.end(); ++id) {
		Subbasin* sub = m_subbasinsInfo->GetSubbasinByID(*id);
		int curCellsNum = sub->GetCellCount();
		int* curCells = sub->GetCells();
		m_prec[*id] = 0.f;
		m_rrtime[*id] = 0.f;
		m_pet[*id] = 0.f;
		m_temp1[*id] = 0.f;
		m_temp2[*id] = 0.f;
		curBasinArea[*id] = 0.f;
		curBasinDem[*id] = 0.f;

		float total_area = 0.f;
		if (m_islake[*id] == 1 || m_isres[*id] == 1) {
			for (int i = 0; i < curCellsNum; i++) {
				int index = curCells[i];
				total_area += m_area[index];
				m_prec[*id] += m_netPcp[index] / 1000.f* m_area[index]; //m3
				m_pet[*id] += m_PET[index] / 1000.f* m_area[index]; //m3
				curBasinArea[*id] += m_area[index];
			}
			m_prec[*id] = m_prec[*id] / total_area * m_lakearea[*id] / m_dt;  //m3/s
			m_pet[*id] = m_pet[*id] / total_area * m_lakearea[*id] / m_dt;
		}
		for (int i = 0; i < curCellsNum; i++) {
			int index = curCells[i];
			total_area += m_area[index];
			m_temp1[*id] += m_area[index];
			//CVT_INT(m_maxSoilLyrs-1)
			//m_temp2[*id]+= m_soilTempprofile[index][CVT_INT(m_maxSoilLyrs)-1]* m_area[index];
			m_temp2[*id] += m_slope[index] * m_area[index];
			curBasinDem[*id] += m_dem[index] * m_area[index];
			curBasinArea[*id] += m_area[index];
		}
		m_temp1[*id] = m_temp1[*id] / total_area;
		m_temp2[*id] = m_temp2[*id] / total_area;
		curBasinDem[*id] = curBasinDem[*id] / total_area;
	}
	for (auto it = m_rteLyrs.begin(); it != m_rteLyrs.end(); ++it) {
		// There are not any flow relationship within each routing layer.
		// So parallelization can be done here.
		int reachNum = CVT_INT(it->second.size());
		size_t errCount = 0;
		// the size of m_rteLyrs (map) is equal to the maximum stream order
//#pragma omp parallel for reduction(+:errCount)
		for (int i = 0; i < reachNum; i++) {
			int reachIndex = it->second[i]; // index in the array, i.e., subbasinID
			if (m_inputSubbsnID == 0 || m_inputSubbsnID == reachIndex) {
				// LFP subbasin
				if (lfpSetFirst.count(reachIndex))
				{
					ptrs_map[reachIndex].counter++;
					for (auto& pair : coupling_map[reachIndex].seims_up_map) {
						SeimsUpstream& upstream = pair.second;
						int upRchID = upstream.seims_id;
						float qsUp = m_qsRchOut[upRchID];
						float qiUp = m_qiRchOut[upRchID];
						float qgUp = m_qgRchOut[upRchID];
						upstream.qIn = (qsUp + qiUp + qgUp);
						// test
						//if (upRchID == 16)
						//{
						//	upstream.qIn = (qsUp + qiUp + qgUp)*5000.0;
						//}
						//else {
						//	upstream.qIn = (qsUp + qiUp + qgUp)*50.0;
						//}

					}
					LISFLOODFPPtrs& LFPPtrs = ptrs_map[reachIndex];
					LISFLOODFPContext* LFPContextPtr = &LFPPtrs.LFPContextPtr;
					int seims_cur_step_start_timestamp = LFPContextPtr->seims_begin_timestamp + (ptrs_map[reachIndex].counter - 1) * m_dt;
					int seims_cur_step_end_timestamp = LFPContextPtr->seims_begin_timestamp + ptrs_map[reachIndex].counter * m_dt;
					int current_timestamp;
					if (seims_cur_step_start_timestamp < LFPContextPtr->rain_end_timestamp && seims_cur_step_end_timestamp > LFPContextPtr->rain_begin_timestamp)
					{
						updateCurrentTimestamp(&current_timestamp, LFPContextPtr);
						// The simulation starts at the begin time of lisfloodfp, but it must end at the end of a SEIMS time step.
						// This means lisfloodfp may run slightly longer than expected if its end time is not aligned with SEIMS.
#pragma omp parallel default(shared)
						{
							while (current_timestamp >= seims_cur_step_start_timestamp && current_timestamp <= seims_cur_step_end_timestamp) {
								Fast_RunStep(&LFPPtrs.Arrptr, &LFPPtrs.FpsPtr, &LFPPtrs.Fnameptr, &LFPPtrs.Statesptr, &LFPPtrs.Parptr, &LFPPtrs.Solverptr, &LFPPtrs.Poisptr, &LFPPtrs.SGCptr, &LFPPtrs.Damptr, &LFPPtrs.Locptr, LFPContextPtr, &LFPPtrs.Super_linksptr, &coupling_map[reachIndex]);
								updateCurrentTimestamp(&current_timestamp, LFPContextPtr);
							}
						}
						// sum and average
						NUMERIC_TYPE qOutOneSeimsStepSum = accumulate(coupling_map[reachIndex].qOutList.begin(), coupling_map[reachIndex].qOutList.end(), static_cast<NUMERIC_TYPE>(0));
						/// for test
						//NUMERIC_TYPE qOutOneSeimsStepSum = 864000.0;
						int lfpSubbasinNum = coupling_map[reachIndex].lfp_ids.size();
						// if config is  16:06601200,18:06602400-16,15,14,-13, means lfp's subbasin contains 16,15,14 series, then qout of 16 and 15 will be zero, qout of 16(most downstream in the three) will be recorded and set as input of 13   
						for (int i = 0; i < lfpSubbasinNum - 1; i++)
						{
							int subbasinId = coupling_map[reachIndex].lfp_ids[i];
							m_qsRchOut[subbasinId] = 0.0;
							m_qRchOut[subbasinId] = 0.0;
						}
						int outSubbasinId = coupling_map[reachIndex].lfp_ids[lfpSubbasinNum - 1];
						coupling_map[reachIndex].qOutOneSeimsStep = qOutOneSeimsStepSum / m_dt;   // m3/s
						m_qsRchOut[outSubbasinId] = coupling_map[reachIndex].qOutOneSeimsStep;
						m_qRchOut[outSubbasinId] = coupling_map[reachIndex].qOutOneSeimsStep;
						coupling_map[reachIndex].qOutList.clear();

					}
				}
				else if (lfpSetOther.count(reachIndex)) {
					continue;
				}
				else {
					// for OpenMP version, all reaches will be executed,
					// for MPI version, only the current reach will be executed.
					if (m_islake[reachIndex] == 1) {
						if (!LakeBudget(reachIndex)) {
							errCount++;
						}
					}
					else if (m_isres[reachIndex] == 1) {
						if (!ResBudget(reachIndex)) {
							errCount++;
						}
					}
					else {
						if (!ChannelFlow(reachIndex)) {
							errCount++;
						}
					}
				}

			}
		}
		if (errCount > 0) {
			throw ModelException(MID_LISFLOODFP_MUSK, "Execute", "Error occurred!");
		}
	}
	/************************************End MUSK_CH**********************************/

	return 0;
}



void LISFLOODFP_MUSK::InitializeLisfloodFP() {

}

void LISFLOODFP_MUSK::RunCalculation() {

}


void LISFLOODFP_MUSK::updateCurrentTimestamp(int * current_timestamp, LISFLOODFPContext* LFPContextPtr) {
	*current_timestamp = LFPContextPtr->curr_time + LFPContextPtr->rain_begin_timestamp;
}

void LISFLOODFP_MUSK::PointSourceLoading() {
	/// load point source water discharge (m3/s) on current day from Scenario
	for (auto it = m_ptSrcFactory.begin(); it != m_ptSrcFactory.end(); ++it) {
		/// reset point source loading water to 0.f
		for (int i = 0; i <= m_nreach; i++) {
			m_ptSub[i] = 0.f;
		}
		//cout<<"unique Point Source Factory ID: "<<it->first<<endl;
		vector<int>& ptSrcMgtSeqs = it->second->GetPointSrcMgtSeqs();
		map<int, PointSourceMgtParams *>& pointSrcMgtMap = it->second->GetPointSrcMgtMap();
		vector<int>& ptSrcIDs = it->second->GetPointSrcIDs();
		map<int, PointSourceLocations *>& pointSrcLocsMap = it->second->GetPointSrcLocsMap();
		// 1. looking for management operations from m_pointSrcMgtMap
		for (auto seqIter = ptSrcMgtSeqs.begin(); seqIter != ptSrcMgtSeqs.end(); ++seqIter) {
			PointSourceMgtParams* curPtMgt = pointSrcMgtMap.at(*seqIter);
			// 1.1 If current day is beyond the date range, then continue to next management
			if (curPtMgt->GetStartDate() != 0 && curPtMgt->GetEndDate() != 0) {
				if (m_date < curPtMgt->GetStartDate() || m_date > curPtMgt->GetEndDate()) {
					continue;
				}
			}
			// 1.2 Otherwise, get the water volume
			float per_wtrVol = curPtMgt->GetWaterVolume(); /// m3/'size'/day
			// 1.3 Sum up all point sources
			for (auto locIter = ptSrcIDs.begin(); locIter != ptSrcIDs.end(); ++locIter) {
				if (pointSrcLocsMap.find(*locIter) != pointSrcLocsMap.end()) {
					PointSourceLocations* curPtLoc = pointSrcLocsMap.at(*locIter);
					int curSubID = curPtLoc->GetSubbasinID();
					m_ptSub[curSubID] += per_wtrVol * curPtLoc->GetSize() / 86400.f; /// m3/'size'/day ==> m3/s
				}
			}
		}
	}
}

void LISFLOODFP_MUSK::SetValueByIndex(const char* key, const int index, const float value) {
	if (m_inputSubbsnID == 0) return;           // Not for omp version
	if (index <= 0 || index > m_nreach) return; // index should belong 1 ~ m_nreach
	if (nullptr == m_qRchOut) InitialOutputs();
	string sk(key);
	/// Set single value of array1D of current subbasin
	/// IN/OUTPUT variables
	if (StringMatch(sk, VAR_QRECH)) m_qRchOut[index] = value;
	else if (StringMatch(sk, VAR_QS)) m_qsRchOut[index] = value;
	else if (StringMatch(sk, VAR_QI)) m_qiRchOut[index] = value;
	else if (StringMatch(sk, VAR_QG)) m_qgRchOut[index] = value;
	else {
		throw ModelException(MID_LISFLOODFP_MUSK, "SetValueByIndex", "Parameter " + sk + " does not exist.");
	}
}

void LISFLOODFP_MUSK::parseCouplingFile(
	const string& filepath,
	unordered_set<int>& lfpSetFirst,
	unordered_set<int>& lfpSetOther,
	unordered_map<int, LfpCouplingInfo>& coupling_map,
	unordered_map<int, LISFLOODFPPtrs>& ptrs_map
) {
	ifstream infile(filepath);
	if (!infile.is_open()) {
		cerr << "无法打开文件：" << filepath << endl;
		return;
	}

	string line;
	while (getline(infile, line)) {
		// 去除前后空白
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		// 跳过空行和注释
		if (line.empty() || line[0] == '#') continue;

		// 按 '-' 分为：上游inflows、LFP子流域组、下游SEIMS
		vector<string> segments;
		stringstream ss(line);
		string segment;
		while (getline(ss, segment, '-')) {
			segments.push_back(segment);
		}

		if (segments.size() != 3) {
			cerr << "格式错误（应为 上游,上游 - LFP组 - 下游）：" << line << endl;
			continue;
		}

		// 1. 上游 inflow 解析（多个 inflow 用 ',' 分隔）
		map<string, SeimsUpstream> seims_up_map;
		stringstream upstream_ss(segments[0]);
		string inflow_item;
		while (getline(upstream_ss, inflow_item, ',')) {
			auto colon_pos = inflow_item.find(':');
			if (colon_pos == string::npos) {
				cerr << "上游 inflow 缺少 ':' 分隔符: " << inflow_item << endl;
				continue;
			}

			int seims_id = stoi(inflow_item.substr(0, colon_pos));
			string inflow_name_str = inflow_item.substr(colon_pos + 1);

			//char* inflow_name_cstr = new char[inflow_name_str.length() + 1];
			//std::strcpy(inflow_name_cstr, inflow_name_str.c_str());

			SeimsUpstream upstream;
			upstream.seims_id = seims_id;
			upstream.qIn = 0.0;
			upstream.ws_index = 0;
			upstream.ps_index = 0;
			upstream.ps_x = 0;
			upstream.ps_y = 0;
			upstream.grid_index = 0;

			seims_up_map[inflow_name_str] = upstream;
		}

		// 2. LFP 子流域组（用 ',' 分隔）
		vector<int> lfp_ids;
		stringstream lfp_ss(segments[1]);
		string id_token;
		while (getline(lfp_ss, id_token, ',')) {
			try {
				lfp_ids.push_back(stoi(id_token));
			}
			catch (...) {
				cerr << "子流域ID解析失败：" << id_token << endl;
			}
		}

		if (lfp_ids.empty()) {
			cerr << "LFP子流域列表为空：" << line << endl;
			continue;
		}

		int head_id = lfp_ids[0];
		lfpSetFirst.insert(head_id);

		for (size_t i = 1; i < lfp_ids.size(); ++i) {
			lfpSetOther.insert(lfp_ids[i]);
		}

		// 3. 下游 SEIMS 子流域 ID
		int seims_down_id;
		try {
			seims_down_id = stoi(segments[2]);
		}
		catch (...) {
			cerr << "下游SEIMS ID解析失败：" << segments[2] << endl;
			continue;
		}

		// 4. 写入 coupling_map
		LfpCouplingInfo info;
		info.seims_up_map = seims_up_map;
		info.seims_down_id = seims_down_id;
		info.lfp_ids = lfp_ids;
		coupling_map[head_id] = info;


		// 5. initialize LFP's pointers
		LISFLOODFPPtrs LFPPtrs;

		LFPPtrs.argc = 2;
		LFPPtrs.argv[0] = "-v";
		LFPPtrs.argv[1] = "F:\\BasinFloodData\\BasinFloodData1729687482509\\prepdata\\Basin\\lisfloodfp\\test-seims.par";
		LFPPtrs.counter = 0;
		//LFPPtrs.Arrptr = Arrays();
		//LFPPtrs.FpsPtr = Files();
		//LFPPtrs.Fnameptr = Fnames();
		//LFPPtrs.Statesptr = States();
		//LFPPtrs.Parptr = Pars();
		//LFPPtrs.Solverptr = Solver();
		//LFPPtrs.Poisptr = Pois();
		//LFPPtrs.BCptr = BoundCs();
		//LFPPtrs.Stageptr = Stage();
		//LFPPtrs.SGCptr = SGCprams();
		//LFPPtrs.Damptr = DamData();
		//LFPPtrs.Locptr = Stage();
		//LFPPtrs.LFPContextPtr = LISFLOODFPContext();
		//LFPPtrs.Super_linksptr = SuperGridLinksList();

		memset(&LFPPtrs.Arrptr, 0, sizeof(Arrays));
		memset(&LFPPtrs.FpsPtr, 0, sizeof(Files));
		memset(&LFPPtrs.Fnameptr, 0, sizeof(Fnames));
		memset(&LFPPtrs.Statesptr, 0, sizeof(States));
		memset(&LFPPtrs.Parptr, 0, sizeof(Pars));
		memset(&LFPPtrs.Solverptr, 0, sizeof(Solver));
		memset(&LFPPtrs.Poisptr, 0, sizeof(Pois));
		memset(&LFPPtrs.BCptr, 0, sizeof(BoundCs));
		memset(&LFPPtrs.Stageptr, 0, sizeof(Stage));
		memset(&LFPPtrs.SGCptr, 0, sizeof(SGCprams));
		memset(&LFPPtrs.Damptr, 0, sizeof(DamData));
		memset(&LFPPtrs.Locptr, 0, sizeof(Stage));
		memset(&LFPPtrs.LFPContextPtr, 0, sizeof(LISFLOODFPContext));
		memset(&LFPPtrs.Super_linksptr, 0, sizeof(SuperGridLinksList));

		ptrs_map[head_id] = LFPPtrs;
	}
}





