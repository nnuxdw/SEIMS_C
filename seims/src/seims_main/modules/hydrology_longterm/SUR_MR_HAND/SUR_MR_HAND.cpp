#include "SUR_MR_HAND.h"
//#define DEBUG_SUR_MR_HAND
//// 只输出18号子流域逐日汇总
//#define DEBUG_SUR_MR_HAND_18
#include <set>
#include "text.h"
#include <iomanip>
#include <ctime>
using namespace std;

SUR_MR_HAND::SUR_MR_HAND() :
    m_dt(-1), m_nCells(-1), m_netPcp(nullptr), m_potRfCoef(nullptr), m_outletID(-1), m_nreach(-1),
    m_maxSoilLyrs(-1), m_nSoilLyrs(nullptr),
    m_soilFC(nullptr), m_soilSat(nullptr), m_soilSumSat(nullptr), m_initSoilWtrStoRatio(nullptr),
    m_rfExp(NODATA_VALUE), m_maxPcpRf(NODATA_VALUE), m_deprSto(nullptr), m_meanTemp(nullptr),
    m_soilFrozenTemp(NODATA_VALUE), m_soilFrozenWtrRatio(NODATA_VALUE), m_soilTemp(nullptr),
    m_potVol(nullptr), m_impndTrig(nullptr),
    m_exsPcp(nullptr), m_infil(nullptr), m_soilWtrSto(nullptr), m_soilWtrStoPrfl(nullptr),
    //ljj++
    m_soilIceSto(nullptr),m_soilIceStoPrfl(nullptr),m_soilPor(nullptr),m_soilThk(nullptr),
    m_dem(nullptr),m_landUse(nullptr),m_soilAWC(nullptr),m_rchID(nullptr),m_pcp(nullptr),m_lakesto(nullptr),
    m_pet(nullptr),m_soilFrozenTemp_1d(nullptr),
	//xdw++
	m_soilFCDepth(nullptr), m_soilPorDepth(nullptr), m_handWtrDep(nullptr), m_subbsnID(nullptr),  m_handArea(nullptr),
	// xiaodw ++
	m_HAND_Subbasin(nullptr), m_HAND_Flood_Level(nullptr), m_HAND_LevelDepth(nullptr),
	m_HAND_SumArea(nullptr), m_HAND_SumVolume(nullptr), m_HAND_AvgDepth(nullptr),
	m_HAND_AccVolume(nullptr), m_HAND_LowerAccDepthFlat(nullptr), m_HAND_LowerAccDepthLen(nullptr),
	m_HAND_Infil(nullptr), handWtrDepAftInfil(nullptr), m_soilWtrStoBfe(nullptr), m_HAND_Runoff_Perc(nullptr),
	m_alpha(nullptr)
    {
}

SUR_MR_HAND::~SUR_MR_HAND() {
    if (m_exsPcp != nullptr) Release1DArray(m_exsPcp);
    if (m_infil != nullptr) Release1DArray(m_infil);
    if (m_soilWtrSto != nullptr) Release2DArray(m_nCells, m_soilWtrSto);
    if (m_soilWtrStoPrfl != nullptr) Release1DArray(m_soilWtrStoPrfl);
	//if (m_soilIceStoPrfl != nullptr) Release1DArray(m_soilIceStoPrfl);   // xiaodw comment, don't need soil temperature now
	if (m_lakesto != nullptr) Release1DArray(m_lakesto);
	if (m_HAND_Infil != nullptr) Release1DArray(m_HAND_Infil);
	if (handWtrDepAftInfil != nullptr) Release1DArray(handWtrDepAftInfil);
	if (m_soilWtrStoBfe != nullptr) Release2DArray(m_nCells, m_soilWtrStoBfe);
	if (m_HAND_Runoff_Perc != nullptr) Release1DArray(m_HAND_Runoff_Perc);
	if (m_alpha != nullptr) Release1DArray(m_alpha);
	
}

bool SUR_MR_HAND::CheckInputData() {
    CHECK_POSITIVE(MID_SUR_MR_HAND, m_date);
    CHECK_POSITIVE(MID_SUR_MR_HAND, m_dt);
    CHECK_POSITIVE(MID_SUR_MR_HAND, m_nCells);
    CHECK_NODATA(MID_SUR_MR_HAND, m_soilFrozenTemp);
    CHECK_NODATA(MID_SUR_MR_HAND, m_rfExp);
    CHECK_NODATA(MID_SUR_MR_HAND, m_maxPcpRf);
    CHECK_NODATA(MID_SUR_MR_HAND, m_soilFrozenWtrRatio);
    CHECK_POINTER(MID_SUR_MR_HAND, m_initSoilWtrStoRatio);
    CHECK_POINTER(MID_SUR_MR_HAND, m_potRfCoef);
    //CHECK_POINTER(MID_SUR_MR_HAND, m_soilFC);
    CHECK_POINTER(MID_SUR_MR_HAND, m_meanTemp);
	CHECK_POINTER(MID_SUR_MR_HAND, m_soilTemp);               // xiaodw comment, don't need soil temperature now
	CHECK_POINTER(MID_SUR_MR_HAND, m_soilFrozenTemp_1d);      // lj
	CHECK_POINTER(MID_SUR_MR_HAND, m_netPcp);
    CHECK_POINTER(MID_SUR_MR_HAND, m_deprSto);
	CHECK_POINTER(MID_SUR_MR_HAND, m_handWtrDep);
    return true;
}

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <new>

static inline bool IsNoData(float v, float nodata) {
	// nodata 约定为 -9999，一般不会出现 NaN，但一起兜底
	return std::isnan(v) || std::fabs(v - nodata) < 1e-6f;
}

