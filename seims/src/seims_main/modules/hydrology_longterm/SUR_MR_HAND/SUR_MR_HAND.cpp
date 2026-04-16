#include "SUR_MR_HAND.h"
#include <set>
#include "text.h"
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
	//CHECK_POINTER(MID_SUR_MR_HAND, m_soilTemp);   // xiaodw comment, don't need soil temperature now
	CHECK_POINTER(MID_SUR_MR_HAND, m_netPcp);
    CHECK_POINTER(MID_SUR_MR_HAND, m_deprSto);
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
		Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
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
	set<int> SPECIFIED_SBID = { 2 };
    CheckInputData();
    InitialOutputs();
    int frez =0;
    m_maxPcpRf *= m_dt * 1.1574074074074073e-05f; /// 1. / 86400. = 1.1574074074074073e-05;


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

					
					if (handWtrDepMM >= m_soilPor[i][ly] * m_soilThk[i][ly] - m_soilWtrSto[i][ly]) {
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
					}
					handWtrDepAftInfil[i] = handWtrDepMM * 0.001;   // m
					if (m_handWtrDep[i] < 0.f)
					{
						cout << "Error: In SUR_MR_HAND module, m_handWtrDep is "  << m_handWtrDep[i] << " at subbasinId=" << sbid << ", i =" << i << endl;
						//exit(-1);
					}
				}
				m_HAND_Infil[i] = hand_infil_acc; // mm
				
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
//                //for frozen soil, no infiltration will occur
//                //if (m_soilTemp[i] <= m_soilFrozenTemp && smFraction >= m_soilFrozenWtrRatio) {
//                if (m_soilTemp[i] <= m_soilFrozenTemp_1d[i] && smFraction >= m_soilFrozenWtrRatio) {
//                    m_exsPcp[i] = m_netPcp[i];
//                    m_infil[i] = 0.f;
//                } else {
//                    alpha = m_rfExp - (m_rfExp - 1.f) * hWater / m_maxPcpRf;
//                    if (hWater >= m_maxPcpRf) {
//                        alpha = 1.f;
//                    }
//
//                    //runoff percentage
//                    float runoffPercentage;
////#ifndef DEBUG_SUR_MR_HAND
////				cout << i << ": " << m_potRfCoef[i] << endl;
////#endif
//                    if (m_potRfCoef[i] > 0.99f ||  (m_landUse[i] == LANDUSE_ID_GLC)) {
//                        runoffPercentage = 1.f;
//                    } else {
//                        runoffPercentage = m_potRfCoef[i] * pow(smFraction, alpha);
//                    }
//                    runoffPercentage = Min(runoffPercentage,1.f);
//                    float surfq = hWater * runoffPercentage;
//                    if (surfq > hWater) surfq = hWater;
//                    m_infil[i] = hWater - surfq;
//                    m_exsPcp[i] = surfq;
//
//                    /// TODO: Why calculate surfq first, rather than infiltration first?
//                    ///       I think we should calculate infiltration first, until saturation,
//                    ///       then surface runoff should be calculated. By LJ.
//                }

				alpha = m_rfExp - (m_rfExp - 1.f) * hWater / m_maxPcpRf;
				if (hWater >= m_maxPcpRf) {
					alpha = 1.f;
				}


				//#ifndef DEBUG_SUR_MR_HAND
				//				cout << i << ": " << m_potRfCoef[i] << endl;
				//#endif
				if (m_potRfCoef[i] > 0.99f || (m_landUse[i] == LANDUSE_ID_GLC)) {
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
			m_alpha[i] = alpha;

        } else {
            m_exsPcp[i] = 0.f;
            m_infil[i] = 0.f;
        }
		m_soilWtrStoBfe[i][0] = m_soilWtrSto[i][0];
        if (m_infil[i] > 0.f) {
            m_soilWtrSto[i][0] += m_infil[i];
        }


#ifdef DEBUG_SUR_MR_HAND
		{
			int sbid = CVT_INT(m_subbsnID[i]);
			if (SPECIFIED_SBID.find(sbid) != SPECIFIED_SBID.end()) {
				cout << " Sbid: " << sbid << "   "
					<< " HandId: " << i << "   "
					<< " hWater=" << hWater << "   "
					<< " infil=" << m_infil[i] << "   "
					<< " exsPcp=" << m_exsPcp[i] << "   "
					<< " surfq=" << surfq << "   "
					<< " WtrStoBfe_0=" << m_soilWtrStoBfe[i][0] << "   "
					<< " WtrStoAft_0=" << m_soilWtrSto[i][0] << "   "
					<< " runPerc=" << runoffPercentage << "   "
					<< " runoffCo=" << m_potRfCoef[i] << "   "
					<< " alpha=" << m_alpha[i] << "   "
					<< endl;
				// 单线程可以不每次 flush；如果想实时看，可以打开下面这一行
				// dbg.flush();
			}
		}
#endif
    }
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
		throw ModelException(MID_MUSK_CH, "SetReaches", "The reaches input can not to be NULL.");
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
