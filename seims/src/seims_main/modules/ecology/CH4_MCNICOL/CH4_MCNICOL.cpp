#include "CH4_MCNICOL.h"
#include "text.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>

using namespace std;


CH4_MCNICOL::CH4_MCNICOL() :
	m_nCells(-1), m_maxSoilLyrs(-1), m_dt(-1),

	// model parameters, default values based on McNicol et al. T5C1
	m_kp(NODATA_VALUE), m_p1(NODATA_VALUE), m_ko(NODATA_VALUE), m_p2(NODATA_VALUE),
	m_Qp(NODATA_VALUE), m_p3(NODATA_VALUE), m_Qo(NODATA_VALUE), 
	m_kEP(NODATA_VALUE), m_p5(NODATA_VALUE), m_laiLagDays(NODATA_VALUE),

	// input variables
	m_nSoilLyrs(nullptr), m_handWtrDep(nullptr), m_lai(nullptr),
	m_soilTemp(nullptr), m_soilThk(nullptr), m_soilWtrSto(nullptr),
	m_soilWP(nullptr), m_soilPor(nullptr), m_maxLai(nullptr), m_maxLaiYr(nullptr),

	// state and output variables
	m_ch4Flux(nullptr), m_ch4Ms(nullptr), m_ch4Prod(nullptr),
	m_ch4Oxid(nullptr), m_ch4Emit(nullptr), m_ch4Zw(nullptr),
	m_ch4FTProd(nullptr), m_ch4FWProd(nullptr), m_ch4FBProd(nullptr),
	m_ch4FTOxid(nullptr), m_ch4FWOxid(nullptr),

	m_laiHist(nullptr), m_laiHistLen(0), m_laiHistIdx(0)
{
}


CH4_MCNICOL::~CH4_MCNICOL() {
	if (m_ch4Flux != nullptr) Release1DArray(m_ch4Flux);
	if (m_ch4Ms != nullptr) Release1DArray(m_ch4Ms);
	if (m_ch4Prod != nullptr) Release1DArray(m_ch4Prod);
	if (m_ch4Oxid != nullptr) Release1DArray(m_ch4Oxid);
	if (m_ch4Emit != nullptr) Release1DArray(m_ch4Emit);
	if (m_ch4Zw != nullptr) Release1DArray(m_ch4Zw);

	if (m_ch4FTProd != nullptr) Release1DArray(m_ch4FTProd);
	if (m_ch4FWProd != nullptr) Release1DArray(m_ch4FWProd);
	if (m_ch4FBProd != nullptr) Release1DArray(m_ch4FBProd);
	if (m_ch4FTOxid != nullptr) Release1DArray(m_ch4FTOxid);
	if (m_ch4FWOxid != nullptr) Release1DArray(m_ch4FWOxid);

	if (m_laiHist != nullptr) Release2DArray(m_nCells, m_laiHist);
}


void CH4_MCNICOL::SetValue(const char* key, const float value) {
	string sk(key);

	if (StringMatch(sk, Tag_TimeStep)) {
		m_dt = CVT_INT(value);
	}
	else if (StringMatch(sk, VAR_CH4_KP)) {
		m_kp = value;
	}
	else if (StringMatch(sk, VAR_CH4_P1)) {
		m_p1 = value;
	}
	else if (StringMatch(sk, VAR_CH4_KO)) {
		m_ko = value;
	}
	else if (StringMatch(sk, VAR_CH4_P2)) {
		m_p2 = value;
	}
	else if (StringMatch(sk, VAR_CH4_QP)) {
		m_Qp = value;
	}
	else if (StringMatch(sk, VAR_CH4_P3)) {
		m_p3 = value;
	}
	else if (StringMatch(sk, VAR_CH4_QO)) {
		m_Qo = value;
	}
	else if (StringMatch(sk, VAR_CH4_KEP)) {
		m_kEP = value;
	}
	else if (StringMatch(sk, VAR_CH4_P5)) {
		m_p5 = value;
	}
	else if (StringMatch(sk, VAR_CH4_LAI_LAG)) {
		m_laiLagDays = value;
	}
	else {
		throw ModelException(MID_CH4_MCNICOL, "SetValue", "Parameter " + sk + " does not exist.");
	}
}