void SUR_MR_HAND::LoadHandLevelsFromArrays(
	int cellsNum,
	int flatLen,
	std::vector<Hand>& m_Hands,
	float nodata /*= -9999.0f*/,
	bool buildHandIds /*= false*/
) {
	// 1) 指针校验
	if (!m_HAND_Subbasin || !m_HAND_Flood_Level || !m_HAND_LevelDepth ||
		!m_HAND_SumArea || !m_HAND_SumVolume || !m_HAND_AvgDepth ||
		!m_HAND_AccVolume || !m_HAND_LowerAccDepthFlat || !m_HAND_LowerAccDepthLen) {
		std::cerr << "[ERROR] HAND arrays not loaded (one or more pointers are null)." << std::endl;
		return;
	}
	if (cellsNum <= 0) {
		std::cerr << "[ERROR] cellsNum <= 0" << std::endl;
		return;
	}

	// 2) 第一遍：统计 max_sbid、每个 sbid 有哪些 level（用于 n_levels）
	int max_sbid = -1;
	std::map<int, std::set<int>> subbasinLevels; // sbid -> unique levels

	for (int i = 0; i < cellsNum; ++i) {
		float sbv = m_HAND_Subbasin[i];
		float levv = m_HAND_Flood_Level[i];
		if (IsNoData(sbv, nodata) || IsNoData(levv, nodata)) continue;

		int sbid = static_cast<int>(sbv);
		int lev = static_cast<int>(levv);
		if (sbid < 0 || lev < 0) continue;

		max_sbid = MAX(max_sbid, sbid);
		subbasinLevels[sbid].insert(lev);
	}

	if (max_sbid < 0) {
		std::cerr << "[WARN] No valid HAND records found in arrays." << std::endl;
		return;
	}

	// 3) resize m_Hands
	if (static_cast<int>(m_Hands.size()) <= max_sbid) {
		m_Hands.resize(max_sbid + 1);
	}

	// 4) 初始化每个 subbasin 的 n_levels，并确保 levels vector 至少能装下最大 level
	for (const auto& kv : subbasinLevels) {
		int sbid = kv.first;
		const auto& levSet = kv.second;

		m_Hands[sbid].n_levels = static_cast<int>(levSet.size());

		int maxLevInSb = (levSet.empty() ? -1 : *levSet.rbegin());
		if (maxLevInSb >= 0 && static_cast<int>(m_Hands[sbid].levels.size()) <= maxLevInSb) {
			m_Hands[sbid].levels.resize(maxLevInSb + 1);
		}
	}

	// 5) 第二遍：逐条写入 level 字段，并还原 LowerAccDepth
	int flatPos = 0;

	// 如果你要构造 handIds：先收集，再一次性 new
	std::map<std::pair<int, int>, std::vector<int>> idsTmp;

	for (int i = 0; i < cellsNum; ++i) {
		float sbv = m_HAND_Subbasin[i];
		float levv = m_HAND_Flood_Level[i];
		if (IsNoData(sbv, nodata) || IsNoData(levv, nodata)) continue;

		int sbid = static_cast<int>(sbv);
		int lev = static_cast<int>(levv);
		if (sbid < 0 || lev < 0) continue;

		Hand& hand = m_Hands[sbid];
		if (lev >= static_cast<int>(hand.levels.size())) {
			hand.levels.resize(lev + 1);
		}
		Level& level = hand.levels[lev];

		// ---- 基本字段（按 nodata 保护）----
		if (!IsNoData(m_HAND_LevelDepth[i], nodata))  level.m_levelDepth = m_HAND_LevelDepth[i];
		if (!IsNoData(m_HAND_SumArea[i], nodata))     level.m_levelSumArea = m_HAND_SumArea[i];
		if (!IsNoData(m_HAND_SumVolume[i], nodata))   level.m_levelSumVol = static_cast<double>(m_HAND_SumVolume[i]);
		if (!IsNoData(m_HAND_AvgDepth[i], nodata))    level.m_levelAvgDepth = m_HAND_AvgDepth[i];
		if (!IsNoData(m_HAND_AccVolume[i], nodata))   level.m_levelAccVol = static_cast<double>(m_HAND_AccVolume[i]);

		// ---- LowerAccDepth：用 Len + Flat 还原 ----
		float lenf = m_HAND_LowerAccDepthLen[i];
		int L = 0;
		if (!IsNoData(lenf, nodata) && lenf > 0.0f) {
			L = static_cast<int>(std::round(lenf));
		}

		if (L > 0) {
			if (flatPos + L > flatLen) {
				std::cerr << "[ERROR] LowerAccDepthFlat overflow: flatPos=" << flatPos
					<< ", need=" << L << ", flatLen=" << flatLen << std::endl;
				return;
			}

			// 释放旧内存（避免重复加载时泄漏）
			if (level.m_levelLowerAccDepth != nullptr) {
				delete[] level.m_levelLowerAccDepth;
				level.m_levelLowerAccDepth = nullptr;
			}

			level.m_levelLowerAccDepth = new(std::nothrow) float[L];
			if (!level.m_levelLowerAccDepth) {
				std::cerr << "[ERROR] new failed for m_levelLowerAccDepth, L=" << L << std::endl;
				return;
			}

			for (int k = 0; k < L; ++k) {
				level.m_levelLowerAccDepth[k] = m_HAND_LowerAccDepthFlat[flatPos + k];
			}
			flatPos += L;

			// 强烈建议：在 Level 里保存长度（你如果没有这个字段，请加上）
			// level.m_levelLowerAccDepthLen = L;
		}

		// ---- (可选) 构造 handIds：这里用 “数组下标 i” 当作 ID ----
		// 如果你有真实 HRU_ID 数组（例如 m_HAND_HRU_ID[i]），把 i 换成真实值即可。
		if (buildHandIds) {
			idsTmp[{sbid, lev}].push_back(i);
		}
	}

	// 6) 如果需要 handIds：统一分配、写入
	if (buildHandIds) {
		for (auto& kv : idsTmp) {
			int sbid = kv.first.first;
			int lev = kv.first.second;
			auto& ids = kv.second;

			Level& level = m_Hands[sbid].levels[lev];

			// 释放旧 handIds
			if (level.handIds != nullptr) {
				delete[] level.handIds;
				level.handIds = nullptr;
			}

			level.m_levelHandNum = static_cast<int>(ids.size());
			if (level.m_levelHandNum > 0) {
				level.handIds = new(std::nothrow) int[level.m_levelHandNum];
				if (!level.handIds) {
					std::cerr << "[ERROR] new failed for level.handIds, n=" << level.m_levelHandNum << std::endl;
					return;
				}
				for (int j = 0; j < level.m_levelHandNum; ++j) {
					level.handIds[j] = ids[j];
				}
			}
		}
	}

	// 7) flatPos 校验（可选但很有用）
	if (flatPos != flatLen) {
		std::cerr << "[WARN] LowerAccDepthFlat not fully consumed: flatPos="
			<< flatPos << ", flatLen=" << flatLen << std::endl;
	}

	std::cout << "[INFO] Finished loading HAND levels from arrays. "
		<< "cellsNum=" << cellsNum << ", flatLen=" << flatLen << std::endl;
}

void SUR_MR_HAND::InitialOutputs() {
    CHECK_POSITIVE(MID_SUR_MR_HAND, m_nCells);
    // allocate the output variables
    if (nullptr == m_exsPcp) {
        Initialize1DArray(m_nCells, m_exsPcp, 0.f);
        Initialize1DArray(m_nCells, m_infil, 0.f);
        Initialize1DArray(m_nCells, m_soilWtrStoPrfl, 0.f);
        Initialize1DArray(m_nCells, m_soilIceStoPrfl, 0.f);//ljj++
		//Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
		Initialize1DArray(m_nCells, m_HAND_Infil, 0.f);//xdw++
		Initialize1DArray(m_nCells, handWtrDepAftInfil, 0.f);//xdw++
		Initialize1DArray(m_nCells, m_alpha, 0.f);//xdw++
		
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilWtrSto, NODATA_VALUE);
        Initialize1DArray(m_nCells, m_lakesto, 0.f);
		Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilFCDepth, NODATA_VALUE);  //xdw++
		Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilPorDepth, NODATA_VALUE);  //xdw++
		Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilWtrStoBfe, NODATA_VALUE);  //xdw++
		Initialize1DArray(m_nCells, m_HAND_Runoff_Perc, 0.f);//xdw++
		
		
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
                // if (m_initSoilWtrStoRatio[i] >= 0.f && m_initSoilWtrStoRatio[i] <= 1.f && m_soilFC[i][j] >= 0.f) {
                //     m_soilWtrSto[i][j] = m_initSoilWtrStoRatio[i] * m_soilFC[i][j];

                if (m_initSoilWtrStoRatio[i] >= 0.f && m_initSoilWtrStoRatio[i] <= 1.f && m_soilAWC[i][j] >= 0.f) {
                    m_soilWtrSto[i][j] = m_initSoilWtrStoRatio[i] * m_soilAWC[i][j];
                } else {
                    m_soilWtrSto[i][j] = 0.f;
                }

				//// 全饱和初始化试验：
	   //         // m_soilWtrSto 表示萎蔫点以上的土壤液态水储量；
	   //         // m_soilSat 对应 SOL_UL，即该变量允许的饱和上限。
				//if (m_soilSat[i][j] >= 0.f) {
				//	m_soilWtrSto[i][j] = m_soilSat[i][j];
				//}
				//else {
				//	m_soilWtrSto[i][j] = 0.f;
				//}

				/*if (m_initSoilWtrStoRatio[i] >= 0.f && m_initSoilWtrStoRatio[i] <= 1.f && m_soilSat[i][j] >= 0.f) {
					m_soilWtrSto[i][j] = m_initSoilWtrStoRatio[i] * m_soilSat[i][j];
				}
				else {
					m_soilWtrSto[i][j] = 0.f;
				}*/

				m_soilWtrStoBfe[i][j] = m_soilWtrSto[i][j];
				m_soilFCDepth[i][j] = m_soilFC[i][j] * m_soilThk[i][j];
				m_soilPorDepth[i][j] = m_soilPor[i][j] * m_soilThk[i][j];
                m_soilWtrStoPrfl[i] += m_soilWtrSto[i][j];
            }
        }

		int lower_flat_len = 0;
		for (int i = 0; i < m_nCells; i++)
		{
			lower_flat_len += (int)m_HAND_LowerAccDepthLen[i];
		}
		LoadHandLevelsFromArrays(m_nCells, lower_flat_len, m_Hands, NODATA_VALUE, TRUE);
    }
    /// update (sol_sumul) amount of water held in soil profile at saturation
    if (nullptr == m_soilSumSat && m_soilSat != nullptr) {
        m_soilSumSat = new(nothrow) float[m_nCells];
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            m_soilSumSat[i] = 0.f;
            for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
                m_soilSumSat[i] += m_soilSat[i][j];
            }
        }
    }
}



