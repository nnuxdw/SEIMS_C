#include "NutrCH_QUAL2E.h"

#include "text.h"

NutrCH_QUAL2E::NutrCH_QUAL2E() :
    m_inputSubbsnID(-1), m_nCells(-1), m_dt(-1), m_reachDownStream(nullptr), m_nReaches(-1),
    m_ai0(-1.f), m_ai1(-1.f),
    m_ai2(-1.f), m_ai3(-1.f), m_ai4(-1.f), m_ai5(-1.f), m_ai6(-1.f), m_lambda0(-1.f),
    m_lambda1(-1.f), m_lambda2(-1.f), m_k_l(-1.f), m_k_n(-1.f), m_k_p(-1.f), m_p_n(-1.f),
    tfact(-1.f), m_rnum1(0.f), igropt(-1), m_mumax(-1.f), m_rhoq(-1.f), m_cod_n(-1), m_cod_k(-1),
    m_rchID(nullptr), m_soilTemp(nullptr), m_dayLen(nullptr), m_sr(nullptr),
    m_qRchOut(nullptr), m_chStorage(nullptr), m_rteWtrIn(nullptr), m_rteWtrOut(nullptr), m_chWtrDepth(nullptr),
    m_chTemp(nullptr), m_bc1(nullptr), m_bc2(nullptr), m_bc3(nullptr),
    m_bc4(nullptr), m_rs1(nullptr), m_rs2(nullptr), m_rs3(nullptr), m_rs4(nullptr),
    m_rs5(nullptr), m_rk1(nullptr), m_rk2(nullptr), m_rk3(nullptr),
    m_rk4(nullptr), m_chOrgNCo(NODATA_VALUE), m_chOrgPCo(NODATA_VALUE), m_latNO3ToCh(nullptr), m_surfRfNO3ToCh(nullptr),
    m_surfRfNH4ToCh(nullptr), m_surfRfSolPToCh(nullptr), m_surfRfCodToCh(nullptr), m_gwNO3ToCh(nullptr),
    m_gwSolPToCh(nullptr), m_surfRfSedOrgNToCh(nullptr), m_surfRfSedOrgPToCh(nullptr),
    m_surfRfSedAbsorbMinPToCh(nullptr), m_surfRfSedSorbMinPToCh(nullptr), m_no2ToCh(nullptr),
    // reaches related
    m_ptNO3ToCh(nullptr), m_ptNH4ToCh(nullptr), m_ptOrgNToCh(nullptr),
    // point source loadings to channel
    m_ptTNToCh(nullptr), m_ptSolPToCh(nullptr), m_ptOrgPToCh(nullptr), m_ptTPToCh(nullptr),
    m_ptCODToCh(nullptr), m_rchDeg(nullptr), m_chAlgae(nullptr), m_chOrgN(nullptr),
    // channel erosion
    m_chNH4(nullptr),
    // nutrient storage in channel
    m_chNO2(nullptr), m_chNO3(nullptr), m_chTN(nullptr), m_chOrgP(nullptr), m_chSolP(nullptr), m_chTP(nullptr),
    m_chCOD(nullptr), m_chDOx(nullptr), m_chChlora(nullptr), m_chSatDOx(NODATA_VALUE), m_chOutAlgae(nullptr),
    m_chOutAlgaeConc(nullptr),
    m_chOutChlora(nullptr),
    // nutrient amount outputs of channels
    m_chOutChloraConc(nullptr), m_chOutOrgN(nullptr), m_chOutOrgNConc(nullptr), m_chOutOrgP(nullptr),
    m_chOutOrgPConc(nullptr),
    m_chOutNH4(nullptr), m_chOutNH4Conc(nullptr), m_chOutNO2(nullptr), m_chOutNO2Conc(nullptr), m_chOutNO3(nullptr),
    m_chOutNO3Conc(nullptr), m_chOutSolP(nullptr),
    // nutrient concentration outputs of channels
    m_chOutSolPConc(nullptr), m_chOutCOD(nullptr), m_chOutCODConc(nullptr), m_chOutDOx(nullptr),
    m_chOutDOxConc(nullptr), m_chOutTN(nullptr), m_chOutTNConc(nullptr),
    m_chOutTP(nullptr), m_chOutTPConc(nullptr),
    m_chDaylen(nullptr), m_chSr(nullptr), m_chCellCount(nullptr),
    //ljj++
    m_subbasinsInfo(nullptr),curBasinArea(nullptr),m_islake(nullptr),m_lakevol(nullptr),m_isres(nullptr),
    m_klrd(-1.f), m_kld(-1.f), m_krd(-1.f), m_sv_lp(-1.f), m_sv_rp(-1.f), m_klp(-1.f), m_kd_lp(-1.f), m_klrp(-1.f),
    m_krp(-1.f), m_kd_rp(-1.f),m_npoc(-1.f),m_area(nullptr),
	m_surfRDOCToCH(nullptr),m_latRDOCToCH(nullptr),m_gwRDOCToCH(nullptr),m_latDICToCH(nullptr),m_surfDICToCH(nullptr),
    m_LPOCToCH(nullptr),m_RPOCToCH(nullptr),m_LDOCToCH(nullptr),m_gwDICToCH(nullptr),
    m_chDOCcon(nullptr),
    m_chOutLDOC(nullptr), m_chOutLDOCConc(nullptr), m_chLDOC(nullptr), 
	m_chOutDIC(nullptr), m_chOutDICConc(nullptr), m_chDIC(nullptr),
	m_chOutLPOC(nullptr), m_chOutLPOCConc(nullptr), m_chLPOC(nullptr), 
	m_chOutRPOC(nullptr), m_chOutRPOCConc(nullptr), m_chRPOC(nullptr), 
	m_chOutRDOC(nullptr), m_chOutRDOCConc(nullptr), m_chRDOC(nullptr),
    m_chsurfRDOC(nullptr), m_chlatRDOC(nullptr), m_chgwRDOC(nullptr), 
    m_chOutlatRDOC(nullptr), m_chOutsurfRDOC(nullptr), m_chOutgwRDOC(nullptr),
    m_chOutTotDOC(nullptr),m_chOutTotDOCConc(nullptr),
    m_chOutTotPOC(nullptr),m_chOutTotPOCConc(nullptr),
    m_Ab(nullptr), m_AbDeath(nullptr), m_AbINb(nullptr), m_AbIPb(nullptr),
    m_INb(nullptr), m_IPb(nullptr), m_sedst(nullptr), m_photo(nullptr),
	m_resp(nullptr), m_scbn(nullptr),m_lakepcp(nullptr),m_lakeperc(nullptr),m_gwdoc_sto(nullptr),
    m_qsRchOut(nullptr), m_qiRchOut(nullptr), m_qgRchOut(nullptr),m_ch_Temp(nullptr),m_chAirTemp_pre(nullptr),
    m_olQ2Rch(nullptr), m_ifluQ2Rch(nullptr), m_gndQ2Rch(nullptr),m_snowMelt(nullptr),
    m_subsnow(nullptr),m_airtemp(nullptr),m_chAirTemp(nullptr),m_chRH(nullptr),m_chWS(nullptr),
    m_rhd(nullptr),m_ws(nullptr),m_rrtime(nullptr),m_GlacierMelt(nullptr),m_subglacier(nullptr),m_lakearea(nullptr),
    m_chTemp_pre(nullptr),m_chSto_pre(nullptr),m_chSlope(nullptr),m_chArea(nullptr),m_intercpt(-1.f),
    m_A_b(nullptr),m_A_a(nullptr),m_A_Vb(nullptr),m_A_Va(nullptr),m_T_LKWB(nullptr),
    m_krp_1d(nullptr),m_kd_rp_1d(nullptr),m_sv_rp_1d(nullptr),m_krd_1d(nullptr)
{
}

NutrCH_QUAL2E::~NutrCH_QUAL2E() {
    /// reach parameters, will be released in ~clsReaches(). By lj, 2017-12-26

    if (nullptr != m_ptNO3ToCh) Release1DArray(m_ptNO3ToCh);
    if (nullptr != m_ptNH4ToCh) Release1DArray(m_ptNH4ToCh);
    if (nullptr != m_ptOrgNToCh) Release1DArray(m_ptOrgNToCh);
    if (nullptr != m_ptTNToCh) Release1DArray(m_ptTNToCh);
    if (nullptr != m_ptSolPToCh) Release1DArray(m_ptSolPToCh);
    if (nullptr != m_ptOrgPToCh) Release1DArray(m_ptOrgPToCh);
    if (nullptr != m_ptTPToCh) Release1DArray(m_ptTPToCh);
    if (nullptr != m_ptCODToCh) Release1DArray(m_ptCODToCh);
    /// storage in channel
    if (nullptr != m_chTN) Release1DArray(m_chTN);
    if (nullptr != m_chTP) Release1DArray(m_chTP);
    if (nullptr != m_chChlora) Release1DArray(m_chChlora);
    /// amount out of channel
    if (nullptr != m_chOutChlora) Release1DArray(m_chOutChlora);
    if (nullptr != m_chOutAlgae) Release1DArray(m_chOutAlgae);
    if (nullptr != m_chOutOrgN) Release1DArray(m_chOutOrgN);
    if (nullptr != m_chOutOrgP) Release1DArray(m_chOutOrgP);
    if (nullptr != m_chOutNH4) Release1DArray(m_chOutNH4);
    if (nullptr != m_chOutNO2) Release1DArray(m_chOutNO2);
    if (nullptr != m_chOutNO3) Release1DArray(m_chOutNO3);
    if (nullptr != m_chOutSolP) Release1DArray(m_chOutSolP);
    if (nullptr != m_chOutDOx) Release1DArray(m_chOutDOx);
    if (nullptr != m_chOutCOD) Release1DArray(m_chOutCOD);
    if (nullptr != m_chOutTN) Release1DArray(m_chOutTN);
    if (nullptr != m_chOutTP) Release1DArray(m_chOutTP);
    /// concentration out of channel
    if (nullptr != m_chOutChloraConc) Release1DArray(m_chOutChloraConc);
    if (nullptr != m_chOutAlgaeConc) Release1DArray(m_chOutAlgaeConc);
    if (nullptr != m_chOutOrgNConc) Release1DArray(m_chOutOrgNConc);
    if (nullptr != m_chOutOrgPConc) Release1DArray(m_chOutOrgPConc);
    if (nullptr != m_chOutNH4Conc) Release1DArray(m_chOutNH4Conc);
    if (nullptr != m_chOutNO2Conc) Release1DArray(m_chOutNO2Conc);
    if (nullptr != m_chOutNO3Conc) Release1DArray(m_chOutNO3Conc);
    if (nullptr != m_chOutSolPConc) Release1DArray(m_chOutSolPConc);
    if (nullptr != m_chOutDOxConc) Release1DArray(m_chOutDOxConc);
    if (nullptr != m_chOutCODConc) Release1DArray(m_chOutCODConc);
    if (nullptr != m_chOutTNConc) Release1DArray(m_chOutTNConc);
    if (nullptr != m_chOutTPConc) Release1DArray(m_chOutTPConc);
    // Parameters used in ParametersSubbasinForChannel()
    if (nullptr != m_chCellCount) Release1DArray(m_chCellCount);
    if (nullptr != m_chDaylen) Release1DArray(m_chDaylen);
    if (nullptr != m_chSr) Release1DArray(m_chSr);
    if (nullptr != m_chTemp) Release1DArray(m_chTemp);
    if (nullptr != m_chAirTemp) Release1DArray(m_chAirTemp);
    if (nullptr != m_chAirTemp_pre) Release1DArray(m_chAirTemp_pre);
    if (nullptr != m_chTemp_pre) Release1DArray(m_chTemp_pre);
    if (nullptr != m_chSto_pre) Release1DArray(m_chSto_pre);
    if (nullptr != m_chWS) Release1DArray(m_chWS);
    if (nullptr != m_chRH) Release1DArray(m_chRH);
	//ljj++
    if (nullptr != m_chDIC) Release1DArray(m_chDIC);
    if (nullptr != m_chRPOC) Release1DArray(m_chRPOC);
    if (nullptr != m_chLPOC) Release1DArray(m_chLPOC);
    if (nullptr != m_chRDOC) Release1DArray(m_chRDOC);
    if (nullptr != m_chLDOC) Release1DArray(m_chLDOC);

	if (nullptr != m_chOutDIC) Release1DArray(m_chOutDIC);
	if (nullptr != m_chOutDICConc) Release1DArray(m_chOutDICConc);
	if (nullptr != m_chOutLDOC) Release1DArray(m_chOutLDOC);
	if (nullptr != m_chOutLDOCConc) Release1DArray(m_chOutLDOCConc);
	if (nullptr != m_chOutLPOC) Release1DArray(m_chOutLPOC);
	if (nullptr != m_chOutLPOCConc) Release1DArray(m_chOutLPOCConc);
	if (nullptr != m_chOutRPOC) Release1DArray(m_chOutRPOC);
	if (nullptr != m_chOutRPOCConc) Release1DArray(m_chOutRPOCConc);
	if (nullptr != m_chOutRDOC) Release1DArray(m_chOutRDOC);
	if (nullptr != m_chOutRDOCConc) Release1DArray(m_chOutRDOCConc);
    if (nullptr != m_chOutTotDOC) Release1DArray(m_chOutTotDOC);
    if (nullptr != m_chOutTotDOCConc) Release1DArray(m_chOutTotDOCConc);
    if (nullptr != m_chOutTotPOC) Release1DArray(m_chOutTotPOC);
    if (nullptr != m_chOutTotPOCConc) Release1DArray(m_chOutTotPOCConc);
    if (nullptr != m_Ab) Release1DArray(m_Ab);
    if (nullptr != m_AbDeath) Release1DArray(m_AbDeath);
    if (nullptr != m_INb) Release1DArray(m_INb);
    if (nullptr != m_IPb) Release1DArray(m_IPb);
    if (nullptr != m_photo) Release1DArray(m_photo);
    if (nullptr != m_resp) Release1DArray(m_resp);
    if (nullptr != m_scbn) Release1DArray(m_scbn);
    if (nullptr != m_AbINb) Release1DArray(m_AbINb);
    if (nullptr != m_AbIPb) Release1DArray(m_AbIPb);

    if (nullptr != m_chOutsurfRDOC) Release1DArray(m_chOutsurfRDOC);
    if (nullptr != m_chOutlatRDOC) Release1DArray(m_chOutlatRDOC);
    if (nullptr != m_chOutgwRDOC) Release1DArray(m_chOutgwRDOC);
    if (nullptr != m_chsurfRDOC) Release1DArray(m_chsurfRDOC);
    if (nullptr != m_chlatRDOC) Release1DArray(m_chlatRDOC);
    if (nullptr != m_chgwRDOC) Release1DArray(m_chgwRDOC);
    if (nullptr != m_chDOCcon) Release1DArray(m_chDOCcon);

    if (nullptr != m_ch_Temp) Release1DArray(m_ch_Temp);
    if (nullptr != m_subsnow) Release1DArray(m_subsnow);
    if (nullptr != m_subglacier) Release1DArray(m_subglacier);

    if (m_T_LKWB != nullptr) Release2DArray(m_nReaches + 1, m_T_LKWB);
}

