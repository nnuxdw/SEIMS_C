#include "NutrientTransportSediment.h"

#include "text.h"
#include "NutrientCommon.h"

NutrientTransportSediment::NutrientTransportSediment() :
    m_nSubbsns(-1), m_inputSubbsnID(-1), m_cellWth(-1.f), m_cellArea(-1.f), m_nCells(-1),
    m_nSoilLyrs(nullptr), m_maxSoilLyrs(-1),
    m_soilRock(nullptr), m_soilSat(nullptr), m_cbnModel(0), m_enratio(nullptr),
    m_olWtrEroSed(nullptr), m_surfRf(nullptr), m_soilBD(nullptr), m_soilThk(nullptr), m_soilMass(nullptr),
    m_subbsnID(nullptr), m_subbasinsInfo(nullptr), m_surfRfSedOrgN(nullptr),
    m_surfRfSedOrgP(nullptr), m_surfRfSedAbsorbMinP(nullptr), m_surfRfSedSorbMinP(nullptr),
    m_surfRfSedOrgNToCh(nullptr), m_surfRfSedOrgPToCh(nullptr),
    /// for CENTURY C/N cycling model inputs
    m_surfRfSedAbsorbMinPToCh(nullptr), m_surfRfSedSorbMinPToCh(nullptr), m_soilActvOrgN(nullptr),
    m_soilFrshOrgN(nullptr),
    m_soilStabOrgN(nullptr),
    m_soilHumOrgP(nullptr), m_soilFrshOrgP(nullptr), m_soilStabMinP(nullptr), m_soilActvMinP(nullptr),
    m_soilManP(nullptr),
    m_sol_LSN(nullptr), m_sol_LMN(nullptr), m_sol_HPN(nullptr), m_sol_HSN(nullptr), m_sol_HPC(nullptr),
    m_sol_HSC(nullptr), m_sol_LMC(nullptr), m_sol_LSC(nullptr),
    /// for C-FARM one carbon model input
    m_sol_LS(nullptr),
    /// for CENTURY C/N cycling model outputs
    m_sol_LM(nullptr), m_sol_LSL(nullptr), m_sol_LSLC(nullptr), m_sol_LSLNC(nullptr), m_sol_BMC(nullptr),
    //outputs
    m_sol_WOC(nullptr), m_soilPerco(nullptr), m_subSurfRf(nullptr), m_soilIfluCbn(nullptr),
    m_soilPercoCbn(nullptr), m_soilIfluCbnPrfl(nullptr), m_soilPercoCbnPrfl(nullptr), m_sedLossCbn(nullptr),
    //ljj++
    m_area(nullptr),m_soilDepth(nullptr),m_rteLyrs(nullptr), m_nRteLyrs(-1),m_flowOutIdxD8(nullptr),
    m_rchID(nullptr),m_soiltemp(nullptr),
    m_enr_POC(NODATA_VALUE),m_kd_oc(-1.f), m_perco_doc(-1.f),
    m_soileroRPOC(nullptr), m_soileroLPOC(nullptr),m_LPOCtoCH(nullptr),m_RPOCtoCH(nullptr),
    m_sol_RSPC(nullptr), m_soilWP(nullptr), m_soilWtrSto(nullptr), m_soilPor(nullptr),
    m_soilWtrDIC(nullptr), m_soilSurfInOrgnCbn(nullptr), m_soilIfluInOrgnCbn(nullptr), m_soilPercoInOrgnCbn(nullptr),
    m_surfDICtoCH(nullptr),m_IfluDICtoCH(nullptr),
    m_soilPercoCbnLowest(nullptr), m_soilSurfCbn(nullptr),m_soilPercoDICLowest(nullptr),
    m_LDOCToCH(nullptr),m_surfRDOCtoCH(nullptr),m_IfluRDOCtoCH(nullptr),m_brt(nullptr),m_surfrunoff(nullptr),
    m_lag_doc(nullptr),m_lag_dic(nullptr),m_lag_rpoc(nullptr),m_lag_lpoc(nullptr),m_landUse(nullptr),
    m_lag_orgn(nullptr),m_lag_orgp(nullptr),m_surfpoc(nullptr),m_surfdoc(nullptr),m_subsurfdoc(nullptr),
    m_lag_minpa(nullptr),m_lag_minps(nullptr)
{
}

NutrientTransportSediment::~NutrientTransportSediment() {
    if (m_soilMass != nullptr) Release2DArray(m_nCells, m_soilMass);
    if (m_enratio != nullptr) Release1DArray(m_enratio);

    if (m_surfRfSedOrgP != nullptr) Release1DArray(m_surfRfSedOrgP);
    if (m_surfRfSedOrgN != nullptr) Release1DArray(m_surfRfSedOrgN);
    if (m_surfRfSedAbsorbMinP != nullptr) Release1DArray(m_surfRfSedAbsorbMinP);
    if (m_surfRfSedSorbMinP != nullptr) Release1DArray(m_surfRfSedSorbMinP);

    if (m_surfRfSedOrgNToCh != nullptr) Release1DArray(m_surfRfSedOrgNToCh);
    if (m_surfRfSedOrgPToCh != nullptr) Release1DArray(m_surfRfSedOrgPToCh);
    if (m_surfRfSedAbsorbMinPToCh != nullptr) Release1DArray(m_surfRfSedAbsorbMinPToCh);
    if (m_surfRfSedSorbMinPToCh != nullptr) Release1DArray(m_surfRfSedSorbMinPToCh);

    /// for CENTURY C/N cycling model outputs
    if (m_soilIfluCbn != nullptr) Release2DArray(m_nCells, m_soilPercoCbn);
    if (m_soilPercoCbn != nullptr) Release2DArray(m_nCells, m_soilPercoCbn);
    if (m_soilIfluCbnPrfl != nullptr) Release1DArray(m_soilIfluCbnPrfl);
    if (m_soilPercoCbnPrfl != nullptr) Release1DArray(m_soilPercoCbnPrfl);
    if (m_sedLossCbn != nullptr) Release1DArray(m_sedLossCbn);

    //ljj++
    if (m_soileroLPOC != nullptr) Release1DArray(m_soileroLPOC);
    if (m_soileroRPOC != nullptr) Release1DArray(m_soileroRPOC);
    if (m_RPOCtoCH != nullptr) Release1DArray(m_RPOCtoCH);
	if (m_LPOCtoCH != nullptr) Release1DArray(m_LPOCtoCH);
    if (m_soilWtrDIC != nullptr) Release2DArray(m_nCells, m_soilWtrDIC);
    if (m_soilSurfInOrgnCbn != nullptr) Release1DArray(m_soilSurfInOrgnCbn);
    if (m_soilIfluInOrgnCbn != nullptr) Release2DArray(m_nCells, m_soilIfluInOrgnCbn);
    if (m_soilPercoInOrgnCbn != nullptr) Release2DArray(m_nCells, m_soilPercoInOrgnCbn);
    if (m_surfDICtoCH != nullptr) Release1DArray(m_surfDICtoCH);
    if (m_IfluDICtoCH != nullptr) Release1DArray(m_IfluDICtoCH);
    if (m_soilSurfCbn != nullptr) Release1DArray(m_soilSurfCbn);
    if (m_soilPercoCbnLowest != nullptr) Release1DArray(m_soilPercoCbnLowest);
    if (m_soilPercoDICLowest != nullptr) Release1DArray(m_soilPercoDICLowest);
    if (m_LDOCToCH != nullptr) Release1DArray(m_LDOCToCH);
    if (m_surfRDOCtoCH != nullptr) Release1DArray(m_surfRDOCtoCH);
    if (m_IfluRDOCtoCH != nullptr) Release1DArray(m_IfluRDOCtoCH);
    if (m_lag_doc != nullptr) Release2DArray(m_nCells, m_lag_doc);
    if (m_lag_dic != nullptr) Release2DArray(m_nCells, m_lag_dic);
    if (m_lag_rpoc != nullptr) Release2DArray(m_nCells, m_lag_rpoc);
    if (m_lag_lpoc != nullptr) Release2DArray(m_nCells, m_lag_lpoc);
    if (m_lag_orgn != nullptr) Release2DArray(m_nCells, m_lag_orgn);
    if (m_lag_orgp != nullptr) Release2DArray(m_nCells, m_lag_orgp);
    if (m_lag_minpa != nullptr) Release2DArray(m_nCells, m_lag_minpa);
    if (m_lag_minps != nullptr) Release2DArray(m_nCells, m_lag_minps);

}