int SUR_MR_HAND::Execute() {

	//int SPECIFIED_ID = 342;
	int SPECIFIED_ID = -1;
	//set<int> SPECIFIED_SBID = { 1,2, 3,4, 5,6, 7 };
	set<int> SPECIFIED_SBID = { 19 };
    CheckInputData();
    InitialOutputs();
    int frez =0;
    m_maxPcpRf *= m_dt * 1.1574074074074073e-05f; /// 1. / 86400. = 1.1574074074074073e-05;

#ifdef DEBUG_SUR_MR_HAND_18

// ============================================================
// 18号子流域调试变量
// 只用于记录，不参与任何模型计算
// ============================================================

	const int dbgSbid18 = 18;

	const std::time_t dbgCurrentDate =
		static_cast<std::time_t>(m_date);

	std::tm dbgDateInfo = {};

#ifdef _WIN32
	localtime_s(&dbgDateInfo, &dbgCurrentDate);
#else
	localtime_r(&dbgCurrentDate, &dbgDateInfo);
#endif

	const int dbgYear =
		dbgDateInfo.tm_year + 1900;

	// 与 CH4_LPJGUESS 保持一致：只输出2010—2015年
	const bool dbgSelectedYear =
		dbgYear >= 2010 &&
		dbgYear <= 2015;

	char dbgDateText[16] = {};

	std::strftime(
		dbgDateText,
		sizeof(dbgDateText),
		"%Y-%m-%d",
		&dbgDateInfo
	);

	// ---------- HAND积水入渗 ----------
	double dbgHandArea_m2 = 0.0;

	double dbgHandBefore_m3 = 0.0;
	double dbgHandAfter_m3 = 0.0;
	double dbgHandInfil_m3 = 0.0;

	double dbgSoilDeficitBefore_m3 = 0.0;
	double dbgSoilGainFromHand_m3 = 0.0;

	int dbgHandCellCount = 0;
	int dbgWetHandCellCount = 0;

#endif


#ifdef DEBUG_SUR_MR_HAND
	{
		cout << "[SUR_MR_HAND_1]" << endl;
	}
#endif
	for (int sbid = 1; sbid <= m_nreach; ++sbid) {
		for (int ll = 1; ll <= m_Hands[sbid].n_levels; ll++)
		{
			for (int idx = 0; idx < m_Hands[sbid].levels[ll].m_levelHandNum; idx++)
			{
				int i = m_Hands[sbid].levels[ll].handIds[idx];
				float hand_infil_acc = 0.f;
				float handWtrDepMM = m_handWtrDep[i] * 1000.0;
				
				for (int ly = CVT_INT(m_nSoilLyrs[i]) - 1; ly >= 0; ly--) {
					//cout << "handId: " << handId << "  area: " << m_handArea[handId] * 0.000001 << endl;
					m_soilWtrStoBfe[i][ly] = m_soilWtrSto[i][ly];
					if (handWtrDepMM <= 0.000001f)
					{
						continue;
					}

					float hand_infil = 0.f;

					// lj++
					if (m_soilSat[i][ly] <= 0.f) {
						continue;
					}

					// 防止负入渗
					if (m_soilWtrSto[i][ly] >= m_soilSat[i][ly]) {
						m_soilWtrSto[i][ly] = m_soilSat[i][ly];
						continue;
					}

					if (handWtrDepMM >= m_soilSat[i][ly] - m_soilWtrSto[i][ly]) {
						hand_infil = m_soilSat[i][ly] - m_soilWtrSto[i][ly];
						m_soilWtrSto[i][ly] = m_soilSat[i][ly];
						hand_infil_acc += hand_infil;
						handWtrDepMM -= hand_infil;
					}
					else {
						hand_infil = handWtrDepMM;
						m_soilWtrSto[i][ly] += hand_infil;
						hand_infil_acc += hand_infil;
						handWtrDepMM = 0.f;
					}

					/*if (handWtrDepMM >= m_soilPor[i][ly] * m_soilThk[i][ly] - m_soilWtrSto[i][ly]) {
						hand_infil = m_soilPor[i][ly] * m_soilThk[i][ly] - m_soilWtrSto[i][ly];
						m_soilWtrSto[i][ly] = m_soilPor[i][ly] * m_soilThk[i][ly];
						hand_infil_acc += hand_infil;
						handWtrDepMM -= hand_infil;
					}
					else {
						hand_infil = handWtrDepMM;
						m_soilWtrSto[i][ly] += hand_infil;
						hand_infil_acc += hand_infil;
						handWtrDepMM = 0.f;
					}*/

					//handWtrDepAftInfil[i] = handWtrDepMM * 0.001;   // m
					if (m_handWtrDep[i] < 0.f)
					{
						cout << "Error: In SUR_MR_HAND module, m_handWtrDep is "  << m_handWtrDep[i] << " at subbasinId=" << sbid << ", i =" << i << endl;
						//exit(-1);
					}
				}
				m_HAND_Infil[i] = hand_infil_acc;    // mm
				handWtrDepAftInfil[i] = handWtrDepMM * 0.001;   // m

				//// 只检查19号子流域的1306号HAND单元
    //            // 想检查19号子流域全部单元时，删除“&& i == 1306”
				//if (sbid == 19 &&(i == 1304 ||i == 1305 ||i == 1306 ||i == 1309 ||i == 1310))
				//{
				//	int validLayerCount = 0;
				//	int saturatedLayerCount = 0;
				//	int unsaturatedLayerCount = 0;

				//	cout << endl;
				//	cout << "***********************************************" << endl;
				//	cout << "*[SUR_MR_HAND_SOIL_SATURATION]*" << endl;

				//	cout << fixed << setprecision(6);

				//	cout << " Sbid: " << sbid
				//		<< "   HandId: " << i
				//		<< "   handBefore_mm: " << m_handWtrDep[i] * 1000.0f
				//		<< "   handAfter_m: " << handWtrDepAftInfil[i]
				//		<< "   handInfil_mm: " << m_HAND_Infil[i]
				//		<< endl;

				//	cout << "---------------------------------------------------------------"
				//		<< endl;

				//	cout << left
				//		<< setw(8) << "Layer"
				//		<< setw(16) << "WtrBefore_mm"
				//		<< setw(16) << "WtrAfter_mm"
				//		<< setw(16) << "SoilSat_mm"
				//		<< setw(16) << "SatRatio"
				//		<< setw(12) << "Saturated"
				//		<< endl;

				//	cout << "---------------------------------------------------------------"
				//		<< endl;

				//	for (int ly = 0; ly < CVT_INT(m_nSoilLyrs[i]); ++ly)
				//	{
				//		if (m_soilSat[i][ly] <= 0.0f)
				//		{
				//			continue;
				//		}

				//		++validLayerCount;

				//		float saturationRatio =
				//			m_soilWtrSto[i][ly] / m_soilSat[i][ly];

				//		bool isSaturated =
				//			saturationRatio >= 0.999f;

				//		if (isSaturated)
				//		{
				//			++saturatedLayerCount;
				//		}
				//		else
				//		{
				//			++unsaturatedLayerCount;
				//		}

				//		cout << left
				//			<< setw(8) << ly
				//			<< setw(16) << m_soilWtrStoBfe[i][ly]
				//			<< setw(16) << m_soilWtrSto[i][ly]
				//			<< setw(16) << m_soilSat[i][ly]
				//			<< setw(16) << saturationRatio
				//			<< setw(12) << (isSaturated ? "YES" : "NO")
				//			<< endl;
				//	}

				//	bool allLayersSaturated =
				//		validLayerCount > 0 &&
				//		unsaturatedLayerCount == 0;

				//	cout << "---------------------------------------------------------------"
				//		<< endl;

				//	cout << " Valid layers: " << validLayerCount
				//		<< "   Saturated layers: " << saturatedLayerCount
				//		<< "   Unsaturated layers: " << unsaturatedLayerCount
				//		<< endl;

				//	cout << " Soil profile saturated: "
				//		<< (allLayersSaturated ? "YES" : "NO")
				//		<< endl;

				//	if (handWtrDepAftInfil[i] > 1.0e-6f &&
				//		!allLayersSaturated)
				//	{
				//		cout << " WARNING: Surface water exists, but soil is unsaturated!"
				//			<< endl;
				//	}

				//	cout << "***********************************************" << endl;
				//}
				
			}
		}

		
#ifdef DEBUG_SUR_MR_HAND
		for (int ll = 1; ll <= m_Hands[sbid].n_levels; ll++)
		{
			for (int idx = 0; idx < m_Hands[sbid].levels[ll].m_levelHandNum; idx++)
			{
				int i = m_Hands[sbid].levels[ll].handIds[idx];
				if (SPECIFIED_SBID.find(sbid) != SPECIFIED_SBID.end())
				{
					//cout << "******************************************" << endl;
					//cout << "*[SUR_MR_HAND_INUNDATION]* " << endl;
					cout << " Sbid: " << sbid << "   "
						<< " HandId: " << i << "   "
						<< " handWtrDep=" << m_handWtrDep[i] * 1000.0 << "   "
						<< " handWtrDepAftInfil=" << handWtrDepAftInfil[i] << "   "
						<< " hand_infil=" << m_HAND_Infil[i] << "   "
						<< " handArea=" << m_handArea[i] << "   "
						<< endl;
					for (int ly = 0; ly <= CVT_INT(m_nSoilLyrs[i]) - 1; ly++) {
						cout
							<< "Layer: " << ly << "   "
							<< " WtrStoBfe=" << m_soilWtrStoBfe[i][ly] << "   "
							<< " WtrStoAft=" << m_soilWtrSto[i][ly] << "   "
							<< endl;
					}
						

				}
			}

			
		}
#endif
	}


	// =====================================================================
// 新增：18号子流域 HAND 入渗调试汇总
// 注意：一定放在上面的 sbid 循环结束之后
//       一定放在 DEBUG_WB19 之前
//       这里只读取数据，不修改模型状态
// =====================================================================
#ifdef DEBUG_SUR_MR_HAND_18

	std::set<int> dbgVisitedHandIds;

	// ============================================================
// 逐HAND单元明细输出
// 只用于调试，不参与模型计算
// ============================================================

#ifdef _WIN32
	const char* dbgHandDetailFile = "D:/SEIMS_C/log/sur_mr_hand_sbid18_hand_detail.csv";
#else
	const char* dbgHandDetailFile = "/data/user/lijing/software/SEIMS_C/log/sur_mr_hand_sbid18_hand_detail.csv";
#endif

	bool dbgHandDetailNeedHeader = false;

	{
		std::ifstream dbgCheck(
			dbgHandDetailFile,
			std::ios::in
		);

		if (!dbgCheck.good())
		{
			dbgHandDetailNeedHeader = true;
		}
		else
		{
			dbgCheck.seekg(0, std::ios::end);

			if (dbgCheck.tellg() == 0)
			{
				dbgHandDetailNeedHeader = true;
			}

			dbgCheck.close();
		}
	}

	std::ofstream dbgHandDetailOut(
		dbgHandDetailFile,
		std::ios::out | std::ios::app
	);

	if (dbgHandDetailOut.is_open() &&
		dbgHandDetailNeedHeader)
	{
		dbgHandDetailOut
			<< "DATE"
			<< ",sbid"
			<< ",hand_id"
			<< ",area_m2"
			<< ",n_layers"

			<< ",hand_before_m"
			<< ",hand_before_mm"
			<< ",hand_before_m3"

			<< ",hand_infil_mm"
			<< ",hand_infil_m3"

			<< ",hand_after_m"
			<< ",hand_after_mm"
			<< ",hand_after_m3"

			<< ",soil_before_profile_mm"
			<< ",soil_after_hand_profile_mm"
			<< ",soil_sat_profile_mm"

			<< ",soil_deficit_before_mm"
			<< ",soil_deficit_before_m3"

			<< ",soil_gain_from_hand_mm"
			<< ",soil_gain_from_hand_m3"

			<< std::endl;
	}

	if (dbgSbid18 > 0 &&
		dbgSbid18 <= m_nreach &&
		dbgSbid18 < static_cast<int>(m_Hands.size()))
	{
		for (int ll = 1;
			ll <= m_Hands[dbgSbid18].n_levels;
			++ll)
		{
			for (int idx = 0;
				idx < m_Hands[dbgSbid18].levels[ll].m_levelHandNum;
				++idx)
			{
				const int i =
					m_Hands[dbgSbid18]
					.levels[ll]
					.handIds[idx];

				// ---------------------------------------------------------
				// HAND ID合法性检查
				// ---------------------------------------------------------
				if (i < 0 || i >= m_nCells)
				{
					continue;
				}

				// 防止同一个HAND单元被重复统计
				if (!dbgVisitedHandIds.insert(i).second)
				{
					continue;
				}

				const double area =
					static_cast<double>(m_handArea[i]);

				if (!std::isfinite(area) || area <= 0.0)
				{
					continue;
				}

				++dbgHandCellCount;
				dbgHandArea_m2 += area;


				// =========================================================
				// 1. HAND入渗前地表积水体积
				//
				// m_handWtrDep 单位：m
				// area 单位：m2
				// 所以结果单位：m3
				// =========================================================

				const double handBefore_m =
					static_cast<double>(
						m_handWtrDep[i]
						);

				dbgHandBefore_m3 +=
					handBefore_m * area;

				if (handBefore_m > 1.0e-8)
				{
					++dbgWetHandCellCount;
				}


				// =========================================================
				// 2. HAND入渗后剩余地表积水体积
				// =========================================================

				const double handAfter_m =
					static_cast<double>(
						handWtrDepAftInfil[i]
						);

				dbgHandAfter_m3 +=
					handAfter_m * area;


				// =========================================================
				// 3. HAND实际入渗量
				//
				// m_HAND_Infil 单位：mm
				// mm × 0.001 × m2 = m3
				// =========================================================

				const double handInfil_mm =
					static_cast<double>(
						m_HAND_Infil[i]
						);

				dbgHandInfil_m3 +=
					handInfil_mm
					* 0.001
					* area;


				// =========================================================
                // 4. 只检查“当天有HAND积水”的单元
                //
                // 因为只有这些单元当天才具备HAND入渗条件。
                // 否则把大量未淹水单元的土壤缺水也统计进来，
                // 会人为压低 deficit_fill_fraction。
                // =========================================================

				if (handBefore_m > 1.0e-8)
				{
					// 当前HAND单元自己的土壤剖面统计
					double cellSoilBefore_mm = 0.0;
					double cellSoilAfterHand_mm = 0.0;
					double cellSoilSat_mm = 0.0;

					double cellSoilDeficitBefore_mm = 0.0;
					double cellSoilGainFromHand_mm = 0.0;


					for (int ly = 0;
						ly < CVT_INT(m_nSoilLyrs[i]);
						++ly)
					{
						if (m_soilSat[i][ly] <= 0.0f)
						{
							continue;
						}

						const double soilBefore_mm =
							static_cast<double>(
								m_soilWtrStoBfe[i][ly]
								);

						const double soilAfterHand_mm =
							static_cast<double>(
								m_soilWtrSto[i][ly]
								);

						const double soilSat_mm =
							static_cast<double>(
								m_soilSat[i][ly]
								);


						// 整个土壤剖面的水量和饱和容量
						cellSoilBefore_mm +=
							soilBefore_mm;

						cellSoilAfterHand_mm +=
							soilAfterHand_mm;

						cellSoilSat_mm +=
							soilSat_mm;


						// 入渗前距离饱和还缺多少
						const double deficitBefore_mm =
							(std::max)(
								0.0,
								soilSat_mm - soilBefore_mm
								);

						cellSoilDeficitBefore_mm +=
							deficitBefore_mm;


						// HAND入渗实际增加多少土壤水
						const double soilGain_mm =
							soilAfterHand_mm
							- soilBefore_mm;

						cellSoilGainFromHand_mm +=
							soilGain_mm;
					}


					// -----------------------------------------------------
					// 当前HAND单元转换成体积 m3
					// -----------------------------------------------------

					const double cellHandBefore_m3 =
						handBefore_m * area;

					const double cellHandInfil_m3 =
						handInfil_mm
						* 0.001
						* area;

					const double cellHandAfter_m3 =
						handAfter_m * area;

					const double cellSoilDeficitBefore_m3 =
						cellSoilDeficitBefore_mm
						* 0.001
						* area;

					const double cellSoilGainFromHand_m3 =
						cellSoilGainFromHand_mm
						* 0.001
						* area;


					// -----------------------------------------------------
					// 保留原来的18号子流域总体统计
					// -----------------------------------------------------

					dbgSoilDeficitBefore_m3 +=
						cellSoilDeficitBefore_m3;

					dbgSoilGainFromHand_m3 +=
						cellSoilGainFromHand_m3;


					// -----------------------------------------------------
					// 写逐HAND单元明细
					// -----------------------------------------------------

					if (dbgSelectedYear &&
						dbgHandDetailOut.is_open())
					{
						dbgHandDetailOut
							<< std::setprecision(12)

							<< dbgDateText
							<< "," << dbgSbid18
							<< "," << i
							<< "," << area
							<< "," << CVT_INT(m_nSoilLyrs[i])

							<< "," << handBefore_m
							<< "," << handBefore_m * 1000.0
							<< "," << cellHandBefore_m3

							<< "," << handInfil_mm
							<< "," << cellHandInfil_m3

							<< "," << handAfter_m
							<< "," << handAfter_m * 1000.0
							<< "," << cellHandAfter_m3

							<< "," << cellSoilBefore_mm
							<< "," << cellSoilAfterHand_mm
							<< "," << cellSoilSat_mm

							<< "," << cellSoilDeficitBefore_mm
							<< "," << cellSoilDeficitBefore_m3

							<< "," << cellSoilGainFromHand_mm
							<< "," << cellSoilGainFromHand_m3

							<< std::endl;
					}
				}
			}       
		}           
	}               


	// 关闭逐HAND明细文件
	if (dbgHandDetailOut.is_open())
	{
		dbgHandDetailOut.close();
	}

#endif
	// =====================================================================
	// 新增结束
	// =====================================================================

#ifdef DEBUG_WB19
{
const int debugSubbasin = 19;

double handBeforeVol = 0.0;  // 入渗前HAND水量，m3
double handAfterVol = 0.0;  // 入渗后HAND水量，m3
double infilVol = 0.0;  // HAND入渗量，m3
double soilGainVol = 0.0;  // 土壤水增加量，m3

int cellCount = 0;
int negativeInfilCount = 0;
int negativeDepthCount = 0;
int invalidAreaCount = 0;

// 防止同一个HAND单元重复统计
std::set<int> visitedIds;

if (debugSubbasin > 0 &&
	debugSubbasin <= m_nreach &&
	debugSubbasin < static_cast<int>(m_Hands.size()))
{
	for (int ll = 1;
		ll <= m_Hands[debugSubbasin].n_levels;
		++ll)
	{
		for (int idx = 0;
			idx < m_Hands[debugSubbasin]
			.levels[ll]
			.m_levelHandNum;
			++idx)
		{
			const int i =
				m_Hands[debugSubbasin]
				.levels[ll]
				.handIds[idx];

			if (i < 0 || i >= m_nCells)
			{
				continue;
			}

			// 防止重复ID被重复累加
			if (!visitedIds.insert(i).second)
			{
				continue;
			}

			const double area =
				static_cast<double>(m_handArea[i]);

			if (!std::isfinite(area) || area <= 0.0)
			{
				++invalidAreaCount;
				continue;
			}

			++cellCount;

			// 入渗前地表水体积：m × m2 = m3
			handBeforeVol +=
				static_cast<double>(m_handWtrDep[i])
				* area;

			// 入渗后地表水体积
			handAfterVol +=
				static_cast<double>(
					handWtrDepAftInfil[i]
					)
				* area;

			// HAND入渗体积：mm × 0.001 × m2 = m3
			infilVol +=
				static_cast<double>(
					m_HAND_Infil[i]
					)
				* 0.001
				* area;

			// 土壤水增加体积
			for (int ly = 0;
				ly < CVT_INT(m_nSoilLyrs[i]);
				++ly)
			{
				soilGainVol +=
					static_cast<double>(
						m_soilWtrSto[i][ly]
						- m_soilWtrStoBfe[i][ly]
						)
					* 0.001
					* area;
			}

			if (m_HAND_Infil[i] < -1.0e-6f)
			{
				++negativeInfilCount;
			}

			if (handWtrDepAftInfil[i] < -1.0e-8f)
			{
				++negativeDepthCount;
			}
		}
	}
}

// 地表水量平衡：
// 入渗前水量 = 入渗后水量 + 入渗量
const double surfaceBalanceError =
handBeforeVol
- handAfterVol
- infilVol;

// 入渗水量应等于土壤水增加量
const double soilBalanceError =
infilVol
- soilGainVol;

const double surfaceScale =
(std::max)(
	1.0,
	(std::max)(
		std::fabs(handBeforeVol),
		std::fabs(handAfterVol + infilVol)
	)
);

const double soilScale =
(std::max)(
	1.0,
	(std::max)(
		std::fabs(infilVol),
		std::fabs(soilGainVol)
	)
);

cout << "[WB19_SUR]"
<< ",date=" << m_date
<< ",cellCount=" << cellCount

<< ",handBefore_m3=" << handBeforeVol
<< ",handAfter_m3=" << handAfterVol
<< ",infil_m3=" << infilVol
<< ",soilGain_m3=" << soilGainVol

<< ",surfaceBalanceError="
<< surfaceBalanceError

<< ",surfaceRelativeError="
<< surfaceBalanceError / surfaceScale

<< ",soilBalanceError="
<< soilBalanceError

<< ",soilRelativeError="
<< soilBalanceError / soilScale

<< ",negativeInfilCount="
<< negativeInfilCount

<< ",negativeDepthCount="
<< negativeDepthCount

<< ",invalidAreaCount="
<< invalidAreaCount

<< endl;
}
#endif

#ifdef DEBUG_SUR_MR_HAND
	{
		cout << "[SUR_MR_HAND_2]" << endl;
	}
#endif

#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
         //if(m_landUse[i] ==LANDUSE_ID_WATR && m_rchID[i]<=0.f){
         //    //坡面湖泊
         //    m_lakesto[i] += m_pcp[i];
         //    m_lakesto[i] -= m_pet[i];
         //    float m_resday = 31.f;
         //    float m_runoff = 1/m_resday  * m_lakesto[i]; // mm
         //    m_lakesto[i] -= m_runoff;
         //    m_exsPcp[i] = m_runoff;
         //    m_infil[i] = 0.f;
         //    continue;
         //}
        float hWater = 0.f;
		/// debug
		float netPcp = m_netPcp[i];
		float deprSto = m_deprSto[i];
        hWater = m_netPcp[i] + m_deprSto[i];
		//runoff percentage
		float runoffPercentage = 0.0;
		float surfq = 0.0;

		if (hWater > 0.f ) {
            /// update total soil water content
            m_soilWtrStoPrfl[i] = 0.f;
            for (int ly = 0; ly < CVT_INT(m_nSoilLyrs[i]); ly++) {
                m_soilWtrStoPrfl[i] += m_soilWtrSto[i][ly];
            }
            float smFraction = Min(m_soilWtrStoPrfl[i] / m_soilSumSat[i], 1.f);
            float alpha = 3;
            float soilIcePrfl = 0.f;
            float soilSatPrfl = 0.f;
            for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
				//soilIcePrfl += m_soilIceSto[i][k];  // xiaodw, don't need soil ice now
				soilSatPrfl += m_soilPor[i][k]*m_soilThk[i][k];
            }
            if(frez==1){
                //float newSumSat = Max( m_soilSumSat[i] - m_soilIceStoPrfl[i], 0.001f);
                //SMCMAX     POROSITY, I.E. SATURATED VALUE OF SOIL MOISTURE (VOLUMETRIC)
                float newSumSat = Max(soilSatPrfl - soilIcePrfl, 0.001f);  // m_soilSumSat[i] is not m_soilPor[i][k]
                smFraction = Min(m_soilWtrStoPrfl[i] / newSumSat, 1.f);
                alpha = m_rfExp - (m_rfExp - 1.f) * hWater / m_maxPcpRf;
                if (hWater >= m_maxPcpRf) {
                    alpha = 1.f;
                }
                //runoff percentage
                if (m_potRfCoef[i] > 0.99f ||  (m_landUse[i] == LANDUSE_ID_GLC)) {
                    runoffPercentage = 1.f;
                } else {
                    runoffPercentage = m_potRfCoef[i] * pow(smFraction, alpha);
                }
                runoffPercentage = Min(runoffPercentage,1.f);
                surfq = hWater *runoffPercentage;
                if (surfq > hWater) surfq = hWater;
                m_infil[i] = hWater - surfq;
                m_exsPcp[i] = surfq;

            }else{

				// for frozen soil, no infiltration will occur
				if (m_soilTemp[i] <= m_soilFrozenTemp_1d[i] && smFraction >= m_soilFrozenWtrRatio) {
					m_exsPcp[i] = m_netPcp[i];
					m_infil[i] = 0.f;
				}
				else {
					alpha = m_rfExp - (m_rfExp - 1.f) * hWater / m_maxPcpRf;
					if (hWater >= m_maxPcpRf) {
						alpha = 1.f;
					}

					if (m_potRfCoef[i] > 0.99f || m_landUse[i] == LANDUSE_ID_GLC) {
						runoffPercentage = 1.f;
					}
					else {
						runoffPercentage = m_potRfCoef[i] * pow(smFraction, alpha);
					}
					runoffPercentage = Min(runoffPercentage, 1.f);
					m_HAND_Runoff_Perc[i] = runoffPercentage;
					surfq = hWater * runoffPercentage;
					if (surfq > hWater) surfq = hWater;
					m_infil[i] = hWater - surfq;
					m_exsPcp[i] = surfq;
				}
			}
////                //for frozen soil, no infiltration will occur
////                //if (m_soilTemp[i] <= m_soilFrozenTemp && smFraction >= m_soilFrozenWtrRatio) {
////                if (m_soilTemp[i] <= m_soilFrozenTemp_1d[i] && smFraction >= m_soilFrozenWtrRatio) {
////                    m_exsPcp[i] = m_netPcp[i];
////                    m_infil[i] = 0.f;
////                } else {
////                    alpha = m_rfExp - (m_rfExp - 1.f) * hWater / m_maxPcpRf;
////                    if (hWater >= m_maxPcpRf) {
////                        alpha = 1.f;
////                    }
////
////                    //runoff percentage
////                    float runoffPercentage;
//////#ifndef DEBUG_SUR_MR_HAND
//////				cout << i << ": " << m_potRfCoef[i] << endl;
//////#endif
////                    if (m_potRfCoef[i] > 0.99f ||  (m_landUse[i] == LANDUSE_ID_GLC)) {
////                        runoffPercentage = 1.f;
////                    } else {
////                        runoffPercentage = m_potRfCoef[i] * pow(smFraction, alpha);
////                    }
////                    runoffPercentage = Min(runoffPercentage,1.f);
////                    float surfq = hWater * runoffPercentage;
////                    if (surfq > hWater) surfq = hWater;
////                    m_infil[i] = hWater - surfq;
////                    m_exsPcp[i] = surfq;
////
////                    /// TODO: Why calculate surfq first, rather than infiltration first?
////                    ///       I think we should calculate infiltration first, until saturation,
////                    ///       then surface runoff should be calculated. By LJ.
////                }
//
//				alpha = m_rfExp - (m_rfExp - 1.f) * hWater / m_maxPcpRf;
//				if (hWater >= m_maxPcpRf) {
//					alpha = 1.f;
//				}
//
//
//				//#ifndef DEBUG_SUR_MR_HAND
//				//				cout << i << ": " << m_potRfCoef[i] << endl;
//				//#endif
//				if (m_potRfCoef[i] > 0.99f || (m_landUse[i] == LANDUSE_ID_GLC)) {
//					runoffPercentage = 1.f;
//				}
//				else {
//					runoffPercentage = m_potRfCoef[i] * pow(smFraction, alpha);
//				}
//				runoffPercentage = Min(runoffPercentage, 1.f);
//				m_HAND_Runoff_Perc[i] = runoffPercentage;
//				surfq = hWater * runoffPercentage;
//				if (surfq > hWater) surfq = hWater;
//				m_infil[i] = hWater - surfq;
//				m_exsPcp[i] = surfq;
//            }
			m_alpha[i] = alpha;

        } else {
            m_exsPcp[i] = 0.f;
            m_infil[i] = 0.f;
        }
		m_soilWtrStoBfe[i][0] = m_soilWtrSto[i][0];
        if (m_infil[i] > 0.f) {
            m_soilWtrSto[i][0] += m_infil[i];
        }