void CH4_MCNICOL::Set1DData(const char* key, const int n, float* data) {
	string sk(key);
	CheckInputSize(MID_CH4_MCNICOL, key, n, m_nCells);

	if (StringMatch(sk, VAR_SOILLAYERS)) {
		m_nSoilLyrs = data;
	}
	else if (StringMatch(sk, VAR_OL_HAND_WTRDEP_AFT_DEP)) {
		m_handWtrDep = data;
	}
	else if (StringMatch(sk, VAR_LAIDAY)) {
		m_lai = data;
	}
	else if (StringMatch(sk, VAR_BLAI)) {
		m_maxLai = data;
	}
	else if (StringMatch(sk, VAR_LAIYRMAX)) {
		m_maxLaiYr = data;
	}
	else {
		throw ModelException(MID_CH4_MCNICOL, "Set1DData", "Parameter " + sk + " does not exist.");
	}
}


void CH4_MCNICOL::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
	string sk(key);
	CheckInputSize2D(MID_CH4_MCNICOL, key, nrows, ncols, m_nCells, m_maxSoilLyrs);

	if (StringMatch(sk, VAR_SOILT)) {
		m_soilTemp = data;
	}
	else if (StringMatch(sk, VAR_SOILTHICK)) {
		m_soilThk = data;
	}
	else if (StringMatch(sk, VAR_SOL_ST)) {
		m_soilWtrSto = data;
	}
	else if (StringMatch(sk, VAR_SOL_WPMM)) {
		m_soilWP = data;
	}
	else if (StringMatch(sk, VAR_POROST)) {
		m_soilPor = data;
	}
	else {
		throw ModelException(MID_CH4_MCNICOL, "Set2DData", "Parameter " + sk + " does not exist.");
	}
}


bool CH4_MCNICOL::CheckInputData() {
	CHECK_POSITIVE(MID_CH4_MCNICOL, m_nCells);
	CHECK_POSITIVE(MID_CH4_MCNICOL, m_maxSoilLyrs);
	CHECK_POSITIVE(MID_CH4_MCNICOL, m_dt);

	CHECK_NODATA(MID_CH4_MCNICOL, m_kp);
	CHECK_NODATA(MID_CH4_MCNICOL, m_p1);
	CHECK_NODATA(MID_CH4_MCNICOL, m_ko);
	CHECK_NODATA(MID_CH4_MCNICOL, m_p2);
	CHECK_NODATA(MID_CH4_MCNICOL, m_Qp);
	CHECK_NODATA(MID_CH4_MCNICOL, m_p3);
	CHECK_NODATA(MID_CH4_MCNICOL, m_Qo);
	CHECK_NODATA(MID_CH4_MCNICOL, m_kEP);
	CHECK_NODATA(MID_CH4_MCNICOL, m_p5);

	CHECK_POINTER(MID_CH4_MCNICOL, m_nSoilLyrs);
	CHECK_POINTER(MID_CH4_MCNICOL, m_handWtrDep);
	CHECK_POINTER(MID_CH4_MCNICOL, m_lai);
	CHECK_POINTER(MID_CH4_MCNICOL, m_soilTemp);
	CHECK_POINTER(MID_CH4_MCNICOL, m_soilThk);
	CHECK_POINTER(MID_CH4_MCNICOL, m_soilWtrSto);
	CHECK_POINTER(MID_CH4_MCNICOL, m_soilWP);
	CHECK_POINTER(MID_CH4_MCNICOL, m_soilPor);

	CHECK_NODATA(MID_CH4_MCNICOL, m_laiLagDays);
	CHECK_POINTER(MID_CH4_MCNICOL, m_maxLaiYr);
	CHECK_POINTER(MID_CH4_MCNICOL, m_maxLai);

	return true;
}