void NutrCH_QUAL2E::ParametersSubbasinForChannel() {
    if (nullptr == m_chCellCount) {
        Initialize1DArray(m_nReaches + 1, m_chCellCount, 0.f);
    }
    if (nullptr == m_chDaylen) {
        Initialize1DArray(m_nReaches + 1, m_chDaylen, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chSr, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chTemp, 0.f);
        Initialize1DArray(m_nReaches + 1, m_subsnow, 0.f);
        Initialize1DArray(m_nReaches + 1, m_subglacier, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chAirTemp, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chRH, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chWS, 0.f);
    }
//#pragma omp parallel
//not allow
    {
        // float* tmp_chDaylen = new(nothrow) float[m_nReaches + 1];
        // float* tmp_chSr = new(nothrow) float[m_nReaches + 1];
        // float* tmp_chTemp = new(nothrow) float[m_nReaches + 1];
        // float* tmp_snow = new(nothrow) float[m_nReaches + 1];
        // float* tmp_glacier = new(nothrow) float[m_nReaches + 1];
        // float* tmp_chAt = new(nothrow) float[m_nReaches + 1];
        // float* tmp_chrhd = new(nothrow) float[m_nReaches + 1];
        // float* tmp_chws = new(nothrow) float[m_nReaches + 1];
        // float* tmp_chCellCount = new(nothrow) float[m_nReaches + 1];
//#pragma omp parallel for
        for (int irch = 0; irch <= m_nReaches; irch++) {
            // tmp_chDaylen[irch] = 0.f;
            // tmp_chSr[irch] = 0.f;
            // tmp_chTemp[irch] = 0.f;
            // tmp_snow[irch] = 0.f;
            // tmp_glacier[irch] = 0.f;
            // tmp_chAt[irch] = 0.f;
            // tmp_chrhd[irch] = 0.f;
            // tmp_chws[irch] = 0.f;
            // tmp_chCellCount[irch] = 0.f;
            
            m_chDaylen[irch] = 0.f;
            m_chSr[irch] = 0.f;
            m_chTemp[irch] = 0.f;
            m_subsnow[irch] = 0.f;
            m_subglacier[irch] = 0.f;
            m_chAirTemp[irch] = 0.f;
            m_chRH[irch] = 0.f;
            m_chWS[irch] = 0.f;
            m_chCellCount[irch] = 0.f;
            
        }
//#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            int irch = CVT_INT(m_rchID[i]);
            m_subsnow[irch] += m_snowMelt[i]*0.001*m_area[i];  //mm to m3
            m_subglacier[irch] += m_GlacierMelt[i]*0.001*m_area[i];  //mm to m3
            if (m_rchID[i] <= 0.f) {
                continue;
            }
            
            //if (m_nReaches == 1) {  // deprecated code, left for remaind. lj
            //    subi = 1;
            //} else
            if (irch >= m_nReaches + 1) {
                throw ModelException(MID_NUTRCH_QUAL2E, "Execute",
                                     "The subbasin " + ValueToString(irch) + " is invalid.");
            }
            m_chDaylen[irch] += m_dayLen[i]*m_area[i];
            m_chSr[irch] += m_sr[i]*m_area[i];
            m_chTemp[irch] += m_airtemp[i]*m_area[i];
            m_chAirTemp[irch] += m_airtemp[i]*m_area[i];
            m_chRH[irch] += m_rhd[i]*m_area[i];
            m_chWS[irch] += m_ws[i]*m_area[i];
            m_chCellCount[irch] += m_area[i]; 
        }
//#pragma omp critical
        // {
        //     for (int irch = 1; irch <= m_nReaches; irch++) {
        //         m_chDaylen[irch] += tmp_chDaylen[irch];
        //         m_chSr[irch] += tmp_chSr[irch];
        //         m_chTemp[irch] += tmp_chTemp[irch];
        //         m_subsnow[irch] += tmp_snow[irch];
        //         m_subglacier[irch] += tmp_glacier[irch];
        //         m_chAirTemp[irch] += tmp_chAt[irch];
        //         m_chRH[irch] += tmp_chrhd[irch];
        //         m_chWS[irch] += tmp_chws[irch];
        //         m_chCellCount[irch] = tmp_chCellCount[irch];         
        //     }
        // }
        //delete[] tmp_chDaylen;
        // delete[] tmp_chSr;
        // delete[] tmp_chTemp;
        // delete[] tmp_snow;
        // delete[] tmp_glacier;
        // delete[] tmp_chAt;
        // delete[] tmp_chrhd;
        // delete[] tmp_chws;
        // delete[] tmp_chCellCount;
    }
//#pragma omp parallel for
    for (int irch = 1; irch <= m_nReaches; irch++) {
        if(m_chCellCount[irch] >0.f){
            m_chDaylen[irch] /= m_chCellCount[irch];
            m_chSr[irch] /= m_chCellCount[irch];
            m_chTemp[irch] /= m_chCellCount[irch];
            m_chAirTemp[irch] /= m_chCellCount[irch];
            m_chRH[irch] /= m_chCellCount[irch];
            m_chWS[irch] /= m_chCellCount[irch];
        }else{
            
        }
        m_subsnow[irch] /= m_dt;
        m_subglacier[irch] /= m_dt;

        // m_chDaylen[0] += m_chDaylen[irch];
        // m_chSr[0] += m_chSr[irch];
        // m_chTemp[0] += m_chTemp[irch];
    }
    // m_chDaylen[0] /= m_nReaches;
    // m_chSr[0] /= m_nReaches;
    // m_chTemp[0] /= m_nReaches;
}

bool NutrCH_QUAL2E::CheckInputCellSize(const char* key, const int n) {
    if (n <= 0) {
        throw ModelException(MID_NUTRCH_QUAL2E, "CheckInputSize", "Input data for " + string(key) +
                             " is invalid. The size could not be less than zero.");
    }
    if (m_nCells != n) {
        if (m_nCells <= 0) {
            m_nCells = n;
        } else {
            throw ModelException(MID_NUTRCH_QUAL2E, "CheckInputCellSize", "Input data for " + string(key) +
                                 " is invalid with size: " + ValueToString(n) + ". The origin size is " +
                                 ValueToString(m_nCells));
        }
    }
    return true;
}

bool NutrCH_QUAL2E::CheckInputData() {
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_dt);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_nReaches);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_rnum1);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, igropt);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_cod_n);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_cod_k);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_ai0);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_ai1);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_ai2);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_ai3);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_ai4);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_ai5);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_ai6);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_lambda0);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_lambda1);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_lambda2);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_k_l);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_k_n);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_k_p);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_p_n);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, tfact);
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_mumax);

    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_dayLen);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_sr);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_qRchOut);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_chStorage);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_rteWtrIn);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_rteWtrOut);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_chWtrDepth);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_latNO3ToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_surfRfNO3ToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_surfRfSolPToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_surfRfCodToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_gwNO3ToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_gwSolPToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_surfRfSedOrgNToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_surfRfSedOrgPToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_surfRfSedAbsorbMinPToCh);
    // CHECK_POINTER(MID_NUTRCH_QUAL2E, m_surfRfSedSorbMinPToCh);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_rchID);
    CHECK_POINTER(MID_NUTRCH_QUAL2E, m_soilTemp);
    return true;
}