bool NutrientTransportSediment::CheckInputData() {
    CHECK_POSITIVE(MID_NUTRSED, m_nCells);
    CHECK_POSITIVE(MID_NUTRSED, m_nSubbsns);
    CHECK_POSITIVE(MID_NUTRSED, m_cellWth);
    CHECK_POSITIVE(MID_NUTRSED, m_maxSoilLyrs);
    CHECK_POINTER(MID_NUTRSED, m_nSoilLyrs);
    CHECK_POINTER(MID_NUTRSED, m_olWtrEroSed);
    CHECK_POINTER(MID_NUTRSED, m_surfRf);
    CHECK_POINTER(MID_NUTRSED, m_soilBD);
    CHECK_POINTER(MID_NUTRSED, m_soilActvMinP);
    CHECK_POINTER(MID_NUTRSED, m_soilStabOrgN);
    CHECK_POINTER(MID_NUTRSED, m_soilHumOrgP);
    CHECK_POINTER(MID_NUTRSED, m_soilStabMinP);
    CHECK_POINTER(MID_NUTRSED, m_soilActvOrgN);
    CHECK_POINTER(MID_NUTRSED, m_soilFrshOrgN);
    CHECK_POINTER(MID_NUTRSED, m_soilFrshOrgP);
    CHECK_POINTER(MID_NUTRSED, m_subbsnID);
    CHECK_POINTER(MID_NUTRSED, m_subbasinsInfo);
    if (!(m_cbnModel == 0 || m_cbnModel == 1 || m_cbnModel == 2)) {
        throw ModelException(MID_NUTRSED, "CheckInputData", "Carbon modeling method must be 0, 1, or 2.");
    }
    return true;
}

bool NutrientTransportSediment::CheckInputDataCenturyModel() {
    CHECK_POINTER(MID_NUTRSED, m_sol_LSN);
    CHECK_POINTER(MID_NUTRSED, m_sol_LMN);
    CHECK_POINTER(MID_NUTRSED, m_sol_HPN);
    CHECK_POINTER(MID_NUTRSED, m_sol_HSN);
    CHECK_POINTER(MID_NUTRSED, m_sol_HPC);
    CHECK_POINTER(MID_NUTRSED, m_sol_HSC);
    CHECK_POINTER(MID_NUTRSED, m_sol_LMC);
    CHECK_POINTER(MID_NUTRSED, m_sol_LSC);
    CHECK_POINTER(MID_NUTRSED, m_sol_LS);
    CHECK_POINTER(MID_NUTRSED, m_sol_LM);
    CHECK_POINTER(MID_NUTRSED, m_sol_LSL);
    CHECK_POINTER(MID_NUTRSED, m_sol_LSLC);
    CHECK_POINTER(MID_NUTRSED, m_sol_LSLNC);
    CHECK_POINTER(MID_NUTRSED, m_sol_BMC);
    CHECK_POINTER(MID_NUTRSED, m_sol_WOC);
    CHECK_POINTER(MID_NUTRSED, m_soilPerco);
    CHECK_POINTER(MID_NUTRSED, m_subSurfRf);
    return true;
}

bool NutrientTransportSediment::CheckInputDataCFarmModel() {
    CHECK_POINTER(MID_NUTRSED, m_soilManP);
    return true;
}