//#ifdef DEBUG_SUR_MR_HAND
//		{
//			int sbid = CVT_INT(m_subbsnID[i]);
//			if (SPECIFIED_SBID.find(sbid) != SPECIFIED_SBID.end()) {
//				cout << " Sbid: " << sbid << "   "
//					<< " HandId: " << i << "   "
//					<< " hWater=" << hWater << "   "
//					<< " infil=" << m_infil[i] << "   "
//					<< " exsPcp=" << m_exsPcp[i] << "   "
//					<< " surfq=" << surfq << "   "
//					<< " WtrStoBfe_0=" << m_soilWtrStoBfe[i][0] << "   "
//					<< " WtrStoAft_0=" << m_soilWtrSto[i][0] << "   "
//					<< " runPerc=" << runoffPercentage << "   "
//					<< " runoffCo=" << m_potRfCoef[i] << "   "
//					<< " alpha=" << m_alpha[i] << "   "
//					<< endl;
//				// 单线程可以不每次 flush；如果想实时看，可以打开下面这一行
//				// dbg.flush();
//			}
//		}
//#endif
    }

#ifdef DEBUG_SUR_MR_HAND_18

	// ============================================================
	// DEBUG B：统计18号子流域当天的降雨产流
	//
	// 注意：
	// 前面的正常SUR_MR_HAND计算已经全部结束。
	// 这里只读取结果，不修改任何模型变量。
	// ============================================================

	double dbgSubArea_m2 = 0.0;
	double dbgActiveArea_m2 = 0.0;

	int dbgSubCellCount = 0;
	int dbgActiveCellCount = 0;


	// ---------- 当天水量输入 ----------
	double dbgNetPcp_m3 = 0.0;
	double dbgDeprSto_m3 = 0.0;
	// 所有18号子流域单元的原始 hWater = netPcp + deprSto
	double dbgHWaterRaw_m3 = 0.0;

	// 真正进入 if (hWater > 0) 产流分支的水量
	double dbgHWaterActive_m3 = 0.0;


	// ---------- 当天产流 ----------
	double dbgSurfq_m3 = 0.0;
	double dbgRainInfil_m3 = 0.0;


	// ---------- runoff percentage ----------
	double dbgRunoffPercAreaSum = 0.0;
	double dbgRunoffPercMax = 0.0;

	int dbgRunoffPercGt099Count = 0;


	// ---------- 强制100%产流 ----------
	int dbgForcedRunoffCellCount = 0;
	double dbgForcedRunoffArea_m2 = 0.0;


	// ---------- Runoff_co ----------
	double dbgRunoffCoAreaSum = 0.0;
	double dbgRunoffCoMax = -1.0e30;


	// ---------- alpha ----------
	double dbgAlphaAreaSum = 0.0;


	// ---------- soil saturation fraction ----------
	double dbgSmFractionAreaSum = 0.0;
	double dbgSmFractionMax = 0.0;


	for (int i = 0; i < m_nCells; ++i)
	{
		if (CVT_INT(m_subbsnID[i]) != dbgSbid18)
		{
			continue;
		}

		const double area =
			static_cast<double>(m_handArea[i]);

		if (!std::isfinite(area) || area <= 0.0)
		{
			continue;
		}


		++dbgSubCellCount;

		dbgSubArea_m2 += area;


		// ========================================================
		// 1. 当天进入SUR_MR_HAND的水量
		// ========================================================

		const double netPcp_mm =
			static_cast<double>(
				m_netPcp[i]
				);

		const double deprSto_mm =
			static_cast<double>(
				m_deprSto[i]
				);

		const double hWater_mm =
			netPcp_mm + deprSto_mm;


		dbgNetPcp_m3 +=
			netPcp_mm
			* 0.001
			* area;

		dbgDeprSto_m3 +=
			deprSto_mm
			* 0.001
			* area;


		// 原始 hWater，完全不筛选
		dbgHWaterRaw_m3 +=
			hWater_mm
			* 0.001
			* area;


		// 真正进入原模型 if(hWater > 0) 分支的水量
		if (hWater_mm > 0.0)
		{
			dbgHWaterActive_m3 +=
				hWater_mm
				* 0.001
				* area;
		}


		// ========================================================
		// 2. SUR_MR_HAND真实产生的地表径流和降雨入渗
		// ========================================================

		const double surfq_mm =
			static_cast<double>(
				m_exsPcp[i]
				);

		const double rainInfil_mm =
			static_cast<double>(
				m_infil[i]
				);


		dbgSurfq_m3 +=
			surfq_mm
			* 0.001
			* area;

		dbgRainInfil_m3 +=
			rainInfil_mm
			* 0.001
			* area;


		// ========================================================
		// hWater <= 0时：
		//
		// 原代码没有重新计算
		// m_HAND_Runoff_Perc 和 m_alpha。
		//
		// 所以这里直接跳过，
		// 避免把以前某一天留下来的值当成今天的值。
		//
		// 注意：
		// 我们没有修改原变量。
		// ========================================================

		if (hWater_mm <= 0.0)
		{
			continue;
		}


		++dbgActiveCellCount;

		dbgActiveArea_m2 += area;


		// ========================================================
		// 3. runoffPercentage
		// ========================================================

		const double runoffPerc =
			static_cast<double>(
				m_HAND_Runoff_Perc[i]
				);


		dbgRunoffPercAreaSum +=
			runoffPerc * area;


		dbgRunoffPercMax =
			(std::max)(
				dbgRunoffPercMax,
				runoffPerc
				);


		if (runoffPerc > 0.99)
		{
			++dbgRunoffPercGt099Count;
		}


		// ========================================================
		// 4. Runoff_co真实值
		// ========================================================

		const double runoffCo =
			static_cast<double>(
				m_potRfCoef[i]
				);


		dbgRunoffCoAreaSum +=
			runoffCo * area;


		dbgRunoffCoMax =
			(std::max)(
				dbgRunoffCoMax,
				runoffCo
				);


		// ========================================================
		// 5. 判断是否触发了原代码中的：
		//
		// if (m_potRfCoef[i] > 0.99f ||
		//     m_landUse[i] == LANDUSE_ID_GLC)
		//
		// 即强制 runoffPercentage = 1
		// ========================================================

		if (m_potRfCoef[i] > 0.99f ||
			m_landUse[i] == LANDUSE_ID_GLC)
		{
			++dbgForcedRunoffCellCount;

			dbgForcedRunoffArea_m2 += area;
		}


		// ========================================================
		// 6. alpha真实值
		// ========================================================

		const double alpha =
			static_cast<double>(
				m_alpha[i]
				);


		dbgAlphaAreaSum +=
			alpha * area;


		// ========================================================
		// 7. 当天真实用于产流计算的 smFraction
		//
		// 原代码计算 runoffPercentage 前：
		//
		// m_soilWtrStoPrfl[i] = sum(m_soilWtrSto)
		//
		// 后面虽然把 m_infil 加到第一层，
		// 但没有再次更新 m_soilWtrStoPrfl。
		//
		// 所以这里保存的仍然是当天真实用于
		// runoffPercentage计算的土壤水状态。
		// ========================================================

		double smFraction = 0.0;


		if (m_soilSumSat[i] > 1.0e-8f)
		{
			smFraction =
				static_cast<double>(
					m_soilWtrStoPrfl[i]
					/ m_soilSumSat[i]
					);


			smFraction =
				(std::max)(
					0.0,
					(std::min)(
						smFraction,
						1.0
						)
					);
		}


		dbgSmFractionAreaSum +=
			smFraction * area;


		dbgSmFractionMax =
			(std::max)(
				dbgSmFractionMax,
				smFraction
				);
	}


	// ============================================================
	// 计算面积加权平均
	// ============================================================

	double dbgRunoffPercMean = 0.0;
	double dbgRunoffCoMean = 0.0;
	double dbgAlphaMean = 0.0;
	double dbgSmFractionMean = 0.0;

	double dbgForcedRunoffAreaFraction = 0.0;


	if (dbgActiveArea_m2 > 0.0)
	{
		dbgRunoffPercMean =
			dbgRunoffPercAreaSum
			/ dbgActiveArea_m2;


		dbgRunoffCoMean =
			dbgRunoffCoAreaSum
			/ dbgActiveArea_m2;


		dbgAlphaMean =
			dbgAlphaAreaSum
			/ dbgActiveArea_m2;


		dbgSmFractionMean =
			dbgSmFractionAreaSum
			/ dbgActiveArea_m2;


		dbgForcedRunoffAreaFraction =
			dbgForcedRunoffArea_m2
			/ dbgActiveArea_m2;
	}


	// ============================================================
	// SUR_MR_HAND当天总产流比例
	// ============================================================

	double dbgSurfqFraction = 0.0;


	if (dbgHWaterActive_m3 > 1.0e-12)
	{
		dbgSurfqFraction =
			dbgSurfq_m3
			/ dbgHWaterActive_m3;
	}


	// ============================================================
	// HAND当天入渗占积水比例
	// ============================================================

	double dbgHandInfilFraction = 0.0;


	if (dbgHandBefore_m3 > 1.0e-12)
	{
		dbgHandInfilFraction =
			dbgHandInfil_m3
			/ dbgHandBefore_m3;
	}


	// ============================================================
	// HAND当天填充了多少土壤缺水库容
	// ============================================================

	double dbgDeficitFillFraction = 0.0;


	if (dbgSoilDeficitBefore_m3 > 1.0e-12)
	{
		dbgDeficitFillFraction =
			dbgHandInfil_m3
			/ dbgSoilDeficitBefore_m3;
	}


	// ============================================================
	// 三个水量平衡检查
	// ============================================================

	// HAND地表水：
	// 入渗前 = 入渗后 + HAND入渗
	const double dbgHandSurfaceBalanceError_m3 =
		dbgHandBefore_m3
		- dbgHandAfter_m3
		- dbgHandInfil_m3;


	// HAND入渗应该等于土壤实际增加
	const double dbgHandSoilBalanceError_m3 =
		dbgHandInfil_m3
		- dbgSoilGainFromHand_m3;


	// 降雨水量：
	// hWater = surfq + rain infiltration
	const double dbgRunoffBalanceError_m3 =
		dbgHWaterActive_m3
		- dbgSurfq_m3
		- dbgRainInfil_m3;


	// ============================================================
	// 写CSV
	// ============================================================

