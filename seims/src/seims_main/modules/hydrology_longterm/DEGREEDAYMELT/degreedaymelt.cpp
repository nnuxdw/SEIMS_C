#include "degreedaymelt.h"
#include "text.h"
#include "utils_time.h"
DEGREEDAYMELT::DEGREEDAYMELT(void) : m_nCells(-1), m_nSubbsns(-1),m_beta0(NODATA_VALUE), m_tMean(NULL), 
m_tGmit(NODATA_VALUE),  m_dd_Snow(NODATA_VALUE), m_dd_glacierMelt(NODATA_VALUE) , m_prec(NULL),

m_dGlacRunoff(NULL), m_albedo(NULL),m_landUse(NULL),
ss(NULL), sw(NULL), su(NULL), sf(NULL), sg(NULL), sfg(NULL),Qfg(NULL),m_snomelt(NULL),
Qf(NULL),Qs(NULL),m_srf(NODATA_VALUE),m_sr(NULL), sublimation(NULL),m_area(NULL),
Kfg(NODATA_VALUE),Ca(NODATA_VALUE),Cg(NODATA_VALUE),m_csnow6(NODATA_VALUE), m_csnow12(NODATA_VALUE),
m_snowTemp(NODATA_VALUE),m_packT(NULL),m_maxTemp(NULL),m_t0(NODATA_VALUE), m_lagSnow(NODATA_VALUE)
{
	///
}
DEGREEDAYMELT::~DEGREEDAYMELT(void) {
	if (this->m_dGlacRunoff != NULL) Release1DArray(this->m_dGlacRunoff);
	//ljj
	if (this->sf != NULL) Release1DArray(this->sf);
	if (this->sg != NULL) Release1DArray(this->sg);
	if (this->sfg != NULL) Release1DArray(this->sfg);
	if (this->sw != NULL) Release1DArray(this->sw);

	if (this->Qfg != NULL) Release1DArray(this->Qfg);
	if (this->Qf != NULL) Release1DArray(this->Qf);
	if (this->Qs != NULL) Release1DArray(this->Qs);
	if (this->m_snomelt != NULL) Release1DArray(this->m_snomelt);
	if (this->sublimation != NULL) Release1DArray(this->sublimation);
	if (this->m_packT != NULL) Release1DArray(this->m_packT);
}

bool DEGREEDAYMELT::CheckInputData(void) {
	CHECK_POSITIVE("MID_DEGREEDAYMELT", this->m_nCells);
	CHECK_POINTER("MID_DEGREEDAYMELT", this->m_sr);
	CHECK_POINTER("MID_DEGREEDAYMELT", this->m_tMean);
	CHECK_POINTER("MID_DEGREEDAYMELT", this->m_prec);
	CHECK_POINTER("MID_DEGREEDAYMELT", this->m_landUse);

    return true;
}

void DEGREEDAYMELT:: InitialOutputs() {
	if (m_dGlacRunoff == NULL) Initialize1DArray(m_nCells, m_dGlacRunoff, 0.f);

	if (sg == NULL) Initialize1DArray(m_nCells, sg, 999999999.f);
	if (sw == NULL) Initialize1DArray(m_nCells, sw, 0.f);
	if (sfg == NULL) Initialize1DArray(m_nCells, sfg, 0.f);
	if (Qfg == NULL) Initialize1DArray(m_nCells, Qfg, 0.f);
    if (Qf == NULL) Initialize1DArray(m_nCells, Qf, 0.f);
    if (Qs == NULL) Initialize1DArray(m_nCells, Qs, 0.f);
	if (m_snomelt == NULL) Initialize1DArray(m_nCells, m_snomelt, 0.f);
	if (sublimation == NULL) Initialize1DArray(m_nCells, sublimation, 0.f);
	if (m_packT == NULL) Initialize1DArray(m_nCells, m_packT, 0.f);
}