void CH4_MCNICOL::InitialOutputs() {
	if (m_ch4Flux == nullptr) Initialize1DArray(m_nCells, m_ch4Flux, 0.f);

	// m_ch4Ms 是状态变量，只能初始化一次，不能每个时间步重置
	if (m_ch4Ms == nullptr) Initialize1DArray(m_nCells, m_ch4Ms, 0.f);

	if (m_ch4Prod == nullptr) Initialize1DArray(m_nCells, m_ch4Prod, 0.f);
	if (m_ch4Oxid == nullptr) Initialize1DArray(m_nCells, m_ch4Oxid, 0.f);
	if (m_ch4Emit == nullptr) Initialize1DArray(m_nCells, m_ch4Emit, 0.f);
	if (m_ch4Zw == nullptr) Initialize1DArray(m_nCells, m_ch4Zw, 0.f);

	if (m_ch4FTProd == nullptr) Initialize1DArray(m_nCells, m_ch4FTProd, 0.f);
	if (m_ch4FWProd == nullptr) Initialize1DArray(m_nCells, m_ch4FWProd, 0.f);
	if (m_ch4FBProd == nullptr) Initialize1DArray(m_nCells, m_ch4FBProd, 0.f);
	if (m_ch4FTOxid == nullptr) Initialize1DArray(m_nCells, m_ch4FTOxid, 0.f);
	if (m_ch4FWOxid == nullptr) Initialize1DArray(m_nCells, m_ch4FWOxid, 0.f);

	if (m_laiHist == nullptr) {
		m_laiHistLen = CVT_INT(Max(1.f, m_laiLagDays)) + 1;

		Initialize2DArray(m_nCells, m_laiHistLen, m_laiHist, 0.f);

#pragma omp parallel for
		for (int i = 0; i < m_nCells; i++) {
			for (int j = 0; j < m_laiHistLen; j++) {
				m_laiHist[i][j] = Max(m_lai[i], 0.f);
			}
		}

		m_laiHistIdx = 0;
	}
}


float CH4_MCNICOL::Clamp(float x, float lo, float hi) {
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}


float CH4_MCNICOL::CalcWaterTableFromLayers(int i) {

	int nlyr = CVT_INT(m_nSoilLyrs[i]);
	nlyr = Min(nlyr, m_maxSoilLyrs);

	float unsatDepth_mm = 0.f;
	float totalDepth_mm = 0.f;

	for (int k = 0; k < nlyr; k++) {
		float sw = m_soilWtrSto[i][k];
		float wp = m_soilWP[i][k];
		float por = m_soilPor[i][k];
		float thick = m_soilThk[i][k];

		if (por <= 0.f || thick <= 0.f) continue;

		float sat = (sw + wp) / (thick * por);
		sat = Clamp(sat, 0.f, 1.f);

		unsatDepth_mm += (1.f - sat) * thick;
		totalDepth_mm += thick;
	}

	if (totalDepth_mm <= 1.e-6f) {
		return -200.f;
	}

	if (unsatDepth_mm > totalDepth_mm) {
		unsatDepth_mm = totalDepth_mm;
	}

	// 地下水位位置，单位 cm；地表为 0，地下为负
	return -unsatDepth_mm / 10.f;
}


float CH4_MCNICOL::CalcMeanSoilTemp(int i) {
	/*float tsum = 0.f;
	float wsum = 0.f;

	int nlyr = CVT_INT(m_nSoilLyrs[i]);
	nlyr = Min(nlyr, m_maxSoilLyrs);

	for (int k = 0; k < nlyr; k++) {
		float t = m_soilTemp[i][k];
		float thick = m_soilThk[i][k];

		if (thick <= 0.f) {
			continue;
		}

		tsum += t * thick;
		wsum += thick;
	}

	if (wsum <= 1.e-12f) {
		return 0.f;
	}

	return tsum / wsum;*/

	// ======================================================
    // Use 0-15 cm shallow soil temperature for CH4_MCNICOL
    //
    // WISE soil layer thickness:
    // layer 0: 10 mm
    // layer 1: 40 mm
    // layer 2: 100 mm
    //
    // The first three layers represent 0-150 mm, i.e. 0-15 cm.
    // Temperature is thickness-weighted:
    // T = (T0*10 + T1*40 + T2*100) / 150
    // ======================================================

	float tsum = 0.f;
	float wsum = 0.f;

	int nlyr = CVT_INT(m_nSoilLyrs[i]);
	nlyr = Min(nlyr, m_maxSoilLyrs);

	// Only use the first three soil layers: 0-15 cm
	int useLyrs = Min(nlyr, 3);

	for (int k = 0; k < useLyrs; k++) {
		float t = m_soilTemp[i][k];
		float thick = m_soilThk[i][k];

		if (thick <= 0.f) {
			continue;
		}

		tsum += t * thick;
		wsum += thick;
	}

	if (wsum <= 1.e-12f) {
		return 0.f;
	}

	return tsum / wsum;
}