void NutrCH_QUAL2E::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
    else if (StringMatch(sk, Tag_ChannelTimeStep)) m_dt = CVT_INT(value);
    else if (StringMatch(sk, VAR_RNUM1)) m_rnum1 = value;
    else if (StringMatch(sk, VAR_IGROPT)) igropt = CVT_INT(value);
    else if (StringMatch(sk, VAR_COD_N)) m_cod_n = value;
    else if (StringMatch(sk, VAR_COD_K)) m_cod_k = value;
    else if (StringMatch(sk, VAR_AI0)) m_ai0 = value;
    else if (StringMatch(sk, VAR_AI1)) m_ai1 = value;
    else if (StringMatch(sk, VAR_AI2)) m_ai2 = value;
    else if (StringMatch(sk, VAR_AI3)) m_ai3 = value;
    else if (StringMatch(sk, VAR_AI4)) m_ai4 = value;
    else if (StringMatch(sk, VAR_AI5)) m_ai5 = value;
    else if (StringMatch(sk, VAR_AI6)) m_ai6 = value;
    else if (StringMatch(sk, VAR_LAMBDA0)) m_lambda0 = value;
    else if (StringMatch(sk, VAR_LAMBDA1)) m_lambda1 = value;
    else if (StringMatch(sk, VAR_LAMBDA2)) m_lambda2 = value;
    else if (StringMatch(sk, VAR_K_L)) {
        m_k_l = value * 1.e-3f * 60.f;
        //convert units on k_l:read in as kJ/(m2*min), use as MJ/(m2*hr)
    } else if (StringMatch(sk, VAR_K_N)) m_k_n = value;
    else if (StringMatch(sk, VAR_K_P)) m_k_p = value;
    else if (StringMatch(sk, VAR_P_N)) m_p_n = value;
    else if (StringMatch(sk, VAR_TFACT)) tfact = value;
    else if (StringMatch(sk, VAR_MUMAX)) m_mumax = value;
    else if (StringMatch(sk, VAR_RHOQ)) m_rhoq = value;
    else if (StringMatch(sk, VAR_CH_ONCO)) m_chOrgNCo = value;
    else if (StringMatch(sk, VAR_CH_OPCO)) m_chOrgPCo = value;
    //ljj
    else if (StringMatch(sk, VAR_KLRD)) m_klrd = value;
    else if (StringMatch(sk, VAR_KLD)) m_kld = value;
    else if (StringMatch(sk, VAR_KRD)) m_krd = value;
    else if (StringMatch(sk, VAR_SVLP)) m_sv_lp = value;
    else if (StringMatch(sk, VAR_SVRP)) m_sv_rp = value;
    else if (StringMatch(sk, VAR_KLP)) m_klp = value;
    else if (StringMatch(sk, VAR_KDLP)) m_kd_lp = value;
    else if (StringMatch(sk, VAR_KLRP)) m_klrp = value;
    else if (StringMatch(sk, VAR_KRP)) m_krp = value;
    else if (StringMatch(sk, VAR_KDRP)) m_kd_rp = value;
    else if (StringMatch(sk, VAR_NPOC)) m_npoc = value;
    else {
        throw ModelException(MID_NUTRCH_QUAL2E, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void NutrCH_QUAL2E::SetValueByIndex(const char* key, const int index, const float data) {
    if (m_inputSubbsnID == 0) return;             // Not for omp version
    if (index <= 0 || index > m_nReaches) return; // index should belong 1 ~ m_nreach
    if (nullptr == m_chOutAlgae) InitialOutputs();
    string sk(key);
    /// IN/OUTPUT variables
    // Concentration (mg/L) and amount (kg)
    if (StringMatch(sk, VAR_CH_ALGAE)) m_chOutAlgae[index] = data;
    else if (StringMatch(sk, VAR_CH_ALGAEConc)) m_chOutAlgaeConc[index] = data;
    else if (StringMatch(sk, VAR_CH_NO2)) m_chOutNO2[index] = data;
    else if (StringMatch(sk, VAR_CH_NO2Conc)) m_chOutNO2Conc[index] = data;
    else if (StringMatch(sk, VAR_CH_COD)) m_chOutCOD[index] = data;
    else if (StringMatch(sk, VAR_CH_CODConc)) m_chOutCODConc[index] = data;
    else if (StringMatch(sk, VAR_CH_CHLORA)) m_chOutChlora[index] = data;
    else if (StringMatch(sk, VAR_CH_CHLORAConc)) m_chOutChloraConc[index] = data;
    else if (StringMatch(sk, VAR_CH_NO3)) m_chOutNO3[index] = data;
    else if (StringMatch(sk, VAR_CH_NO3Conc)) m_chOutNO3Conc[index] = data;
    else if (StringMatch(sk, VAR_CH_SOLP)) m_chOutSolP[index] = data;
    else if (StringMatch(sk, VAR_CH_SOLPConc)) m_chOutSolPConc[index] = data;
    else if (StringMatch(sk, VAR_CH_ORGN)) m_chOutOrgN[index] = data;
    else if (StringMatch(sk, VAR_CH_ORGNConc)) m_chOutOrgNConc[index] = data;
    else if (StringMatch(sk, VAR_CH_ORGP)) m_chOutOrgP[index] = data;
    else if (StringMatch(sk, VAR_CH_ORGPConc)) m_chOutOrgPConc[index] = data;
    else if (StringMatch(sk, VAR_CH_NH4)) m_chOutNH4[index] = data;
    else if (StringMatch(sk, VAR_CH_NH4Conc)) m_chOutNH4Conc[index] = data;
    else if (StringMatch(sk, VAR_CH_DOX)) m_chOutDOx[index] = data;
    else if (StringMatch(sk, VAR_CH_DOXConc)) m_chOutDOxConc[index] = data;
    else if (StringMatch(sk, VAR_CH_TN)) m_chOutTN[index] = data;
    else if (StringMatch(sk, VAR_CH_TNConc)) m_chOutTNConc[index] = data;
    else if (StringMatch(sk, VAR_CH_TP)) m_chOutTP[index] = data;
    else if (StringMatch(sk, VAR_CH_TPConc)) m_chOutTPConc[index] = data;
        // nutrient stored in reaches
    else if (StringMatch(sk, VAR_CHSTR_NO3)) m_chNO3[index] = data;
    else if (StringMatch(sk, VAR_CHSTR_NH4)) m_chNH4[index] = data;
    else if (StringMatch(sk, VAR_CHSTR_TN)) m_chTN[index] = data;
    else if (StringMatch(sk, VAR_CHSTR_TP)) m_chTP[index] = data;
    //ljj++
	else if (StringMatch(sk, VAR_CHSTR_DIC)) m_chDIC[index] = data;
	else if (StringMatch(sk, VAR_CH_DICConc)) m_chOutDICConc[index] = data;
	else if (StringMatch(sk, VAR_CH_DIC)) m_chOutDIC[index] = data;
	else if (StringMatch(sk, VAR_CHSTR_LDOC)) m_chLDOC[index] = data;
	else if (StringMatch(sk, VAR_CH_LDOCConc)) m_chOutLDOCConc[index] = data;
	else if (StringMatch(sk, VAR_CH_LDOC)) m_chOutLDOC[index] = data;
	else if (StringMatch(sk, VAR_CHSTR_LPOC)) m_chLPOC[index] = data;
	else if (StringMatch(sk, VAR_CH_LPOCConc)) m_chOutLPOCConc[index] = data;
	else if (StringMatch(sk, VAR_CH_LPOC)) m_chOutLPOC[index] = data;
	else if (StringMatch(sk, VAR_CHSTR_RPOC)) m_chRPOC[index] = data;
	else if (StringMatch(sk, VAR_CH_RPOCConc)) m_chOutRPOCConc[index] = data;
	else if (StringMatch(sk, VAR_CH_RPOC)) m_chOutRPOC[index] = data;
	else if (StringMatch(sk, VAR_CHSTR_RDOC)) m_chRDOC[index] = data;
	else if (StringMatch(sk, VAR_CH_RDOCConc)) m_chOutRDOCConc[index] = data;
	else if (StringMatch(sk, VAR_CH_RDOC)) m_chOutRDOC[index] = data;
    else if (StringMatch(sk, VAR_CH_TOTDOC)) m_chOutTotDOC[index] = data;
	else if (StringMatch(sk, VAR_CH_TOTDOCConc)) m_chOutTotDOCConc[index] = data;
    else if (StringMatch(sk, "CH_TOTPOC")) m_chOutTotPOC[index] = data;
	else if (StringMatch(sk, "CH_TOTPOCConc")) m_chOutTotPOCConc[index] = data;
    else if (StringMatch(sk, VAR_CHSTR_SURFRDOC)) m_chsurfRDOC[index] = data;
    else if (StringMatch(sk, VAR_CHSTR_LATRDOC)) m_chlatRDOC[index] = data;
    else if (StringMatch(sk, VAR_CHSTR_GWDRDOC)) m_chgwRDOC[index] = data;
    else if (StringMatch(sk, VAR_CH_SURFRDOC)) m_chOutsurfRDOC[index] = data;
    else if (StringMatch(sk, VAR_CH_LATRDOC)) m_chOutlatRDOC[index] = data;
    else if (StringMatch(sk, "CH_GWRDOC")) m_chOutgwRDOC[index] = data;
    else if (StringMatch(sk, "chdoc")) m_chDOCcon[index] = data;
    else {
        throw ModelException(MID_NUTRCH_QUAL2E, "SetValueByIndex", "Parameter " + sk + " does not exist.");
    }
}

void NutrCH_QUAL2E::Set1DData(const char* key, const int n, float* data) {
    string sk(key);
    if (StringMatch(sk, VAR_DAYLEN)) {
        CheckInputCellSize(key, n);
        m_dayLen = data;
        return;
    }
    if (StringMatch(sk, DataType_SolarRadiation)) {
        CheckInputCellSize(key, n);
        m_sr = data;
        return;
    }
    if (StringMatch(sk, DataType_MeanTemperature)) {
        CheckInputCellSize(key, n);
        m_airtemp = data;
        return;
    }
    if (StringMatch(sk, DataType_RelativeAirMoisture)) {
        CheckInputCellSize(key, n);
        m_rhd = data;
        return;
    }
    if (StringMatch(sk, DataType_WindSpeed)) {
        CheckInputCellSize(key, n);
        m_ws = data;
        return;
    }
    if (StringMatch(sk, VAR_STREAM_LINK)) {
        CheckInputCellSize(key, n);
        m_rchID = data;
        return;
    }
    if (StringMatch(sk, VAR_SOTE)) {
        CheckInputCellSize(key, n);
        m_soilTemp = data;
        return;
    }
//ljj++
    if (StringMatch(sk, VAR_AHRU)) {
        CheckInputCellSize(key, n);
        m_area = data;
        return;
    }
    if (StringMatch(sk, VAR_SNME)) {
        CheckInputCellSize(key, n);
        m_snowMelt = data;
        return;
    }
    if (StringMatch(sk, "GL_RO")) { 
        CheckInputCellSize(key, n);
        m_GlacierMelt = data;
        return;
	}
    if (StringMatch(sk, "krp_1d")) {
        CheckInputCellSize(key, n);
        m_krp_1d = data;
        return;
    }
    if (StringMatch(sk, "kd_rp_1d")) {
        CheckInputCellSize(key, n);
        m_kd_rp_1d = data;
        return;
    }
    if (StringMatch(sk, "sv_rp_1d")) {
        CheckInputCellSize(key, n);
        m_sv_rp_1d = data;
        return;
    }
    if (StringMatch(sk, "krd_1d")) {
        CheckInputCellSize(key, n);
        m_krd_1d = data;
        return;
    }
    CheckInputSize(MID_NUTRCH_QUAL2E, key, n - 1, m_nReaches);
    if (StringMatch(sk, VAR_QRECH)) m_qRchOut = data;
    else if (StringMatch(sk, VAR_CHST)) {
        m_chStorage = data;
        for (int i = 0; i <= m_nReaches; i++) {
            // input from SetReaches(), unit is mg/L, need to be converted to kg
            float cvt_conc2amount = m_chStorage[i] * 0.001f;
            m_chAlgae[i] *= cvt_conc2amount;
            m_chOrgN[i] *= cvt_conc2amount;
            m_chOrgP[i] *= cvt_conc2amount;
            m_chNH4[i] *= cvt_conc2amount;
            m_chNO2[i] *= cvt_conc2amount;
            m_chNO3[i] *= cvt_conc2amount;
            m_chSolP[i] *= cvt_conc2amount;
            m_chDOx[i] *= cvt_conc2amount;
            m_chCOD[i] *= cvt_conc2amount;
            //ljj++
			m_chDIC[i] *= cvt_conc2amount;
			m_chLDOC[i] *= cvt_conc2amount;
			m_chLPOC[i] *= cvt_conc2amount;
			m_chRPOC[i] *= cvt_conc2amount;
			m_chRDOC[i] *= cvt_conc2amount;
            m_chsurfRDOC[i] *= cvt_conc2amount;
            m_chlatRDOC[i] *= cvt_conc2amount;
            m_chgwRDOC[i] *= cvt_conc2amount;
        }
    } else if (StringMatch(sk, VAR_RTE_WTRIN)) m_rteWtrIn = data;
    else if (StringMatch(sk, VAR_RTE_WTROUT)) m_rteWtrOut = data;
    else if (StringMatch(sk, VAR_CHWTRDEPTH)) m_chWtrDepth = data;
    else if (StringMatch(sk, VAR_WATTEMP)) m_chTemp = data;

    else if (StringMatch(sk, VAR_LATNO3_TOCH)) m_latNO3ToCh = data;
    else if (StringMatch(sk, VAR_SUR_NO3_TOCH)) m_surfRfNO3ToCh = data;
    else if (StringMatch(sk, VAR_SUR_NH4_TOCH)) m_surfRfNH4ToCh = data;
    else if (StringMatch(sk, VAR_SUR_SOLP_TOCH)) m_surfRfSolPToCh = data;
    else if (StringMatch(sk, VAR_SUR_COD_TOCH)) m_surfRfCodToCh = data;
    else if (StringMatch(sk, VAR_NO3GW_TOCH)) m_gwNO3ToCh = data;
    else if (StringMatch(sk, VAR_MINPGW_TOCH)) m_gwSolPToCh = data;
    else if (StringMatch(sk, VAR_SEDORGN_TOCH)) m_surfRfSedOrgNToCh = data;
    else if (StringMatch(sk, VAR_SEDORGP_TOCH)) m_surfRfSedOrgPToCh = data;
    else if (StringMatch(sk, VAR_SEDMINPA_TOCH)) m_surfRfSedAbsorbMinPToCh = data;
    else if (StringMatch(sk, VAR_SEDMINPS_TOCH)) m_surfRfSedSorbMinPToCh = data;
    else if (StringMatch(sk, VAR_NO2_TOCH)) m_no2ToCh = data;
    else if (StringMatch(sk, VAR_RCH_DEG)) m_rchDeg = data;
    //ljj++
	else if (StringMatch(sk, VAR_surfDICtoCH)) m_surfDICToCH = data;
	else if (StringMatch(sk, VAR_latDICtoCH)) m_latDICToCH = data;
	else if (StringMatch(sk, VAR_LPOCtoCH)) m_LPOCToCH = data;
	else if (StringMatch(sk, VAR_RPOCtoCH)) m_RPOCToCH = data;
    else if (StringMatch(sk, VAR_LDOCtoCH)) m_LDOCToCH = data;
	else if (StringMatch(sk, VAR_surfRDOCtoCH)) m_surfRDOCToCH = data;
	else if (StringMatch(sk, VAR_latRDOCtoCH)) m_latRDOCToCH = data;
	else if (StringMatch(sk, "gw_RDOCtoCH")) m_gwRDOCToCH = data;
    else if (StringMatch(sk, "gw_DICtoCH")) m_gwDICToCH = data;
    else if (StringMatch(sk, VAR_SEDSTO_CH)) m_sedst = data;
    else if (StringMatch(sk, VAR_QS)) m_qsRchOut= data;
    else if (StringMatch(sk, VAR_QI)) m_qiRchOut= data;
    else if (StringMatch(sk, VAR_QG)) m_qgRchOut= data;
    else if (StringMatch(sk, VAR_SBOF)) m_olQ2Rch = data;
    else if (StringMatch(sk, VAR_SBIF)) m_ifluQ2Rch = data;
    else if (StringMatch(sk, VAR_SBQG)) m_gndQ2Rch = data;
    else if (StringMatch(sk, "rrtime")) m_rrtime = data;
    else if (StringMatch(sk, "LAKE_P")) m_lakepcp = data;
    else if (StringMatch(sk, "LAKE_E")) m_lakeperc = data;
    else if (StringMatch(sk, "gw_DOCsto")) m_gwdoc_sto = data;
    else {
        throw ModelException(MID_NUTRCH_QUAL2E, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void NutrCH_QUAL2E::SetReaches(clsReaches* reaches) {
    if (nullptr == reaches) {
        throw ModelException(MID_NUTRCH_QUAL2E, "SetReaches", "The reaches input can not to be NULL.");
    }
    m_nReaches = reaches->GetReachNumber();

    if (nullptr == m_reachDownStream) reaches->GetReachesSingleProperty(REACH_DOWNSTREAM, &m_reachDownStream);
    if (nullptr == m_bc1) reaches->GetReachesSingleProperty(REACH_BC1, &m_bc1);
    if (nullptr == m_bc2) reaches->GetReachesSingleProperty(REACH_BC2, &m_bc2);
    if (nullptr == m_bc3) reaches->GetReachesSingleProperty(REACH_BC3, &m_bc3);
    if (nullptr == m_bc4) reaches->GetReachesSingleProperty(REACH_BC4, &m_bc4);
    if (nullptr == m_rk1) reaches->GetReachesSingleProperty(REACH_RK1, &m_rk1);
    if (nullptr == m_rk2) reaches->GetReachesSingleProperty(REACH_RK2, &m_rk2);
    if (nullptr == m_rk3) reaches->GetReachesSingleProperty(REACH_RK3, &m_rk3);
    if (nullptr == m_rk4) reaches->GetReachesSingleProperty(REACH_RK4, &m_rk4);
    if (nullptr == m_rs1) reaches->GetReachesSingleProperty(REACH_RS1, &m_rs1);
    if (nullptr == m_rs2) reaches->GetReachesSingleProperty(REACH_RS2, &m_rs2);
    if (nullptr == m_rs3) reaches->GetReachesSingleProperty(REACH_RS3, &m_rs3);
    if (nullptr == m_rs4) reaches->GetReachesSingleProperty(REACH_RS4, &m_rs4);
    if (nullptr == m_rs5) reaches->GetReachesSingleProperty(REACH_RS5, &m_rs5);
    /// these parameters' unit is mg/L now, and will be converted to kg in Set1DData.
    if (nullptr == m_chAlgae) reaches->GetReachesSingleProperty(REACH_ALGAE, &m_chAlgae);
    if (nullptr == m_chOrgN) reaches->GetReachesSingleProperty(REACH_ORGN, &m_chOrgN);
    if (nullptr == m_chOrgP) reaches->GetReachesSingleProperty(REACH_ORGP, &m_chOrgP);
    if (nullptr == m_chNH4) reaches->GetReachesSingleProperty(REACH_NH4, &m_chNH4);
    if (nullptr == m_chNO2) reaches->GetReachesSingleProperty(REACH_NO2, &m_chNO2);
    if (nullptr == m_chNO3) reaches->GetReachesSingleProperty(REACH_NO3, &m_chNO3);
    if (nullptr == m_chSolP) reaches->GetReachesSingleProperty(REACH_SOLP, &m_chSolP);
    if (nullptr == m_chDOx) reaches->GetReachesSingleProperty(REACH_DISOX, &m_chDOx);
    if (nullptr == m_chCOD) reaches->GetReachesSingleProperty(REACH_BOD, &m_chCOD);

	//ljj++
    if (nullptr == m_chArea) reaches->GetReachesSingleProperty(REACH_AREA, &m_chArea);
    if (nullptr == m_chSlope) reaches->GetReachesSingleProperty(REACH_SLOPE, &m_chSlope);
	// if (nullptr == m_chDIC) reaches->GetReachesSingleProperty(REACH_DIC, &m_chDIC);
	// if (nullptr == m_chLDOC) reaches->GetReachesSingleProperty(REACH_LDOC, &m_chLDOC);
	// if (nullptr == m_chLPOC) reaches->GetReachesSingleProperty(REACH_LPOC, &m_chLPOC);
	// if (nullptr == m_chRPOC) reaches->GetReachesSingleProperty(REACH_RPOC, &m_chRPOC);
	// if (nullptr == m_chRDOC) reaches->GetReachesSingleProperty(REACH_RDOC, &m_chRDOC);
    // if (nullptr == m_chsurfRDOC) reaches->GetReachesSingleProperty(REACH_SURFRDOC, &m_chsurfRDOC);
	// if (nullptr == m_chlatRDOC) reaches->GetReachesSingleProperty(REACH_LATRDOC, &m_chlatRDOC);
	// if (nullptr == m_chgwRDOC) reaches->GetReachesSingleProperty(REACH_GWDRDOC, &m_chgwRDOC);
    // if (nullptr == m_chDOCcon) reaches->GetReachesSingleProperty("chDOC", &m_chDOCcon);
    
    // if (nullptr == m_chArea) Initialize1DArray(m_nReaches + 1, m_chArea, 0.f);
    // if (nullptr == m_chSlope) Initialize1DArray(m_nReaches + 1, m_chSlope, 0.f);
	if (nullptr == m_chDIC) Initialize1DArray(m_nReaches + 1, m_chDIC, 0.f);
	if (nullptr == m_chLDOC) Initialize1DArray(m_nReaches + 1, m_chLDOC, 0.f);
	if (nullptr == m_chLPOC) Initialize1DArray(m_nReaches + 1, m_chLPOC, 0.f);
	if (nullptr == m_chRPOC) Initialize1DArray(m_nReaches + 1, m_chRPOC, 0.f);
	if (nullptr == m_chRDOC) Initialize1DArray(m_nReaches + 1, m_chRDOC, 0.f);
    if (nullptr == m_chsurfRDOC) Initialize1DArray(m_nReaches + 1, m_chsurfRDOC, 0.f);
	if (nullptr == m_chlatRDOC) Initialize1DArray(m_nReaches + 1, m_chlatRDOC, 0.f);
	if (nullptr == m_chgwRDOC) Initialize1DArray(m_nReaches + 1, m_chgwRDOC, 0.f);
    if (nullptr == m_chDOCcon) Initialize1DArray(m_nReaches + 1, m_chDOCcon, 0.f);

    if (nullptr == m_chChlora) Initialize1DArray(m_nReaches + 1, m_chChlora, 0.f);
    if (nullptr == m_chTP) Initialize1DArray(m_nReaches + 1, m_chTP, 0.f);
    if (nullptr == m_chTN) Initialize1DArray(m_nReaches + 1, m_chTN, 0.f);
    if (nullptr == m_islake) reaches->GetReachesSingleProperty(REACH_ISLAKE, &m_islake);
    if (nullptr == m_isres) reaches->GetReachesSingleProperty(REACH_ISRES, &m_isres);
    if (nullptr == m_lakevol) reaches->GetReachesSingleProperty(REACH_LAKEVOL, &m_lakevol);
    if (nullptr == m_lakearea) reaches->GetReachesSingleProperty(REACH_LAKEAREA, &m_lakearea);
    if (nullptr == m_A_Va) reaches->GetReachesSingleProperty("A_Va", &m_A_Va);
    if (nullptr == m_A_Vb) reaches->GetReachesSingleProperty("A_Vb", &m_A_Vb);
    if (nullptr == m_A_a) reaches->GetReachesSingleProperty("A_a", &m_A_a);
    if (nullptr == m_A_b) reaches->GetReachesSingleProperty("A_b", &m_A_b);

    m_reachUpStream = reaches->GetUpStreamIDs();
    m_reachLayers = reaches->GetReachLayers();
}

void NutrCH_QUAL2E::SetScenario(Scenario* sce) {
    if (nullptr == sce) {
        throw ModelException(MID_NUTRCH_QUAL2E, "SetScenario", "The scenario can not to be NULL.");
    }
    map<int, BMPFactory *>& tmpBMPFactories = sce->GetBMPFactories();
    for (auto it = tmpBMPFactories.begin(); it != tmpBMPFactories.end(); ++it) {
        /// Key is uniqueBMPID, which is calculated by BMP_ID * 100000 + subScenario;
        if (it->first / 100000 != BMP_TYPE_POINTSOURCE) continue;
#ifdef HAS_VARIADIC_TEMPLATES
        m_ptSrcFactory.emplace(it->first, static_cast<BMPPointSrcFactory *>(it->second));
#else
        m_ptSrcFactory.insert(make_pair(it->first, static_cast<BMPPointSrcFactory *>(it->second)));
#endif
    }
}

void NutrCH_QUAL2E::InitialOutputs() {
    CHECK_POSITIVE(MID_NUTRCH_QUAL2E, m_nReaches);
    if (m_T_LKWB == nullptr) Initialize2DArray(m_nReaches + 1, 8, m_T_LKWB, 0.f);
    if (nullptr == m_chOutAlgae) {
        m_chSatDOx = 0.f;
        Initialize1DArray(m_nReaches + 1, m_chOutChlora, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutAlgae, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutOrgN, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutOrgP, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutNH4, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutNO2, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutNO3, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutSolP, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutDOx, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutCOD, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutTN, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutTP, 0.f);

        Initialize1DArray(m_nReaches + 1, m_chOutChloraConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutAlgaeConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutOrgNConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutOrgPConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutNH4Conc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutNO2Conc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutNO3Conc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutSolPConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutDOxConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutCODConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutTNConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutTPConc, 0.f);
		//ljj++
		Initialize1DArray(m_nReaches + 1, m_chOutDIC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutDICConc, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutLDOC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutLDOCConc, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutLPOC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutLPOCConc, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutRPOC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutRPOCConc, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutRDOC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutRDOCConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutTotDOC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutTotDOCConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutTotPOC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_chOutTotPOCConc, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutsurfRDOC, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutlatRDOC, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chOutgwRDOC, 0.f);
		Initialize1DArray(m_nReaches + 1, m_Ab, 0.001f);
		Initialize1DArray(m_nReaches + 1, m_AbDeath, 0.f);
		Initialize1DArray(m_nReaches + 1, m_INb, 0.f);
		Initialize1DArray(m_nReaches + 1, m_IPb, 0.f);
		Initialize1DArray(m_nReaches + 1, m_photo, 0.f);
		Initialize1DArray(m_nReaches + 1, m_resp, 0.f);
		Initialize1DArray(m_nReaches + 1, m_scbn, 0.01f);
		Initialize1DArray(m_nReaches + 1, m_AbINb, 0.01f);
		Initialize1DArray(m_nReaches + 1, m_AbIPb, 0.01f);
        Initialize1DArray(m_nReaches + 1, m_ch_Temp, 1.f);
        Initialize1DArray(m_nReaches + 1, m_chAirTemp_pre, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chTemp_pre, 0.f);
        Initialize1DArray(m_nReaches + 1, m_chSto_pre, 0.f);
        if (curBasinArea == nullptr) {
            Initialize1DArray(m_nReaches + 1, curBasinArea, 0.f);
            for (auto it = m_subbasinIDs.begin(); it != m_subbasinIDs.end(); ++it) {
            int subID = *it;
            Subbasin* curSub = m_subbasinsInfo->GetSubbasinByID(subID);
            // get percolation from the bottom soil layer at the subbasin scale
            int curCellsNum = curSub->GetCellCount();
            int* curCells = curSub->GetCells();
                for (int i = 0; i < curCellsNum; i++) {
                    curBasinArea[subID] += m_area[curCells[i]];
                }
            }
        }
    }
}

void NutrCH_QUAL2E::SetSubbasins(clsSubbasins* subbasins) {
	if (nullptr == m_subbasinsInfo) {
		m_subbasinsInfo = subbasins;
		m_subbasinIDs = m_subbasinsInfo->GetSubbasinIDs();
	}
}

void NutrCH_QUAL2E::PointSourceLoading() {
    /// initialization and reset to 0.f
    if (nullptr == m_ptNO3ToCh) {
        Initialize1DArray(m_nReaches + 1, m_ptNO3ToCh, 0.f);
        Initialize1DArray(m_nReaches + 1, m_ptNH4ToCh, 0.f);
        Initialize1DArray(m_nReaches + 1, m_ptOrgNToCh, 0.f);
        Initialize1DArray(m_nReaches + 1, m_ptTNToCh, 0.f);
        Initialize1DArray(m_nReaches + 1, m_ptSolPToCh, 0.f);
        Initialize1DArray(m_nReaches + 1, m_ptOrgPToCh, 0.f);
        Initialize1DArray(m_nReaches + 1, m_ptTPToCh, 0.f);
        Initialize1DArray(m_nReaches + 1, m_ptCODToCh, 0.f);
    } else {
        /// reset to zero for the current timestep
#pragma omp parallel for
        for (int i = 0; i <= m_nReaches; i++) {
            m_ptNO3ToCh[i] = 0.f;
            m_ptNH4ToCh[i] = 0.f;
            m_ptOrgNToCh[i] = 0.f;
            m_ptTNToCh[i] = 0.f;
            m_ptSolPToCh[i] = 0.f;
            m_ptOrgPToCh[i] = 0.f;
            m_ptTPToCh[i] = 0.f;
            m_ptCODToCh[i] = 0.f;
        }
    }
    /// load point source nutrient (kg) on current day from Scenario
    for (auto it = m_ptSrcFactory.begin(); it != m_ptSrcFactory.end(); ++it) {
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
            // 1.2 Otherwise, get the nutrient concentration, mg/L
            float per_wtr = curPtMgt->GetWaterVolume();
            float per_no3 = curPtMgt->GetNO3();
            float per_nh4 = curPtMgt->GetNH4();
            float per_orgn = curPtMgt->GetOrgN();
            float per_solp = curPtMgt->GetSolP();
            float per_orgp = curPtMgt->GetOrgP();
            float per_cod = curPtMgt->GetCOD();
            // 1.3 Sum up all point sources
            for (auto locIter = ptSrcIDs.begin(); locIter != ptSrcIDs.end(); ++locIter) {
                if (pointSrcLocsMap.find(*locIter) != pointSrcLocsMap.end()) {
                    PointSourceLocations* curPtLoc = pointSrcLocsMap.at(*locIter);
                    int curSubID = curPtLoc->GetSubbasinID();
                    float cvt = per_wtr * curPtLoc->GetSize() * 0.001f * m_dt * 1.1574074074074073e-05f;
                    /// mg/L ==> kg / timestep
                    m_ptNO3ToCh[curSubID] += per_no3 * cvt;
                    m_ptNH4ToCh[curSubID] += per_nh4 * cvt;
                    m_ptOrgNToCh[curSubID] += per_orgn * cvt;
                    m_ptOrgPToCh[curSubID] += per_orgp * cvt;
                    m_ptSolPToCh[curSubID] += per_solp * cvt;
                    m_ptCODToCh[curSubID] += per_cod * cvt;
                    m_ptTNToCh[curSubID] += (per_no3 + per_nh4 + per_orgn) * cvt;
                    m_ptTPToCh[curSubID] += (per_solp + per_orgp) * cvt;
                }
            }
        }
    }
    // sum up point sources loadings of all subbasins
    for (int i = 1; i <= m_nReaches; i++) {
        m_ptTNToCh[0] += m_ptTNToCh[i];
        m_ptTPToCh[0] += m_ptTPToCh[i];
        m_ptCODToCh[0] += m_ptCODToCh[i];
    }
}

int NutrCH_QUAL2E::Execute() {
    CheckInputData();
    InitialOutputs();
    // load point source loadings from Scenarios
    PointSourceLoading();
    // Calculate average day length, solar radiation, and temperature for each channel
    ParametersSubbasinForChannel();

    for (auto it = m_reachLayers.begin(); it != m_reachLayers.end(); ++it) {
        // There are not any flow relationship within each routing layer.
        // So parallelization can be done here.
        int reachNum = CVT_INT(it->second.size());
        // the size of m_reachLayers (map) is equal to the maximum stream order
//#pragma omp parallel for
        for (int i = 0; i < reachNum; i++) {
            int reachIndex = it->second[i];
            if (m_inputSubbsnID == 0 || m_inputSubbsnID == reachIndex) {
                AddInputNutrient(reachIndex);
                swat_rtmp(reachIndex);
                NutrientTransform(reachIndex);
                RouteOut(reachIndex);
            }
        }
    }
    return 0;
}

void NutrCH_QUAL2E::AddInputNutrient(const int i) {
    /// nutrient amount from upstream routing will be accumulated to current storage
    m_T_LKWB[i][0] = 0.f;
    for (auto upRchID = m_reachUpStream.at(i).begin(); upRchID != m_reachUpStream.at(i).end(); ++upRchID) {
        int upReachId = *upRchID;
        m_chOrgN[i] += m_chOutOrgN[upReachId];
        m_chNO3[i] += m_chOutNO3[upReachId];
        m_chNO2[i] += m_chOutNO2[upReachId];
        m_chNH4[i] += m_chOutNH4[upReachId];
        m_chOrgP[i] += m_chOutOrgP[upReachId];
        m_chSolP[i] += m_chOutSolP[upReachId];
        m_chCOD[i] += m_chOutCOD[upReachId];
        m_chDOx[i] += m_chOutDOx[upReachId];
        m_chChlora[i] += m_chOutChlora[upReachId];
        m_chAlgae[i] += m_chOutAlgae[upReachId];
        //ljj++
		m_chDIC[i] += m_chOutDIC[upReachId];        //kg
		m_chLDOC[i] +=  m_chOutLDOC[upReachId];
		m_chLPOC[i] += m_chOutLPOC[upReachId];
		m_chRPOC[i] += m_chOutRPOC[upReachId];
		m_chRDOC[i] +=  m_chOutRDOC[upReachId];
        m_chsurfRDOC[i] += m_chOutsurfRDOC[upReachId];        //kg
        m_chlatRDOC[i] += m_chOutlatRDOC[upReachId];        //kg
        m_chgwRDOC[i] += m_chOutgwRDOC[upReachId];        //kg
        m_T_LKWB[i][0] += m_chOutRDOC[upReachId] + m_chOutLDOC[upReachId];
        if(m_islake[upReachId]==1|| m_isres[upReachId]==1){
            m_chRDOC[i] +=  m_gwRDOCToCH[upReachId];
        }
    }
    /// absorbed organic N, P from overland sediment routing
    //m_chOrgN[i] += m_surfRfSedOrgNToCh[i];
    //m_chOrgP[i] += m_surfRfSedOrgPToCh[i];
    // //ljj++
	// m_chDIC[i] += m_surfDICToCH[i] + m_latDICToCH[i]+ m_gwDICToCH[i];
	// m_chLDOC[i] += m_LDOCToCH[i];
	// m_chLPOC[i] += m_LPOCToCH[i];
	m_chRPOC[i] += m_RPOCToCH[i];
	if(m_islake[i]!=1 && m_isres[i]!=1) m_chRDOC[i] += m_surfRDOCToCH[i] + m_latRDOCToCH[i] + m_gwRDOCToCH[i];
    if(m_islake[i]==1 || m_isres[i]==1) {
        m_chRDOC[i] += m_surfRDOCToCH[i] + m_latRDOCToCH[i];
        m_chRDOC[i] += m_lakepcp[i] * 1.2f * 0.001f;
    }
    m_chsurfRDOC[i]+= m_surfRDOCToCH[i];
    m_chlatRDOC[i] +=  m_latRDOCToCH[i];
    m_chgwRDOC[i] += m_gwRDOCToCH[i];

    
    m_T_LKWB[i][1] = m_surfRDOCToCH[i] + m_latRDOCToCH[i];
    m_T_LKWB[i][2] = m_lakepcp[i] * 1.2f * 0.001f;
    // /// organic N, P contribution from channel erosion
    // if (nullptr != m_rchDeg && m_chOrgPCo != NODATA_VALUE && m_chOrgNCo != NODATA_VALUE) {
    //     m_chOrgN[i] += m_rchDeg[i] * m_chOrgNCo * 0.001f;
    //     m_chOrgP[i] += m_rchDeg[i] * m_chOrgPCo * 0.001f;
    // }
    // /// dissolved N, P from overland surface flow routing and groundwater
    // //m_chNO3[i] += m_surfRfNO3ToCh[i] + m_latNO3ToCh[i] + m_gwNO3ToCh[i];
    // if (nullptr != m_surfRfNH4ToCh && m_surfRfNH4ToCh[i] > 0.f) {
    //     m_chNH4[i] += m_surfRfNH4ToCh[i];
    // }
    // //m_chSolP[i] += m_surfRfSolPToCh[i] + m_gwSolPToCh[i];
    // if (nullptr != m_no2ToCh && m_no2ToCh[i] > 0.f) {
    //     m_chNO2[i] += m_no2ToCh[i];
    // }
    // if (nullptr != m_surfRfCodToCh && m_surfRfCodToCh[i] > 0.f) {
    //     m_chCOD[i] += m_surfRfCodToCh[i];
    // }
    // /// add point source loadings to channel
    // if (nullptr != m_ptNO3ToCh && m_ptNO3ToCh[i] > 0.f) m_chNO3[i] += m_ptNO3ToCh[i];
    // if (nullptr != m_ptNH4ToCh && m_ptNH4ToCh[i] > 0.f) m_chNH4[i] += m_ptNH4ToCh[i];
    // if (nullptr != m_ptOrgNToCh && m_ptOrgNToCh[i] > 0.f) m_chOrgN[i] += m_ptOrgNToCh[i];
    // if (nullptr != m_ptSolPToCh && m_ptSolPToCh[i] > 0.f) m_chSolP[i] += m_ptSolPToCh[i];
    // if (nullptr != m_ptOrgPToCh && m_ptOrgPToCh[i] > 0.f) m_chOrgP[i] += m_ptOrgPToCh[i];
    // if (nullptr != m_ptCODToCh && m_ptCODToCh[i] > 0.f) m_chCOD[i] += m_ptCODToCh[i];
}

void NutrCH_QUAL2E::RouteOut(const int i) {
    // reinitialize out variables to 0
    m_chOutAlgae[i] = 0.f;
    m_chOutAlgaeConc[i] = 0.f;
    m_chOutChlora[i] = 0.f;
    m_chOutChloraConc[i] = 0.f;
    m_chOutOrgN[i] = 0.f;
    m_chOutOrgNConc[i] = 0.f;
    m_chOutNH4[i] = 0.f;
    m_chOutNH4Conc[i] = 0.f;
    m_chOutNO2[i] = 0.f;
    m_chOutNO2Conc[i] = 0.f;
    m_chOutNO3[i] = 0.f;
    m_chOutNO3Conc[i] = 0.f;
    m_chOutOrgP[i] = 0.f;
    m_chOutOrgPConc[i] = 0.f;
    m_chOutSolP[i] = 0.f;
    m_chOutSolPConc[i] = 0.f;
    m_chOutCOD[i] = 0.f;
    m_chOutCODConc[i] = 0.f;
    m_chOutDOx[i] = 0.f;
    m_chOutDOxConc[i] = 0.f;
    m_chOutTN[i] = 0.f;
    m_chOutTNConc[i] = 0.f;
    m_chOutTP[i] = 0.f;
    m_chOutTPConc[i] = 0.f;
	//ljj++
	m_chOutDIC[i] = 0.f;
	m_chOutDICConc[i] = 0.f;
	m_chOutLDOC[i] = 0.f;
	m_chOutLDOCConc[i] = 0.f;
	m_chOutLPOC[i] = 0.f;
	m_chOutLPOCConc[i] = 0.f;
	m_chOutRPOC[i] = 0.f;
	m_chOutRPOCConc[i] = 0.f;
	m_chOutRDOC[i] = 0.f;
	m_chOutRDOCConc[i] = 0.f;
    m_chOutTotDOC[i] = 0.f;
	m_chOutTotDOCConc[i] = 0.f;
    m_chOutTotPOC[i] = 0.f;
	m_chOutTotPOCConc[i] = 0.f;

    m_chOutsurfRDOC[i] = 0.f;
    m_chOutlatRDOC[i] = 0.f;
    m_chOutgwRDOC[i] = 0.f;
    
    //get out flow water fraction
    float wtrTotal = m_chStorage[i] + m_rteWtrOut[i]; // m^3
    if(m_islake[i] == 1|| m_isres[i]==1) wtrTotal += m_lakeperc[i];
    if (wtrTotal <= UTIL_ZERO || m_rteWtrOut[i] <= UTIL_ZERO) {
        // return with initialized values directly
        return;
    }
    float outFraction = m_rteWtrOut[i] / wtrTotal;
    //if(i == 12) cout << "wtrOut: " << wtrOut << ", m_chStorage: " << m_chStorage[i] << ", outFrac: "<<outFraction<<endl;
    if (outFraction >= 1.f) outFraction = 1.f;
    if (outFraction <= UTIL_ZERO) outFraction = UTIL_ZERO;
    m_chOutOrgN[i] = m_chOrgN[i] * outFraction;
    m_chOutNO3[i] = m_chNO3[i] * outFraction;
    m_chOutNO2[i] = m_chNO2[i] * outFraction;
    m_chOutNH4[i] = m_chNH4[i] * outFraction;
    m_chOutOrgP[i] = m_chOrgP[i] * outFraction;
    m_chOutSolP[i] = m_chSolP[i] * outFraction;
    m_chOutCOD[i] = m_chCOD[i] * outFraction;
    m_chOutDOx[i] = m_chDOx[i] * outFraction;
    m_chOutAlgae[i] = m_chAlgae[i] * outFraction;
    m_chOutChlora[i] = m_chChlora[i] * outFraction;
    m_chOutTN[i] = m_chOutOrgN[i] + m_chOutNH4[i] + m_chOutNO2[i] + m_chOutNO3[i];
    m_chOutTP[i] = m_chOutOrgP[i] + m_chOutSolP[i];
    //if(i == 12) cout << "m_chOutOrgP: " << m_chOutOrgP[i] << ", m_chOrgP: " << m_chOrgP[i] << ", outFrac: "<<outFraction<<endl;
    // kg ==> mg/L
    
    //ljj++
	m_chOutDIC[i] = m_chDIC[i] * outFraction;    //kg
	m_chOutLDOC[i] = m_chLDOC[i] * outFraction;
	m_chOutLPOC[i] = m_chLPOC[i] * outFraction;
	m_chOutRPOC[i] = m_chRPOC[i] * outFraction;
	m_chOutRDOC[i] = m_chRDOC[i] *outFraction;
    //cout<<m_chOutRDOC[i]<<"   "<<outFraction<<endl;
    m_chOutTotDOC[i] = m_chOutLDOC[i] + m_chOutRDOC[i];
    m_chOutTotPOC[i] = m_chOutLPOC[i] + m_chOutRPOC[i];
    m_chOutsurfRDOC[i] = m_chsurfRDOC[i] *outFraction;
    m_chOutlatRDOC[i] = m_chlatRDOC[i] *outFraction;
    m_chOutgwRDOC[i] = m_chgwRDOC[i] *outFraction;

    float cvt = 1000.f / (m_rteWtrOut[i]);
    m_chOutOrgNConc[i] = m_chOutOrgN[i] * cvt;
    m_chOutNO3Conc[i] = m_chOutNO3[i] * cvt;
    m_chOutNO2Conc[i] = m_chOutNO2[i] * cvt;
    m_chOutNH4Conc[i] = m_chOutNH4[i] * cvt;
    m_chOutOrgPConc[i] = m_chOutOrgP[i] * cvt;
    m_chOutSolPConc[i] = m_chOutSolP[i] * cvt;
    m_chOutCODConc[i] = m_chOutCOD[i] * cvt;
    m_chOutDOxConc[i] = m_chOutDOx[i] * cvt;
    m_chOutAlgaeConc[i] = m_chOutAlgae[i] * cvt;
    m_chOutChloraConc[i] = m_chOutChlora[i] * cvt;
    /// total N and total P
    m_chOutTNConc[i] = m_chOutTN[i] * cvt;
    m_chOutTPConc[i] = m_chOutTP[i] * cvt;
	//ljj++
	m_chOutDICConc[i] = m_chOutDIC[i] * cvt;     //mg/L
	m_chOutLDOCConc[i] = m_chOutLDOC[i] * cvt;
	m_chOutLPOCConc[i] = m_chOutLPOC[i] * cvt;
	m_chOutRPOCConc[i] = m_chOutRPOC[i] * cvt;
	m_chOutRDOCConc[i] = m_chOutRDOC[i] * cvt;

//if (i == 10 )cout << "m_chOutGwDOCConc: " << m_chOutGwDOCConc[i] << ", m_chOutGwOrgC: " << m_chOutGwOrgC[i] << ", cvt: " << cvt << endl;
	m_chOutTotDOCConc[i] = m_chOutLDOCConc[i] + m_chOutRDOCConc[i];
	m_chOutTotDOCConc[i] = Max(m_chOutTotDOCConc[i],0.f);
    m_chOutTotPOCConc[i] = m_chOutRPOCConc[i] + m_chOutRPOCConc[i];
	m_chOutTotPOCConc[i] = Max(m_chOutTotPOCConc[i],0.f);
    m_chNO3[i] -= m_chOutNO3[i];
    m_chNO2[i] -= m_chOutNO2[i];
    m_chNH4[i] -= m_chOutNH4[i];
    m_chOrgN[i] -= m_chOutOrgN[i];
    m_chOrgP[i] -= m_chOutOrgP[i];
    m_chSolP[i] -= m_chOutSolP[i];
    m_chCOD[i] -= m_chOutCOD[i];
    m_chDOx[i] -= m_chOutDOx[i];
    m_chAlgae[i] -= m_chOutAlgae[i];
    m_chChlora[i] -= m_chOutChlora[i];
    // recalculate TN TP stored in reach
    m_chTN[i] = m_chOrgN[i] + m_chNH4[i] + m_chNO2[i] + m_chNO3[i];
    m_chTP[i] = m_chOrgP[i] + m_chSolP[i];
    //ljj++
	m_chDIC[i] -= m_chOutDIC[i];       //kg
	m_chLDOC[i] -= m_chOutLDOC[i];
	m_chLPOC[i] -= m_chOutLPOC[i];
	m_chRPOC[i] -= m_chOutRPOC[i];
    if(m_islake[i]==1||m_isres[i]==1) {
        m_chRDOC[i] -= (m_chRDOC[i] * m_lakeperc[i] / wtrTotal);
        m_gwdoc_sto[i] +=(m_chRDOC[i] * m_lakeperc[i] / wtrTotal)/ (m_lakearea[i]* 1.e-4f );  //kh/ha
    }
    //if(i==442) cout<<m_chOutTotDOCConc[i]<<"    "<<m_gwdoc_sto[i]<<"    "<<(m_chRDOC[i] * m_lakeperc[i] / wtrTotal)<<"   "<<m_chOutTotDOCConc[i]*(m_lakeperc[i])*0.001<<endl;
	m_chRDOC[i] -= m_chOutRDOC[i];
    m_chsurfRDOC[i] -= m_chOutsurfRDOC[i];
    m_chlatRDOC[i] -= m_chOutlatRDOC[i];
    m_chgwRDOC[i] -= m_chOutgwRDOC[i];
    /// in case of zero
    if (m_chNO3[i] < UTIL_ZERO) m_chNO3[i] = UTIL_ZERO;
    if (m_chNO2[i] < UTIL_ZERO) m_chNO2[i] = UTIL_ZERO;
    if (m_chNH4[i] < UTIL_ZERO) m_chNH4[i] = UTIL_ZERO;
    if (m_chOrgN[i] < UTIL_ZERO) m_chOrgN[i] = UTIL_ZERO;
    if (m_chOrgP[i] < UTIL_ZERO) m_chOrgP[i] = UTIL_ZERO;
    if (m_chSolP[i] < UTIL_ZERO) m_chSolP[i] = UTIL_ZERO;
    if (m_chCOD[i] < UTIL_ZERO) m_chCOD[i] = UTIL_ZERO;
    if (m_chDOx[i] < UTIL_ZERO) m_chDOx[i] = UTIL_ZERO;
    if (m_chAlgae[i] < UTIL_ZERO) m_chAlgae[i] = UTIL_ZERO;
    if (m_chChlora[i] < UTIL_ZERO) m_chChlora[i] = UTIL_ZERO;
    //ljj++
	if (m_chDIC[i] < 0.f) m_chDIC[i] = 0.f;
	if (m_chLDOC[i] < 0.f) m_chLDOC[i] = 0.f;
	if (m_chLPOC[i] < 0.f) m_chLPOC[i] = 0.f;
	if (m_chRPOC[i] < 0.f) m_chRPOC[i] = 0.f;
	if (m_chRDOC[i] < 0.f) m_chRDOC[i] = 0.f;
    if (m_chsurfRDOC[i] < 0.f) m_chsurfRDOC[i] = 0.f;
    if (m_chlatRDOC[i] < 0.f) m_chlatRDOC[i] = 0.f;
    if (m_chgwRDOC[i] < 0.f) m_chgwRDOC[i] = 0.f;

    m_T_LKWB[i][3] = (m_chRDOC[i] * m_lakeperc[i] / wtrTotal);
    m_T_LKWB[i][4] = m_chRDOC[i];
    m_T_LKWB[i][5] = m_chOutTotDOC[i];
}

void NutrCH_QUAL2E::NutrientTransform(const int i) {
    float thbc1 = 1.083f; ///temperature adjustment factor for local biological oxidation of NH3 to NO2
    float thbc2 = 1.047f; ///temperature adjustment factor for local biological oxidation of NO2 to NO3
    float thbc3 = 1.04f;  ///temperature adjustment factor for local hydrolysis of organic N to ammonia N
    float thbc4 = 1.047f; ///temperature adjustment factor for local decay of organic P to dissolved P

    float thgra = 1.047f; ///temperature adjustment factor for local algal growth rate
    float thrho = 1.047f; ///temperature adjustment factor for local algal respiration rate

    float thm_rk1 = 1.047f; ///temperature adjustment factor for local CBOD deoxygenation
    float thm_rk2 = 1.024f; ///temperature adjustment factor for local oxygen reaeration rate
    float thm_rk3 = 1.024f; ///temperature adjustment factor for loss of CBOD due to settling
    float thm_rk4 = 1.060f; ///temperature adjustment factor for local sediment oxygen demand

    float thrs1 = 1.024f; ///temperature adjustment factor for local algal settling rate
    float thrs2 = 1.074f; ///temperature adjustment factor for local benthos source rate for dissolved phosphorus
    float thrs3 = 1.074f; ///temperature adjustment factor for local benthos source rate for ammonia nitrogen
    float thrs4 = 1.024f; ///temperature adjustment factor for local organic N settling rate
    float thrs5 = 1.024f; ///temperature adjustment factor for local organic P settling rate
    // Currently, by junzhi
    // assume the water volume used to contain nutrients at current time step equals to :
    //     flowout plus the storage at the end of day (did not consider the nutrients
    //     from stream to groundwater through seepage and bank storage)

    float wtrTotal = m_rteWtrOut[i] + m_chStorage[i]; /// m3
    if(m_islake[i] == 1 || m_isres[i]==1) wtrTotal += m_lakeperc[i];
    if (m_chWtrDepth[i] <= 0.01f) {
        return;
    }
    if (wtrTotal <= 0.f) {
        /// which means no flow out of current channel    || wtrOut <= 0.f
        m_chAlgae[i] = 0.f;
        m_chChlora[i] = 0.f;
        m_chOrgN[i] = 0.f;
        m_chNH4[i] = 0.f;
        m_chNO2[i] = 0.f;
        m_chNO3[i] = 0.f;
        m_chTN[i] = 0.f;
        m_chOrgP[i] = 0.f;
        m_chSolP[i] = 0.f;
        m_chTP[i] = 0.f;
        m_chDOx[i] = 0.f;
        m_chCOD[i] = 0.f;
        m_chSatDOx = 0.f;
        //ljj+
		m_chDIC[i] = 0.f;
		m_chLDOC[i] = 0.f;
		m_chLPOC[i] = 0.f;
		m_chRPOC[i] = 0.f;
		m_chRDOC[i] = 0.f;
        return; // return and route out with 0.f
    }
    if (wtrTotal <= 1.e-6f) {
        return;
    }
    // initial algal biomass concentration in reach (algcon mg/L, i.e. g/m3)   kg ==> mg/L
    float cvt_amout2conc = 1000.f / wtrTotal;
    float algcon = cvt_amout2conc * m_chAlgae[i];
    // initial organic N concentration in reach (orgncon mg/L)
    float orgncon = cvt_amout2conc * m_chOrgN[i];
    // initial ammonia concentration in reach (nh4con mg/L)
    float nh4con = cvt_amout2conc * m_chNH4[i];
    // initial nitrite concentration in reach (no2con mg/L)
    float no2con = cvt_amout2conc * m_chNO2[i];
    // initial nitrate concentration in reach (no3con mg/L)
    float no3con = cvt_amout2conc * m_chNO3[i];
    // initial organic P concentration in reach (orgpcon  mg/L)
    float orgpcon = cvt_amout2conc * m_chOrgP[i];
    // initial soluble P concentration in reach (solpcon mg/L)
    float solpcon = cvt_amout2conc * m_chSolP[i];
    // initial carbonaceous biological oxygen demand (cbodcon mg/L)
    float cbodcon = cvt_amout2conc * m_chCOD[i];
    // initial dissolved oxygen concentration in reach (o2con mg/L)
    float o2con = cvt_amout2conc * m_chDOx[i];
	//ljj++
	float diccon = cvt_amout2conc * m_chDIC[i];
	float ldoccon = cvt_amout2conc * m_chLDOC[i];
	float lpoccon = cvt_amout2conc * m_chLPOC[i];
	float rpoccon = cvt_amout2conc * m_chRPOC[i];
	float rdoccon = cvt_amout2conc * m_chRDOC[i];
	float rpocon = cvt_amout2conc * m_chDOx[i];
    lpoccon = Max(0.f,lpoccon);
    rpoccon = Max(0.f,rpoccon);
    rdoccon = Max(0.f,rdoccon);
    ldoccon = Max(0.f,ldoccon);
    diccon = Max(0.f,diccon);
    o2con = Max(0.f,o2con);
    orgncon = Max(0.f,orgncon);
    orgpcon = Max(0.f,orgpcon);
    //ljj++
    ldoccon = 0.f;
    lpoccon = 0.f;
    //cout<<nh4con<<"        "<<no2con<<"        "<<solpcon<<"        "<<no3con<<", ";
    // temperature of water in reach (wtmp deg C)
    float wtmp = Max(m_chTemp[i], 0.1f);
    // calculate effective concentration of available nitrogen (cinn), QUAL2E equation III-15
    float cinn = nh4con + no3con;

    // calculate saturation concentration for dissolved oxygen, QUAL2E section 3.6.1 equation III-29
    // variable to hold intermediate calculation result
    float ww = -139.34410f + 1.575701e+05f / (wtmp + 273.15f);
    float xx = 6.642308e+07f / pow(wtmp + 273.15f, 2.f);
    float yy = 1.243800e+10f / pow(wtmp + 273.15f, 3.f);
    float zz = 8.621949e+11f / pow(wtmp + 273.15f, 4.f);
    m_chSatDOx = 0.f;
    m_chSatDOx = exp(ww - xx + yy - zz);
    if (m_chSatDOx < 1.e-6f) {
        m_chSatDOx = 0.f;
    }

    // O2 impact calculations
    // calculate nitrification rate correction factor for low oxygen QUAL2E equation III-21(cordo)
    float cordo = 0.f;
    float o2con2 = o2con;
    if (o2con2 <= 0.1f) {
        o2con2 = 0.1f;
    }
    if (o2con2 > 30.f) {
        o2con2 = 30.f;
    }
    cordo = 1.f - exp(-0.6f * o2con2);
    if (o2con <= 0.001f) {
        o2con = 0.001f;
    }
    if (o2con > 30.f) {
        o2con = 30.f;
    }
    cordo = 1.f - exp(-0.6f * o2con);


    // modify ammonia and nitrite oxidation rates to account for low oxygen
    // rate constant for biological oxidation of NH3 to NO2 modified to reflect impact of low oxygen concentration (bc1mod)
    float bc1mod = 0.f;
    // rate constant for biological oxidation of NO2 to NO3 modified to reflect impact of low oxygen concentration (bc1mod)
    float bc2mod = 0.f;
    bc1mod = m_bc1[i] * cordo;
    bc2mod = m_bc2[i] * cordo;

    //	tday is the calculation time step = 1 day
    float tday = 1.f;

    // algal growth
    // calculate light extinction coefficient (lambda)
    float lambda = 0.f;
    if (m_ai0 * algcon > 1.e-6f) {
        lambda = m_lambda0 + m_lambda1 * m_ai0 * algcon + m_lambda2 * pow(m_ai0 * algcon, 0.66667f);
    } else {
        lambda = m_lambda0;
    }
    if (lambda > m_lambda0) lambda = m_lambda0;
    // calculate algal growth limitation factors for nitrogen and phosphorus, QUAL2E equations III-13 & III-14
    float fnn = 0.f;
    float fpp = 0.f;
    fnn = cinn / (cinn + m_k_n);
    fpp = solpcon / (solpcon + m_k_p);

    // calculate daylight average, photo synthetically active (algi)
    float algi = 0.f;
    if (m_chDaylen[i] > 0.f) {
        algi = m_chSr[i] * tfact / m_chDaylen[i];
    } else {
        algi = 0.00001f;
    }

    // calculate growth attenuation factor for light, based on daylight average light intensity
    float fl_1 = 0.f;
    float fll = 0.f;
    fl_1 = 1.f / (lambda * m_chWtrDepth[i]) * log((m_k_l + algi) / (m_k_l + algi * exp(-lambda * m_chWtrDepth[i])));

    fll = 0.92f * (m_chDaylen[i] / 24.f) * fl_1;

    // temporary variables
    float gra = 0.f;
    //float dchla = 0.f;
    float dbod = 0.f;
    float ddisox = 0.f;
    float dorgn = 0.f;
    float dnh4 = 0.f;
    float dno2 = 0.f;
    float dno3 = 0.f;
    float dorgp = 0.f;
    float dsolp = 0.f;
    switch (igropt) {
        case 1:
            // multiplicative
            gra = m_mumax * fll * fnn * fpp;
        case 2:
            // limiting nutrient
            gra = m_mumax * fll * Min(fnn, fpp);
        case 3:
            // harmonic mean
            if (fnn > 1.e-6f && fpp > 1.e-6f) {
                gra = m_mumax * fll * 2.f / (1.f / fnn + 1.f / fpp);
            } else {
                gra = 0.f;
            }
        default: break;
    }

    // calculate algal biomass concentration at end of day (phytoplanktonic algae), QUAL2E equation III-2
    float dalgae = 0.f;
    float setl = 0;
    dalgae = algcon + (corTempc(gra, thgra, wtmp) * algcon -
        corTempc(m_rhoq, thrho, wtmp) * algcon - setl * algcon) * tday;
    if (dalgae < 1.e-6f) {
        dalgae = 1.e-6f;
    }
    float dcoef = 3.f;
    /// set algae limit (watqual.f)
    if (dalgae > 5000.f) dalgae = 5000.f;
    if (dalgae > dcoef * algcon) dalgae = dcoef * algcon;
    // calculate chlorophyll-a concentration at end of day, QUAL2E equation III-1
    //dchla = dalgae * m_ai0 / 1000.f;

    // oxygen calculations
    // calculate carbonaceous biological oxygen demand at end of day (dbod)
    float yyy = 0.f;
    float zzz = 0.f;
    //1. COD convert to CBOD
    //if(i == 12) cout << "pre_cod, mg/L: " << cbodcon << ", ";
    cbodcon /= m_cod_n * (1.f - exp(-5.f * m_cod_k));
    //if(i == 12) cout << "pre_cbod, mg/L: " << cbodcon << ", ";
    yyy = corTempc(m_rk1[i], thm_rk1, wtmp) * cbodcon;
    zzz = corTempc(m_rk3[i], thm_rk3, wtmp) * cbodcon;
    dbod = 0.f;
    dbod = cbodcon - (yyy + zzz) * tday;
    /********* watqual.f code ***********/
    //float coef = 0.f;
    ///// deoxygenation rate
    //coef = exp(-1.f * corTempc(m_rk1[i], thm_rk1, wtmp)*tday);
    //float tmp = coef * cbodcon;
    //// cbod rate loss due to setting
    //coef = exp(-1.f * corTempc(m_rk3[i], thm_rk1, wtmp)*tday);
    //tmp *= coef;
    //dbod = tmp;
    ////if(i == 12) cout << "trans_cbod, mg/L: " << dbod << ", ";
    //if (dbod < 1.e-6f) dbod = 1.e-6f;
    //if (dbod > dcoef * cbodcon) dbod = dcoef * cbodcon;
    /********* watqual2.f code ***********/
    if (dbod < 1.e-6f) dbod = 1.e-6f;
    //if(i == 12) cout << "trans_cbod2, mg/L: " << dbod << ", ";
    //2. CBOD convert to COD, now dbod is COD
    dbod *= m_cod_n * (1.f - exp(-5.f * m_cod_k));
    //if(i == 12) cout << "cod: " << dbod << endl;

    // calculate dissolved oxygen concentration if reach at end of day (ddisox)
    float uu = 0.f; // variable to hold intermediate calculation result
    float vv = 0.f; // variable to hold intermediate calculation result
    ww = 0.f;       // variable to hold intermediate calculation result
    xx = 0.f;       // variable to hold intermediate calculation result
    yy = 0.f;       // variable to hold intermediate calculation result
    zz = 0.f;       // variable to hold intermediate calculation result

    //m_rk2[i] =1.f;	// Why define this value?

    //float hh = corTempc(m_rk2[i], thm_rk2, wtmp);
    uu = corTempc(m_rk2[i], thm_rk2, wtmp) * (m_chSatDOx - o2con);
    //vv = (m_ai3 * corTempc(gra, thgra, wtmp) - m_ai4 * corTempc(m_rhoq, thrho, wtmp)) * algcon;
    if (algcon > 0.001f) {
        vv = (m_ai3 * corTempc(gra, thgra, wtmp) - m_ai4 * corTempc(m_rhoq, thrho, wtmp)) * algcon;
    } else {
        algcon = 0.001f;
    }

    ww = corTempc(m_rk1[i], thm_rk1, wtmp) * cbodcon;
    if (m_chWtrDepth[i] > 0.001f) {
        xx = corTempc(m_rk4[i], thm_rk4, wtmp) / (m_chWtrDepth[i] * 1000.f);
    }
    if (nh4con > 0.001f) {
        yy = m_ai5 * corTempc(bc1mod, thbc1, wtmp) * nh4con;
    } else {
        nh4con = 0.001f;
    }
    if (no2con > 0.001f) {
        zz = m_ai6 * corTempc(bc2mod, thbc2, wtmp) * no2con;
    } else {
        no2con = 0.001f;
    }
    ddisox = 0.f;
    ddisox = o2con + (uu + vv - ww - xx - yy - zz) * tday;
    //o2proc = o2con - ddisox;   // not found variable "o2proc"
    if (ddisox < 0.1f || ddisox != ddisox) {
        ddisox = 0.1f;
    }
    // algea O2 production minus respiration
    //float doxrch = m_chSatDOx;
    // cbod deoxygenation
    //coef = exp(-0.1f * ww);
    //doxrch *= coef;
    // benthic sediment oxidation
    //coef = 1.f - corTempc(m_rk4[i], thm_rk4, wtmp) / 100.f;
    //doxrch *= coef;
    // ammonia oxydation
    //coef = exp(-0.05f * yy);
    //doxrch *= coef;
    // nitrite oxydation
    //coef = exp(-0.05f * zz);
    //doxrch *= coef;
    // reaeration
    //uu = corTempc(m_rk2[i], thm_rk2, wtmp) / 100.f * (m_chSatDOx - doxrch);
    //ddisox = doxrch + uu;
    //if (ddisox < 1.e-6f) ddisox = 0.f;
    //if (ddisox > m_chSatDOx) ddisox = m_chSatDOx;
    //if (ddisox > dcoef * o2con) ddisox = dcoef * o2con;
    //////end oxygen calculations//////
    // nitrogen calculations
    // calculate organic N concentration at end of day (dorgn)
    xx = 0.f;
    yy = 0.f;
    zz = 0.f;
    xx = m_ai1 * corTempc(m_rhoq, thrho, wtmp) * algcon;
    yy = corTempc(m_bc3[i], thbc3, wtmp) * orgncon;
    zz = corTempc(m_rs4[i], thrs4, wtmp) * orgncon;
    dorgn = 0.f;
    dorgn = orgncon + (xx - yy - zz) * tday;
    if (dorgn < 1.e-6f) dorgn = 0.f;
    if (dorgn > dcoef * orgncon) dorgn = dcoef * orgncon;
    // calculate fraction of algal nitrogen uptake from ammonia pool
    float f1 = 0.f;
    f1 = m_p_n * nh4con / (m_p_n * nh4con + (1.f - m_p_n) * no3con + 1.e-6f);

    //cout<<"subID: "<<i<<", initial nh4 conc: "<<nh4con<<", "<<"initial orgn: "<<orgncon<<", ";
    // calculate ammonia nitrogen concentration at end of day (dnh4)
    ww = 0.f;
    xx = 0.f;
    yy = 0.f;
    zz = 0.f;
    ww = corTempc(m_bc3[i], thbc3, wtmp) * orgncon;
    xx = corTempc(bc1mod, thbc1, wtmp) * nh4con;
    yy = corTempc(m_rs3[i], thrs3, wtmp) / (m_chWtrDepth[i] * 1000.f);
    zz = f1 * m_ai1 * algcon * corTempc(gra, thgra, wtmp);
    dnh4 = 0.f;
    dnh4 = nh4con + (ww - xx + yy - zz) * tday;
    if (dnh4 < 1.e-6f) dnh4 = 0.f;
    if (dnh4 > dcoef * nh4con && nh4con > 0.f) {
        dnh4 = dcoef * nh4con;
    }
    // calculate concentration of nitrite at end of day (dno2)
    yy = 0.f;
    zz = 0.f;
    yy = corTempc(bc1mod, thbc1, wtmp) * nh4con;
    zz = corTempc(bc2mod, thbc2, wtmp) * no2con;
    dno2 = 0.f;
    dno2 = no2con + (yy - zz) * tday;
    if (dno2 < 1.e-6f) dno2 = 0.f;
    if (dno2 > dcoef * no2con && no2con > 0.f) {
        dno2 = dcoef * no2con;
    }
    // calculate nitrate concentration at end of day (dno3)
    yy = 0.f;
    zz = 0.f;
    yy = corTempc(bc2mod, thbc2, wtmp) * no2con;
    zz = (1.f - f1) * m_ai1 * algcon * corTempc(gra, thgra, wtmp);
    dno3 = 0.f;
    dno3 = no3con + (yy - zz) * tday;
    if (dno3 < 1.e-6) dno3 = 0.f;
    /////end nitrogen calculations//////
    // phosphorus calculations
    // calculate organic phosphorus concentration at end of day
    xx = 0.f;
    yy = 0.f;
    zz = 0.f;
    xx = m_ai2 * corTempc(m_rhoq, thrho, wtmp) * algcon;
    yy = corTempc(m_bc4[i], thbc4, wtmp) * orgpcon;
    zz = corTempc(m_rs5[i], thrs5, wtmp) * orgpcon;
    dorgp = 0.f;
    dorgp = orgpcon + (xx - yy - zz) * tday;
    if (dorgp < 1.e-6f) dorgp = 0.f;
    if (dorgp > dcoef * orgpcon) {
        dorgp = dcoef * orgpcon;
    }

    // calculate dissolved phosphorus concentration at end of day (dsolp)
    xx = 0.f;
    yy = 0.f;
    zz = 0.f;
    xx = corTempc(m_bc4[i], thbc4, wtmp) * orgpcon;
    yy = corTempc(m_rs2[i], thrs2, wtmp) / (m_chWtrDepth[i] * 1000.f);
    zz = m_ai2 * corTempc(gra, thgra, wtmp) * algcon;
    dsolp = 0.f;
    dsolp = solpcon + (xx + yy - zz) * tday;
    if (dsolp < 1.e-6) dsolp = 0.f;
    if (dsolp > dcoef * solpcon) {
        dsolp = dcoef * solpcon;
    }
    /////////end phosphorus calculations/////////

    //ljj++ OC riverine Metabolism
    //paramters for bottom algae in the reach
    int light = 1;       //light limitation option(1, 2, 3)
    int ipho = 1;       //photosynthesis model option (0, zero order; 1, first order)
    float Abmax = 200.0;//First - order carrying capacity of bottom algae[gD / m2 / d]
    float klb = 1.5807; //bottom algae light parameter(MJ / m2 / day)
    float kph = 36.2;   //maximum photosynthesis rate[d - 1 or gD / m2 / d]
    float kdb  = 0.02;  //bottom algae death rate (/day)
    float fdic = 0.6;   //the fraction of DIC in H2CO3* and HCO3-,for calculating nutrient limitation
    float qoN = 4.17;   // minimum cell quotas of N (mgN/gD/day)
    float qoP = 1.93;   // minimum cell quotas of P (mgN/gD/day)
    float pmN = 284.7;  //maximum uptake rate for N(mgN / gD / day)
    float pmP = 37.85;  //maximum uptake rate for P(mgP / gD / day)
    float kscb = 0.792; //Inorganic carbon half saturation constant(mg / L, unit converted)
    float Ksnb = 0.07;  //external N half - saturation constant(mg N / L)
    float Kspb = 0.098; // external P half - saturation constant(mg P / L)
    float KqN = 2.54;   //intracelluar N half - saturation constant(mgN / gD)
    float KqP = 4.93;   //intracelluar P half - saturation constant(mgP / gD)
    float kexb = 0.21;  //bottom algae excretion rate(/ day)
    float keb = 0.02;     //background light extinction coefficient(/ m)
    float kiss = 0.052;   //light extinction coefficient by inorganic suspended solids(L / mgD / m)
    float kpom = 0.174;   //light extinction coefficient by particular organic matter(POM) (L / mgD / m)
    float kap = 0.0088;   //linear light extinction coefficient by algae(L / ugA / m)
    float kapn = 0.054;   //non - linear light extinction coefficient by algae(L / ugA)2 / 3 / m
    float kAb = 0.024;    //linear light extinction coefficient by bottom[m3 / gD / m]
    float f_ldp = 0.05; //fraction of algal mortality into LDOC
    float f_ldb = 0.05; //fraction of bottom algal mortality into LDOC
    float rca = 0.4;  //algal carbon to biomass ratio
    float ksdocf = 1.0; //half-saturation oxygen attenuation for DOC oxidation (mgO2/L)
    float ksoxdn = 0.1; //half-saturation oxygen attenuation for denitrification (mgO2/L)
    float kdnit = 0.002; //NO3 denitrification rate (/day) LDOC consumned
    float f_lpp = 0.1; //fraction of algal mortality into LPOC
    float f_rpp = 0.8; //fraction of algal mortality into RPOC
    float f_lpb = 0.1; //fraction of bottom algal mortality into LPOC
    float f_rpb = 0.8; // fraction of bottom algal mortality into RPOC
    float krb1 = 0.042;      //bottom algae basal respiration
    float krb2 = 0.389;      //photo - respiration rate parameter
    float ksob = 0.6;        //half - saturation oxygen inhibition constant for respiration
    float p_co2 = 391.0;      //partial pressure of CO2(ppm, parts per million)
    float f_co2 = 0.2;        //fraction of DIC in CO2
    float ksed = 0.02;        //first-order sediment decay rate (/day)
    float kbur = 0.05;        //first - order sediment burial rate(/ day)
        
    //bottom algea death procese
    float Ab_con = 0.f;	
    Ab_con = m_Ab[i];
    m_AbDeath[i] = 0.f;
    m_AbDeath[i] = Ab_con * corTempc(kdb, 1.04f, wtmp); //bottom algea death biomass(g / m2 / day)

    //bottom algae photosynthesis/growth process
    //kinetics for intracellular N and P process in bottom algae
    float Ab_UN = 0.f;
    float Ab_UP = 0.f;
    float qN_Ab, qP_Ab;
    qN_Ab = m_AbINb[i] / Ab_con;
    qP_Ab = m_AbIPb[i] / Ab_con;
    float DIN_con = nh4con + no3con + no2con; //DIN_con = nh3con + no3con + no2con !calculate DIN concentration in the reach(mgN / L)
    // N&P uptakes in bottom algae
    Ab_UN = pmN * (DIN_con / (DIN_con + Ksnb)) * Ab_con * (KqN / (KqN + qN_Ab - qoN));
    if (Ab_UN < 0.f) Ab_UN = 0.f;
    Ab_UP = pmP * (solpcon / (solpcon + Kspb)) * Ab_con * (KqP / (KqP + qP_Ab - qoP));
    if (Ab_UP < 0.f) Ab_UP = 0.f;

    //intracellular N&P mass balance
    m_AbINb[i] = m_AbINb[i] + Ab_UN - qN_Ab * m_AbDeath[i] - qN_Ab * corTempc(kexb, 1.04f, wtmp) *Ab_con;
    if (m_AbINb[i] < 0.f) m_AbINb[i] = 0.f;
    m_AbIPb[i] = m_AbIPb[i] + Ab_UP - qP_Ab * m_AbDeath[i] - qP_Ab * corTempc(kexb, 1.04f, wtmp) *Ab_con;
    if (m_AbIPb[i] < 0.f) m_AbIPb[i] = 0.f;

    //For calculating nutrient limitation factor
    float fnl_dic = 0.f;
    fnl_dic = fdic * diccon;
    float fnl_Ab = 0.f;
    fnl_Ab = Min((1.f - qoN/ qN_Ab), (1.f - qoP/ qP_Ab));
    fnl_Ab = Min(fnl_Ab, (fnl_dic/ (fnl_dic + kscb)));
    if (fnl_Ab < 0.f) fnl_Ab = 0.f;

    //For calculating light limation factor
    float I0_Ab = 0.f;
    I0_Ab = m_chSr[i]*tfact;
    float POMcon = (lpoccon + rpoccon) / 0.58;
    float sedc = 0.f;
    if (wtrTotal > 0.f) sedc = m_sedst[i] * cvt_amout2conc; //TSS concentration  kg to mg/L
    float ke_Ab = keb + kiss * sedc + kpom * POMcon + kap * algcon + kapn * pow(algcon, 0.66667f) + kAb * Ab_con / m_chWtrDepth[i];
    ke_Ab = Min(ke_Ab, 1e6);  //ljj
    ke_Ab = Max(ke_Ab, 0.f);
    float IH_Ab = 0.f;
    IH_Ab = I0_Ab * exp(-ke_Ab * m_chWtrDepth[i]);

    float fll_Ab = 0.f;
    if (light == 1) {
        fll_Ab = IH_Ab / (klb + IH_Ab);
    }
    else if (light == 2) {
        fll_Ab = IH_Ab / pow((klb*klb + IH_Ab * IH_Ab), 0.5);
    }
    else if (light == 3) {
        fll_Ab = (IH_Ab / klb) * exp (1 + IH_Ab / klb);
    }

    //temperature correction for photosynthesis rate
    float kph_Ab = 0.f;
    kph_Ab = corTempc(kph, 1.04f, wtmp);
    m_photo[i] = 0.f;
    if (ipho = 0) {
        m_photo[i] = kph_Ab * fll_Ab * fnl_Ab;
    }else {
        //Space limitation factor
        float fsl_Ab = 0.f;
        fsl_Ab = Max(0.f, 1.f - Ab_con / Abmax);
        m_photo[i] = kph_Ab * fll_Ab * fnl_Ab*fsl_Ab*Ab_con;
    }
    if (m_photo[i] < 0.f) m_photo[i] = 0.000001;


    //bottom algea respiration process
    float foxb_Ab = 0.f;
    foxb_Ab = o2con / (o2con + ksob);
    m_resp[i] = 0.f;
    m_resp[i] = foxb_Ab * (krb1 * Ab_con + krb2 * m_photo[i]);
    if (m_resp[i] < 0.f) m_resp[i] = 0.000001;
    //Bottom algae biomass mass balance
    xx = 0.f;
    xx = m_Ab[i];
    m_Ab[i] = m_Ab[i] + m_photo[i] - m_resp[i] - m_AbDeath[i];
    if (m_Ab[i] < 0.f) m_Ab[i] = 0.000001;
    m_Ab[i] = Min(m_Ab[i], 1e6);  //ljj m_Ab[i] actually always <1
    m_Ab[i] = Max(m_Ab[i], 0.f);
    //calculating algal death to organic carbon
    float Alg_cbn = 0.f;
    Alg_cbn = rca * corTempc(m_rhoq, thrho, wtmp) * algcon;  //Algal death to total organic carbon (mg/L)
    float Ab_cbn = 0.f;
    Ab_cbn = rca * m_AbDeath[i] / m_chWtrDepth[i];    //bottom algae death to total organic carbon (mg/L)
    //LPOC's 6 reaction pathways
    //ljj++ LPOC==0 RPOC==POC
    // float Alg_LP = 0.f;
    // Alg_LP = f_lpp * Alg_cbn;
    // float Ab_LP = 0.f;
    // Ab_LP = f_lpb * Ab_cbn;
    // float LP_set = 0.f;
    // LP_set = m_sv_lp  / m_chWtrDepth[i] * lpoccon;
    // float LP_LD = 0.f;
    // LP_LD = corTempc(m_klp, 1.047f, wtmp) * lpoccon;
    // float LP_DIC = 0.f;
    // LP_DIC = corTempc(m_kd_lp, 1.047f, wtmp) * lpoccon;
    // float LR_POC = 0.f;
    // LR_POC = corTempc(m_klrp, 1.047f, wtmp) * lpoccon;
    // //ljj modified
    // // float blclpoc =  m_npoc;  //!!todo 用户自定义的平衡浓度
    // // //黄河：80%以上的POC集中在<16μm的TSS中,而粒径<32μm的TSS承载了95%以上的POC
    // // float lpoc_stlr = exp(-.184 * 30); //怎么定义POC的粒径呢？？
    // // float LP_set2 = 0.f;
    // // if (lpoccon > blclpoc) {
    // //     float inilpoc = lpoccon;
    // //     float finlpoc = (lpoccon - blclpoc) *lpoc_stlr + blclpoc;
    // //     LP_set2 = inilpoc - finlpoc;
    // // }
    // // LP_set = LP_set2;
    // m_chLPOC[i] = lpoccon + (Alg_LP + Ab_LP - LP_set - LP_LD - LP_DIC - LR_POC) * tday;
    // if (m_chLPOC[i] <= 0.f) {
    //     m_chLPOC[i] = 0.f;
    //     LP_set = lpoccon / tday + Alg_LP + Ab_LP - LP_LD - LP_DIC - LR_POC;
    // }
    m_chLPOC[i] = lpoccon;

    //RPOC reaction pathway
    float Alg_RP = 0.f;
    Alg_RP = f_rpp * Alg_cbn;
    float Ab_RP = 0.f;
    Ab_RP = f_rpb * Ab_cbn;
    float RP_LD = 0.f;
    RP_LD = corTempc(m_krp, 1.047f, wtmp) * rpoccon;
    //RP_LD = corTempc(m_krp_1d[i], 1.047f, wtmp) * rpoccon;
    RP_LD = Min(RP_LD,rpoccon);
    RP_LD = Max(RP_LD,0.f);
    float RP_DIC = 0.f;
    RP_DIC = corTempc(m_kd_rp, 1.047f, wtmp) * rpoccon;
    //RP_DIC = corTempc(m_kd_rp_1d[i], 1.047f, wtmp) * rpoccon;
    RP_DIC = Min(RP_DIC,rpoccon);
    RP_DIC = Max(RP_DIC,0.f);
    float RP_set = 0.f;
    RP_set = m_sv_rp / m_chWtrDepth[i] * rpoccon;
    //RP_set = m_sv_rp_1d[i] / m_chWtrDepth[i] * rpoccon;
    RP_set = Min(RP_set,rpoccon*0.5f);
    // //ljj modified
    // float blcrpoc =  m_npoc;  //!!todo 用户自定义的平衡浓度
    // //黄河：80%以上的POC集中在<16μm的TSS中,而粒径<32μm的TSS承载了95%以上的POC
    // float rpoc_stlr = exp(-.184 * 30); //怎么定义POC的粒径呢？？
    // float RP_set2 = 0.f;
    // if (rpoccon > blcrpoc) {
    //     float inirpoc = rpoccon;
    //     float finrpoc = (rpoccon - blcrpoc) *rpoc_stlr + blcrpoc;
    //     RP_set2 = inirpoc - finrpoc;
    // }
    // RP_set = Min(RP_set2,rpoccon);
    // RP_set = RP_set2;

    RP_set = Max(RP_set,0.f);
    float LR_POC = 0.f;
    //m_chRPOC[i] = rpoccon + (Alg_RP + Ab_RP - RP_set - RP_LD - RP_DIC + LR_POC) * tday;
    m_chRPOC[i] = rpoccon + (Alg_RP + Ab_RP - RP_set - RP_LD - RP_DIC) * tday;
    if (m_chRPOC[i] <= 0.f) {
        m_chRPOC[i] = 0.f;
        //RP_set = rpoccon / tday + Alg_RP + Ab_RP - RP_LD - RP_DIC + LR_POC;
        RP_set = rpoccon / tday + Alg_RP + Ab_RP - RP_LD - RP_DIC;
    }
    //LDOC reaction pathways
    //ljj++ LDOC==0 RDOC==DOC
    // float Alg_LD = 0.f;
    // Alg_LD = f_ldp * Alg_cbn;
    // float Ab_LD = 0.f;
    // Ab_LD = f_ldb * Ab_cbn;
    // float LD_DIC = 0.f;
    // float Foxc = 0.f;
    // Foxc = o2con / (o2con + ksdocf);
    // LD_DIC = Foxc * corTempc (m_kld, 1.047, wtmp)*ldoccon;
    // float LR_DOC = 0.f;
    // LR_DOC = corTempc(m_klrd, 1.047, wtmp)*ldoccon;
    // float LD_NO3 = 0.f;
    // float Fxodn = 0.f;
    // Fxodn = o2con / (o2con + ksoxdn);
    // LD_NO3 = no3con * (15.f / 14.f)*(1.f - Fxodn) * corTempc(kdnit, 1.045, wtmp);
    // m_chLDOC[i] = ldoccon + (Alg_LD + Ab_LD - LR_DOC - LD_DIC - LD_NO3 + LP_LD + RP_LD)* tday;
    // if (m_chLDOC[i] <= 0.f) m_chLDOC[i] = 0.f;
    m_chLDOC[i] = ldoccon;

    //RDOC reation pathways
    float Alg_RD = 0.f;
    float f_rdp = 1 - f_lpp - f_rpp - f_ldp;
    float f_rdb = 1 - f_lpb - f_rpb - f_ldb;
    Alg_RD = f_rdp * Alg_cbn;
    float Ab_RD = 0.f;
    Ab_RD = f_rdb * Ab_cbn;
    float RD_DIC = 0.f;
    float Foxc = 0.f;
    Foxc = o2con / (o2con + ksdocf);
    RD_DIC = Foxc * corTempc(m_krd, 1.047, wtmp)*rdoccon;
    //RD_DIC = Foxc * corTempc(m_krd_1d[i], 1.047, wtmp)*rdoccon;
    RD_DIC = Min(RD_DIC,rdoccon);
    RD_DIC = Max(RD_DIC,0.f);
    float Fxodn = 0.f;
    Fxodn = o2con / (o2con + ksoxdn);
    float LD_NO3 = 0.f;
    LD_NO3 = no3con * (15.f / 14.f)*(1.f - Fxodn) * corTempc(kdnit, 1.045, wtmp);
    LD_NO3 = Min(LD_NO3,rdoccon);
    LD_NO3 = Max(LD_NO3,0.f);
    LD_NO3 = 0.f;
    //m_chRDOC[i] = rdoccon + (Alg_RD + Ab_RD + LR_DOC - RD_DIC - LD_NO3)* tday;
    m_chRDOC[i] = rdoccon + (Alg_RD + Ab_RD + RP_LD - RD_DIC - LD_NO3)* tday;
    m_T_LKWB[i][6] = (Alg_RD + Ab_RD + RP_LD)* wtrTotal * 0.001f;  
    m_T_LKWB[i][7] = (RD_DIC)* wtrTotal * 0.001f; 
    if (m_chRDOC[i] <= 0.f) m_chRDOC[i] = 0.f;

    //DIC reaction pathways
    float Atm_DIC = 0.f;
    float kac = corTempc(m_rk2[i], thm_rk2, wtmp)*0.923;          //CO2 reaeration rate(/ day)
    float wtmpk = wtmp + 273.15;  //convet the unit of water temp to Kelvin
    float kh_DIC = pow(10.0f,((2385.73f / wtmpk) + 0.0152642f * wtmpk - 14.0184f));   //Henry's constant [mol/L/atm] 
    float CO2_sat = 12.0f * kh_DIC * p_co2 / 1000.0f;        //CO2 saturation(unit converting to mg / L)
    Atm_DIC = kac * (CO2_sat - f_co2 * diccon);   //Atmospheric CO2 reaeration
    float SC = 1911.1 - 118.11*wtmp + 3.4527*pow(wtmp,2) - 0.04132*pow(wtmp,3);
    float K600 = m_chSlope[i] * m_qRchOut[i]/(m_chArea[i]*90*90/0.00083*1000) * 2841.6f + 2.03f;
    float Kdeg = K600 * pow(SC/600.f,-0.5);
    float Kh = 0.34 * exp(2400.f * (1.f/wtmpk - 1.f/298.f));

    float Alg_DIC = 0.f;
    Alg_DIC = Alg_cbn;    //algae respiration to DIC
    float DIC_Alg = 0.f;
    DIC_Alg = rca * corTempc(gra, thgra, wtmp) * algcon;  //DIC consumbed by algal photosynthesis
    DIC_Alg = Max(DIC_Alg,0.f);
    float Ab_DIC = 0.f;
    Ab_DIC = rca * m_resp[i] / m_chWtrDepth[i];      //bottom algae respiration to DIC
    Ab_DIC = 0.f;
    if (m_chWtrDepth[i] < 0.001f)Ab_DIC = rca * m_resp[i] / 0.001f;
    float DIC_Ab = 0.f;
    DIC_Ab = rca * m_photo[i] / m_chWtrDepth[i];      //DIC consumbed by bottom algae photosynthesis
    DIC_Ab = 0.f;
    if (m_chWtrDepth[i] < 0.001f) DIC_Ab = rca * m_photo[i] / 0.001f;
    float CBOD_DIC = 0.f;
    CBOD_DIC = corTempc(m_rk1[i], thm_rk1, wtmp) * cbodcon * (12.0f / 32.0f);   //CBOD oxidation to DIC
    CBOD_DIC = Max(CBOD_DIC,0.f);
    CBOD_DIC = 0.f;
    //first order sediment diagenesis model - from W2 model
    float algset = 0.f;
    algset = corTempc(m_rs1[i], thrs1, wtmp) / m_chWtrDepth[i] * algcon * rca;  //Algae settling to bed sediment
    if (m_chWtrDepth[i] < 0.001f) algset = corTempc(m_rs1[i], thrs1, wtmp) / 0.001f * algcon * rca;
    //m_scbn[i] = m_scbn[i] + RP_set + LP_set + algset; //add RPOC and LPOC settling from water column to bed sediment
    m_scbn[i] = m_scbn[i] + RP_set + algset;
    float dic_bed = 0.f;
    dic_bed = m_scbn[i] * ksed;   //DIC release from bed sediment compartment(first - order equation)
    float cbn_bur = 0.f;
    cbn_bur = m_scbn[i] * kbur;  // sediment burial amount
    m_scbn[i] = m_scbn[i] - dic_bed - cbn_bur; //update sediment carbon amount after loss
    if (m_scbn[i] < 0.f) m_scbn[i] = 0.f;
    float bed_DIC = dic_bed;          //DIC release from bed sediment
    //m_chDIC[i] = diccon + (Atm_DIC + LP_DIC + RP_DIC + LD_DIC + RD_DIC  //transformations from organic carbon pools
    m_chDIC[i] = diccon + (Atm_DIC + RP_DIC  + RD_DIC 
                        + Alg_DIC + Ab_DIC - DIC_Alg - DIC_Ab + CBOD_DIC + bed_DIC)*tday;
    if (m_chDIC[i] < 0.f) m_chDIC[i] = 0.f;
    ////ljj-- end OC Metabolism/////

    // storage amount (kg) at end of day
    m_chAlgae[i] = dalgae * wtrTotal * 0.001f;
    m_chChlora[i] = m_chAlgae[i] * m_ai0;
    m_chOrgN[i] = dorgn * wtrTotal * 0.001f;
    m_chNH4[i] = dnh4 * wtrTotal * 0.001f;
    m_chNO2[i] = dno2 * wtrTotal * 0.001f;
    m_chNO3[i] = dno3 * wtrTotal * 0.001f;
    m_chOrgP[i] = dorgp * wtrTotal * 0.001f;
    m_chSolP[i] = dsolp * wtrTotal * 0.001f;
    m_chCOD[i] = dbod * wtrTotal * 0.001f;
    m_chDOx[i] = ddisox * wtrTotal / 1000.f;
    //ljj++
    m_chDIC[i] = m_chDIC[i] * wtrTotal * 0.001f;    //mg/L to kg
    m_chLDOC[i] = m_chLDOC[i] * wtrTotal * 0.001f;
    m_chLPOC[i] = m_chLPOC[i] * wtrTotal * 0.001f;
    m_chRPOC[i] = m_chRPOC[i] * wtrTotal * 0.001f;
    m_chRDOC[i] = m_chRDOC[i] * wtrTotal * 0.001f;
}

float NutrCH_QUAL2E::corTempc(const float r20, const float thk, const float tmp) {
    return r20 * pow(thk, tmp - 20.f);
}

void NutrCH_QUAL2E::swat_rtmp(const int i){
    // the dischagre from upstream subbains
	float q_up_all = 0.0;
	// the sum of dischange mulit temperature
	float tmp_up_all = 0.0;
    for (auto upRchID = m_reachUpStream.at(i).begin(); upRchID != m_reachUpStream.at(i).end(); ++upRchID) {
        int upReachId = *upRchID;
        q_up_all += m_qsRchOut[*upRchID] + m_qiRchOut[*upRchID] + m_qgRchOut[*upRchID];
        tmp_up_all += (m_qsRchOut[*upRchID] + m_qiRchOut[*upRchID] + m_qgRchOut[*upRchID]) * m_ch_Temp[*upRchID];
        if(m_islake[*upRchID]==1|| m_isres[*upRchID]==1){
            q_up_all += m_gndQ2Rch[*upRchID];
            tmp_up_all += (m_gndQ2Rch[*upRchID]) * m_ch_Temp[*upRchID];
        }
    }
    // the initial stream temperature
    float sub_surq = m_olQ2Rch[i];
    float sub_gw = 0.f; /// groundwater flow
    if (nullptr != m_gndQ2Rch && m_gndQ2Rch[i] >= 0.f) {
        sub_gw = m_gndQ2Rch[i];
    }
    if(m_islake[i]==1|| m_isres[i]==1){
        sub_gw = 0.f;
    }
	float sub_latq = 0.f;
    if (nullptr != m_ifluQ2Rch && m_ifluQ2Rch[i] >= 0.f) {
        sub_latq = m_ifluQ2Rch[i];
    }
    float sub_snow = m_subsnow[i];
    if(sub_snow >0.f) {
        sub_surq = sub_surq - sub_snow;
        sub_surq = Max(sub_surq, 0.f);
    }
    float sub_glacier = m_subglacier[i];
    if(sub_glacier >0.f) {
        sub_surq = sub_surq - sub_glacier;
        sub_surq = Max(sub_surq, 0.f);
    }

	float tmp_snow = 0.1;
	float tmp_gw = 8.0;  //cali 0~8
	float coefficient_tmp = 1.0; //cali 0~1
    float m_lag = 0.7f;//cali 0~1

    float tw_initail = 0.f;
    m_chTemp_pre[i] = m_ch_Temp[i]*m_chSto_pre[i];
    m_chAirTemp_pre[i] = m_chAirTemp_pre[i] * (1 - m_lag) + m_chAirTemp[i] * m_lag;
	if(sub_snow + sub_gw + sub_surq + sub_latq + q_up_all > 1.e-6) {
        tw_initail = ((sub_snow+sub_glacier) * tmp_snow + sub_gw * tmp_gw + (sub_surq + sub_latq) * coefficient_tmp * m_chAirTemp_pre[i] + tmp_up_all + m_chTemp_pre[i]) / (sub_snow+sub_glacier + sub_gw + sub_surq + sub_latq + q_up_all + m_chSto_pre[i]);
    }
    m_chSto_pre[i] = m_chStorage[i]/86400;
    
	if (tw_initail <= -1.0)
	{
		tw_initail = -1.0;
	}
	// calculate dew point temperature based on air temperature or air temperature & relative humidity
	float tmp_dew = 0.0;
	int case_num = 1;
    float sub_rhd = 0.6;
	switch (case_num) {
		case 1:
		{
			float wtmp_add = 3.2; //Du et al., 2018  //cali -5~5
			tmp_dew = m_chAirTemp[i] + wtmp_add;
			break;
		}
		
		case 2:
		{
			float em = 0.0;
			float e = 0.0;
			if (m_chAirTemp[i] >= 0)
			{
				em = 611.0 * exp((17.27 * m_chAirTemp[i]) / (m_chAirTemp[i] + 237.3));
			}
			else
			{
				em = 611.0 * exp((21.81 * m_chAirTemp[i]) / (m_chAirTemp[i] + 265.5));
			}
			e = sub_rhd * em;
			tmp_dew = (log(e) - 6.415) / (0.0999 - 0.00421 * log(e));
		}
	}
    // desity of water (kg/m3)
	float pwater = 999.973 * (1.0 - (tw_initail + 288.9414) * (tw_initail - 3.9863) * (tw_initail - 3.9863) / 508929.2 / (tw_initail + 68.12693));
	//specific heat capacity of water, J/kg/C
	float cpwater = 4186.0;

	//calculating the overall heat exchange coeffcient(Kt)
	float tdw = 0.5 * (tmp_dew + tw_initail);
	float beta = 0.35 + 0.015 * tdw + 0.0012 * tdw * tdw;
	float fwind = 9.2 + 0.46 * m_chWS[i] * m_chWS[i];
	float kt = 4.5 + 0.05 * tw_initail + beta * fwind + 0.47 * fwind;
	//equilibrium temperature
	float tmp_eq = tmp_dew + m_chSr[i] / kt;
	//the temperature of outlet water
	float tmp_outlet = tw_initail;
    float delta_wtmp =0.f;

	if (m_chWtrDepth[i] > 0.05)
	{
		delta_wtmp = kt * (tmp_eq - tw_initail) / (pwater * cpwater * m_chWtrDepth[i]) * Min(m_rrtime[i]/24, 1.0) * 86400;

		if (delta_wtmp > 10.0)
		{
			delta_wtmp = 6.0;
		}
		tmp_outlet += delta_wtmp;
	}
	else
	{
		tmp_outlet = tmp_eq;
	}
	//modified by Du, avioding some unrelealistic simulations
	if (tmp_outlet <= -1.0)
	{
		tmp_outlet = -1.0;
	}
	//modified by Du, avioding some unrelealistic simulations
	if (tmp_outlet > 40.0)
	{
		tmp_outlet = 40.0;
	}
    m_ch_Temp[i] = tmp_outlet;
    m_chTemp[i] = tmp_outlet;
    
}


void NutrCH_QUAL2E::GetValue(const char* key, float* value) {
    string sk(key);
    if (StringMatch(sk, VAR_SOXY)) *value = m_chSatDOx;
        /// Get value for transferring across subbasin
    else if (StringMatch(sk, VAR_CH_ALGAE)) *value = m_chOutAlgae[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_ALGAEConc)) *value = m_chOutAlgaeConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_NO2)) *value = m_chOutNO2[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_NO2Conc)) *value = m_chOutNO2Conc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_COD)) *value = m_chOutCOD[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_CODConc)) *value = m_chOutCODConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_CHLORA)) *value = m_chOutChlora[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_CHLORAConc)) *value = m_chOutChloraConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_NO3)) *value = m_chOutNO3[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_NO3Conc)) *value = m_chOutNO3Conc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_SOLP)) *value = m_chOutSolP[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_SOLPConc)) *value = m_chOutSolPConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_ORGN)) *value = m_chOutOrgN[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_ORGNConc)) *value = m_chOutOrgNConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_ORGP)) *value = m_chOutOrgP[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_ORGPConc)) *value = m_chOutOrgPConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_NH4)) *value = m_chOutNH4[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_NH4Conc)) *value = m_chOutNH4Conc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_DOX)) *value = m_chOutDOx[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_DOXConc)) *value = m_chOutDOxConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_TN)) *value = m_chOutTN[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_TNConc)) *value = m_chOutTNConc[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_TP)) *value = m_chOutTP[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_TPConc)) *value = m_chOutTPConc[m_inputSubbsnID];
        /// output nutrient storage in channel
    else if (StringMatch(sk, VAR_CHSTR_NO3)) *value = m_chNO3[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CHSTR_NH4)) *value = m_chNH4[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CHSTR_TN)) *value = m_chTN[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CHSTR_TP)) *value = m_chTP[m_inputSubbsnID];
    	//ljj++
	else if (StringMatch(sk, VAR_CH_DIC)) *value = m_chOutDIC[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CH_DICConc)) *value = m_chOutDICConc[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CHSTR_DIC)) *value = m_chDIC[m_inputSubbsnID];

	else if (StringMatch(sk, VAR_CH_LDOC)) *value = m_chOutLDOC[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CH_LDOCConc)) *value = m_chOutLDOCConc[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CHSTR_LDOC)) *value = m_chLDOC[m_inputSubbsnID];

	else if (StringMatch(sk, VAR_CH_LPOC)) *value = m_chOutLPOC[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CH_LPOCConc)) *value = m_chOutLPOCConc[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CHSTR_LPOC)) *value = m_chLPOC[m_inputSubbsnID];

	else if (StringMatch(sk, VAR_CH_RPOC)) *value = m_chOutRPOC[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CH_RPOCConc)) *value = m_chOutRPOCConc[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CHSTR_RPOC)) *value = m_chRPOC[m_inputSubbsnID];

	else if (StringMatch(sk, VAR_CH_RDOC)) *value = m_chOutRDOC[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CH_RDOCConc)) *value = m_chOutRDOCConc[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CHSTR_RDOC)) *value = m_chRDOC[m_inputSubbsnID];

    else if (StringMatch(sk, VAR_CH_TOTDOC)) *value = m_chOutTotDOC[m_inputSubbsnID];
	else if (StringMatch(sk, VAR_CH_TOTDOCConc)) *value = m_chOutTotDOCConc[m_inputSubbsnID];

    else if (StringMatch(sk, "CH_TOTPOC")) *value = m_chOutTotPOC[m_inputSubbsnID];
	else if (StringMatch(sk, "CH_TOTPOCConc")) *value = m_chOutTotPOCConc[m_inputSubbsnID];

    else if (StringMatch(sk, VAR_CH_SURFRDOC)) *value = m_chOutsurfRDOC[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CH_LATRDOC)) *value = m_chOutlatRDOC[m_inputSubbsnID];
    else if (StringMatch(sk, "CH_GWRDOC")) *value = m_chOutgwRDOC[m_inputSubbsnID];

    else if (StringMatch(sk, VAR_CHSTR_SURFRDOC)) *value = m_chsurfRDOC[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CHSTR_LATRDOC)) *value = m_chlatRDOC[m_inputSubbsnID];
    else if (StringMatch(sk, VAR_CHSTR_GWDRDOC)) *value = m_chgwRDOC[m_inputSubbsnID];
     
    else {
        throw ModelException(MID_NUTRCH_QUAL2E, "GetValue", "Parameter " + sk + " does not exist.");
    }
}

