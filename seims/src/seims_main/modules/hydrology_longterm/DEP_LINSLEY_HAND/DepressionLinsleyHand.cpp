#include "DepressionLinsleyHand.h"

#include "text.h"

DepressionLinsleyHand::DepressionLinsleyHand() :
    m_nCells(-1), m_impoundTriger(nullptr), m_outletID(-1), m_nreach(-1),
    m_potVol(nullptr),
    m_depCo(NODATA_VALUE), m_depCap(nullptr), m_pet(nullptr),
    m_ei(nullptr), m_pe(nullptr), m_sd(nullptr),
    m_ed(nullptr), m_sr(nullptr), m_handWtrDep(nullptr), m_subbsnID(nullptr), m_chSto(nullptr), m_handArea(nullptr), m_hand_eavp(nullptr),
	m_HAND_Subbasin(nullptr), m_HAND_Flood_Level(nullptr), m_HAND_LevelDepth(nullptr),
	m_HAND_SumArea(nullptr), m_HAND_SumVolume(nullptr), m_HAND_AvgDepth(nullptr),
	m_HAND_AccVolume(nullptr), m_HAND_LowerAccDepthFlat(nullptr), m_HAND_LowerAccDepthLen(nullptr),
	handWtrDepBfe(nullptr), m_chStoBfe(nullptr)
{

}

DepressionLinsleyHand::~DepressionLinsleyHand() {
    if (m_sd != nullptr) Release1DArray(m_sd);
    if (m_ed != nullptr) Release1DArray(m_ed);
    if (m_sr != nullptr) Release1DArray(m_sr);
	if (handWtrDepBfe != nullptr) Release1DArray(handWtrDepBfe);
	if (m_chStoBfe != nullptr) Release1DArray(m_chStoBfe);
	
	
}

bool DepressionLinsleyHand::CheckInputData() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_date);
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
    CHECK_NODATA(MID_DEP_LINSLEY, m_depCo);
    CHECK_POINTER(MID_DEP_LINSLEY, m_depCap);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pet);
    CHECK_POINTER(MID_DEP_LINSLEY, m_ei);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pe);
    return true;
}

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

void DepressionLinsleyHand::LoadHandLevelsFromArrays(
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

void DepressionLinsleyHand::InitialOutputs() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
	if (nullptr == m_hand_eavp) {
		Initialize1DArray(m_nCells, m_hand_eavp, 0.f);
	}
	if (nullptr == m_ed) {
		Initialize1DArray(m_nCells, m_ed, 0.f);
	}
	if (nullptr == m_sr) {
		Initialize1DArray(m_nCells, m_sr, 0.f);
	}
	if (m_handWtrDep == nullptr)
	{
		Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
	}
	if (handWtrDepBfe == nullptr)
	{
		Initialize1DArray(m_nCells, handWtrDepBfe, 0.f);//xdw++
	}
	if (m_chStoBfe == nullptr)
	{
		Initialize1DArray(m_nreach, m_chStoBfe, 0.f);//xdw++
	}
	
    if (nullptr == m_sd) {
        Initialize1DArray(m_nCells, m_sd, 0.f);
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            m_sd[i] = m_depCo * m_depCap[i];
        }
    }

	int lower_flat_len = 0;
	for (int i = 0; i < m_nCells; i++)
	{
		lower_flat_len += (int)m_HAND_LowerAccDepthLen[i];
	}
	LoadHandLevelsFromArrays(m_nCells, lower_flat_len, m_Hands, NODATA_VALUE, TRUE);
}