bool DEGREEDAYMELT::glacier(const int i) {
	float Ps = 0.f; //snow
	float P1 = 0.f; //rain
	//Gao-2020-Stepwise modeling and the importance of internal variables validation to test model realism in a data scarce glacier basin
	//eq(2)
	if (m_tMean[i] > m_snowTemp) { //m_tGmit = Threshold temperature to split snowfall and rainfall 
		P1 = m_prec[i];
	}
	else {
		Ps = m_prec[i];
	}

	// float Ca = 1.f; //Factor for the influence of aspect on melt  //todo, 1D raster
	// //float m_albedo = 0.5f;
	// float Cg = 1.5;
	float m_iceStorage_init = 30.f;
	//eq(4)
	float sinv = CVT_FLT(sin(2.f * PI / 365.f * (m_dayOfYear - 81.f)));
    float cmelt = (m_csnow6 + m_csnow12) * 0.5f + (m_csnow6 - m_csnow12) * 0.5f * sinv;
	m_packT[i] = m_packT[i] * (1 - m_lagSnow) + m_tMean[i] * m_lagSnow;
	if (m_maxTemp[i] - m_t0 <= 0.f) {
		m_snomelt[i] = 0.f;
	}
	else {
		//m_snomelt[i] = 1 * m_tMean[i] + m_srf ;//* (1 - m_albedo[i]) * m_sr[i];  //m_dd_Snow = degree-day factor 
		
		//ljj++ modified the snowmelt (SNO_SP)
		//m_snomelt[i] = cmelt * m_tMean[i]*Ca*Cg;
		m_snomelt[i] = cmelt * ((m_packT[i] + m_maxTemp[i]) * 0.5f - m_t0);
	}

	sw[i] += Ps;
	//Snow sublimation
	sublimation[i] = 0.0864*(-7.093*m_tMean[i] + 28.26) / (pow(m_tMean[i], 2) - 3.593*m_tMean[i] + 5.175);
	sublimation[i] = Max(sublimation[i],0.f);
	sw[i] -= sublimation[i];
	if (sw[i] < 0.f) sw[i] = 0.f;
	m_snomelt[i] = Min(m_snomelt[i], sw[i]);
	sw[i] -= m_snomelt[i];

	//eq(6) 
	if (m_tMean[i] > 0 && sw[i] == 0) {
		//m_dGlacRunoff[i] = 10.f * m_tMean[i];
		m_dGlacRunoff[i] = m_dd_Snow * Cg * Ca * m_tMean[i];
	}
	if (m_tMean[i] <= 0 || sw[i] > 0) {
		m_dGlacRunoff[i] = 0.f;
	}

	m_dGlacRunoff[i] = Min(m_dGlacRunoff[i], sg[i]);
	sg[i] -= m_dGlacRunoff[i]*0.001*m_area[i];
	
	if (sg[i] < 0.f) sg[i] = 0.f;
	m_dGlacRunoff[i] += m_snomelt[i];

	//eq(7)
	//float Kfg = 5.8; //Recession coefficient of glacier runoff //todo, 1D / single?
	
	sfg[i] += P1 + m_dGlacRunoff[i];
	if (sfg[i] < 0.f) sfg[i] = 0.f;
	Qfg[i] = sfg[i] / Kfg;
	sfg[i] -= Qfg[i];
	return true;
}

int DEGREEDAYMELT::Execute() {
	this-> CheckInputData();
	this-> InitialOutputs();

	for (int i = 0; i < m_nCells; i++) {
		if (m_landUse[i] == LANDUSE_ID_GLC) {
			glacier(i);
		}
	}
    return 0;
}

void DEGREEDAYMELT::SetValue(const char *key, float data) {
    string s(key);
	if (StringMatch(s, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(data);
	else if (StringMatch(s, "Kfg")) this->Kfg = data;
	else if (StringMatch(s, "Ca")) this->Ca = data;
	else if (StringMatch(s, "Cg")) this->Cg = data;
	else if (StringMatch(s, "GL_SNDD")) this->m_dd_Snow = data;
	else if (StringMatch(s, VAR_C_SNOW6)) m_csnow6 = data;
    else if (StringMatch(s, VAR_C_SNOW12)) m_csnow12 = data;
	else if (StringMatch(s, VAR_T_SNOW)) m_snowTemp = data;
	else if (StringMatch(s, VAR_T0)) m_t0 = data;
    else if (StringMatch(s, VAR_LAG_SNOW)) m_lagSnow = data;
	else
	{
		throw ModelException("MID_DEGREEDAYMELT", "SetValue", "Parameter " + s
            + " does not exist in current module. Please contact the module developer.");
    }
}
//parameter and input 
void DEGREEDAYMELT::Set1DData(const char *key, int n, float *data) {
    string s(key);
	CheckInputSize("MID_DEGREEDAYMELT", key, n, m_nCells);

	if (StringMatch(s, VAR_LANDUSE)) m_landUse = data;
    else if (StringMatch(s, VAR_TMEAN)) { this->m_tMean = data; }
	else if (StringMatch(s, VAR_TMAX)) { this->m_maxTemp = data; }
    else if (StringMatch(s, VAR_PCP)) {this->m_prec = data; }
	else if (StringMatch(s, DataType_SolarRadiation)) { this->m_sr = data; }
	else if (StringMatch(s, VAR_AHRU)) {
		m_area = data;
	}
	else {
        throw ModelException("MID_DEGREEDAYMELT", "Set1DData", "Parameter " + s +
            " does not exist in current module. Please contact the module developer.");
    }
}
//output
void DEGREEDAYMELT::Get1DData(const char *key, int *n, float **data) {
    InitialOutputs();
    string s(key);
	*n = m_nCells;
    if (StringMatch(s, "GL_RO")) { 
		*data = this->m_dGlacRunoff;
		*n = m_nCells; 
	}
	if (StringMatch(s, "GL_V")) { 
		*data = this->sg;
		*n = m_nCells; 
	}
	if (StringMatch(s, "Qfg")) { 
		*n = m_nCells; 
		*data = this->Qfg; 
	}
}