int CH4_MCNICOL::Execute() {
	CheckInputData();
	InitialOutputs();

	const float zs_cm = 0.f;
	const float D = 1.3f;
	const float p4 = 2.f;
	const float eps = 1.e-12f;

	float dt_day = m_dt / 86400.f;   // m_dt is in seconds

	//cout << "[CH4_MCNICOL] m_dt = " << m_dt
	//	<< " s, dt_day = " << dt_day << " d" << endl;

#pragma omp parallel for
	for (int i = 0; i < m_nCells; i++) {
		float handWtrDep_m = m_handWtrDep[i];
		float lai = m_lai[i];
		float T = CalcMeanSoilTemp(i);

		if (i == 1312) {
			cout << "[CH4 TEMP CHECK] cell=" << i
				<< ", T=" << T
				<< ", soilT0=" << m_soilTemp[i][0]
				<< ", flux_before=" << m_ch4Flux[i]
				<< endl;
		}

		float totalDepth_mm = 0.f;

		int nlyr = CVT_INT(m_nSoilLyrs[i]);
		nlyr = Min(nlyr, m_maxSoilLyrs);

		for (int k = 0; k < nlyr; k++) {
			float thick = m_soilThk[i][k];
			if (thick > 0.f) {
				totalDepth_mm += thick;
			}
		}

		float zb_cm = -totalDepth_mm / 10.f;   // mm 转 cm，地下为负值

		if (zb_cm >= -0.1f) {
			zb_cm = -200.f;   // 兜底：如果厚度异常，就默认 2 m
		}

		// 1. Estimate water table position zw, unit: cm
		float zw_cm = 0.f;

		if (handWtrDep_m > 1.e-6f) {
			zw_cm = handWtrDep_m * 100.f;
		}
		else {
			zw_cm = CalcWaterTableFromLayers(i);
			zw_cm = Clamp(zw_cm, zb_cm + 1.e-6f, zs_cm);
		}

		float depth_cm = zw_cm - zb_cm;
		depth_cm = Max(depth_cm, 0.1f);

		// ======================================================
        // McNicol original winter dormancy treatment
        // When soil temperature <= 0 °C, all CH4 processes stop.
        // Ms is NOT reset; it is frozen.
        // ======================================================
		if (T <= 0.f) {
			// Do not change m_ch4Ms[i]
			// Original model sets dMs/dt = 0, not Ms = 0.

			m_ch4Prod[i] = 0.f;
			m_ch4Oxid[i] = 0.f;
			m_ch4Emit[i] = 0.f;
			m_ch4Flux[i] = 0.f;

			// keep water table diagnostic
			m_ch4Zw[i] = zw_cm;

			// diagnostic factors
			m_ch4FTProd[i] = 0.f;
			m_ch4FWProd[i] = 0.f;
			m_ch4FBProd[i] = 0.f;
			m_ch4FTOxid[i] = 0.f;
			m_ch4FWOxid[i] = 0.f;

			continue;
		}

		// 2. Temperature factor for CH4 production
		float fT_p = 0.f;
		if (T <= 0.f) {
			fT_p = 0.f;
		}
		else if (T >= 30.f) {
			fT_p = 1.f;
		}
		else {
			fT_p = static_cast<float>(pow(Max(m_Qp, 1.f), (T - 30.f) / 10.f));
		}
		fT_p = Clamp(fT_p, 0.f, 1.f);

		// 3. Temperature factor for CH4 oxidation
		float fT_o = 0.f;
		if (T <= 0.f) {
			fT_o = 0.f;
		}
		else if (T >= 30.f) {
			fT_o = 1.f;
		}
		else {
			fT_o = static_cast<float>(pow(Max(m_Qo, 1.f), (T - 30.f) / 10.f));
		}
		fT_o = Clamp(fT_o, 0.f, 1.f);

		// 4. LAI as proxy of vegetation-derived substrate
        // using lagged LAI: LAI(t - tau)

		int lagSteps = CVT_INT(Max(0.f, m_laiLagDays) / Max(dt_day, 1.e-6f));
		lagSteps = Min(lagSteps, m_laiHistLen - 1);

		int lagIdx = m_laiHistIdx - lagSteps;
		if (lagIdx < 0) {
			lagIdx += m_laiHistLen;
		}

		float lai_lag = Max(m_laiHist[i][lagIdx], 0.f);

		float laiRef = m_maxLaiYr[i];
		if (laiRef <= 1.e-6f && m_maxLai != nullptr) {
			laiRef = m_maxLai[i];   // fallback to BLAI
		}
		laiRef = Max(laiRef, 1.e-6f);

		float lai_norm = lai_lag / laiRef;
		lai_norm = Clamp(lai_norm, 0.f, 1.f);

		float fb_p = static_cast<float>(pow(lai_norm, Max(m_p3, 0.f)));
		fb_p = Clamp(fb_p, 0.f, 1.f);

		// 5. Water-table factor for CH4 production
		float fw_p = 0.f;

		if (zw_cm < zs_cm) {
			float ratio = (zw_cm - zb_cm) / (zs_cm - zb_cm);
			ratio = Clamp(ratio, 0.f, 1.f);
			fw_p = static_cast<float>(pow(ratio, Max(m_p5, 0.f)));
		}
		else {
			float ratio = (zw_cm - zb_cm) / (zs_cm - zb_cm);
			ratio = Max(ratio, 1.e-6f);
			fw_p = static_cast<float>(pow(ratio, -Max(m_p1, 0.f)));
		}
		fw_p = Clamp(fw_p, 0.f, 1.f);

		// 6. CH4 production
		float fp = m_kp * fT_p * fb_p * fw_p;
		fp = Max(fp, 0.f);

		// 7. Water-table factor for CH4 oxidation
		float fw_o = static_cast<float>(exp(-Max(m_p2, 0.f) * depth_cm));
		fw_o = Clamp(fw_o, 0.f, 1.f);

		// 8. Emission rate coefficient
		float kD = D / static_cast<float>(pow(depth_cm, p4));
		float ke = kD + Max(m_kEP, 0.f);

		// 9. ODE update
		float lambda = Max(m_ko, 0.f) * fT_o * fw_o + ke;
		lambda = Max(lambda, 0.f);

		float Ms_old = Max(0.f, m_ch4Ms[i]);

		float Ms_new = Ms_old;
		float Ms_avg = Ms_old;

		if (lambda > eps) {
			float e = static_cast<float>(exp(-lambda * dt_day));

			Ms_new = Ms_old * e + fp / lambda * (1.f - e);

			Ms_avg = Ms_old * (1.f - e) / (lambda * dt_day)
				+ fp / lambda * (1.f - (1.f - e) / (lambda * dt_day));
		}
		else {
			Ms_new = Ms_old + fp * dt_day;
			Ms_avg = 0.5f * (Ms_old + Ms_new);
		}

		Ms_new = Max(0.f, Ms_new);
		Ms_avg = Max(0.f, Ms_avg);

		// 10. Oxidation and emission
		float fo = Max(m_ko, 0.f) * fT_o * fw_o * Ms_avg;
		float fe = ke * Ms_avg;

		fo = Max(0.f, fo);
		fe = Max(0.f, fe);

		// 11. Unit conversion
		float flux_mg_m2_h = fe * 10000.f / 24.f;

		m_ch4Ms[i] = Ms_new;
		m_ch4Prod[i] = fp;
		m_ch4Oxid[i] = fo;
		m_ch4Emit[i] = fe;
		m_ch4Flux[i] = flux_mg_m2_h;
		m_ch4Zw[i] = zw_cm;

		m_ch4FTProd[i] = fT_p;
		m_ch4FWProd[i] = fw_p;
		m_ch4FBProd[i] = fb_p;
		m_ch4FTOxid[i] = fT_o;
		m_ch4FWOxid[i] = fw_o;
	}

	m_laiHistIdx++;

	if (m_laiHistIdx >= m_laiHistLen) {
		m_laiHistIdx = 0;
	}

#pragma omp parallel for
	for (int i = 0; i < m_nCells; i++) {
		m_laiHist[i][m_laiHistIdx] = Max(m_lai[i], 0.f);
	}

	return 0;
}


void CH4_MCNICOL::Get1DData(const char* key, int* n, float** data) {
	InitialOutputs();

	string sk(key);
	*n = m_nCells;

	if (StringMatch(sk, VAR_CH4_MCNICOL_FLUX)) {
		*data = m_ch4Flux;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_MS)) {
		*data = m_ch4Ms;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_PROD)) {
		*data = m_ch4Prod;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_OXID)) {
		*data = m_ch4Oxid;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_EMIT)) {
		*data = m_ch4Emit;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_ZW)) {
		*data = m_ch4Zw;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_FTP)) {
		*data = m_ch4FTProd;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_FWP)) {
		*data = m_ch4FWProd;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_FBP)) {
		*data = m_ch4FBProd;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_FTO)) {
		*data = m_ch4FTOxid;
	}
	else if (StringMatch(sk, VAR_CH4_MCNICOL_FWO)) {
		*data = m_ch4FWOxid;
	}
	else {
		throw ModelException(MID_CH4_MCNICOL, "Get1DData", "Result " + sk + " does not exist.");
	}
}


void CH4_MCNICOL::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	string sk(key);
	throw ModelException(MID_CH4_MCNICOL, "Get2DData", "Result " + sk + " does not exist.");
}







