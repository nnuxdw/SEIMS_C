#include "DepressionLinsleyHand.h"
//#define DEBUG_WB19
#include "text.h"

DepressionLinsleyHand::DepressionLinsleyHand() :
    m_nCells(-1), m_impoundTriger(nullptr), m_outletID(-1), m_nreach(-1),
    m_potVol(nullptr),
    m_depCo(NODATA_VALUE), m_depCap(nullptr), m_pet(nullptr),
    m_ei(nullptr), m_pe(nullptr), m_sd(nullptr),
    m_ed(nullptr), m_sr(nullptr), m_handWtrDep(nullptr), m_subbsnID(nullptr),  m_handArea(nullptr), m_hand_eavp(nullptr), m_hand_dep(nullptr),
	m_HAND_Subbasin(nullptr), m_HAND_Flood_Level(nullptr), m_HAND_LevelDepth(nullptr),
	m_HAND_SumArea(nullptr), m_HAND_SumVolume(nullptr), m_HAND_AvgDepth(nullptr),
	m_HAND_AccVolume(nullptr), m_HAND_LowerAccDepthFlat(nullptr), m_HAND_LowerAccDepthLen(nullptr),
	handWtrDepAftDep(nullptr)
{

}

DepressionLinsleyHand::~DepressionLinsleyHand() {
    if (m_sd != nullptr) Release1DArray(m_sd);
    if (m_ed != nullptr) Release1DArray(m_ed);
    if (m_sr != nullptr) Release1DArray(m_sr);
	if (handWtrDepAftDep != nullptr) Release1DArray(handWtrDepAftDep);
	
	
}

bool DepressionLinsleyHand::CheckInputData() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_date);
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
    CHECK_NODATA(MID_DEP_LINSLEY, m_depCo);
    CHECK_POINTER(MID_DEP_LINSLEY, m_depCap);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pet);
    CHECK_POINTER(MID_DEP_LINSLEY, m_ei);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pe);
	CHECK_POINTER(MID_DEP_LINSLEY, m_handWtrDep);
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

void DepressionLinsleyHand::InitialOutputs() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
	if (nullptr == m_hand_eavp) {
		Initialize1DArray(m_nCells, m_hand_eavp, 0.f);
	}
	if (nullptr == m_hand_dep) {
		Initialize1DArray(m_nCells, m_hand_dep, 0.f);
	}
	
	if (nullptr == m_ed) {
		Initialize1DArray(m_nCells, m_ed, 0.f);
	}
	if (nullptr == m_sr) {
		Initialize1DArray(m_nCells, m_sr, 0.f);
	}
	//if (m_handWtrDep == nullptr)
	//{
	//	Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
	//}
	if (handWtrDepAftDep == nullptr)
	{
		Initialize1DArray(m_nCells, handWtrDepAftDep, 0.f);//xdw++
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
}