#ifdef _WIN32
	const char* dbgFile = "D:/SEIMS_C/log/sur_mr_hand_sbid18_debug.csv";
#else
	const char* dbgFile = "/data/user/lijing/software/SEIMS_C/log/sur_mr_hand_sbid18_debug.csv";
#endif


	// 判断文件是否已经存在以及是否为空
	bool dbgNeedHeader = false;

	{
		std::ifstream dbgCheck(
			dbgFile,
			std::ios::in
		);

		if (!dbgCheck.good())
		{
			dbgNeedHeader = true;
		}
		else
		{
			dbgCheck.seekg(
				0,
				std::ios::end
			);

			if (dbgCheck.tellg() == 0)
			{
				dbgNeedHeader = true;
			}

			dbgCheck.close();
		}
	}


	std::ofstream dbgOut(
		dbgFile,
		std::ios::out | std::ios::app
	);


	if (dbgSelectedYear &&
		dbgOut.is_open())
	{
		if (dbgNeedHeader)
		{
			dbgOut

				<< "DATE"

				<< ",hand_cells"
				<< ",wet_hand_cells"
				<< ",hand_area_m2"

				<< ",hand_before_m3"
				<< ",soil_deficit_before_m3"

				<< ",hand_infil_m3"
				<< ",soil_gain_from_hand_m3"

				<< ",hand_after_m3"

				<< ",hand_infil_fraction"
				<< ",deficit_fill_fraction"

				<< ",hand_surface_balance_error_m3"
				<< ",hand_soil_balance_error_m3"

				<< ",sub_cells"
				<< ",active_runoff_cells"

				<< ",sub_area_m2"
				<< ",active_area_m2"

				<< ",netPcp_m3"
				<< ",deprSto_m3"
				<< ",hWaterRaw_m3"
				<< ",hWaterActive_m3"

				<< ",surfq_m3"
				<< ",rain_infil_m3"

				<< ",surfq_fraction"

				<< ",runoffPerc_mean"
				<< ",runoffPerc_max"
				<< ",runoffPerc_gt099_count"

				<< ",forced_runoff_cells"
				<< ",forced_runoff_area_fraction"

				<< ",runoffCo_mean"
				<< ",runoffCo_max"

				<< ",alpha_mean"

				<< ",smFraction_mean"
				<< ",smFraction_max"

				<< ",runoff_balance_error_m3"

				<< std::endl;
		}


		dbgOut
			<< std::setprecision(12)

			<< dbgDateText

			<< "," << dbgHandCellCount
			<< "," << dbgWetHandCellCount
			<< "," << dbgHandArea_m2

			<< "," << dbgHandBefore_m3
			<< "," << dbgSoilDeficitBefore_m3

			<< "," << dbgHandInfil_m3
			<< "," << dbgSoilGainFromHand_m3

			<< "," << dbgHandAfter_m3

			<< "," << dbgHandInfilFraction
			<< "," << dbgDeficitFillFraction

			<< "," << dbgHandSurfaceBalanceError_m3
			<< "," << dbgHandSoilBalanceError_m3

			<< "," << dbgSubCellCount
			<< "," << dbgActiveCellCount

			<< "," << dbgSubArea_m2
			<< "," << dbgActiveArea_m2

			<< "," << dbgNetPcp_m3
			<< "," << dbgDeprSto_m3
			<< "," << dbgHWaterRaw_m3
			<< "," << dbgHWaterActive_m3

			<< "," << dbgSurfq_m3
			<< "," << dbgRainInfil_m3

			<< "," << dbgSurfqFraction

			<< "," << dbgRunoffPercMean
			<< "," << dbgRunoffPercMax
			<< "," << dbgRunoffPercGt099Count

			<< "," << dbgForcedRunoffCellCount
			<< "," << dbgForcedRunoffAreaFraction

			<< "," << dbgRunoffCoMean
			<< "," << dbgRunoffCoMax

			<< "," << dbgAlphaMean

			<< "," << dbgSmFractionMean
			<< "," << dbgSmFractionMax

			<< "," << dbgRunoffBalanceError_m3

			<< std::endl;


		dbgOut.close();
	}