void NutrCH_QUAL2E::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    *n = m_nReaches + 1;
    if (StringMatch(sk, VAR_CH_ALGAE)) *data = m_chOutAlgae;
    else if (StringMatch(sk, VAR_CH_ALGAEConc)) *data = m_chOutAlgaeConc;
    else if (StringMatch(sk, VAR_CH_NO2)) *data = m_chOutNO2;
    else if (StringMatch(sk, VAR_CH_NO2Conc)) *data = m_chOutNO2Conc;
    else if (StringMatch(sk, VAR_CH_COD)) *data = m_chOutCOD;
    else if (StringMatch(sk, VAR_CH_CODConc)) *data = m_chOutCODConc;
    else if (StringMatch(sk, VAR_CH_CHLORA)) *data = m_chOutChlora;
    else if (StringMatch(sk, VAR_CH_CHLORAConc)) *data = m_chOutChloraConc;
    else if (StringMatch(sk, VAR_CH_NO3)) *data = m_chOutNO3;
    else if (StringMatch(sk, VAR_CH_NO3Conc)) *data = m_chOutNO3Conc;
    else if (StringMatch(sk, VAR_CH_SOLP)) *data = m_chOutSolP;
    else if (StringMatch(sk, VAR_CH_SOLPConc)) *data = m_chOutSolPConc;
    else if (StringMatch(sk, VAR_CH_ORGN)) *data = m_chOutOrgN;
    else if (StringMatch(sk, VAR_CH_ORGNConc)) *data = m_chOutOrgNConc;
    else if (StringMatch(sk, VAR_CH_ORGP)) *data = m_chOutOrgP;
    else if (StringMatch(sk, VAR_CH_ORGPConc)) *data = m_chOutOrgPConc;
    else if (StringMatch(sk, VAR_CH_NH4)) *data = m_chOutNH4;
    else if (StringMatch(sk, VAR_CH_NH4Conc)) *data = m_chOutNH4Conc;
    else if (StringMatch(sk, VAR_CH_DOX)) *data = m_chOutDOx;
    else if (StringMatch(sk, VAR_CH_DOXConc)) *data = m_chOutDOxConc;
    else if (StringMatch(sk, VAR_CH_TN)) *data = m_chOutTN;
    else if (StringMatch(sk, VAR_CH_TNConc)) *data = m_chOutTNConc;
    else if (StringMatch(sk, VAR_CH_TP)) *data = m_chOutTP;
    else if (StringMatch(sk, VAR_CH_TPConc)) *data = m_chOutTPConc;
    else if (StringMatch(sk, VAR_PTTN2CH)) *data = m_ptTNToCh;
    else if (StringMatch(sk, VAR_PTTP2CH)) *data = m_ptTPToCh;
    else if (StringMatch(sk, VAR_PTCOD2CH)) *data = m_ptCODToCh;
        /// output nutrient storage in channel
    else if (StringMatch(sk, VAR_CHSTR_NO3)) *data = m_chNO3;
    else if (StringMatch(sk, VAR_CHSTR_NH4)) *data = m_chNH4;
    else if (StringMatch(sk, VAR_CHSTR_TN)) *data = m_chTN;
    else if (StringMatch(sk, VAR_CHSTR_TP)) *data = m_chTP;
    	//ljj++
    else if (StringMatch(sk, VAR_chtotdoc)) *data = m_chOutTotDOC;
    else if (StringMatch(sk, VAR_chsurfdoc)) *data = m_chOutsurfRDOC;
    else if (StringMatch(sk, "chlatdoc")) *data = m_chOutlatRDOC;
    else if (StringMatch(sk, "chgwdoc")) *data = m_chOutgwRDOC;
    else if (StringMatch(sk, "chdoc")) *data = m_chDOCcon;
	else if (StringMatch(sk, VAR_CHSTR_DIC)) *data = m_chDIC;
	else if (StringMatch(sk, VAR_CH_DICConc)) *data = m_chOutDICConc;
	else if (StringMatch(sk, VAR_CH_DIC)) *data = m_chOutDIC;

	else if (StringMatch(sk, VAR_CHSTR_LDOC)) *data = m_chLDOC;
	else if (StringMatch(sk, VAR_CH_LDOCConc)) *data = m_chOutLDOCConc;
	else if (StringMatch(sk, VAR_CH_LDOC)) *data = m_chOutLDOC;

	else if (StringMatch(sk, VAR_CHSTR_LPOC)) *data = m_chLPOC;
	else if (StringMatch(sk, VAR_CH_LPOCConc)) *data = m_chOutLPOCConc;
	else if (StringMatch(sk, VAR_CH_LPOC)) *data = m_chOutLPOC;

	else if (StringMatch(sk, VAR_CHSTR_RPOC)) *data = m_chRPOC;
	else if (StringMatch(sk, VAR_CH_RPOCConc)) *data = m_chOutRPOCConc;
	else if (StringMatch(sk, VAR_CH_RPOC)) *data = m_chOutRPOC;

	else if (StringMatch(sk, VAR_CHSTR_RDOC)) *data = m_chRDOC;
	else if (StringMatch(sk, VAR_CH_RDOCConc)) *data = m_chOutRDOCConc;
	else if (StringMatch(sk, VAR_CH_RDOC)) *data = m_chOutRDOC;

    else if (StringMatch(sk, VAR_CH_TOTDOC)) *data = m_chOutTotDOC;
	else if (StringMatch(sk, VAR_CH_TOTDOCConc)) *data = m_chOutTotDOCConc;

    else if (StringMatch(sk, "CH_TOTPOC")) *data = m_chOutTotPOC;
	else if (StringMatch(sk, "CH_TOTPOCConc")) *data = m_chOutTotPOCConc;

    else if (StringMatch(sk, VAR_CH_SURFRDOC)) *data = m_chOutsurfRDOC;
    else if (StringMatch(sk, VAR_CH_LATRDOC)) *data = m_chOutlatRDOC;
    else if (StringMatch(sk, "CH_GWRDOC")) *data = m_chOutgwRDOC;
    else if (StringMatch(sk, VAR_CHSTR_SURFRDOC)) *data = m_chsurfRDOC;
    else if (StringMatch(sk, VAR_CHSTR_LATRDOC)) *data = m_chlatRDOC;
    else if (StringMatch(sk, VAR_CHSTR_GWDRDOC)) *data = m_chgwRDOC;

    else if (StringMatch(sk, "chtemp")) *data = m_ch_Temp;

    else {
        throw ModelException(MID_NUTRCH_QUAL2E, "Get1DData", "Parameter " + sk + " does not exist.");
    }
}

void NutrCH_QUAL2E::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, "lake_ocwb")) {
        *data = m_T_LKWB;
        *nrows = m_nReaches + 1;
        *ncols = 8;
    } else {
        throw ModelException(MID_GWA_RE, "Get2DData", "Parameter " + sk + " does not exist in current module.");
    }
}