void NutrientTransportSediment::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
    else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
    else if (StringMatch(sk, Tag_CellWidth)) m_cellWth = value;
    else if (StringMatch(sk, VAR_CSWAT)) m_cbnModel = CVT_INT(value);
    //ljj++
    else if (StringMatch(sk, VAR_ENRPOC)) m_enr_POC = value;
    else if (StringMatch(sk, VAR_KDOC)) m_kd_oc = value;
	else if (StringMatch(sk, VAR_PERCO_DOC)) m_perco_doc = value;
    else {
        throw ModelException(MID_NUTRSED, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void NutrientTransportSediment::Set1DData(const char* key, const int n, float* data) {
    CheckInputSize(MID_NUTRSED, key, n, m_nCells);
    string sk(key);
    if (StringMatch(sk, VAR_SUBBSN)) {
        m_subbsnID = data;
    } else if (StringMatch(sk, VAR_SOILLAYERS)) {
        m_nSoilLyrs = data;
    } else if (StringMatch(sk, VAR_SEDYLD)) {
        m_olWtrEroSed = data;
    } else if (StringMatch(sk, VAR_OLFLOW)) {
        m_surfRf = data;
    } 
        //ljj++
    else if (StringMatch(sk, VAR_AHRU)) m_area = data;  
    else if (StringMatch(sk, Tag_FLOWOUT_INDEX_D8)) m_flowOutIdxD8 = data;
    else if (StringMatch(sk, VAR_STREAM_LINK)) m_rchID = data;
    else if (StringMatch(sk, "BRT")) m_brt = data;
    else if (StringMatch(sk, VAR_SURU)) m_surfrunoff = data;
    else if (StringMatch(sk, VAR_LANDUSE)) m_landUse = data;
    else if (StringMatch(sk, "SURFDOC")) m_surfdoc = data;
    else if (StringMatch(sk, "SURFPOC")) m_surfpoc = data;
    else {
        throw ModelException(MID_NUTRSED, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void NutrientTransportSediment::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    string sk(key);
    if (StringMatch(sk, Tag_ROUTING_LAYERS)) {
        CheckInputSize(MID_NUTRSED, key, nrows, m_nRteLyrs);
        //m_nRteLyrs = nrows;
        m_rteLyrs = data;
        return;
    }

    CheckInputSize2D(MID_NUTRSED, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
    
    if (StringMatch(sk, VAR_SOILTHICK)) m_soilThk = data;
    else if (StringMatch(sk, VAR_SOL_BD)) m_soilBD = data;
    else if (StringMatch(sk, VAR_SOL_AORGN)) m_soilActvOrgN = data;
    else if (StringMatch(sk, VAR_SOL_SORGN)) m_soilStabOrgN = data;
    else if (StringMatch(sk, VAR_SOL_HORGP)) m_soilHumOrgP = data;
    else if (StringMatch(sk, VAR_SOL_FORGP)) m_soilFrshOrgP = data;
    else if (StringMatch(sk, VAR_SOL_FORGN)) m_soilFrshOrgN = data;
    else if (StringMatch(sk, VAR_SOL_ACTP)) m_soilActvMinP = data;
    else if (StringMatch(sk, VAR_SOL_STAP)) m_soilStabMinP = data;
        /// for CENTURY C/Y cycling model, optional inputs
    else if (StringMatch(sk, VAR_ROCK)) m_soilRock = data;
    else if (StringMatch(sk, VAR_SOL_UL)) m_soilSat = data;
    else if (StringMatch(sk, VAR_SOL_LSN)) m_sol_LSN = data;
    else if (StringMatch(sk, VAR_SOL_LMN)) m_sol_LMN = data;
    else if (StringMatch(sk, VAR_SOL_HPN)) m_sol_HPN = data;
    else if (StringMatch(sk, VAR_SOL_HSN)) m_sol_HSN = data;
    else if (StringMatch(sk, VAR_SOL_HPC)) m_sol_HPC = data;
    else if (StringMatch(sk, VAR_SOL_HSC)) m_sol_HSC = data;
    else if (StringMatch(sk, VAR_SOL_LMC)) m_sol_LMC = data;
    else if (StringMatch(sk, VAR_SOL_LSC)) m_sol_LSC = data;
    else if (StringMatch(sk, VAR_SOL_LS)) m_sol_LS = data;
    else if (StringMatch(sk, VAR_SOL_LM)) m_sol_LM = data;
    else if (StringMatch(sk, VAR_SOL_LSL)) m_sol_LSL = data;
    else if (StringMatch(sk, VAR_SOL_LSLC)) m_sol_LSLC = data;
    else if (StringMatch(sk, VAR_SOL_LSLNC)) m_sol_LSLNC = data;
    else if (StringMatch(sk, VAR_SOL_BMC)) m_sol_BMC = data;
    else if (StringMatch(sk, VAR_SOL_WOC)) m_sol_WOC = data;
    else if (StringMatch(sk, VAR_PERCO)) m_soilPerco = data;
    else if (StringMatch(sk, VAR_SSRU)) m_subSurfRf = data;
        /// for C-FARM one carbon model
    else if (StringMatch(sk, VAR_SOL_MP)) m_soilManP = data;
    else if (StringMatch(sk, VAR_SOL_RSPC)) m_sol_RSPC = data;
    else if (StringMatch(sk, VAR_SOL_ST)) m_soilWtrSto = data;
    else if (StringMatch(sk, VAR_SOL_WPMM)) m_soilWP = data;
    else if (StringMatch(sk, VAR_POROST)) m_soilPor = data;
    else if (StringMatch(sk, VAR_SOILDEPTH)) m_soilDepth = data;
    else if (StringMatch(sk, VAR_SOILT)) m_soiltemp = data;
    else if (StringMatch(sk, "SUBSURFDOC")) m_subsurfdoc = data;
    else {
        throw ModelException(MID_NUTRSED, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void NutrientTransportSediment::InitialOutputs() {
    CHECK_POSITIVE(MID_NUTRSED, m_nCells);
    // initial enrichment ratio
    if (nullptr == m_enratio) {
        Initialize1DArray(m_nCells, m_enratio, 0.f);
    }
    if (m_cellArea < 0) {
        m_cellArea = m_cellWth * m_cellWth * 0.0001f; //Unit is ha
    }
    /// initialize m_soilMass
    if (m_soilMass == nullptr) {
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilMass, 0.f);
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                m_soilMass[i][k] = 10000.f * m_soilThk[i][k] * m_soilBD[i][k] * (1.f - m_soilRock[i][k] * 0.01f);
            }
        }
    }
    // allocate the output variables
    if (nullptr == m_surfRfSedOrgN) {
        Initialize1DArray(m_nCells, m_surfRfSedOrgN, 0.f);
        Initialize1DArray(m_nCells, m_surfRfSedOrgP, 0.f);
        Initialize1DArray(m_nCells, m_surfRfSedAbsorbMinP, 0.f);
        Initialize1DArray(m_nCells, m_surfRfSedSorbMinP, 0.f);

        Initialize1DArray(m_nSubbsns + 1, m_surfRfSedOrgNToCh, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_surfRfSedOrgPToCh, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_surfRfSedAbsorbMinPToCh, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_surfRfSedSorbMinPToCh, 0.f);
    }
    /// for CENTURY C/N cycling model outputs
    if (m_cbnModel == 2 && nullptr == m_soilIfluCbn) {
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilIfluCbn, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilPercoCbn, 0.f);
        Initialize1DArray(m_nCells, m_soilIfluCbnPrfl, 0.f);
        Initialize1DArray(m_nCells, m_soilPercoCbnPrfl, 0.f);
        Initialize1DArray(m_nCells, m_sedLossCbn, 0.f);

        //ljj++
        Initialize1DArray(m_nCells, m_soileroRPOC, 0.f);
        Initialize1DArray(m_nCells, m_soileroLPOC, 0.f);

        Initialize1DArray(m_nSubbsns + 1, m_LPOCtoCH, 0.f);
		Initialize1DArray(m_nSubbsns + 1, m_RPOCtoCH, 0.f);
        Initialize1DArray(m_nCells, m_soilSurfInOrgnCbn, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilWtrDIC, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilIfluInOrgnCbn, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilPercoInOrgnCbn, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_surfDICtoCH, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_IfluDICtoCH, 0.f);
        Initialize1DArray(m_nCells, m_soilSurfCbn, 0.f);
        Initialize1DArray(m_nCells, m_soilPercoCbnLowest, 0.f);
        Initialize1DArray(m_nCells, m_soilPercoDICLowest, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_LDOCToCH, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_surfRDOCtoCH, 0.f);
        Initialize1DArray(m_nSubbsns + 1, m_IfluRDOCtoCH, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_doc, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_dic, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_lpoc, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_rpoc, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_orgn, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_orgp, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_minpa, 0.f);
        Initialize2DArray(m_nCells, 2, m_lag_minps, 0.f);
    }
}

void NutrientTransportSediment::SetSubbasins(clsSubbasins* subbasins) {
    if (nullptr == m_subbasinsInfo) {
        m_subbasinsInfo = subbasins;
        // m_nSubbasins = m_subbasinsInfo->GetSubbasinNumber(); // Set in SetValue()
        m_subbasinIDs = m_subbasinsInfo->GetSubbasinIDs();
    }
}

int NutrientTransportSediment::Execute() {
    CheckInputData();
    if (m_cbnModel == 1) {
        if (!CheckInputDataCFarmModel()) return false;
    }
    if (m_cbnModel == 2) {
        if (!CheckInputDataCenturyModel()) return false;
    }
    InitialOutputs();
    // initial nutrient to channel for each day
    for (int i = 0; i < m_nSubbsns + 1; i++) {
        m_surfRfSedOrgNToCh[i] = 0.f;
        m_surfRfSedOrgPToCh[i] = 0.f;
        m_surfRfSedAbsorbMinPToCh[i] = 0.f;
        m_surfRfSedSorbMinPToCh[i] = 0.f;
        m_LPOCtoCH[i] = 0.f;
		m_RPOCtoCH[i] = 0.f;
        m_surfDICtoCH[i] = 0.f;
		m_IfluDICtoCH[i] = 0.f;
        m_LDOCToCH[i] = 0.f;  //no LDOC from HRU or landscape
		m_surfRDOCtoCH[i] = 0.f;
		m_IfluRDOCtoCH[i] = 0.f;
    }
    for (int i = 0; i < m_nCells; i++) {
        for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
            m_soilIfluCbn[i][k] = 0.f;
            m_soilIfluInOrgnCbn[i][k] = 0.f;
        }
    }
    for (int ilyr = 0; ilyr < m_nRteLyrs; ilyr++) {
        // There are not any flow relationship within each routing layer.
        // So parallelization can be done here.
        int ncells = CVT_INT(m_rteLyrs[ilyr][0]);
#pragma omp parallel for
        for (int icell = 1; icell <= ncells; icell++) {
            int i = CVT_INT(m_rteLyrs[ilyr][icell]); // cell ID
            //if (m_rchID[i] > 0) continue;            // Skip the reach (stream) cells
            if(m_soilMass[i][0]<=UTIL_ZERO) continue;
            if (m_olWtrEroSed[i] < 1.e-4f) m_olWtrEroSed[i] = 0.f;
            // CREAMS method for calculating enrichment ratio
            //m_enratio[i] = CalEnrichmentRatio(m_olWtrEroSed[i], m_surfRf[i], m_cellArea);
            m_enratio[i] = CalEnrichmentRatio(m_olWtrEroSed[i], m_surfRf[i], m_area[i] * 0.0001f);
            if(m_landUse[i] == LANDUSE_ID_WATR|| m_landUse[i] == LANDUSE_ID_GLC) continue;
            //Calculates the amount of organic nitrogen removed in surface runoff
            if (m_cbnModel == 0) {
                OrgNRemovedInRunoffStaticMethod(i);
            } else if (m_cbnModel == 1) {
                OrgNRemovedInRunoffCFarmOneCarbonModel(i);
            } else if (m_cbnModel == 2) {
                OrgNRemovedInRunoffCenturyModel(i);
            }
            //Calculates the amount of organic and mineral phosphorus attached to sediment in surface runoff. psed.f of SWAT
            OrgPAttachedtoSed(i);
        }
    // for (int i = 0; i < m_nCells; i++) {
    //     if (m_olWtrEroSed[i] < 1.e-4f) m_olWtrEroSed[i] = 0.f;
    //     // CREAMS method for calculating enrichment ratio
    //     //m_enratio[i] = CalEnrichmentRatio(m_olWtrEroSed[i], m_surfRf[i], m_cellArea);
    //     m_enratio[i] = CalEnrichmentRatio(m_olWtrEroSed[i], m_surfRf[i], m_area[i] * 0.0001f);

    //     //Calculates the amount of organic nitrogen removed in surface runoff
    //     if (m_cbnModel == 0) {
    //         OrgNRemovedInRunoffStaticMethod(i);
    //     } else if (m_cbnModel == 1) {
    //         OrgNRemovedInRunoffCFarmOneCarbonModel(i);
    //     } else if (m_cbnModel == 2) {
    //         OrgNRemovedInRunoffCenturyModel(i);
    //     }
    //     //Calculates the amount of organic and mineral phosphorus attached to sediment in surface runoff. psed.f of SWAT
    //     OrgPAttachedtoSed(i);
    // }
    }
    // sum by subbasin
    // See https://github.com/lreis2415/SEIMS/issues/36 for more descriptions. By lj

#pragma omp parallel
    {
        float* tmp_orgn2ch = new(nothrow) float[m_nSubbsns + 1];
        float* tmp_orgp2ch = new(nothrow) float[m_nSubbsns + 1];
        float* tmp_minpa2ch = new(nothrow) float[m_nSubbsns + 1];
        float* tmp_minps2ch = new(nothrow) float[m_nSubbsns + 1];
        // float* tmp_lpoc2ch = new(nothrow) float[m_nSubbsns + 1];
        float* tmp_rpoc2ch = new(nothrow) float[m_nSubbsns + 1];
        //float* tmp_surfdic2ch = new(nothrow) float[m_nSubbsns + 1];
        //float* tmp_latdic2ch = new(nothrow) float[m_nSubbsns + 1];
        float* tmp_surfrdoc2ch = new(nothrow) float[m_nSubbsns + 1];
        float* tmp_latrdoc2ch = new(nothrow) float[m_nSubbsns + 1];

        for (int i = 0; i <= m_nSubbsns; i++) {
            tmp_orgn2ch[i] = 0.f;
            tmp_orgp2ch[i] = 0.f;
            tmp_minpa2ch[i] = 0.f;
            tmp_minps2ch[i] = 0.f;
            //ljj++
            // tmp_lpoc2ch[i] = 0.f;
            tmp_rpoc2ch[i] = 0.f;
            //tmp_surfdic2ch[i] = 0.f;
            //tmp_latdic2ch[i] = 0.f;
            tmp_surfrdoc2ch[i] = 0.f;
            tmp_latrdoc2ch[i] = 0.f;
        }
#pragma omp for
        for (int i = 0; i < m_nCells; i++) {
            //m_soilSurfCbn[i] = Max(m_soilSurfCbn[i],0.f);
            //m_soilSurfInOrgnCbn[i] = Max(m_soilSurfInOrgnCbn[i],0.f);
            //m_soileroLPOC[i] = Max(m_soileroLPOC[i],0.f);
            //m_soileroRPOC[i] = Max(m_soileroRPOC[i],0.f);
            //m_lag_doc[i][1] += m_surfdoc[i];
            m_lag_doc[i][1] += m_soilSurfCbn[i];
            //m_lag_dic[i][1] += m_soilSurfInOrgnCbn[i];
            //m_lag_lpoc[i][1] += m_soileroLPOC[i];
            //m_lag_rpoc[i][1] += m_surfpoc[i];
            m_lag_rpoc[i][1] += m_soileroRPOC[i];
            m_lag_orgn[i][1] += m_surfRfSedOrgN[i];
            m_lag_orgp[i][1] += m_surfRfSedOrgP[i];
            m_lag_minpa[i][1] += m_surfRfSedAbsorbMinP[i];
            m_lag_minps[i][1] += m_surfRfSedSorbMinP[i];
        }
#pragma omp for
        for (int i = 0; i < m_nCells; i++) {
            // tmp_orgn2ch[CVT_INT(m_subbsnID[i])] += m_surfRfSedOrgN[i] * m_area[i] * 0.0001f;
            // tmp_orgp2ch[CVT_INT(m_subbsnID[i])] += m_surfRfSedOrgP[i] * m_area[i] * 0.0001f;
            tmp_orgn2ch[CVT_INT(m_subbsnID[i])] += m_lag_orgn[i][1] *m_brt[i] * m_area[i] * 0.0001f;
            tmp_orgp2ch[CVT_INT(m_subbsnID[i])] += m_lag_orgp[i][1] * m_brt[i] * m_area[i] * 0.0001f;
            // tmp_minpa2ch[CVT_INT(m_subbsnID[i])] += m_surfRfSedAbsorbMinP[i] * m_area[i] * 0.0001f;
            // tmp_minps2ch[CVT_INT(m_subbsnID[i])] += m_surfRfSedSorbMinP[i] * m_area[i] * 0.0001f;
            tmp_minpa2ch[CVT_INT(m_subbsnID[i])] += m_lag_minpa[i][1] *m_brt[i] * m_area[i] * 0.0001f;
            tmp_minps2ch[CVT_INT(m_subbsnID[i])] += m_lag_minps[i][1] * m_brt[i] * m_area[i] * 0.0001f;
            //ljj++
            // tmp_lpoc2ch[CVT_INT(m_subbsnID[i])] += m_soileroLPOC[i] * m_area[i] * 0.0001f;
            // tmp_rpoc2ch[CVT_INT(m_subbsnID[i])] += m_soileroRPOC[i] * m_area[i] * 0.0001f;
            //tmp_surfdic2ch[CVT_INT(m_subbsnID[i])] += m_soilSurfInOrgnCbn[i] * m_area[i] * 0.0001f;
            //tmp_surfrdoc2ch[CVT_INT(m_subbsnID[i])] += m_soilSurfCbn[i] * m_area[i] * 0.0001f;

            //tmp_lpoc2ch[CVT_INT(m_subbsnID[i])] += m_lag_lpoc[i][1] * m_brt[i] * m_area[i] * 0.0001f;
            tmp_rpoc2ch[CVT_INT(m_subbsnID[i])] += m_lag_rpoc[i][1] * m_brt[i] * m_area[i] * 0.0001f;
            //tmp_surfdic2ch[CVT_INT(m_subbsnID[i])] += m_lag_dic[i][1] *m_brt[i] * m_area[i] * 0.0001f;
            tmp_surfrdoc2ch[CVT_INT(m_subbsnID[i])] += m_lag_doc[i][1] * m_brt[i] * m_area[i] * 0.0001f;
            m_lag_doc[i][1] -= m_lag_doc[i][1] * m_brt[i];
            //m_lag_dic[i][1] -= m_lag_dic[i][1] * m_brt[i];
            m_lag_lpoc[i][1] -= m_lag_lpoc[i][1] * m_brt[i];
            m_lag_rpoc[i][1] -= m_lag_rpoc[i][1] * m_brt[i];
            m_lag_orgn[i][1] -= m_lag_orgn[i][1] * m_brt[i];
            m_lag_orgp[i][1] -= m_lag_orgp[i][1] * m_brt[i];
            m_lag_minps[i][1] -= m_lag_minps[i][1] * m_brt[i];
            m_lag_minpa[i][1] -= m_lag_minpa[i][1] * m_brt[i];
            m_lag_doc[i][1] = Max(m_lag_doc[i][1],0.f);
            //m_lag_dic[i][1] = Max(m_lag_dic[i][1],0.f);
            m_lag_lpoc[i][1] = Max(m_lag_lpoc[i][1],0.f);
            m_lag_rpoc[i][1] = Max(m_lag_rpoc[i][1],0.f);
            m_lag_orgn[i][1] = Max(m_lag_orgn[i][1],0.f);
            m_lag_orgp[i][1] = Max(m_lag_orgp[i][1],0.f);
            m_lag_minps[i][1] = Max(m_lag_minps[i][1],0.f);
            m_lag_minpa[i][1] = Max(m_lag_minpa[i][1],0.f);
            if (m_rchID[i] > 0) {
                for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                    //tmp_latdic2ch[CVT_INT(m_rchID[i])] += m_soilIfluInOrgnCbn[i][k]* m_area[i] * 0.0001f; //kg
                    //tmp_latrdoc2ch[CVT_INT(m_rchID[i])] += m_subsurfdoc[i][k]* m_area[i] * 0.0001f; //kg
                    tmp_latrdoc2ch[CVT_INT(m_rchID[i])] += m_soilIfluCbn[i][k]* m_area[i] * 0.0001f; //kg
                }
            }
        }
#pragma omp critical
        {
            for (int i = 1; i <= m_nSubbsns; i++) {
                m_surfRfSedOrgNToCh[i] += tmp_orgn2ch[i];
                m_surfRfSedOrgPToCh[i] += tmp_orgp2ch[i];
                m_surfRfSedAbsorbMinPToCh[i] += tmp_minpa2ch[i];
                m_surfRfSedSorbMinPToCh[i] += tmp_minps2ch[i];
                //ljj++
                //m_LPOCtoCH[i] += tmp_lpoc2ch[i];
				m_RPOCtoCH[i] += tmp_rpoc2ch[i];
                //m_surfDICtoCH[i] += tmp_surfdic2ch[i];
                //m_IfluDICtoCH[i] += tmp_latdic2ch[i];
                m_surfRDOCtoCH[i] += tmp_surfrdoc2ch[i];
                m_IfluRDOCtoCH[i] += tmp_latrdoc2ch[i];
            }
        }
        delete[] tmp_orgn2ch;
        delete[] tmp_orgp2ch;
        delete[] tmp_minpa2ch;
        delete[] tmp_minps2ch;
        tmp_orgn2ch = nullptr;
        tmp_orgp2ch = nullptr;
        tmp_minpa2ch = nullptr;
        tmp_minps2ch = nullptr;
        //ljj++
		// delete[] tmp_lpoc2ch;
        // tmp_lpoc2ch = nullptr;
        delete[] tmp_rpoc2ch;
        tmp_rpoc2ch = nullptr;
        // delete[] tmp_surfdic2ch;
        // tmp_surfdic2ch = nullptr;
        // delete[] tmp_latdic2ch;
        // tmp_latdic2ch = nullptr;
        delete[] tmp_surfrdoc2ch;
        tmp_surfrdoc2ch = nullptr;
        delete[] tmp_latrdoc2ch;
        tmp_latrdoc2ch = nullptr;
    } /* END of #pragma omp parallel */
    // sum all the subbasins and put the sum value in the zero-index of the array
    for (int i = 1; i < m_nSubbsns + 1; i++) {
        // m_surfRfSedOrgNToCh[i] *= m_cellArea;
        // m_surfRfSedOrgPToCh[i] *= m_cellArea;
        // m_surfRfSedAbsorbMinPToCh[i] *= m_cellArea;
        // m_surfRfSedSorbMinPToCh[i] *= m_cellArea;
        m_surfRfSedOrgNToCh[0] += m_surfRfSedOrgNToCh[i];
        m_surfRfSedOrgPToCh[0] += m_surfRfSedOrgPToCh[i];
        m_surfRfSedAbsorbMinPToCh[0] += m_surfRfSedAbsorbMinPToCh[i];
        m_surfRfSedSorbMinPToCh[0] += m_surfRfSedSorbMinPToCh[i];
        //ljj++
        //m_LPOCtoCH[0] += m_LPOCtoCH[i];   //units: kg
		m_RPOCtoCH[0] += m_RPOCtoCH[i];   //units: kg
        //m_surfDICtoCH[0] += m_surfDICtoCH[i];   //units: kg
        //m_IfluDICtoCH[0] += m_IfluDICtoCH[i];   //units: kg
        m_surfRDOCtoCH[0] += m_surfRDOCtoCH[i];   //units: kg
        m_IfluRDOCtoCH[0] += m_IfluRDOCtoCH[i];   //units: kg

    }
    return 0;
}

void NutrientTransportSediment::OrgNRemovedInRunoffStaticMethod(const int i) {
    //amount of organic N in first soil layer (orgninfl)
    float orgninfl = 0.f;
    //conversion factor (wt)
    float wt = 0.f;
    orgninfl = m_soilStabOrgN[i][0] + m_soilActvOrgN[i][0] + m_soilFrshOrgN[i][0];
    wt = m_soilBD[i][0] * m_soilThk[i][0] * 0.01f;
    //concentration of organic N in soil (concn)
    float concn = orgninfl * m_enratio[i] / wt;
    //Calculate the amount of organic nitrogen transported with sediment to the stream, equation 4:2.2.1 in SWAT Theory 2009, p271
    m_surfRfSedOrgN[i] = 0.001f * concn * m_olWtrEroSed[i] * 0.001f / (m_area[i] * 0.0001f); /// kg/ha
    //update soil nitrogen pools
    if (orgninfl > 1.e-6f) {
        m_soilActvOrgN[i][0] = m_soilActvOrgN[i][0] - m_surfRfSedOrgN[i] * (m_soilActvOrgN[i][0] / orgninfl);
        m_soilStabOrgN[i][0] = m_soilStabOrgN[i][0] - m_surfRfSedOrgN[i] * (m_soilStabOrgN[i][0] / orgninfl);
        m_soilFrshOrgN[i][0] = m_soilFrshOrgN[i][0] - m_surfRfSedOrgN[i] * (m_soilFrshOrgN[i][0] / orgninfl);
        if (m_soilActvOrgN[i][0] < 0.f) {
            m_surfRfSedOrgN[i] = m_surfRfSedOrgN[i] + m_soilActvOrgN[i][0];
            m_soilActvOrgN[i][0] = 0.f;
        }
        if (m_soilStabOrgN[i][0] < 0.f) {
            m_surfRfSedOrgN[i] = m_surfRfSedOrgN[i] + m_soilStabOrgN[i][0];
            m_soilStabOrgN[i][0] = 0.f;
        }
        if (m_soilFrshOrgN[i][0] < 0.f) {
            m_surfRfSedOrgN[i] = m_surfRfSedOrgN[i] + m_soilFrshOrgN[i][0];
            m_soilFrshOrgN[i][0] = 0.f;
        }
    }
}

void NutrientTransportSediment::OrgNRemovedInRunoffCFarmOneCarbonModel(const int i) {
    /// TODO
}

void NutrientTransportSediment::OrgNRemovedInRunoffCenturyModel(const int i) {
    float totOrgN_lyr0 = 0.f; /// kg N/ha, amount of organic N in first soil layer, i.e., xx in SWAT src.
    float wt1 = 0.f;          /// conversion factor, mg/kg => kg/ha
    float er = 0.f;           /// enrichment ratio
    float conc = 0.f;         /// concentration of organic N in soil
    float QBC = 0.f;          /// C loss with runoff or lateral flow
    float VBC = 0.f;          /// C loss with vertical flow
    float YBC = 0.f;          /// BMC loss with sediment
    float YOC = 0.f;          /// Organic C loss with sediment
    float YW = 0.f;           /// Wind erosion, kg
    float TOT = 0.f;          /// total organic carbon in layer 1
    float YEW = 0.f;          /// fraction of soil erosion of total soil mass
    float X1 = 0.f, PRMT_21 = 0.f;
    float PRMT_44 = 0.f; /// ratio of soluble C concentration in runoff to percolate (0.1 - 1.0)
    float XX = 0.f, DK = 0.f, V = 0.f, X3 = 0.f;
    float CO = 0.f; /// the vertical concentration
    float CS = 0.f; /// the horizontal concentration
    float perc_clyr = 0.f, latc_clyr = 0.f;

    totOrgN_lyr0 = m_sol_LSN[i][0] + m_sol_LMN[i][0] + m_sol_HPN[i][0] + m_sol_HSN[i][0];
    wt1 = m_soilBD[i][0] * m_soilDepth[i][0] * 0.01f;
    er = m_enratio[i];
    if (er < .001) er =0.001f;
    conc = totOrgN_lyr0 * er / wt1;
    if(wt1<=0.f) conc=0.f;
    //m_surfRfSedOrgN[i] = 0.001f * conc * m_olWtrEroSed[i] * 0.001f / m_cellArea;
    m_surfRfSedOrgN[i] = 0.001f * conc * m_olWtrEroSed[i] * 0.001f / (m_area[i] * 0.0001f);
    m_surfRfSedOrgN[i] = Min(totOrgN_lyr0 *0.9f,m_surfRfSedOrgN[i]);
    m_surfRfSedOrgN[i] = Max(0.f,m_surfRfSedOrgN[i]);
    /// update soil nitrogen pools
    if (totOrgN_lyr0 > UTIL_ZERO) {
        float xx1 = 1.f - m_surfRfSedOrgN[i] / totOrgN_lyr0;
        xx1 = Max(xx1,0.1);
        xx1 = Min(xx1,1.f);
        m_sol_LSN[i][0] *= xx1;
        m_sol_LMN[i][0] *= xx1;
        m_sol_HPN[i][0] *= xx1;
        m_sol_HSN[i][0] *= xx1;
    }
    /// Calculate runoff and leached C&N from micro-biomass
    /// total organic carbon in layer 1
    TOT = m_sol_HPC[i][0] + m_sol_HSC[i][0] + m_sol_LMC[i][0] + m_sol_LSC[i][0];
    /// fraction of soil erosion of total soil mass
    //YEW = Min((m_olWtrEroSed[i] / m_cellArea + YW / m_cellArea) / m_soilMass[i][0], 0.9f);
    YEW = Min(m_enr_POC*(m_olWtrEroSed[i] / (m_area[i] * 0.0001f) + YW / (m_area[i] * 0.0001f)) / m_soilMass[i][0], 0.9f);
    if(m_soilMass[i][0]<=UTIL_ZERO) YEW = 0.f;
    YEW = Max(YEW,0.f);

    X1 = 1.f - YEW;
    YOC = YEW * TOT;
    m_sol_HSC[i][0] *= X1;
    m_sol_HPC[i][0] *= X1;
    m_sol_LS[i][0] *= X1;
    m_sol_LM[i][0] *= X1;
    m_sol_LSL[i][0] *= X1;
    m_sol_LSC[i][0] *= X1;
    m_sol_LMC[i][0] *= X1;
    m_sol_LSLC[i][0] *= X1;
    m_sol_LSLNC[i][0] = m_sol_LSC[i][0] - m_sol_LSLC[i][0];

    //ljj++ POC transport, only for first layer
    //m_soileroRPOC[i] = (m_sol_HPC[i][0] + m_sol_HSC[i][0]) * YEW;
    //m_soileroLPOC[i] = (m_sol_LMC[i][0] + m_sol_LSC[i][0]) * YEW;
    m_soileroRPOC[i] = (m_sol_HPC[i][0] + m_sol_HSC[i][0] + m_sol_LMC[i][0] + m_sol_LSC[i][0]) * YEW;
    //DIC processes in top soil layer  NCQYL.F90
    float DIC_sat = 0.01; // DIC saturation constant (kg/m3)
    float k_eva = 0.1; //DIC evasion rate, DIC to air
    float perco_DIC = 0.95; // DIC percolation coefficient
    float X2 = 0.f,sat_DIC = 0.f,sat_ly = 0.f,cw_DIC = 0.f;
    float peric_clyr = 0.f, latic_clyr = 0.f;

    m_soilWtrDIC[i][0] += m_sol_RSPC[i][0];
    X2 = m_soilWP[i][0] + m_soilWtrSto[i][0];
    sat_DIC = 0.1f * X2 * DIC_sat;   //DIC saturation (kg/ha)
    if (m_soilWtrDIC[i][0] > sat_DIC) m_soilWtrDIC[i][0] = m_soilWtrDIC[i][0] - (m_soilWtrDIC[i][0] - sat_DIC) * k_eva;
    //V = m_surfRf[i] + m_soilPerco[i][0] + m_subSurfRf[i][0];
    V = m_surfrunoff[i] + m_soilPerco[i][0] + m_subSurfRf[i][0];  //ljj++ consider the lag
    if (m_soilWtrDIC[i][0] > 0.f && V > 0) {
    	sat_ly =  (m_soilPor[i][0] * m_soilDepth[i][0]); //soilPor unit is m3/m3, while in swat is none
    	cw_DIC = Max(0.f, m_soilWtrDIC[i][0] * (1 - exp(-V / sat_ly)) / V);     //DIC concentration in mobile water (kg/ha/mm)
    	//m_soilSurfInOrgnCbn[i] = perco_DIC * cw_DIC * m_surfRf[i];			//loss by surface runoff
        m_soilSurfInOrgnCbn[i] = perco_DIC * cw_DIC * m_surfrunoff[i];			//loss by surface runoff
    	m_soilIfluInOrgnCbn[i][0] = perco_DIC * cw_DIC *  m_subSurfRf[i][0];   //loss by lateral flow
    	m_soilPercoInOrgnCbn[i][0] = cw_DIC * m_soilPerco[i][0];				//loss by percolation into underlying soil layer
    	//m_soilWtrDIC[i][0] = m_soilWtrDIC[i][0] - cw_DIC * (m_soilPerco[i][0] + perco_DIC * (m_surfRf[i] + m_subSurfRf[i][0]));
        m_soilWtrDIC[i][0] = m_soilWtrDIC[i][0] - cw_DIC * (m_soilPerco[i][0] + perco_DIC * (m_surfrunoff[i] + m_subSurfRf[i][0]));
    }else{
    	m_soilSurfInOrgnCbn[i] =0;
    	m_soilIfluInOrgnCbn[i][0] = 0;
    	m_soilPercoInOrgnCbn[i][0] = 0.;
    }
    if (m_soilWtrDIC[i][0] < UTIL_ZERO) m_soilWtrDIC[i][0] = 0.f;
    if (m_sol_BMC[i][0] > 0.01f) {
        ///KOC FOR CARBON LOSS IN WATER AND SEDIMENT(500._1500.) KD = KOC * C
        PRMT_21 = 1000.f;
        m_sol_WOC[i][0] = m_sol_LSC[i][0] + m_sol_LMC[i][0] + m_sol_HPC[i][0] + m_sol_HSC[i][0] + m_sol_BMC[i][0];
        //DK = 0.0001f * PRMT_21 * m_sol_WOC[i][0];
        DK = 0.0001f * m_kd_oc * m_sol_WOC[i][0];
        X1 = m_soilSat[i][0];
        //X1 = m_soilPor[i][0] * m_soilThk[i][0] - m_soilWP[i][0];
        if (X1 <= 0.f) X1 = 0.01f;
        XX = X1 + DK;
        //V = m_surfRf[i] + m_soilPerco[i][0] + m_subSurfRf[i][0];
        V = m_surfrunoff[i] + m_soilPerco[i][0] + m_subSurfRf[i][0];
        if (V >= UTIL_ZERO) {
            X3 = m_sol_BMC[i][0] * (1.f - exp(-V / XX)); /// loss of biomass C
            X3 = Max(X3,0.f);
            //PRMT_44 = 0.5;
            //CO = X3 / (m_soilPerco[i][0] + PRMT_44 * (m_surfRf[i] + m_subSurfRf[i][0]));
            //CO = X3 / (m_soilPerco[i][0] + m_perco_doc * (m_surfRf[i] + m_subSurfRf[i][0]));
            CO = X3 / (m_soilPerco[i][0] + m_perco_doc * (m_surfrunoff[i] + m_subSurfRf[i][0]));
            //CS = PRMT_44 * CO;
            CS = m_perco_doc * CO;
            VBC = CO * m_soilPerco[i][0];
            m_sol_BMC[i][0] -= X3;
            //QBC = CS * (m_surfRf[i] + m_subSurfRf[i][0]);
            QBC = CS * (m_surfrunoff[i] + m_subSurfRf[i][0]);
            /// Compute WBMC loss with sediment
            if (YEW > 0.f) {
                CS = DK * m_sol_BMC[i][0] / XX;
                YBC = YEW * CS;
            }
        }
    }
    m_sol_BMC[i][0] -= YBC;
    m_sol_BMC[i][0] = Max(m_sol_BMC[i][0],0.f);
    /// surfqc_d(j) = QBC*(surfq(j)/(surfq(j)+flat(1,j)+1.e-6))  is for print purpose, thus not implemented.
    // m_soilSurfCbn[i] = QBC * (m_surfRf[i] / (m_surfRf[i] + m_subSurfRf[i][0] + UTIL_ZERO)); 
    // m_soilIfluCbn[i][0] = QBC * (m_subSurfRf[i][0] / (m_surfRf[i] + m_subSurfRf[i][0] + UTIL_ZERO));
    m_soilSurfCbn[i] = QBC * (m_surfrunoff[i] / (m_surfrunoff[i] + m_subSurfRf[i][0] + UTIL_ZERO)); 
    m_soilIfluCbn[i][0] = QBC * (m_subSurfRf[i][0] / (m_surfrunoff[i] + m_subSurfRf[i][0] + UTIL_ZERO));
    m_soilPercoCbn[i][0] = VBC;
    m_sedLossCbn[i] = YOC + YBC;
    
    latc_clyr += m_soilIfluCbn[i][0];
    for (int k = 1; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        m_sol_WOC[i][k] = m_sol_LSC[i][k] + m_sol_LMC[i][k] + m_sol_HPC[i][k] + m_sol_HSC[i][k];
        float Y1 = m_sol_BMC[i][k] + VBC;
        VBC = 0.f;
        float VBC1 = 0.f;
        if (Y1 > 0) {
            V = m_soilPerco[i][k] + m_subSurfRf[i][k];
            if (V >= UTIL_ZERO) {
                //VBC = Y1 * (1.f - exp(-V / (m_soilSat[i][k] + 0.0001f * PRMT_21 * m_sol_WOC[i][k])));
                VBC = Y1 * (1.f - exp(-V / (m_soilSat[i][k] + 0.0001f * m_kd_oc * m_sol_WOC[i][k])));
                VBC1 = Max(VBC,0.f);
            }
        }
        VBC1 = Min(VBC1,m_sol_BMC[i][k]);
        m_soilIfluCbn[i][k] = VBC1 * (m_subSurfRf[i][k] / (m_subSurfRf[i][k] + m_soilPerco[i][k] + UTIL_ZERO));
        m_soilPercoCbn[i][k] = VBC1 - m_soilIfluCbn[i][k];
        m_sol_BMC[i][k] = Y1 - VBC1;
        /// calculate nitrate in percolate and lateral flow
        perc_clyr += m_soilPercoCbn[i][k];
        latc_clyr += m_soilIfluCbn[i][k];
        
        float DIC_ly = 0.f;
        DIC_ly = m_soilWtrDIC[i][k] + m_soilPercoInOrgnCbn[i][k - 1] + m_sol_RSPC[i][k]; 
        //ljj++ cw_DIC is always << DIC_ly in the previous version, soil wtrdic is continue increase
        //add the sat_DIC according to the surface layer
        X2 = m_soilWP[i][k] + m_soilWtrSto[i][k];
        sat_DIC = 0.1f * X2 * DIC_sat;   //DIC saturation (kg/ha)

        if (DIC_ly >= 0.f) {
            V = m_soilPerco[i][k] + m_subSurfRf[i][k];
            cw_DIC = 0.f;
            if (V > 0.f) cw_DIC = DIC_ly * (1.f - exp(-V / (m_soilPor[i][k] * m_soilDepth[i][k]))) / V;
            m_soilIfluInOrgnCbn[i][k] = cw_DIC * m_subSurfRf[i][k];
            m_soilPercoInOrgnCbn[i][k] = cw_DIC * m_soilPerco[i][k];
            m_soilWtrDIC[i][k] = DIC_ly - cw_DIC * V;
        if (m_soilWtrDIC[i][k] < 0.f) m_soilWtrDIC[i][k] = 0.f;
        
        }
        if(k >= CVT_INT(m_nSoilLyrs[i]) - 1){
            if (m_soilWtrDIC[i][k] - sat_DIC > 1.e-4f) {
                float ul_excess = (DIC_ly - sat_DIC) * k_eva;
                m_soilWtrDIC[i][k] = sat_DIC;
                for (int ly = CVT_INT(m_nSoilLyrs[i]) - 2; ly >= 1; ly--) {
                    m_soilWtrDIC[i][ly] += ul_excess;
                    if (m_soilWtrDIC[i][ly] > 0.1f * (m_soilWP[i][ly] + m_soilWtrSto[i][ly]) * DIC_sat) {
                        ul_excess = (m_soilWtrDIC[i][ly] - 0.1f * (m_soilWP[i][ly] + m_soilWtrSto[i][ly]) * DIC_sat) * k_eva;
                        m_soilWtrDIC[i][ly] =m_soilWtrDIC[i][ly] - ul_excess;
                    } else {
                        ul_excess = 0.f;
                        break;
                    }
                }
            }
        }
        //ljj++ consider routing
        int id_downstream = CVT_INT(m_flowOutIdxD8[i]);
        for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
            if (id_downstream >= 0) {
                if(m_landUse[id_downstream] != 18){
                    m_soilWtrDIC[id_downstream][k] += m_soilIfluInOrgnCbn[i][k]*m_area[i]/m_area[id_downstream];
                    m_sol_BMC[id_downstream][k] += m_soilIfluCbn[i][k]*m_area[i]/m_area[id_downstream];
                }else{
                    m_soilIfluCbn[id_downstream][k] += m_soilIfluCbn[i][k];
                    m_soilIfluInOrgnCbn[id_downstream][k] += m_soilIfluInOrgnCbn[i][k];
                }
            }
        }
    }
    m_soilIfluCbnPrfl[i] = latc_clyr;
    m_soilPercoCbnPrfl[i] = perc_clyr;
    m_soilPercoCbnLowest[i] = m_soilPercoCbn[i][CVT_INT(m_nSoilLyrs[i])-1];
    m_soilPercoDICLowest[i] = m_soilPercoInOrgnCbn[i][CVT_INT(m_nSoilLyrs[i])-1];
    }

void NutrientTransportSediment::OrgPAttachedtoSed(const int i) {
    //amount of phosphorus attached to sediment in soil (sol_attp)
    float sol_attp = 0.f;
    //fraction of active mineral/organic/stable mineral phosphorus in soil (sol_attp_o, sol_attp_a, sol_attp_s)
    float sol_attp_o = 0.f;
    float sol_attp_a = 0.f;
    float sol_attp_s = 0.f;
    //Calculate sediment
    sol_attp = m_soilHumOrgP[i][0] + m_soilFrshOrgP[i][0] + m_soilActvMinP[i][0] + m_soilStabMinP[i][0];
    if (m_soilManP != nullptr) {
        sol_attp += m_soilManP[i][0];
    }
    if (sol_attp > 1.e-3f) {
        sol_attp_o = (m_soilHumOrgP[i][0] + m_soilFrshOrgP[i][0]) / sol_attp;
        if (m_soilManP != nullptr) {
            sol_attp_o += m_soilManP[i][0] / sol_attp;
        }
        sol_attp_a = m_soilActvMinP[i][0] / sol_attp;
        sol_attp_s = m_soilStabMinP[i][0] / sol_attp;
    }
    sol_attp_o = Min(sol_attp_o,1.f);
    sol_attp_a = Min(sol_attp_a,1.f);
    sol_attp_s = Min(sol_attp_s,1.f);
    //conversion factor (mg/kg => kg/ha) (wt)
    float wt = m_soilBD[i][0] * m_soilThk[i][0] * 0.01f;
    //concentration of organic P in soil (concp)
    float concp = 0.f;
    concp = sol_attp * m_enratio[i] / wt; /// mg/kg
    if(wt<=0.f) concp=0.f;
    //total amount of P removed in sediment erosion (sedp)
    //float sedp = 1.e-6f * concp * m_olWtrEroSed[i] / m_cellArea; /// kg/ha
    float sedp = 1.e-6f * concp * m_olWtrEroSed[i] / (m_area[i]* 0.0001f); /// kg/ha
    m_surfRfSedOrgP[i] = sedp * sol_attp_o;
    m_surfRfSedAbsorbMinP[i] = sedp * sol_attp_a;
    m_surfRfSedSorbMinP[i] = sedp * sol_attp_s;

    //if(i==100)cout << "sedp: " << sedp<< ",sol_attp_o: "  << sol_attp_o << endl;
    //modify phosphorus pools

    //total amount of P in mineral sediment pools prior to sediment removal (psedd)		// Not used
    //float psedd = 0.f;
    //psedd = m_sol_actp[i][0] + m_sol_stap[i][0];

    //total amount of P in organic pools prior to sediment removal (porgg)
    float porgg = 0.f;
    porgg = m_soilHumOrgP[i][0] + m_soilFrshOrgP[i][0];
    if (porgg > 1.e-3f) {
        m_soilHumOrgP[i][0] = m_soilHumOrgP[i][0] - m_surfRfSedOrgP[i] * (m_soilHumOrgP[i][0] / porgg);
        m_soilFrshOrgP[i][0] = m_soilFrshOrgP[i][0] - m_surfRfSedOrgP[i] * (m_soilFrshOrgP[i][0] / porgg);
    }
    m_soilActvMinP[i][0] = m_soilActvMinP[i][0] - m_surfRfSedAbsorbMinP[i];
    m_soilStabMinP[i][0] = m_soilStabMinP[i][0] - m_surfRfSedSorbMinP[i];
    if (m_soilHumOrgP[i][0] < 0.f) {
        m_surfRfSedOrgP[i] = m_surfRfSedOrgP[i] + m_soilHumOrgP[i][0];
        m_soilHumOrgP[i][0] = 0.f;
    }
    if (m_soilFrshOrgP[i][0] < 0.f) {
        m_surfRfSedOrgP[i] = m_surfRfSedOrgP[i] + m_soilFrshOrgP[i][0];
        m_soilFrshOrgP[i][0] = 0.f;
    }
    if (m_soilActvMinP[i][0] < 0.f) {
        m_surfRfSedAbsorbMinP[i] = m_surfRfSedAbsorbMinP[i] + m_soilActvMinP[i][0];
        m_soilActvMinP[i][0] = 0.f;
    }
    if (m_soilStabMinP[i][0] < 0.f) {
        m_surfRfSedSorbMinP[i] = m_surfRfSedSorbMinP[i] + m_soilStabMinP[i][0];
        m_soilStabMinP[i][0] = 0.f;
    }
}

void NutrientTransportSediment::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_SEDORGN)) {
        *data = m_surfRfSedOrgN;
        *n = m_nCells;
    } else if (StringMatch(sk, VAR_SEDORGP)) {
        *data = m_surfRfSedOrgP;
        *n = m_nCells;
    } else if (StringMatch(sk, VAR_SEDMINPA)) {
        *data = m_surfRfSedAbsorbMinP;
        *n = m_nCells;
    } else if (StringMatch(sk, VAR_SEDMINPS)) {
        *data = m_surfRfSedSorbMinP;
        *n = m_nCells;
    } else if (StringMatch(sk, VAR_SEDORGN_TOCH)) {
        *data = m_surfRfSedOrgNToCh;
        *n = m_nSubbsns + 1;
    } else if (StringMatch(sk, VAR_SEDORGP_TOCH)) {
        *data = m_surfRfSedOrgPToCh;
        *n = m_nSubbsns + 1;
    } else if (StringMatch(sk, VAR_SEDMINPA_TOCH)) {
        *data = m_surfRfSedAbsorbMinPToCh;
        *n = m_nSubbsns + 1;
    } else if (StringMatch(sk, VAR_SEDMINPS_TOCH)) {
        *data = m_surfRfSedSorbMinPToCh;
        *n = m_nSubbsns + 1;
    }
        /// outputs of CENTURY C/N cycling model
    else if (StringMatch(sk, VAR_LATERAL_C)) {
        *data = m_soilIfluCbnPrfl;
        *n = m_nCells;
    } else if (StringMatch(sk, VAR_PERCO_C)) {
        *data = m_soilPercoCbnPrfl;
        *n = m_nCells;
    } else if (StringMatch(sk, VAR_SEDLOSS_C)) {
        *data = m_sedLossCbn;
        *n = m_nCells;
    } 
    //ljj++
    else if (StringMatch(sk, VAR_SURF_DOC)) {
		*data = m_soilSurfCbn;
		*n = m_nCells;
	}
    else if (StringMatch(sk, VAR_SURF_DIC)) {
		*data = m_soilSurfInOrgnCbn;
		*n = m_nCells;
	}
    else if (StringMatch(sk, VAR_ENR_LPOC)) {
		*data = m_soileroLPOC;
		*n = m_nCells;
	}
    else if (StringMatch(sk, VAR_ENR_RPOC)) {
		*data = m_soileroRPOC;
		*n = m_nCells;
	}
    else if (StringMatch(sk, VAR_PERC_LOWEST_DOC)) {
		*data = m_soilPercoCbnLowest;
		*n = m_nCells;
	}
    else if (StringMatch(sk, VAR_PERC_LOWEST_DIC)) {
		*data = m_soilPercoDICLowest;
		*n = m_nCells;
	}
    else if (StringMatch(sk, VAR_LPOCtoCH)) {
		*data = m_LPOCtoCH;
		*n = m_nSubbsns + 1;
	}
	else if (StringMatch(sk, VAR_RPOCtoCH)) {
		*data = m_RPOCtoCH;
		*n = m_nSubbsns + 1;
	}
    else if (StringMatch(sk, VAR_surfDICtoCH)) {
		*data = m_surfDICtoCH;
		*n = m_nSubbsns + 1;
	}
	else if (StringMatch(sk, VAR_latDICtoCH)) {
		*data = m_IfluDICtoCH;
		*n = m_nSubbsns + 1;
	}
    else if (StringMatch(sk, VAR_LDOCtoCH)) {
		*data = m_LDOCToCH;
		*n = m_nSubbsns + 1;
	}
    else if (StringMatch(sk, VAR_surfRDOCtoCH)) {
		*data = m_surfRDOCtoCH;
		*n = m_nSubbsns + 1;
	}
    else if (StringMatch(sk, VAR_latRDOCtoCH)) {
		*data = m_IfluRDOCtoCH;
		*n = m_nSubbsns + 1;
	}
    else {
        throw ModelException(MID_NUTRSED, "Get1DData", "Parameter " + sk + " does not exist");
    }
}