void DepressionLinsleyHand::SetReaches(clsReaches* reaches) {
	if (nullptr == reaches) {
		throw ModelException(MID_MUSK_CH, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_nreach = reaches->GetReachNumber();
}

bool DepressionLinsleyHand::HandInundation_BinarySearch(const int reachId, float sto) {
	if (sto <= 0.000001f)
	{
		for (int ll = 1; ll <= m_Hands[reachId].n_levels; ll++)
		{
			for (int idx = 0; idx < m_Hands[reachId].levels[ll].m_levelHandNum; idx++)
			{
				int handId = m_Hands[reachId].levels[ll].handIds[idx];
				m_handWtrDep[handId] = 0.f;

			}
			m_chSto[reachId] = 0.f;

		}

		return true;
	}

	Hand& hand = m_Hands[reachId];
	const int n = hand.n_levels;
	vector<Level>& levels = hand.levels;

	// 二分查找：找到 sto 落在哪一层（第一个 AccVol >= sto）
	int left = 1, right = n, target_level = n;

	while (left <= right) {
		int mid = (left + right) / 2;
		if (sto <= levels[mid].m_levelAccVol) {
			target_level = mid;
			right = mid - 1;
		}
		else {
			left = mid + 1;
		}
	}

	hand.m_CurInundationLevel = target_level;  // 层编号从 1 开始
	float remaining = 0.0;
	if (target_level == 1)
	{
		remaining = sto;
	}
	else {
		remaining = sto - levels[target_level - 1].m_levelAccVol;
	}
	if (remaining < 0.0)
	{
		cout << "reachId: " << reachId << " target_level-1: " << target_level - 1 << " sto: " << sto << " AccVol: " << levels[target_level - 1].m_levelAccVol << " remaining: " << remaining << endl;
		exit(0);
	}

	//  target_level 层之下的水深  加上 target_level 层中当前未填满的那一小截水深
	float partial_depth = (levels[target_level].m_levelSumArea > 0.0f) ? remaining / levels[target_level].m_levelSumArea : 0.0f;
	for (int i = 1; i <= target_level; ++i) {
		levels[i].m_levelWtrDep = levels[i].m_levelLowerAccDepth[target_level] + partial_depth;
	}

	// 后面所有层水深为 0
	for (int i = target_level + 1; i <= n; ++i) {
		levels[i].m_levelWtrDep = 0.0f;
	}

	// 若超出最大体积，则剩余部分作为超额水
	float maxVolume = levels[n].m_levelAccVol;
	if (sto > maxVolume) {
		hand.excessWtrVol = sto - maxVolume;
	}

	updateAllHandsWtrDep(reachId);

	return true;
}



void DepressionLinsleyHand::updateAllHandsWtrDep(const int reachId) {
	//m_Hands[reachId].levels[lev].m_levelWtrDep = 0.0;
	for (int ll = 1; ll <= m_Hands[reachId].n_levels; ll++)
	{
		for (int idx = 0; idx < m_Hands[reachId].levels[ll].m_levelHandNum; idx++)
		{
			int handId = m_Hands[reachId].levels[ll].handIds[idx];
			m_handWtrDep[handId] = m_Hands[reachId].levels[ll].m_levelWtrDep;
		}
	}
	//m_chWtrDepth[reachId] = m_Hands[reachId].levels[1].m_levelWtrDep;
	return;
}

int DepressionLinsleyHand::Execute() {
    CheckInputData();
    InitialOutputs();
#ifdef DEBUG_DEP_LINSLEY_HAND
	{
		cout << "[DEP_LINSLEY_HAND]" << endl;
	}
#endif
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
		
        //////////////////////////////////////////////////////////////////////////
		float handWtrDepMM = MAX(m_handWtrDep[i] * 1000.0, 0.0);
		int subbasinId = CVT_INT(m_subbsnID[i]);
		float depStoDeficit =MAX(m_depCap[i] - m_sd[i], 0.0);

		m_chStoBfe[subbasinId] = m_chSto[subbasinId];
		handWtrDepBfe[i] = handWtrDepMM;
        // runoff
		// don't need depression
        if (m_depCap[i] < 0.001f) {
            m_sr[i] = m_pe[i];
            m_sd[i] = 0.f;
		}
		//  inundation depth > depression Deficit
		else if (handWtrDepMM >= depStoDeficit)
		{
			m_chSto[subbasinId] -= m_handArea[i] * depStoDeficit  * 0.001;
			m_sd[i] = m_depCap[i];
			//m_sr[i] = m_pe[i];
			handWtrDepMM -= depStoDeficit;
		}
		else{
			m_sd[i] += handWtrDepMM;
			m_chSto[subbasinId] -= m_handArea[i] * handWtrDepMM  * 0.001;
			handWtrDepMM = 0.0;
		}

        //////////////////////////////////////////////////////////////////////////
        // evaporation
        if (handWtrDepMM > 0) {
			// xiaodw, handWtrDepMM has pirority to evap
			if (m_pet[i] - m_ei[i] < handWtrDepMM)
			{
				m_hand_eavp[i] = m_pet[i] - m_ei[i];
				
			}  else {
				m_hand_eavp[i] = handWtrDepMM;
            }
			handWtrDepMM -= m_hand_eavp[i];
			m_chSto[subbasinId] -= m_handArea[i] * m_hand_eavp[i] * 0.001;
        } else {
			m_hand_eavp[i] = 0.f;
        }


    }
	for (int sbid = 1; sbid <= m_nreach; ++sbid) {

		HandInundation_BinarySearch(sbid, m_chSto[sbid]);


#ifdef DEBUG_DEP_LINSLEY_HAND
		
		int SPECIFIED_SBID = 2;
		if (sbid == SPECIFIED_SBID)
		{
			for (int ll = 1; ll <= m_Hands[sbid].n_levels; ll++)
			{
				for (int idx = 0; idx < m_Hands[sbid].levels[ll].m_levelHandNum; idx++)
				{
					int i = m_Hands[sbid].levels[ll].handIds[idx];
					//int SPECIFIED_ID = 342;

						cout << "Sbid: " << sbid << "   "
							<< "HandId: " << i << "   "
							<< " handWtrDepBfe=" << handWtrDepBfe[i] << "   "
							<< " handWtrDepAft=" << m_handWtrDep[i] << "   "
							<< " chStoBfe=" << m_chStoBfe[sbid] << "   "
							<< " chStoAft=" << m_chSto[sbid] << "   "
							<< " depCap=" << m_depCap[i] << "   "
							<< " sd=" << m_sd[i] << "   "
							<< " pe=" << m_pe[i] << "   "
							<< " eavp=" << m_hand_eavp[i] << "   "
							<< " m_handArea=" << m_handArea[i] << "   "
							<< endl;
				}
			}
			
		}
		
#endif
	}
    return true;
}

void DepressionLinsleyHand::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, VAR_DEPREIN)) m_depCo = value;
	else if (StringMatch(sk, VAR_OUTLETID)) m_outletID = CVT_INT(value);
    else {
        throw ModelException(MID_DEP_LINSLEY, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void DepressionLinsleyHand::Set1DData(const char* key, const int n, float* data) {
    
    string sk(key);
    if (StringMatch(sk, VAR_DEPRESSION)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_depCap = data;
    } else if (StringMatch(sk, VAR_INET)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_ei = data;
    } else if (StringMatch(sk, VAR_PET)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_pet = data;
    } else if (StringMatch(sk, VAR_EXCP)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_pe = data;
    } else if (StringMatch(sk, VAR_IMPOUND_TRIG)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_impoundTriger = data;
    } else if (StringMatch(sk, VAR_POT_VOL)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_potVol = data;
    } else if (StringMatch(sk, VAR_SUBBSN)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_subbsnID = data;
	} else if (StringMatch(sk, VAR_CHST)) {
		CheckInputSize(MID_SUR_MR, key, n - 1, m_nreach);
		m_chSto = data;
	} else if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_handArea = data;
	} else if (StringMatch(sk, VAR_DPST)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_sd = data;
	}
	else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		m_handWtrDep = data;
	}
	else if (StringMatch(sk, VAR_HAND_Subbasin)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_Subbasin = data;
	}
	else if (StringMatch(sk, VAR_HAND_Flood_Level)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_Flood_Level = data;
	}
	else if (StringMatch(sk, VAR_HAND_LevelDepth)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_LevelDepth = data;
	}
	else if (StringMatch(sk, VAR_HAND_SumArea)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_SumArea = data;
	}
	else if (StringMatch(sk, VAR_HAND_SumVolume)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_SumVolume = data;
	}
	else if (StringMatch(sk, VAR_HAND_AvgDepth)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_AvgDepth = data;
	}
	else if (StringMatch(sk, VAR_HAND_AccVolume)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_AccVolume = data;
	}
	else if (StringMatch(sk, VAR_HAND_LowerAccDepthFlat)) {
		m_HAND_LowerAccDepthFlat = data;
	}
	else if (StringMatch(sk, VAR_HAND_LowerAccDepthLen)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_HAND_LowerAccDepthLen = data;
	}
	else {
        throw ModelException(MID_DEP_LINSLEY, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void DepressionLinsleyHand::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    *n = m_nCells;
    if (StringMatch(sk, VAR_DPST)) {
        *data = m_sd;
    } else if (StringMatch(sk, VAR_DEET)) {
        *data = m_ed;
    } else if (StringMatch(sk, VAR_SURU)) {
        *data = m_sr;
    } else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		*data = m_handWtrDep;
	}else if (StringMatch(sk, VAR_CHST)) {
		m_chSto[0] = m_chSto[m_outletID];
		*data = m_chSto;
	}else if (StringMatch(sk, VAR_HAND_EVAP)) {
		*data = m_hand_eavp;
	}

	
	else {
        throw ModelException(MID_DEP_LINSLEY, "Get1DData", "Output " + sk + " does not exist.");
    }
}