void DepressionLinsleyHand::SetReaches(clsReaches* reaches) {
	if (nullptr == reaches) {
		throw ModelException(MID_MUSK_CH, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_nreach = reaches->GetReachNumber();
}


int DepressionLinsleyHand::Execute() {

    CheckInputData();
    InitialOutputs();


#ifdef DEBUG_DEP_LINSLEY_HAND
	{
		cout << "[DEP_LINSLEY_HAND]" << endl;
	}
#endif


#ifdef DEBUG_WB19
	double dbgSdBeforeHandVol19 = 0.0;

	for (int i = 0; i < m_nCells; ++i)
	{
		if (CVT_INT(m_subbsnID[i]) != 19)
		{
			continue;
		}

		const double area =
			static_cast<double>(m_handArea[i]);

		// 洼地储水单位mm，转成m3
		dbgSdBeforeHandVol19 +=
			static_cast<double>(m_sd[i])
			* 0.001
			* area;
	}
#endif


	// depression from HAND's water
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
		
        //////////////////////////////////////////////////////////////////////////
		float handWtrDepMM = MAX(m_handWtrDep[i] * 1000.0, 0.0);
		int subbasinId = CVT_INT(m_subbsnID[i]);
		float depStoDeficit =MAX(m_depCap[i] - m_sd[i], 0.0);
		float depMM = 0.0;
		// don't need depression
        if (m_depCap[i] < 0.001f) {
            m_sd[i] = 0.f;
			depMM = 0.f;
		}
		//  inundation depth > depression Deficit
		else if (handWtrDepMM > depStoDeficit)
		{
			m_sd[i] = m_depCap[i];
			depMM = depStoDeficit;
			//m_sr[i] = m_pe[i];
			handWtrDepMM -= depStoDeficit;
		}
		else{
			m_sd[i] += handWtrDepMM;
			depMM = handWtrDepMM;
			handWtrDepMM = 0.0;
		}

		m_hand_dep[i] = depMM;  // mm
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
        } else {
			m_hand_eavp[i] = 0.f;
        }

		handWtrDepAftDep[i] = handWtrDepMM * 0.001;  // m
    }


	// ============================================================
// 19号子流域：HAND水填洼和蒸发水量平衡
// ============================================================
#ifdef DEBUG_WB19
	{
		double handBeforeVol = 0.0;       // 进入本模块的HAND水，m3
		double handAfterVol = 0.0;        // 处理后剩余HAND水，m3
		double handDepVol = 0.0;          // HAND水转入洼地，m3
		double handEvapVol = 0.0;         // HAND水面蒸发，m3
		double sdAfterHandVol = 0.0;      // HAND处理后的洼地储水，m3

		double negativeEvapGainVol = 0.0;

		int cellCount = 0;
		int negativeHandEvapCount = 0;
		int negativeHandDepthCount = 0;
		int invalidAreaCount = 0;

		for (int i = 0; i < m_nCells; ++i)
		{
			if (CVT_INT(m_subbsnID[i]) != 19)
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

			// 原代码实际使用MAX(m_handWtrDep*1000, 0)
			const double handBeforeDepthM =
				(std::max)(
					static_cast<double>(m_handWtrDep[i]),
					0.0
				);

			handBeforeVol +=
				handBeforeDepthM * area;

			handAfterVol +=
				static_cast<double>(
					handWtrDepAftDep[i]
					)
				* area;

			// m_hand_dep单位mm
			handDepVol +=
				static_cast<double>(m_hand_dep[i])
				* 0.001
				* area;

			// m_hand_eavp单位mm；保留符号，以检查负蒸发
			handEvapVol +=
				static_cast<double>(m_hand_eavp[i])
				* 0.001
				* area;

			sdAfterHandVol +=
				static_cast<double>(m_sd[i])
				* 0.001
				* area;

			if (m_hand_eavp[i] < -1.0e-6f)
			{
				++negativeHandEvapCount;

				// 负蒸发会增加多少水
				negativeEvapGainVol +=
					-static_cast<double>(m_hand_eavp[i])
					* 0.001
					* area;
			}

			if (handWtrDepAftDep[i] < -1.0e-8f)
			{
				++negativeHandDepthCount;
			}
}

		// HAND地表水平衡：
		// 处理前 = 处理后 + 填洼 + 蒸发
		const double handBalanceError =
			handBeforeVol
			- handAfterVol
			- handDepVol
			- handEvapVol;

		// 洼地储水变化应等于HAND转入洼地的水
		const double depressionStorageError =
			(
				sdAfterHandVol
				- dbgSdBeforeHandVol19
				)
			- handDepVol;

		const double scale =
			(std::max)(
				1.0,
				std::fabs(handBeforeVol)
			);

		cout << "[WB19_DEP_HAND]"
			<< ",date=" << m_date
			<< ",cellCount=" << cellCount

			<< ",handBefore_m3=" << handBeforeVol
			<< ",handAfter_m3=" << handAfterVol
			<< ",toDepression_m3=" << handDepVol
			<< ",handEvap_m3=" << handEvapVol

			<< ",sdBefore_m3="
			<< dbgSdBeforeHandVol19
			<< ",sdAfterHand_m3="
			<< sdAfterHandVol

			<< ",handBalanceError="
			<< handBalanceError
			<< ",handRelativeError="
			<< handBalanceError / scale

			<< ",depressionStorageError="
			<< depressionStorageError

			<< ",negativeHandEvapCount="
			<< negativeHandEvapCount
			<< ",negativeEvapGain_m3="
			<< negativeEvapGainVol

			<< ",negativeHandDepthCount="
			<< negativeHandDepthCount
			<< ",invalidAreaCount="
			<< invalidAreaCount

			<< endl;

		cout.flush();
}
#endif




	for (int sbid = 1; sbid <= m_nreach; ++sbid) {


#ifdef DEBUG_DEP_LINSLEY_HAND
		cout << "*[DEP_LINSLEY_HAND]* " << endl;
		int SPECIFIED_ID = 342;
		for (int ll = 1; ll <= m_Hands[sbid].n_levels; ll++)
		{
			for (int idx = 0; idx < m_Hands[sbid].levels[ll].m_levelHandNum; idx++)
			{
				int i = m_Hands[sbid].levels[ll].handIds[idx];
				if (i == SPECIFIED_ID)
				{
					cout << "Sbid: " << sbid << "   "
						<< "HandId: " << i << "   "
						<< " handWtrDepAftDep=" << handWtrDepAftDep[i] << "   "
						<< " handWtrDepAft=" << m_handWtrDep[i] << "   "
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


#ifdef DEBUG_WB19
	double dbgSdBeforeRunoffVol19 = 0.0;
	double dbgPeInputVol19 = 0.0;
	double dbgPotBeforeVol19 = 0.0;

	for (int i = 0; i < m_nCells; ++i)
	{
		if (CVT_INT(m_subbsnID[i]) != 19)
		{
			continue;
		}

		const double area =
			static_cast<double>(m_handArea[i]);

		dbgSdBeforeRunoffVol19 +=
			static_cast<double>(m_sd[i])
			* 0.001
			* area;

		dbgPeInputVol19 +=
			static_cast<double>(m_pe[i])
			* 0.001
			* area;

		if (m_potVol != nullptr)
		{
			dbgPotBeforeVol19 +=
				static_cast<double>(m_potVol[i])
				* 0.001
				* area;
		}
	}
#endif


#pragma omp parallel for
	for (int i = 0; i < m_nCells; i++) {

		//////////////////////////////////////////////////////////////////////////
		// runoff
		float depDeficit = m_depCap[i] - m_sd[i];
		if (m_depCap[i] < 0.001f) {
			m_sr[i] = m_pe[i];
			m_sd[i] = 0.f;
		}
		// xiaodw, if depDeficit is zero, there will be a false number, thus depDeficit should be larger that 0.001f
		else if (depDeficit < 0.001f) {
			m_sd[i] = m_depCap[i];
			m_sr[i] = m_pe[i];
		}
		else if (m_pe[i] > 0.f) {
			float pc = m_pe[i] - m_depCap[i] * log(1.f - m_sd[i] / m_depCap[i]);
			float deltaSd = m_pe[i] * exp(-pc / m_depCap[i]);
			if (deltaSd > m_depCap[i] - m_sd[i]) {
				deltaSd = m_depCap[i] - m_sd[i];
			}
			m_sd[i] += deltaSd;
			m_sr[i] = m_pe[i] - deltaSd;
		}
		else {
			m_sd[i] += m_pe[i];
			m_sr[i] = 0.f;
		}

		//////////////////////////////////////////////////////////////////////////
		// evaporation
		if (m_sd[i] > 0) {
			/// TODO: Is this logically right? PET is just potential, which include
			///       not only ET from surface water, but also from plant and soil.
			///       Please Check the corresponding theory. By LJ.
			// evaporation from depression storage
			if (MAX(m_pet[i] - m_ei[i] - m_hand_eavp[i], 0.f) < m_sd[i]) {
				m_ed[i] = m_pet[i] - m_ei[i] - m_hand_eavp[i];
			}
			else {
				m_ed[i] = m_sd[i];
			}
			m_sd[i] -= m_ed[i];
		}
		else {
			m_ed[i] = 0.f;
			m_sd[i] = 0.f;
		}


		if (m_impoundTriger != nullptr && FloatEqual(m_impoundTriger[i], 0.f)) {
			if (m_potVol != nullptr) {
				m_potVol[i] += m_sr[i];
				m_potVol[i] += m_sd[i];
				m_sr[i] = 0.f;
				m_sd[i] = 0.f;
			}
		}
#ifdef DEBUG_DEP_LINSLEY_HAND
		{
			int SPECIFIED_ID = 342;
			if (i == SPECIFIED_ID)
			{
				cout << "*[DEP_LINSLEY]* " << endl;
				cout

					<< " depCap=" << m_depCap[i] << "   "
					<< " sd=" << m_sd[i] << "   "
					<< " pe=" << m_pe[i] << "   "
					<< " sr=" << m_sr[i] << "   "
					<< " pet=" << m_pet[i] << "   "
					<< " ei=" << m_ei[i] << "   "
					<< " eavp=" << m_hand_eavp[i] << "   "
					<< endl;
			}

		}
#endif
	}


#ifdef DEBUG_WB19
	{
		double sdAfterVol = 0.0;
		double runoffVol = 0.0;
		double depEvapVol = 0.0;
		double potAfterVol = 0.0;

		double negativeDepEvapGainVol = 0.0;

		int negativeDepEvapCount = 0;
		int negativeDepStorageCount = 0;
		int negativeRunoffCount = 0;
		int negativePeCount = 0;
		int impoundCellCount = 0;

		for (int i = 0; i < m_nCells; ++i)
		{
			if (CVT_INT(m_subbsnID[i]) != 19)
			{
				continue;
			}

			const double area =
				static_cast<double>(m_handArea[i]);

			sdAfterVol +=
				static_cast<double>(m_sd[i])
				* 0.001
				* area;

			runoffVol +=
				static_cast<double>(m_sr[i])
				* 0.001
				* area;

			// 保留蒸发符号
			depEvapVol +=
				static_cast<double>(m_ed[i])
				* 0.001
				* area;

			if (m_potVol != nullptr)
			{
				potAfterVol +=
					static_cast<double>(m_potVol[i])
					* 0.001
					* area;
			}

			if (m_ed[i] < -1.0e-6f)
			{
				++negativeDepEvapCount;

				negativeDepEvapGainVol +=
					-static_cast<double>(m_ed[i])
					* 0.001
					* area;
			}

			if (m_sd[i] < -1.0e-6f)
			{
				++negativeDepStorageCount;
			}

			if (m_sr[i] < -1.0e-6f)
			{
				++negativeRunoffCount;
			}

			if (m_pe[i] < -1.0e-6f)
			{
				++negativePeCount;
			}

			if (m_impoundTriger != nullptr &&
				FloatEqual(m_impoundTriger[i], 0.f))
			{
				++impoundCellCount;
			}
		}

		const double potGainVol =
			potAfterVol - dbgPotBeforeVol19;

		// 完整平衡：
		// 初始洼地储水 + PE输入
		// = 最终洼地储水 + 地表径流 + 洼地蒸发 + 转入塘库的水
		const double balanceError =
			dbgSdBeforeRunoffVol19
			+ dbgPeInputVol19
			- sdAfterVol
			- runoffVol
			- depEvapVol
			- potGainVol;

		const double scale =
			(std::max)(
				1.0,
				(std::max)(
					std::fabs(
						dbgSdBeforeRunoffVol19
						+ dbgPeInputVol19
					),
					std::fabs(
						sdAfterVol
						+ runoffVol
						+ depEvapVol
						+ potGainVol
					)
				)
			);

		cout << "[WB19_DEP_STO]"
			<< ",date=" << m_date

			<< ",sdBefore_m3="
			<< dbgSdBeforeRunoffVol19
			<< ",peInput_m3="
			<< dbgPeInputVol19

			<< ",sdAfter_m3="
			<< sdAfterVol
			<< ",surfaceRunoff_m3="
			<< runoffVol
			<< ",depEvap_m3="
			<< depEvapVol
			<< ",potGain_m3="
			<< potGainVol

			<< ",balanceError="
			<< balanceError
			<< ",relativeError="
			<< balanceError / scale

			<< ",negativeDepEvapCount="
			<< negativeDepEvapCount
			<< ",negativeDepEvapGain_m3="
			<< negativeDepEvapGainVol

			<< ",negativeDepStorageCount="
			<< negativeDepStorageCount
			<< ",negativeRunoffCount="
			<< negativeRunoffCount
			<< ",negativePeCount="
			<< negativePeCount
			<< ",impoundCellCount="
			<< impoundCellCount

			<< endl;

		cout.flush();
	}
#endif


#ifdef DEBUG_WB19
{
double sumM3 = 0.0;
double sumRawMM = 0.0;
double areaSumM2 = 0.0;

int nonzeroCount = 0;
float maxMM = 0.f;

for (int i = 0; i < m_nCells; ++i)
{
	if (CVT_INT(m_subbsnID[i]) != 19)
	{
		continue;
	}

	const double areaM2 =
		static_cast<double>(m_handArea[i]);

	const double runoffMM =
		static_cast<double>(m_sr[i]);

	sumM3 += runoffMM * 0.001 * areaM2;
	sumRawMM += runoffMM;
	areaSumM2 += areaM2;

	if (m_sr[i] > 1.e-10f)
	{
		++nonzeroCount;
	}

	maxMM = Max(maxMM, m_sr[i]);
}

cout << "[WB19_SURU_OUT]"
<< ",date=" << m_date
<< ",ptr="
<< static_cast<const void*>(m_sr)
<< ",sumM3=" << sumM3
<< ",sumRawMM=" << sumRawMM
<< ",areaSum_m2=" << areaSumM2
<< ",nonzeroCount=" << nonzeroCount
<< ",maxMM=" << maxMM
<< endl;

cout.flush();
}
#endif



#ifdef DEBUG_WB19
{
	const int debugSubbasin = 19;

	double sumM3 = 0.0;
	double sumRawMM = 0.0;
	double areaSumM2 = 0.0;

	int nonzeroCount = 0;
	int maxCell = -1;

	float maxMM = -1.0e30f;
	float maxCellAreaM2 = NODATA_VALUE;

	for (int i = 0; i < m_nCells; ++i)
	{
		if (CVT_INT(m_subbsnID[i]) != debugSubbasin)
		{
			continue;
		}

		const double areaM2 =
			static_cast<double>(m_handArea[i]);

		const double runoffMM =
			static_cast<double>(m_sr[i]);

		sumM3 += runoffMM * 0.001 * areaM2;
		sumRawMM += runoffMM;
		areaSumM2 += areaM2;

		if (m_sr[i] > 1.0e-10f)
		{
			++nonzeroCount;
		}

		if (m_sr[i] > maxMM)
		{
			maxMM = m_sr[i];
			maxCell = i;
			maxCellAreaM2 = m_handArea[i];
		}
	}

	cout << "[WB19_SURU_OUT]"
		<< ",date=" << m_date

		<< ",suruPtr="
		<< static_cast<const void*>(m_sr)

		<< ",areaPtr="
		<< static_cast<const void*>(m_handArea)

		<< ",sumM3=" << sumM3
		<< ",sumRawMM=" << sumRawMM
		<< ",areaSum_m2=" << areaSumM2

		<< ",nonzeroCount=" << nonzeroCount
		<< ",maxCell=" << maxCell
		<< ",maxMM=" << maxMM
		<< ",maxCellArea_m2=" << maxCellAreaM2

		<< endl;

	// 输出19号子流域所有非零径流像元
	for (int i = 0; i < m_nCells; ++i)
	{
		if (CVT_INT(m_subbsnID[i]) != debugSubbasin)
		{
			continue;
		}

		if (m_sr[i] > 1.0e-10f)
		{
			cout << "[WB19_SURU_OUT_CELL]"
				<< ",date=" << m_date
				<< ",cell=" << i

				<< ",suruMM=" << m_sr[i]
				<< ",areaM2=" << m_handArea[i]

				<< ",suruAddr="
				<< static_cast<const void*>(&m_sr[i])

				<< ",areaAddr="
				<< static_cast<const void*>(&m_handArea[i])

				<< endl;
		}
	}

	cout.flush();
}
#endif


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
	}  else if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_handArea = data;
	} 
	else if (StringMatch(sk, VAR_OL_HAND_WTRDEP_AFT_INFIL)) {
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
    } else if (StringMatch(sk, VAR_OL_HAND_WTRDEP_AFT_DEP)) {
		*data = handWtrDepAftDep;
	} else if (StringMatch(sk, VAR_HAND_EVAP)) {
		*data = m_hand_eavp;
	} else if (StringMatch(sk, VAR_HAND_DEP)) {
		*data = m_hand_dep;
	}
	else {
        throw ModelException(MID_DEP_LINSLEY, "Get1DData", "Output " + sk + " does not exist.");
    }
}