void NutrientTransportSediment::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
    InitialOutputs();
    string sk(key);
    *nrows = m_nCells;
    *ncols = m_maxSoilLyrs;
    if (StringMatch(sk, VAR_SOL_AORGN)) *data = m_soilActvOrgN;
    else if (StringMatch(sk, VAR_SOL_FORGN)) *data = m_soilFrshOrgN;
    else if (StringMatch(sk, VAR_SOL_SORGN)) *data = m_soilStabOrgN;
    else if (StringMatch(sk, VAR_SOL_HORGP)) *data = m_soilHumOrgP;
    else if (StringMatch(sk, VAR_SOL_FORGP)) *data = m_soilFrshOrgP;
    else if (StringMatch(sk, VAR_SOL_STAP)) *data = m_soilStabMinP;
    else if (StringMatch(sk, VAR_SOL_ACTP)) *data = m_soilActvMinP;
        /// outputs of CENTURY C/N cycling model
    else if (StringMatch(sk, VAR_SOL_LATERAL_C)) *data = m_soilIfluCbn;
    else if (StringMatch(sk, VAR_SOL_PERCO_C)) *data = m_soilPercoCbn;
    else {
        throw ModelException(MID_NUTRSED, "Get2DData", "Output " + sk + " does not exist.");
    }
}