#endif

    return 0;
}

void SUR_MR_HAND::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, Tag_HillSlopeTimeStep)) m_dt = value;
    else if (StringMatch(sk, VAR_T_SOIL)) m_soilFrozenTemp = value;
    else if (StringMatch(sk, VAR_K_RUN)) m_rfExp = value;
    else if (StringMatch(sk, VAR_P_MAX)) m_maxPcpRf = value;
    else if (StringMatch(sk, VAR_S_FROZEN)) m_soilFrozenWtrRatio = value;
	else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
	else if (StringMatch(sk, VAR_OUTLETID)) m_outletID = CVT_INT(value);
    else {
        throw ModelException(MID_SUR_MR_HAND, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void SUR_MR_HAND::Set1DData(const char* key, const int n, float* data) {
    //CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
    string sk(key);
	if (StringMatch(sk, VAR_RUNOFF_CO)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_potRfCoef = data;
	}
	else if (StringMatch(sk, VAR_NEPR)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_netPcp = data;
	}
	else if (StringMatch(sk, VAR_TMEAN)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_meanTemp = data;
	}
	else if (StringMatch(sk, VAR_MOIST_IN)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_initSoilWtrStoRatio = data;
	}
	else if (StringMatch(sk, VAR_SOL_SUMSAT)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_soilSumSat = data;
	}
	else if (StringMatch(sk, VAR_DPST)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_deprSto = data;
	}
	else if (StringMatch(sk, VAR_SOTE)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_soilTemp = data;
	}
	else if (StringMatch(sk, VAR_SOILLAYERS)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_nSoilLyrs = data;
	}
	else if (StringMatch(sk, VAR_POT_VOL)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_potVol = data;
	}
	else if (StringMatch(sk, VAR_IMPOUND_TRIG)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_impndTrig = data;
	}
	else if (StringMatch(sk, VAR_DEM)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_dem = data;
	}
	else if (StringMatch(sk, VAR_LANDUSE)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_landUse = data;
	}
	else if (StringMatch(sk, VAR_STREAM_LINK)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_rchID = data;
	}
	else if (StringMatch(sk, VAR_PCP)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_pcp = data;
	}
	else if (StringMatch(sk, VAR_PET)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_pet = data;
	}
	else if (StringMatch(sk, "t_soil_1d")) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_soilFrozenTemp_1d = data;
	}
	else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_handWtrDep = data;
	}
	else if (StringMatch(sk, VAR_SUBBSN)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_subbsnID = data;
	}
	else if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_SUR_MR_HAND, key, n, m_nCells);
		m_handArea = data;
	}
	else if (StringMatch(sk, VAR_HAND_Subbasin)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_Subbasin = data;
	}
	else if (StringMatch(sk, VAR_HAND_Flood_Level)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_Flood_Level = data;
	}
	else if (StringMatch(sk, VAR_HAND_LevelDepth)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_LevelDepth = data;
	}
	else if (StringMatch(sk, VAR_HAND_SumArea)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_SumArea = data;
	}
	else if (StringMatch(sk, VAR_HAND_SumVolume)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_SumVolume = data;
	}
	else if (StringMatch(sk, VAR_HAND_AvgDepth)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_AvgDepth = data;
	}
	else if (StringMatch(sk, VAR_HAND_AccVolume)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_AccVolume = data;
	}
	else if (StringMatch(sk, VAR_HAND_LowerAccDepthFlat)) {
		m_HAND_LowerAccDepthFlat = data;
	}
	else if (StringMatch(sk, VAR_HAND_LowerAccDepthLen)) {
		CheckInputSize(MID_MUSK_CH_HAND, key, n, m_nCells);
		m_HAND_LowerAccDepthLen = data;
	}
	else {
		throw ModelException(MID_SUR_MR_HAND, "Set1DData", "Parameter " + sk + " does not exist.");
	}

}

void SUR_MR_HAND::SetReaches(clsReaches* reaches) {
	if (nullptr == reaches) {
		throw ModelException(MID_SUR_MR_HAND, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_nreach = reaches->GetReachNumber();
}

void SUR_MR_HAND::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    string sk(key);
    CheckInputSize2D(MID_SUR_MR_HAND, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
    if (StringMatch(sk, VAR_SOL_AWC)) m_soilAWC = data; //m_soilFC = data;
    else if (StringMatch(sk, VAR_SOL_UL)) m_soilSat = data;
    else if (StringMatch(sk, VAR_SOLICE)) m_soilIceSto = data;
    else if (StringMatch(sk, VAR_POROST)) m_soilPor = data;
    else if (StringMatch(sk, VAR_SOILTHICK)) m_soilThk = data;
	else if (StringMatch(sk, VAR_FIELDCAP)) m_soilFC = data;
    else {
        throw ModelException(MID_SUR_MR_HAND, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void SUR_MR_HAND::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_INFIL)) {
        *data = m_infil; //infiltration
    } else if (StringMatch(sk, VAR_EXCP)) {
        *data = m_exsPcp; // excess precipitation
    } else if (StringMatch(sk, VAR_SOL_SW)) {
        *data = m_soilWtrStoPrfl;
    }
	else if (StringMatch(sk, VAR_RUNOFF_PERCENTAGE)) {
		*data = m_HAND_Runoff_Perc;
	}
	else if (StringMatch(sk, VAR_RUNOFF_CO)) {
		*data = m_potRfCoef;
	}
	else if (StringMatch(sk, VAR_OL_HAND_WTRDEP_AFT_INFIL)) {
		*data = handWtrDepAftInfil;
	}
	else if (StringMatch(sk, VAR_OL_HAND_INFIL)) {
		*data = m_HAND_Infil;
	}
	/*else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		*data = m_handWtrDep;
		*n = m_nCells;
	}*/
	else {
        throw ModelException(MID_SUR_MR_HAND, "Get1DData", "Result " + sk + " does not exist.");
    }
    *n = m_nCells;
}

void SUR_MR_HAND::Get2DData(const char* key, int* nRows, int* nCols, float*** data) {
    InitialOutputs();
    string sk(key);
    *nRows = m_nCells;
    *nCols = m_maxSoilLyrs;
    if (StringMatch(sk, VAR_SOL_ST)) {
        *data = m_soilWtrSto;
    }
	else if (StringMatch(sk, VAR_FIELDCAPDEP)) {
		*data = m_soilFCDepth;
	}
	else if (StringMatch(sk, VAR_POROSTDEP)) {
		*data = m_soilPorDepth;
	}
	else if (StringMatch(sk, VAR_SOL_UL)) {
		*data = m_soilSat;
	}
	else if (StringMatch(sk, VAR_SOL_AWC)) {
		*data = m_soilAWC;
	}
	else {
        throw ModelException(MID_SUR_MR_HAND, "Get2DData", "Output " + sk + " does not exist.");
    }
}





