#include "Nutrient_Transformation.h"

#include "text.h"

#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <vector>
#include <string>

namespace {
inline bool IsFinitePositive(const float value) {
    return std::isfinite(value) && value > UTIL_ZERO;
}

inline float SafePositiveDivide(const float numerator,
                                const float denominator,
                                const float fallback = 0.f) {
    if (!std::isfinite(numerator) || !std::isfinite(denominator) ||
        denominator <= UTIL_ZERO) {
        return fallback;
    }
    const float result = numerator / denominator;
    return std::isfinite(result) ? result : fallback;
}
}  // namespace

Nutrient_Transformation::Nutrient_Transformation() :
    m_cellWth(-1.f), m_nCells(-1), m_cellAreaFr(NODATA_VALUE),
    m_nSoilLyrs(nullptr), m_maxSoilLyrs(-1), m_cbnModel(0), m_solP_model(0),
    m_phpApldDays(nullptr), m_phpDefDays(nullptr),
    m_tillSwitch(nullptr), m_tillDepth(nullptr), m_tillDays(nullptr), m_tillFactor(nullptr),
    m_minrlCoef(-1.f), m_orgNFrActN(-1.f), m_denitThres(-1.f), m_phpSorpIdxBsn(-1.f),
    m_phpSorpIdx(nullptr), m_psp_store(nullptr), m_ssp_store(nullptr), m_denitCoef(-1.f), m_landCover(nullptr),
    m_pltRsdDecCoef(nullptr), m_rsdCovSoil(nullptr), m_rsdInitSoil(nullptr), m_soilTemp(nullptr),
    m_soilBD(nullptr), m_soilMass(nullptr), m_soilCbn(nullptr),
    m_soilWtrSto(nullptr), m_soilFC(nullptr), m_soilDepth(nullptr),
    m_soilClay(nullptr), m_soilRock(nullptr), m_soilThk(nullptr),
    m_soilActvOrgN(nullptr), m_soilFrshOrgN(nullptr), m_soilFrshOrgP(nullptr),
    m_soilActvMinP(nullptr), m_soilStabMinP(nullptr), m_soilSat(nullptr), m_soilPor(nullptr),
    /// from other modules
    m_soilSand(nullptr), m_sol_WOC(nullptr),
    /// cell scale output
    m_sol_WON(nullptr), m_sol_BM(nullptr), m_sol_BMC(nullptr),
    m_sol_BMN(nullptr), m_sol_HP(nullptr), m_sol_HS(nullptr), m_sol_HSC(nullptr), m_sol_HSN(nullptr),
    /// watershed scale statistics
    m_sol_HPC(nullptr), m_sol_HPN(nullptr), m_sol_LM(nullptr), m_sol_LMC(nullptr), m_sol_LMN(nullptr),
    m_sol_LSC(nullptr), m_sol_LSN(nullptr), m_sol_LS(nullptr), m_sol_LSL(nullptr), m_sol_LSLC(nullptr),
    m_sol_LSLNC(nullptr), m_sol_RNMN(nullptr),
    /// tillage factor on SOM decomposition, used by CENTURY model
    m_sol_RSPC(nullptr), m_hmntl(nullptr), m_hmptl(nullptr), m_rmn2tl(nullptr),
    /// CENTURY model related variables
    m_rmptl(nullptr), m_rwntl(nullptr), m_wdntl(nullptr), m_rmp1tl(nullptr), m_roctl(nullptr),
    m_soilNO3(nullptr), m_soilStabOrgN(nullptr), m_soilHumOrgP(nullptr), m_soilRsd(nullptr), m_soilSolP(nullptr),
    m_soilNH4(nullptr), m_soilWP(nullptr), m_wshd_dnit(-1.f), m_wshd_hmn(-1.f), m_wshd_hmp(-1.f),
    m_wshd_rmn(-1.f), m_wshd_rmp(-1.f), m_wshd_rwn(-1.f), m_wshd_nitn(-1.f), m_wshd_voln(-1.f),
    m_wshd_pal(-1.f), m_wshd_pas(-1.f),m_phpSorpIdxBsn_temp(nullptr),
    m_conv_wt(nullptr),m_soil_carbon(nullptr),m_dem(nullptr),m_landUse(nullptr),
    m_soilTempprofile(nullptr),m_soilAWC(nullptr),m_surfrunoff(nullptr),m_subSurfRf(nullptr),m_soilPerco(nullptr),
    // m_soilpH(nullptr),MA(nullptr),MD(nullptr),MIC(nullptr),MAOM(nullptr),
    m_soilpH(nullptr),MIC(nullptr),MAOM(nullptr),
    m_forc_npp(nullptr),m_forc_litter(nullptr),POM(nullptr),LMWC(nullptr),
    MIC1(nullptr),MAOM1(nullptr),POM1(nullptr),LMWC1(nullptr),m_biomassDelta(nullptr),
    SOM(nullptr),soilCO2(nullptr),SOL_OUT(nullptr),DOC(nullptr),f_LM_leach(nullptr),
    m_poc_1st(nullptr),m_doc_1st(nullptr),m_maoc_1st(nullptr),m_mbc_1st(nullptr),m_leach(nullptr),
    m_bmdieoff(nullptr),m_biomass(nullptr),m_frRoot(nullptr),tmp_rtfr(nullptr),m_stoSoilRootD(nullptr),
    kaff_pl(nullptr),Vpl0(nullptr),kaff_ml(nullptr),Vml0(nullptr),kaff_lb(nullptr),Vlb0(nullptr),param_fpl(nullptr),
    param_p2(nullptr),kaff_des(nullptr),cue_t(nullptr),rate_bd(nullptr),rate_Kbd(nullptr),cue_ref(nullptr),param_pb(nullptr),
    param_pc(nullptr),r0(nullptr),Ma(nullptr),beta(nullptr),acue(nullptr),param_pi(nullptr),m_dormFlag(nullptr),
    m_soilSurfCbn(nullptr), m_soilPercoCbn(nullptr), m_soilIfluCbn(nullptr),m_olWtrEroSed(nullptr),m_area(nullptr),
    m_soileroPOC(nullptr),m_flowOutIdxD8(nullptr),m_soilPercoCbnLowest(nullptr),fDOC(nullptr){
}

Nutrient_Transformation::~Nutrient_Transformation() {
    if (m_hmntl != nullptr) Release1DArray(m_hmntl);
    if (m_hmptl != nullptr) Release1DArray(m_hmptl);
    if (m_rmn2tl != nullptr) Release1DArray(m_rmn2tl);
    if (m_rmptl != nullptr) Release1DArray(m_rmptl);
    if (m_rwntl != nullptr) Release1DArray(m_rwntl);
    if (m_wdntl != nullptr) Release1DArray(m_wdntl);
    if (m_rmp1tl != nullptr) Release1DArray(m_rmp1tl);
    if (m_roctl != nullptr) Release1DArray(m_roctl);
    if (m_phpSorpIdxBsn_temp != nullptr) Release1DArray(m_phpSorpIdxBsn_temp);
    if (m_phpApldDays != nullptr) Release1DArray(m_phpApldDays);
    if (m_phpDefDays != nullptr) Release1DArray(m_phpDefDays);
    if (m_soilMass != nullptr) Release2DArray(m_nCells, m_soilMass);
    /// release CENTURY related variables
    if (m_sol_WOC != nullptr) Release2DArray(m_nCells, m_sol_WOC);
    if (m_sol_WON != nullptr) Release2DArray(m_nCells, m_sol_WON);
    if (m_sol_BM != nullptr) Release2DArray(m_nCells, m_sol_BM);
    if (m_sol_BMC != nullptr) Release2DArray(m_nCells, m_sol_BMC);
    if (m_sol_BMN != nullptr) Release2DArray(m_nCells, m_sol_BMN);
    if (m_sol_HP != nullptr) Release2DArray(m_nCells, m_sol_HP);
    if (m_sol_HS != nullptr) Release2DArray(m_nCells, m_sol_HS);
    if (m_sol_HSC != nullptr) Release2DArray(m_nCells, m_sol_HSC);
    if (m_sol_HSN != nullptr) Release2DArray(m_nCells, m_sol_HSN);
    if (m_sol_HPC != nullptr) Release2DArray(m_nCells, m_sol_HPC);
    if (m_sol_HPN != nullptr) Release2DArray(m_nCells, m_sol_HPN);
    if (m_sol_LM != nullptr) Release2DArray(m_nCells, m_sol_LM);
    if (m_sol_LMC != nullptr) Release2DArray(m_nCells, m_sol_LMC);
    if (m_sol_LMN != nullptr) Release2DArray(m_nCells, m_sol_LMN);
    if (m_sol_LSC != nullptr) Release2DArray(m_nCells, m_sol_LSC);
    if (m_sol_LSN != nullptr) Release2DArray(m_nCells, m_sol_LSN);
    if (m_sol_LS != nullptr) Release2DArray(m_nCells, m_sol_LS);
    if (m_sol_LSL != nullptr) Release2DArray(m_nCells, m_sol_LSL);
    if (m_sol_LSLC != nullptr) Release2DArray(m_nCells, m_sol_LSLC);
    if (m_sol_LSLNC != nullptr) Release2DArray(m_nCells, m_sol_LSLNC);
    if (m_sol_RNMN != nullptr) Release2DArray(m_nCells, m_sol_RNMN);
    if (m_sol_RSPC != nullptr) Release2DArray(m_nCells, m_sol_RSPC);

    if (m_conv_wt != nullptr) Release2DArray(m_nCells, m_conv_wt);

    if (m_soil_carbon != nullptr) Release2DArray(m_nCells, m_soil_carbon);

    if (m_forc_litter != nullptr) Release2DArray(m_nCells,m_forc_litter);
    if (m_forc_npp != nullptr) Release2DArray(m_nCells,m_forc_npp);
    if (m_soilpH != nullptr) Release2DArray(m_nCells, m_soilpH);
    // if (MA != nullptr) Release2DArray(m_nCells, MA);
    // if (MD != nullptr) Release2DArray(m_nCells, MD);
    if (MIC != nullptr) Release2DArray(m_nCells, MIC);
    if (MAOM != nullptr) Release2DArray(m_nCells, MAOM);
    if (POM != nullptr) Release2DArray(m_nCells, POM);
    if (LMWC != nullptr) Release2DArray(m_nCells, LMWC);
    if (SOM != nullptr) Release2DArray(m_nCells, SOM);
    if (DOC != nullptr) Release2DArray(m_nCells, DOC);
    if (soilCO2 != nullptr) Release2DArray(m_nCells, soilCO2);
    if (SOL_OUT != nullptr) Release2DArray(m_nCells, SOL_OUT);
    if (tmp_rtfr != nullptr) Release2DArray(m_nCells, tmp_rtfr);
    if (MIC1 != nullptr) Release1DArray(MIC1);
    if (MAOM1 != nullptr) Release1DArray(MAOM1);
    if (POM1 != nullptr) Release1DArray(POM1);
    if (LMWC1 != nullptr) Release1DArray(LMWC1);
    if (f_LM_leach != nullptr) Release2DArray(m_nCells, f_LM_leach);
    if (fDOC != nullptr) Release2DArray(m_nCells, fDOC);

    if (m_soilSurfCbn != nullptr) Release1DArray(m_soilSurfCbn);
    if (m_soileroPOC != nullptr) Release1DArray(m_soileroPOC);
    if (m_soilPercoCbnLowest != nullptr) Release1DArray(m_soilPercoCbnLowest);
    if (m_soilIfluCbn != nullptr) Release2DArray(m_nCells, m_soilIfluCbn);
    if (m_soilPercoCbn != nullptr) Release2DArray(m_nCells, m_soilPercoCbn);
}

bool Nutrient_Transformation::CheckInputData() {
    CHECK_POSITIVE(MID_NUTR_TF, m_nCells);
    CHECK_POSITIVE(MID_NUTR_TF, m_cellAreaFr);
    CHECK_POSITIVE(MID_NUTR_TF, m_maxSoilLyrs);
    CHECK_POSITIVE(MID_NUTR_TF, m_cellWth);
    CHECK_POINTER(MID_NUTR_TF, m_nSoilLyrs);
    CHECK_POSITIVE(MID_NUTR_TF, m_minrlCoef);
    CHECK_POSITIVE(MID_NUTR_TF, m_denitCoef);
    CHECK_POSITIVE(MID_NUTR_TF, m_orgNFrActN);
    CHECK_POSITIVE(MID_NUTR_TF, m_denitThres);
    CHECK_POSITIVE(MID_NUTR_TF, m_phpSorpIdxBsn);
    CHECK_POINTER(MID_NUTR_TF, m_landCover);
    CHECK_POINTER(MID_NUTR_TF, m_soilClay);
    CHECK_POINTER(MID_NUTR_TF, m_soilDepth);
    CHECK_POINTER(MID_NUTR_TF, m_rsdInitSoil);
    CHECK_POINTER(MID_NUTR_TF, m_soilThk);
    CHECK_POINTER(MID_NUTR_TF, m_soilBD);
    CHECK_POINTER(MID_NUTR_TF, m_pltRsdDecCoef);
    CHECK_POINTER(MID_NUTR_TF, m_soilCbn);
    //CHECK_POINTER(MID_NUTR_TF, m_soilFC);
    CHECK_POINTER(MID_NUTR_TF, m_soilWP);
    CHECK_POINTER(MID_NUTR_TF, m_soilNO3);
    CHECK_POINTER(MID_NUTR_TF, m_soilNH4);
    CHECK_POINTER(MID_NUTR_TF, m_soilStabOrgN);
    CHECK_POINTER(MID_NUTR_TF, m_soilHumOrgP);
    CHECK_POINTER(MID_NUTR_TF, m_soilSolP);
    CHECK_POINTER(MID_NUTR_TF, m_soilWtrSto);
    CHECK_POINTER(MID_NUTR_TF, m_soilTemp);
    CHECK_POINTER(MID_NUTR_TF, m_soilSat);

    if (!(m_cbnModel == 0 || m_cbnModel == 1 || m_cbnModel == 2)) {
        throw ModelException(MID_NUTR_TF, "CheckInputData", "Carbon modeling method must be 0, 1, or 2.");
    }
    return true;
}

void Nutrient_Transformation::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, Tag_CellWidth)) {
        m_cellWth = value;
    } else if (StringMatch(sk, VAR_NACTFR)) {
        m_orgNFrActN = value;
    } else if (StringMatch(sk, VAR_SDNCO)) {
        m_denitThres = value;
    } else if (StringMatch(sk, VAR_CMN)) {
        m_minrlCoef = value;
    } else if (StringMatch(sk, VAR_CDN)) {
        m_denitCoef = value;
    } else if (StringMatch(sk, VAR_PSP)) {
        m_phpSorpIdxBsn = value;
    } else if (StringMatch(sk, VAR_CSWAT)) {
        m_cbnModel = CVT_INT(value);
    // } else if (StringMatch(sk, "param_leach")) {
    //     m_leach = value;
    } else {
        throw ModelException(MID_NUTR_TF, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void Nutrient_Transformation::Set1DData(const char* key, const int n, float* data) {
    CheckInputSize(MID_NUTR_TF, key, n, m_nCells);
    string sk(key);
    if (StringMatch(sk, VAR_LANDCOVER)) {
        m_landCover = data;
    } else if (StringMatch(sk, VAR_PL_RSDCO)) {
        m_pltRsdDecCoef = data;
    } else if (StringMatch(sk, VAR_SOL_RSDIN)) {
        m_rsdInitSoil = data;
    } else if (StringMatch(sk, VAR_SOL_COV)) {
        m_rsdCovSoil = data;
    } else if (StringMatch(sk, VAR_SOILLAYERS)) {
        m_nSoilLyrs = data;
    } else if (StringMatch(sk, VAR_SOTE)) {
        m_soilTemp = data;
    }else if (StringMatch(sk, VAR_DEM)) m_dem = data;
        // Currently not used, initialized in this module
        //else if (StringMatch(sk, VAR_A_DAYS)) {
        //    m_phpApldDays = data;
        //}
        //else if (StringMatch(sk, VAR_B_DAYS)) {
        //    m_phpDefDays = data;
        //}
        /// tillage related variables of CENTURY model
    else if (StringMatch(sk, VAR_TILLAGE_DAYS)) {
        m_tillDays = data;
    } else if (StringMatch(sk, VAR_TILLAGE_DEPTH)) {
        m_tillDepth = data;
    } else if (StringMatch(sk, VAR_TILLAGE_FACTOR)) {
        m_tillFactor = data;
    } else if (StringMatch(sk, VAR_TILLAGE_SWITCH)) {
        m_tillSwitch = data;
    } 
    else if (StringMatch(sk, VAR_LANDUSE)) m_landUse = data;
    else if (StringMatch(sk, VAR_BM_DIEOFF)) m_bmdieoff = data;
    else if (StringMatch(sk, "POC")) m_poc_1st = data;
    else if (StringMatch(sk, "DOC")) m_doc_1st = data;
    else if (StringMatch(sk, "MAOC")) m_maoc_1st = data;
    else if (StringMatch(sk, "MBC")) m_mbc_1st = data;
    else if (StringMatch(sk, VAR_BIOMASS)) m_biomass = data;
    else if (StringMatch(sk, "BIOMASS_DELTA")) m_biomassDelta = data;
    else if (StringMatch(sk, VAR_FR_ROOT)) m_frRoot = data;
    else if (StringMatch(sk, VAR_LAST_SOILRD)) m_stoSoilRootD = data;
    else if (StringMatch(sk, VAR_SURU)) m_surfrunoff = data;
    else if (StringMatch(sk, VAR_AHRU)) m_area = data;  
    else if (StringMatch(sk, Tag_FLOWOUT_INDEX_D8)) m_flowOutIdxD8 = data;
    else if (StringMatch(sk, VAR_DORMI)) m_dormFlag = data;
    else if (StringMatch(sk, "KAFF_PL")) {
        kaff_pl = data;
    } else if (StringMatch(sk, "VPL0")) {
        Vpl0 = data;
    } else if (StringMatch(sk, "KAFF_ML")) {
        kaff_ml = data;
    }else if (StringMatch(sk, "VML0")) {
        Vml0 = data;
    }else if (StringMatch(sk, "KAFF_LB")) {
        kaff_lb = data;
    }else if (StringMatch(sk, "VLB0")) {
        Vlb0 = data;
    }else if (StringMatch(sk, "PARAM_FPL")) {
        param_fpl = data;
    }else if (StringMatch(sk, "PARAM_P2")) {
        param_p2 = data;
    }else if (StringMatch(sk, "KAFF_DES")) {
        kaff_des = data;
    }else if (StringMatch(sk, "CUE_T")) {
        cue_t = data;
    }else if (StringMatch(sk, "RATE_BD")) {
        rate_bd = data;
    }else if (StringMatch(sk, "RATE_KBD")) {
        rate_Kbd = data;
    }else if (StringMatch(sk, "cue_ref")) {
        cue_ref = data;
    }else if (StringMatch(sk, "param_pb")) {
        param_pb = data;
    }else if (StringMatch(sk, "param_pc")) {
        param_pc = data;
    }else if (StringMatch(sk, "r0")) {
        r0 = data;
    }else if (StringMatch(sk, "Ma")) {
        Ma = data;
    }else if (StringMatch(sk, "beta")) {
        beta = data;
    }else if (StringMatch(sk, "acue")) {
        acue = data;
    }else if (StringMatch(sk, "param_pi")) {
        param_pi = data;
    }else if (StringMatch(sk, "param_leach")) {
        m_leach = data;
    }else if (StringMatch(sk, VAR_SOER)) {
        m_olWtrEroSed = data;
    }
    else {
        throw ModelException(MID_NUTR_TF, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void Nutrient_Transformation::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    CheckInputSize2D(MID_NUTR_TF, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
    string sk(key);
    if (StringMatch(sk, VAR_SOL_CBN)) {
        m_soilCbn = data;
    } else if (StringMatch(sk, VAR_SOL_BD)) {
        m_soilBD = data;
    } else if (StringMatch(sk, VAR_CLAY)) {
        m_soilClay = data;
    } else if (StringMatch(sk, VAR_ROCK)) {
        m_soilRock = data;
    } else if (StringMatch(sk, VAR_SOL_ST)) {
        m_soilWtrSto = data;
    } else if (StringMatch(sk, VAR_SOL_AWC)) {
        //m_soilFC = data;
        m_soilAWC = data;
    } else if (StringMatch(sk, VAR_SOL_NO3)) {
        m_soilNO3 = data;
    } else if (StringMatch(sk, VAR_SOL_NH4)) {
        m_soilNH4 = data;
    } else if (StringMatch(sk, VAR_SOL_SORGN)) {
        m_soilStabOrgN = data;
    } else if (StringMatch(sk, VAR_SOL_HORGP)) {
        m_soilHumOrgP = data;
    } else if (StringMatch(sk, VAR_SOL_SOLP)) {
        m_soilSolP = data;
    } else if (StringMatch(sk, VAR_SOL_WPMM)) {
        m_soilWP = data;
    } else if (StringMatch(sk, VAR_SOILDEPTH)) {
        m_soilDepth = data;
    } else if (StringMatch(sk, VAR_SOILTHICK)) {
        // Soil-layer thickness, mm.
        m_soilThk = data;
    } else if (StringMatch(sk, VAR_SOL_RSD)) {
        m_soilRsd = data;
    } else if (StringMatch(sk, VAR_SOL_UL)) {
        m_soilSat = data;
    } else if (StringMatch(sk, VAR_POROST)) {
        m_soilPor = data;
    } else if (StringMatch(sk, VAR_SAND)) {
        m_soilSand = data;
    } 
    else if (StringMatch(sk, VAR_SOILT)) {
        m_soilTempprofile = data;
    }
    else if (StringMatch(sk, VAR_PERCO)) m_soilPerco = data;
    else if (StringMatch(sk, VAR_SSRU)) m_subSurfRf = data;
    else {
        throw ModelException(MID_NUTR_TF, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void Nutrient_Transformation::InitialOutputs() {
    CHECK_POSITIVE(MID_NUTR_TF, m_nCells);
    if (m_cellAreaFr < 0.f) m_cellAreaFr = 1.f / m_nCells;
    CHECK_POSITIVE(MID_NUTR_TF, m_maxSoilLyrs);
    //ljj++
    if (MAOM1 == nullptr) Initialize1DArray(m_nCells, MAOM1, 0.f);
    if (MIC1 == nullptr) Initialize1DArray(m_nCells, MIC1, 0.f);
    if (POM1 == nullptr) Initialize1DArray(m_nCells, POM1, 0.f);
    if (LMWC1 == nullptr) Initialize1DArray(m_nCells, LMWC1, 0.f);
    if (fDOC == nullptr) Initialize2DArray(m_nCells,m_maxSoilLyrs, fDOC, 0.f);

    if (m_forc_litter == nullptr) Initialize2DArray(m_nCells,m_maxSoilLyrs, m_forc_litter, 0.f);
    if (DOC == nullptr) Initialize2DArray(m_nCells,m_maxSoilLyrs, DOC, 0.f);
    if (m_forc_npp == nullptr) Initialize2DArray(m_nCells,m_maxSoilLyrs, m_forc_npp, 0.f);
    if (m_soilpH == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilpH, 5.9667f);
    if (SOM == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, SOM, 0.f);
    if (soilCO2 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, soilCO2, 0.f);
    if (SOL_OUT == nullptr) Initialize2DArray(m_nCells, 6, SOL_OUT, 0.f);
    if (tmp_rtfr == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, tmp_rtfr, 0.f);
    if (f_LM_leach == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, f_LM_leach, 0.f);

    if (m_soilSurfCbn == nullptr) Initialize1DArray(m_nCells, m_soilSurfCbn, 0.f);
    if (m_soileroPOC == nullptr) Initialize1DArray(m_nCells, m_soileroPOC, 0.f);
    if (m_soilPercoCbnLowest == nullptr) Initialize1DArray(m_nCells, m_soilPercoCbnLowest, 0.f);
    if (m_soilIfluCbn == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilIfluCbn, 0.f);
    if (m_soilPercoCbn == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilPercoCbn, 0.f);
    if (MIC == nullptr) {
        Initialize2DArray(m_nCells, m_maxSoilLyrs, MIC, 0.f);
        // Initialize2DArray(m_nCells, m_maxSoilLyrs, MA, 0.f);
        // Initialize2DArray(m_nCells, m_maxSoilLyrs, MD, 0.f);
        float r0 = 0.8270435;
#pragma omp parallel for
		for (int i = 0; i < m_nCells; i++) {
			for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
				float mbcConcentration;  // mg C/kg soil
				if (m_soilDepth[i][k] <= 300.f) {
					mbcConcentration = m_mbc_1st[i];
				}
				else {
					mbcConcentration = m_mbc_1st[i] * exp(-(m_soilDepth[i][k] - 300.f) / 300.f);
				}

				// mg C/kg * Mg soil/m3 * mm / 1000 = g C/m2.
				MIC[i][k] = mbcConcentration / 1000.f * m_soilBD[i][k] * m_soilThk[i][k];  // g C/m²
			}

		}
	}
    if (MAOM == nullptr) {
        Initialize2DArray(m_nCells, m_maxSoilLyrs, MAOM, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, POM, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, LMWC, 0.f);
#pragma omp parallel for
		for (int i = 0; i < m_nCells; i++) {
			for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
				if (m_soilDepth[i][k] > 1000.f) {
                    // Protect against a zero first-layer thickness.
                    const float thicknessRatio = SafePositiveDivide(
                        m_soilThk[i][k], m_soilThk[i][0], 0.f);
                    const float depthDecay = exp(-m_soilDepth[i][k] / 1000.f);
					POM[i][k] = POM[i][0] * depthDecay * thicknessRatio;
					MAOM[i][k] = MAOM[i][0] * depthDecay * thicknessRatio;
				}
				else {
					// m_poc_1st and m_maoc_1st: mg C/kg soil.
					// m_soilBD: Mg/m3; m_soilThk: mm.
					// concentration / 1000 * BD * thickness = g C/m2.
					POM[i][k] = m_poc_1st[i] / 1000.f
					            * m_soilBD[i][k]
					            * m_soilThk[i][k];
					MAOM[i][k] = m_maoc_1st[i] / 1000.f
					             * m_soilBD[i][k]
					             * m_soilThk[i][k];
				}
				float docConcentration;  // mg C/kg soil
				if (m_soilDepth[i][k] <= 300.f) {
					docConcentration = m_doc_1st[i];
				}
				else {
					docConcentration = m_doc_1st[i] * exp(-(m_soilDepth[i][k] - 300.f) / 300.f);
				}

				LMWC[i][k] = docConcentration / 1000.f * m_soilBD[i][k] * m_soilThk[i][k];  // g C/m²
				DOC[i][k] = docConcentration;  // mg C/kg soil
			}
		}
	}

    if (m_hmntl == nullptr) Initialize1DArray(m_nCells, m_hmntl, 0.f);
    if (m_hmptl == nullptr) Initialize1DArray(m_nCells, m_hmptl, 0.f);
    if (m_rmn2tl == nullptr) Initialize1DArray(m_nCells, m_rmn2tl, 0.f);
    if (m_rmptl == nullptr) Initialize1DArray(m_nCells, m_rmptl, 0.f);
    if (m_rwntl == nullptr) Initialize1DArray(m_nCells, m_rwntl, 0.f);
    if (m_wdntl == nullptr) Initialize1DArray(m_nCells, m_wdntl, 0.f);
    if (m_rmp1tl == nullptr) Initialize1DArray(m_nCells, m_rmp1tl, 0.f);
    if (m_roctl == nullptr) Initialize1DArray(m_nCells, m_roctl, 0.f);
    if (m_phpApldDays == nullptr) Initialize1DArray(m_nCells, m_phpApldDays, 0.f);
    if (m_phpDefDays == nullptr) Initialize1DArray(m_nCells, m_phpDefDays, 0.f);
    if (m_rsdCovSoil == nullptr || m_soilRsd == nullptr) {
        Initialize1DArray(m_nCells, m_rsdCovSoil, m_rsdInitSoil);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilRsd, 0.f);
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            m_soilRsd[i][0] = m_rsdCovSoil[i];
        }
    }
    /// initialize m_conv_wt
    if (m_conv_wt == nullptr) {
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_conv_wt, 0.f);
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                float wt1 = 0.f;
                float conv_wt = 0.f;
                // BD (Mg/m3) * thickness (mm) * 10000 = kg soil/ha.
                // Keep the original two-step expression; numerically it is BD * thickness * 10000.
                wt1 = m_soilBD[i][k] * m_soilThk[i][k] * 0.01f;
                conv_wt = 1.e6f * wt1;
                m_conv_wt[i][k] = conv_wt;
            }
        }
    }
    /// initialize m_soilMass
    if (m_soilMass == nullptr) {
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilMass, 0.f);
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                // m_soilThk is mm and m_soilBD is Mg/m3; result is kg soil/ha.
                m_soilMass[i][k] = 10000.f * m_soilThk[i][k] * m_soilBD[i][k] * (1.f - m_soilRock[i][k] * 0.01f);
            }
        }
    }
    // initial input soil chemical in first run

    /// TODO, these variables should be reconsidered carefully according to SWAT (pminrl2.f)! lj
    if (m_phpSorpIdxBsn <= 0.f) m_phpSorpIdxBsn = 0.4f;
    if (nullptr == m_phpSorpIdx) Initialize1DArray(m_nCells, m_phpSorpIdx, m_phpSorpIdxBsn);
    if (nullptr == m_phpSorpIdxBsn_temp) Initialize1DArray(m_nCells, m_phpSorpIdxBsn_temp, m_phpSorpIdxBsn);
    if (nullptr == m_psp_store) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_psp_store, 0.f);
    if (nullptr == m_ssp_store) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_ssp_store, 0.f);

    if (m_soilNO3 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilNO3, 0.f);
    if (m_soilFrshOrgN == nullptr || m_soilFrshOrgP == nullptr || m_soilActvOrgN == nullptr ||
        m_soilActvMinP == nullptr || m_soilStabMinP == nullptr) {
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilFrshOrgN, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilFrshOrgP, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilActvOrgN, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilActvMinP, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilStabMinP, 0.f);

#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            // fresh organic P / N
            m_soilFrshOrgP[i][0] = m_rsdCovSoil[i] * .0010f;
            m_soilFrshOrgN[i][0] = m_rsdCovSoil[i] * .0055f;
            for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                float wt1 = m_conv_wt[i][k] * 1.e-6f;
                /// if m_sol_no3 is not provided, { initialize it.
                if (m_soilNO3[i][k] <= 0.f) {
                    m_soilNO3[i][k] = 0.f;
                    float zdst = 0.f;
                    zdst = exp(-m_soilDepth[i][k] * 0.001f);
                    m_soilNO3[i][k] = 10.f * zdst * 0.7f;
                    m_soilNO3[i][k] *= wt1; // mg/kg => kg/ha
                }
                /// if m_sol_orgn is not provided, { initialize it.
                if (m_soilStabOrgN[i][k] <= 0.f) {
                    // CN ratio changed back to 14
                    m_soilStabOrgN[i][k] = 10000.f * (m_soilCbn[i][k] * 0.07142857f) * wt1; // 1 / 14 = 0.07142857
                }
                // assume C:N ratio of 10:1
                // nitrogen active pool fraction (nactfr)
                float nactfr = .02f;
                m_soilActvOrgN[i][k] = m_soilStabOrgN[i][k] * nactfr;
                m_soilStabOrgN[i][k] *= 1.f - nactfr;

                // currently not used
                //sumorgn = sumorgn + m_sol_aorgn[i][k] + m_sol_orgn[i][k] + m_sol_fon[i][k];

                if (m_soilHumOrgP[i][k] <= 0.f) {
                    // assume N:P ratio of 8:1
                    m_soilHumOrgP[i][k] = 0.125f * m_soilStabOrgN[i][k];
                }

                if (m_soilSolP[i][k] <= 0.f) {
                    // assume initial concentration of 5 mg/kg
                    m_soilSolP[i][k] = 5.f * wt1;
                }

                float solp = 0.f;
                float actp = 0.f;
                if (m_solP_model == 0) {
                    // Set active pool based on dynamic PSP MJW
                    // Allow Dynamic PSP Ratio
                    if (m_conv_wt[i][k] != 0) solp = m_soilSolP[i][k] / m_conv_wt[i][k] * 1000000.f;
                    if (m_soilClay[i][k] > 0.f) {
                        m_phpSorpIdx[i] = -0.045f * log(m_soilClay[i][k]) + 0.001f * solp;
                        m_phpSorpIdx[i] = m_phpSorpIdx[i] - 0.035f * m_soilCbn[i][k] + 0.43f;
                    }
                    //else { // although SWAT has this code, in my view, this should be commented.
                    //    m_phpSorpIdx[i] = 0.4f;
                    //}
                    // Limit PSP range
                    if (m_phpSorpIdx[i] < .1f) {
                        m_phpSorpIdx[i] = 0.1f;
                    } else if (m_phpSorpIdx[i] > 0.7f) {
                        m_phpSorpIdx[i] = 0.7f;
                    }
                    /// Calculate smoothed PSP average
                    if (m_psp_store[i][k] > 0.f) {
                        m_phpSorpIdx[i] = (m_psp_store[i][k] * 29.f + m_phpSorpIdx[i]) * 0.033333333f;
                    }
                    // Store PSP for tomorrow's smoothing calculation
                    m_psp_store[i][k] = m_phpSorpIdx[i];
                }

                m_soilActvMinP[i][k] = m_soilSolP[i][k] * (1.f - m_phpSorpIdx[i]) / m_phpSorpIdx[i];

                if (m_solP_model == 0) {
                    // Set Stable pool based on dynamic coefficient, From White et al 2009
                    // convert to concentration for ssp calculation
                    if (m_conv_wt[i][k] != 0) actp = m_soilActvMinP[i][k] / m_conv_wt[i][k] * 1000000.f;
                    if (m_conv_wt[i][k] != 0) solp = m_soilSolP[i][k] / m_conv_wt[i][k] * 1000000.f;
                    // estimate Total Mineral P in this soil based on data from sharpley 2004
                    float ssp = 25.044f * pow(actp + solp, -0.3833f);
                    // limit SSP Range
                    if (ssp > 7.f) ssp = 7.f;
                    if (ssp < 1.f) ssp = 1.f;
                    // define stableP
                    m_soilStabMinP[i][k] = ssp * (m_soilActvMinP[i][k] + m_soilSolP[i][k]);
                } else {
                    // The original code
                    m_soilStabMinP[i][k] = 4.f * m_soilActvMinP[i][k];
                }
                //m_sol_hum[i][k] = m_sol_cbn[i][k] * wt1 * 17200.f;
                //summinp = summinp + m_sol_solp[i][k] + m_sol_actp[i][k] + m_sol_stap[i][k];
                //sumorgp = sumorgp + m_sol_orgp[i][k] + m_sol_fop[i][k];
            }
        }
    }
    /**** initialization of CENTURY model related variables *****/
    /****                soil_chem.f of SWAT                *****/
    if (m_cbnModel == 2) {
        /// definition of temporary parameters
        float FBM = 0.f, FHP = 0.f;
        float x1 = 0.f, RTO = 0.f; //, FHS = 0.f, sol_min_n = 0.f;
        if (m_sol_WOC == nullptr) {
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_WOC, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_WON, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_BM, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_BMC, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_BMN, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_HP, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_HS, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_HSC, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_HSN, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_HPC, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_HPN, 0.f);

            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LM, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LMC, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LMN, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LSC, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LSN, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LS, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LSL, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LSLC, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_LSLNC, 0.f);

            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_RNMN, 0.f);
            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_sol_RSPC, 0.f);

            Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soil_carbon, 0.f);

#pragma omp parallel for
            for (int i = 0; i < m_nCells; i++) {
                for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                    /// mineral nitrogen, kg/ha
                    //sol_min_n = m_sol_no3[i][k] + m_sol_nh4[i][k];
                    m_sol_WOC[i][k] = m_soilMass[i][k] * m_soilCbn[i][k] * 0.01f;
                    if(m_sol_WOC[i][k]<= UTIL_ZERO) continue;
                    m_sol_WON[i][k] = m_soilActvOrgN[i][k] + m_soilStabOrgN[i][k];
                    /// fraction of Mirobial biomass, humus passive C pools
                    if (FBM < 1.e-10f) FBM = 0.04f;
                    if (FHP < 1.e-10f) FHP = 0.6999999999996266f; // FHP = 0.7f - 0.4f * exp(-0.277f * 100.f);
                    //FHS = 1.f - FBM - FHP; // not used in SWAT
                    m_sol_BM[i][k] = FBM * m_sol_WOC[i][k];
                    m_sol_BMC[i][k] = m_sol_BM[i][k];
                    RTO = m_sol_WON[i][k] / m_sol_WOC[i][k];
                    m_sol_BMN[i][k] = RTO * m_sol_BMC[i][k];
                    m_sol_HP[i][k] = FHP * (m_sol_WOC[i][k] - m_sol_BM[i][k]);
                    m_sol_HS[i][k] = m_sol_WOC[i][k] - m_sol_BM[i][k] - m_sol_HP[i][k];
                    m_sol_HSC[i][k] = m_sol_HS[i][k];
                    m_sol_HSN[i][k] = RTO * m_sol_HSC[i][k];
                    m_sol_HPC[i][k] = m_sol_HP[i][k];        /// same as sol_aorgn
                    m_sol_HPN[i][k] = RTO * m_sol_HPC[i][k];  // same as sol_orgn
                    x1 = m_soilRsd[i][k] * 0.001f;
                    m_sol_LM[i][k] = 500.f * x1;
                    m_sol_LS[i][k] = m_sol_LM[i][k];
                    m_sol_LSL[i][k] = 0.8f * m_sol_LS[i][k];
                    m_sol_LMC[i][k] = 0.42f * m_sol_LM[i][k];

                    m_sol_LMN[i][k] = 0.1f * m_sol_LMC[i][k];
                    m_sol_LSC[i][k] = 0.42f * m_sol_LS[i][k];
                    m_sol_LSLC[i][k] = 0.8f * m_sol_LSC[i][k];
                    m_sol_LSLNC[i][k] = 0.2f * m_sol_LSC[i][k];
                    m_sol_LSN[i][k] = m_sol_LSC[i][k] * 0.006666666666666667f; // * 1.f / 150.f;

                    m_sol_WOC[i][k] += m_sol_LSC[i][k] + m_sol_LMC[i][k];
                    m_sol_WON[i][k] += m_sol_LSN[i][k] + m_sol_LMN[i][k];

                    m_soilStabOrgN[i][k] = m_sol_HPN[i][k];
                    m_soilActvOrgN[i][k] = m_sol_HSN[i][k];
                    m_soilFrshOrgN[i][k] = m_sol_LMN[i][k] + m_sol_LSN[i][k];
                }
            }
        }
    }
    // allocate the output variables
    if (!FloatEqual(m_wshd_dnit, 0.f)) {
        m_wshd_dnit = 0.f;
        m_wshd_hmn = 0.f;
        m_wshd_hmp = 0.f;
        m_wshd_rmn = 0.f;
        m_wshd_rmp = 0.f;
        m_wshd_rwn = 0.f;
        m_wshd_nitn = 0.f;
        m_wshd_voln = 0.f;
        m_wshd_pal = 0.f;
        m_wshd_pas = 0.f;
    }
}

int Nutrient_Transformation::Execute() {
    CheckInputData();
    InitialOutputs();

    // Phase 1: calculate the local carbon transformations and DOC export fluxes.
    // Do not write to downstream cells in this parallel section, otherwise several
    // upstream cells may modify the same downstream LMWC value simultaneously.
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        // Execute() is called once per model time step (daily in the current setup).
        // Clear the previous day's values so soilCO2 stores the current daily flux,
        // rather than the cumulative flux since the beginning of the simulation.
        const int nSoilLyrs = CVT_INT(m_nSoilLyrs[i]);
        for (int k = 0; k < nSoilLyrs; ++k) {
            soilCO2[i][k] = 0.f;
        }

        if (m_landUse[i] == LANDUSE_ID_WATR || m_landUse[i] == LANDUSE_ID_GLC) continue;

        // Compute nitrogen and phosphorus mineralization.
        if (m_cbnModel == 0) {
            MineralizationStaticCarbonMethod(i);
        } else if (m_cbnModel == 1) {
            MineralizationCfarmOneCarbonModel(i);
        } else if (m_cbnModel == 2) {
            MineralizationCenturyModel(i);
        }

        for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
            tmp_rtfr[i][j] = 0.f;
        }
        RootFraction(i, tmp_rtfr[i]);

        /*ModifiedMillennial(i);*/

        // Calculate daily mineralization (NH3 to NO3) and volatilization of NH3.
        Volatilization(i);
        // Calculate P flux between the labile, active mineral and stable mineral P pools.
        CalculatePflux(i);
    }

	// ============================================================================
	// SAVE: Daily CENTURY heterotrophic respiration for ALL cells
	// Period: 2009-05-19 ~ 2015-12-31
	// One row = one date + one cell
	// Each soil layer is stored in a separate column.
	// ============================================================================
	{
		const bool DEBUG_WRITE_RH_CSV = false;

		if (DEBUG_WRITE_RH_CSV && m_cbnModel == 2) {

			// ------------------------------------------------------------
			// Convert model time to calendar date
			// ------------------------------------------------------------
			const std::time_t currentDate =
				static_cast<std::time_t>(m_date);

			std::tm dateInfo = {};

#ifdef _WIN32
			localtime_s(&dateInfo, &currentDate);
#else
			localtime_r(&currentDate, &dateInfo);
#endif

			const int year = dateInfo.tm_year + 1900;
			const int month = dateInfo.tm_mon + 1;
			const int day = dateInfo.tm_mday;

			// YYYYMMDD integer
			const int dateKey =
				year * 10000 +
				month * 100 +
				day;

			// ------------------------------------------------------------
			// Only save:
			// 2009-05-19 ~ 2015-12-31
			// ------------------------------------------------------------
			const int START_DATE = 20090519;
			const int END_DATE = 20151231;

			if (dateKey >= START_DATE &&
				dateKey <= END_DATE) {

				static std::ofstream rhFile;
				static bool rhFileInitialized = false;
				static int outputDayCounter = 0;

				// --------------------------------------------------------
				// Initialize CSV only once
				// --------------------------------------------------------
				if (!rhFileInitialized) {

#ifdef _WIN32
					const char* outputPath = "D:/SEIMS_C/data/CW_2/CENTURY_daily_Rh_allcells_2009_2015-2.csv";
#else
					const char* outputPath = "/data/user/lijing/software/SEIMS_C/data/CW_2/CENTURY_daily_Rh_allcells_2009_2015-2.csv";
#endif

					rhFile.open(
						outputPath,
						std::ios::out | std::ios::trunc
					);

					if (!rhFile.is_open()) {

						std::cerr
							<< "[RH_CSV_ERROR] Cannot open: "
							<< outputPath
							<< '\n';

				}
					else {

						rhFile
							<< "DATE,"
							<< "CELL,"
							<< "RSPC_L1_kgCha_d,"
							<< "RSPC_L2_kgCha_d,"
							<< "RSPC_L3_kgCha_d,"
							<< "RSPC_L4_kgCha_d,"
							<< "RSPC_L5_kgCha_d,"
							<< "RSPC_L6_kgCha_d,"
							<< "RSPC_L7_kgCha_d,"
							<< "RSPC_PROFILE_kgCha_d,"
							<< "Rh_PROFILE_gCm2d"
							<< '\n';
					}

					rhFileInitialized = true;
			}

				// --------------------------------------------------------
				// Write current date
				// --------------------------------------------------------
				if (rhFile.is_open()) {

					char dateText[16] = {};

					std::strftime(
						dateText,
						sizeof(dateText),
						"%Y-%m-%d",
						&dateInfo
					);

					rhFile
						<< std::fixed
						<< std::setprecision(8);

					// ====================================================
					// ALL cells
					// ====================================================
					for (int i = 0; i < m_nCells; ++i) {

						const int nLayers =
							CVT_INT(m_nSoilLyrs[i]);

						double rspcProfile = 0.0;

						// ------------------------------------------------
						// Sum whole-profile RSPC
						// ------------------------------------------------
						for (int k = 0; k < nLayers; ++k) {

							if (std::isfinite(m_sol_RSPC[i][k])) {
								rspcProfile +=
									static_cast<double>(
										m_sol_RSPC[i][k]
										);
							}
						}

						rhFile
							<< dateText << ','
							<< i << ',';

						// ------------------------------------------------
						// Save seven soil layers
						//
						// Unit:
						// kg C ha-1 d-1
						// ------------------------------------------------
						for (int k = 0; k < 7; ++k) {

							if (k < nLayers &&
								std::isfinite(m_sol_RSPC[i][k])) {

								rhFile
									<< m_sol_RSPC[i][k]
									<< ',';

							}
							else {

								rhFile
									<< "0,";
							}
						}

						// ------------------------------------------------
						// Whole-profile RSPC
						// kg C ha-1 d-1
						// ------------------------------------------------
						rhFile
							<< rspcProfile
							<< ',';

						// ------------------------------------------------
						// Convert:
						//
						// kg C ha-1 d-1
						// -> g C m-2 d-1
						//
						// 1 kg/ha = 0.1 g/m2
						// ------------------------------------------------
						rhFile
							<< rspcProfile / 10.0
							<< '\n';
					}

					++outputDayCounter;

					// ----------------------------------------------------
					// Do NOT flush every day.
					// All-cell output is large.
					// Flush every 30 days for better performance.
					// ----------------------------------------------------
					if (outputDayCounter % 30 == 0 ||
						dateKey == END_DATE) {

						rhFile.flush();
					}
				}
		}
	}
}

	// ============================================================================
    // DEBUG: Annual CENTURY carbon-pool stability
    // Sum LSC + LMC + BMC + HSC + HPC over ALL soil layers.
    // Compare Dec. 31 of current year with Dec. 31 of previous year.
    // ============================================================================
{
const bool DEBUG_WRITE_CENTURY_STABILITY = false;

if (DEBUG_WRITE_CENTURY_STABILITY && m_cbnModel == 2) {

	// --------------------------------------------------------------------
	// Convert model time to calendar date
	// --------------------------------------------------------------------
	const std::time_t currentDate =
		static_cast<std::time_t>(m_date);

	std::tm dateInfo = {};

#ifdef _WIN32
	localtime_s(&dateInfo, &currentDate);
#else
	localtime_r(&currentDate, &dateInfo);
#endif

	const int year =
		dateInfo.tm_year + 1900;

	const int month =
		dateInfo.tm_mon + 1;

	const int day =
		dateInfo.tm_mday;

	// --------------------------------------------------------------------
	// Only record once every year: December 31
	// --------------------------------------------------------------------
	if (month == 12 && day == 31) {

		// ================================================================
		// Five wetland cells used for CH4
		// ================================================================
		const int debugCells[] = {
			1366,
			1367,
			1368,
			1370,
			1375
		};

		const int nDebugCells =
			sizeof(debugCells) / sizeof(debugCells[0]);

		// ----------------------------------------------------------------
		// Previous year's total C for these five cells
		// ----------------------------------------------------------------
		static std::vector<double> previousTotalC(
			nDebugCells,
			-1.0
		);

		// Number of consecutive years with change < 1%
		static std::vector<int> stableYears(
			nDebugCells,
			0
		);

		// ----------------------------------------------------------------
		// Open CSV only once
		// ----------------------------------------------------------------
		static std::ofstream stabilityFile;
		static bool stabilityFileInitialized = false;

		if (!stabilityFileInitialized) {

#ifdef _WIN32
			const char* outputPath = "D:/SEIMS_C/data/CW_2/CENTURY_carbon_stability.csv";
#else
			const char* outputPath = "/data/user/lijing/software/SEIMS_C/data/CW_2/CENTURY_carbon_stability.csv";
#endif

			stabilityFile.open(
				outputPath,
				std::ios::out | std::ios::trunc
			);

			if (!stabilityFile.is_open()) {

				std::cerr
					<< "[CENTURY_STABILITY_ERROR] Cannot open: "
					<< outputPath
					<< '\n';

			}
			else {

				stabilityFile
					<< "YEAR,"
					<< "CELL,"
					<< "LSC_PROFILE_kgCha,"
					<< "LMC_PROFILE_kgCha,"
					<< "BMC_PROFILE_kgCha,"
					<< "HSC_PROFILE_kgCha,"
					<< "HPC_PROFILE_kgCha,"
					<< "TOTAL_C_PROFILE_kgCha,"
					<< "CHANGE_PERCENT,"
					<< "STABLE_LT_1PCT,"
					<< "CONSECUTIVE_STABLE_YEARS"
					<< '\n';
			}

			stabilityFileInitialized = true;
		}

		// ----------------------------------------------------------------
		// Calculate whole-profile C
		// ----------------------------------------------------------------
		if (stabilityFile.is_open()) {

			stabilityFile
				<< std::fixed
				<< std::setprecision(8);

			for (int p = 0; p < nDebugCells; ++p) {

				const int i = debugCells[p];

				if (i < 0 || i >= m_nCells) {
					continue;
				}

				const int nLayers =
					CVT_INT(m_nSoilLyrs[i]);

				double totalLSC = 0.0;
				double totalLMC = 0.0;
				double totalBMC = 0.0;
				double totalHSC = 0.0;
				double totalHPC = 0.0;

				// =========================================================
				// Sum each CENTURY carbon pool across ALL soil layers
				// =========================================================
				for (int k = 0; k < nLayers; ++k) {

					if (std::isfinite(m_sol_LSC[i][k])) {
						totalLSC +=
							static_cast<double>(
								m_sol_LSC[i][k]
								);
					}

					if (std::isfinite(m_sol_LMC[i][k])) {
						totalLMC +=
							static_cast<double>(
								m_sol_LMC[i][k]
								);
					}

					if (std::isfinite(m_sol_BMC[i][k])) {
						totalBMC +=
							static_cast<double>(
								m_sol_BMC[i][k]
								);
					}

					if (std::isfinite(m_sol_HSC[i][k])) {
						totalHSC +=
							static_cast<double>(
								m_sol_HSC[i][k]
								);
					}

					if (std::isfinite(m_sol_HPC[i][k])) {
						totalHPC +=
							static_cast<double>(
								m_sol_HPC[i][k]
								);
					}
				}

				// =========================================================
				// Total CENTURY carbon pool
				// =========================================================
				const double totalC =
					totalLSC
					+ totalLMC
					+ totalBMC
					+ totalHSC
					+ totalHPC;

				// =========================================================
				// Compare with previous year's Dec. 31
				// =========================================================
				double changePercent = -9999.0;

				int stable = 0;

				if (previousTotalC[p] > 0.0) {

					changePercent =
						std::fabs(
							totalC
							- previousTotalC[p]
						)
						/ previousTotalC[p]
						* 100.0;

					if (changePercent < 1.0) {

						stable = 1;

						stableYears[p] += 1;

					}
					else {

						stable = 0;

						stableYears[p] = 0;
					}
				}

				// =========================================================
				// Write one row
				// =========================================================
				stabilityFile
					<< year << ','
					<< i << ','
					<< totalLSC << ','
					<< totalLMC << ','
					<< totalBMC << ','
					<< totalHSC << ','
					<< totalHPC << ','
					<< totalC << ',';

				// First year has no previous year
				if (changePercent < 0.0) {

					stabilityFile
						<< "NA,"
						<< 0 << ','
						<< 0
						<< '\n';

				}
				else {

					stabilityFile
						<< changePercent << ','
						<< stable << ','
						<< stableYears[p]
						<< '\n';
				}

				// =========================================================
				// Save this year's state for next year's comparison
				// =========================================================
				previousTotalC[p] = totalC;
			}

			// Flush immediately, so you can inspect while SEIMS is running
			stabilityFile.flush();
		}
	}
}
}


// ============================================================================
// DEBUG: CENTURY carbon-pool stability between 40-year climate cycles
//
// Climate mapping:
//
//   1980-2019  = original climate data
//   2020-2059  = exact copy of 1980-2019
//
// Compare carbon pools at:
//   2019 -> 2059
//
// This is separate from CENTURY_carbon_stability.csv,
// which keeps annual Dec.31 values.
// ============================================================================
{
const bool DEBUG_WRITE_CENTURY_CYCLE_STABILITY = false;

if (DEBUG_WRITE_CENTURY_CYCLE_STABILITY &&
	m_cbnModel == 2) {

	// ------------------------------------------------------------
	// Convert model time to calendar date
	// ------------------------------------------------------------
	const std::time_t currentDate =
		static_cast<std::time_t>(m_date);

	std::tm dateInfo = {};

#ifdef _WIN32
	localtime_s(
		&dateInfo,
		&currentDate
	);
#else
	localtime_r(
		&currentDate,
		&dateInfo
	);
#endif

	const int year =
		dateInfo.tm_year + 1900;

	const int month =
		dateInfo.tm_mon + 1;

	const int day =
		dateInfo.tm_mday;

	// First complete 40-year climate cycle:
    //   1980-2019 = original climate
    //
    // Second complete 40-year climate cycle:
    // 2020-2059 = exact copy of 1980-2019
    //
    // Therefore:
    // 2019 = end of cycle 1
    // 2059 = end of cycle 2
	const int FIRST_COMPARE_YEAR = 2019;
	const int CLIMATE_CYCLE_YEARS = 40;

	const bool isCycleEnd =
		year >= FIRST_COMPARE_YEAR &&
		((year - FIRST_COMPARE_YEAR)
			% CLIMATE_CYCLE_YEARS == 0);

	// ------------------------------------------------------------
	// Only execute on Dec.31 at the end of each 40-year cycle
	// ------------------------------------------------------------
	if (month == 12 &&
		day == 31 &&
		isCycleEnd) {

		// --------------------------------------------------------
		// Five wetland cells
		// --------------------------------------------------------
		const int debugCells[] = {
			1366,
			1367,
			1368,
			1370,
			1375
		};

		const int nDebugCells =
			sizeof(debugCells) /
			sizeof(debugCells[0]);

		// --------------------------------------------------------
		// Save the previous cycle-end total carbon
		//
		// Example:
		// 2019 stores the end-of-cycle-1 carbon state.
        // 2059 compares the end-of-cycle-2 state with 2019.
		// --------------------------------------------------------
		static std::vector<double>
			previousCycleTotalC(
				nDebugCells,
				-1.0
			);

		// Number of consecutive cycles with change < 1%
		static std::vector<int>
			stableCycleCount(
				nDebugCells,
				0
			);

		// --------------------------------------------------------
		// Open output CSV only once
		// --------------------------------------------------------
		static std::ofstream cycleFile;
		static bool cycleFileInitialized = false;

		if (!cycleFileInitialized) {

#ifdef _WIN32
			const char* outputPath = "D:/SEIMS_C/data/CW_2/CENTURY_cycle_stability.csv";
#else
			const char* outputPath = "/data/user/lijing/software/SEIMS_C/data/CW_2/CENTURY_cycle_stability.csv";
#endif

			cycleFile.open(
				outputPath,
				std::ios::out |
				std::ios::trunc
			);

			if (!cycleFile.is_open()) {

				std::cerr
					<< "[CENTURY_CYCLE_ERROR] Cannot open: "
					<< outputPath
					<< '\n';

			}
			else {

				cycleFile
					<< "YEAR,"
					<< "CYCLE,"
					<< "CELL,"
					<< "LSC_PROFILE_kgCha,"
					<< "LMC_PROFILE_kgCha,"
					<< "BMC_PROFILE_kgCha,"
					<< "HSC_PROFILE_kgCha,"
					<< "HPC_PROFILE_kgCha,"
					<< "TOTAL_C_PROFILE_kgCha,"
					<< "CHANGE_FROM_PREVIOUS_CYCLE_PERCENT,"
					<< "STABLE_LT_1PCT,"
					<< "CONSECUTIVE_STABLE_CYCLES"
					<< '\n';
			}

			cycleFileInitialized = true;
		}

		// --------------------------------------------------------
		// Calculate carbon pools
		// --------------------------------------------------------
		if (cycleFile.is_open()) {

			cycleFile
				<< std::fixed
				<< std::setprecision(8);

			const int cycleNumber =
				(year - FIRST_COMPARE_YEAR)
				/ CLIMATE_CYCLE_YEARS
				+ 1;

			for (int p = 0;
				p < nDebugCells;
				++p) {

				const int i =
					debugCells[p];

				if (i < 0 ||
					i >= m_nCells) {
					continue;
				}

				const int nLayers =
					CVT_INT(
						m_nSoilLyrs[i]
					);

				double totalLSC = 0.0;
				double totalLMC = 0.0;
				double totalBMC = 0.0;
				double totalHSC = 0.0;
				double totalHPC = 0.0;

				bool valid = true;

				// ------------------------------------------------
				// Sum all valid soil layers
				// ------------------------------------------------
				for (int k = 0;
					k < nLayers;
					++k) {

					if (
						!std::isfinite(
							m_sol_LSC[i][k]
						) ||
						!std::isfinite(
							m_sol_LMC[i][k]
						) ||
						!std::isfinite(
							m_sol_BMC[i][k]
						) ||
						!std::isfinite(
							m_sol_HSC[i][k]
						) ||
						!std::isfinite(
							m_sol_HPC[i][k]
						)
						) {

						valid = false;
						break;
					}

					totalLSC +=
						static_cast<double>(
							m_sol_LSC[i][k]
							);

					totalLMC +=
						static_cast<double>(
							m_sol_LMC[i][k]
							);

					totalBMC +=
						static_cast<double>(
							m_sol_BMC[i][k]
							);

					totalHSC +=
						static_cast<double>(
							m_sol_HSC[i][k]
							);

					totalHPC +=
						static_cast<double>(
							m_sol_HPC[i][k]
							);
				}

				if (!valid) {

					std::cerr
						<< "[CENTURY_CYCLE_WARNING] "
						<< "Invalid carbon pool: "
						<< "YEAR=" << year
						<< ", CELL=" << i
						<< '\n';

					continue;
				}

				// ------------------------------------------------
				// Total whole-profile CENTURY carbon
				// ------------------------------------------------
				const double totalC =
					totalLSC
					+ totalLMC
					+ totalBMC
					+ totalHSC
					+ totalHPC;

				// ------------------------------------------------
				// Compare with previous 40-year cycle
				// ------------------------------------------------
				double changePercent =
					-9999.0;

				int stable = 0;

				if (
					previousCycleTotalC[p] >
					0.0
					) {

					changePercent =
						std::fabs(
							totalC
							-
							previousCycleTotalC[p]
						)
						/
						previousCycleTotalC[p]
						*
						100.0;

					if (changePercent < 1.0) {

						stable = 1;

						stableCycleCount[p] += 1;

					}
					else {

						stable = 0;

						stableCycleCount[p] = 0;
					}
				}

				// ------------------------------------------------
				// Write CSV
				// ------------------------------------------------
				cycleFile
					<< year << ','
					<< cycleNumber << ','
					<< i << ','
					<< totalLSC << ','
					<< totalLMC << ','
					<< totalBMC << ','
					<< totalHSC << ','
					<< totalHPC << ','
					<< totalC << ',';

				// First cycle has no previous cycle
				if (changePercent < 0.0) {

					cycleFile
						<< "NA,"
						<< 0 << ','
						<< 0
						<< '\n';

				}
				else {

					cycleFile
						<< changePercent
						<< ','
						<< stable
						<< ','
						<< stableCycleCount[p]
						<< '\n';
				}

				// ------------------------------------------------
				// Save current cycle state
				// ------------------------------------------------
				previousCycleTotalC[p] =
					totalC;
			}

			cycleFile.flush();
		}
	}
}
}


// ============================================================================
// Save CENTURY state at the end of one 2009-2015 spin-up cycle
// ============================================================================
{
const bool WRITE_CENTURY_RESTART = false;

if (WRITE_CENTURY_RESTART && m_cbnModel == 2) {

	const std::time_t currentDate =
		static_cast<std::time_t>(m_date);

	std::tm dateInfo = {};

#ifdef _WIN32
	localtime_s(&dateInfo, &currentDate);
#else
	localtime_r(&currentDate, &dateInfo);
#endif

	const int year = dateInfo.tm_year + 1900;
	const int month = dateInfo.tm_mon + 1;
	const int day = dateInfo.tm_mday;

	// ------------------------------------------------------------
	// Save the end-of-cycle CENTURY state on 2015-05-18.
    //
    // The next spin-up cycle starts again at 2009-05-19.
    // Hydrology is reinitialized at 2009-05-19,
    // while CENTURY C/N pools are inherited from this restart.
	// ------------------------------------------------------------
	const bool saveRestartDate =
		(year == 2015 &&
			month == 5 &&
			day == 18);

	if (saveRestartDate) {

#ifdef _WIN32
		const char* outputPath = "D:/SEIMS_C/data/CW_2/CENTURY_spinup_out.csv";
#else
		const char* outputPath = "/data/user/lijing/software/SEIMS_C/data/CW_2/CENTURY_spinup_out.csv";
#endif

		std::ofstream stateFile(
			outputPath,
			std::ios::out | std::ios::trunc
		);

		if (!stateFile.is_open()) {

			std::cerr
				<< "[CENTURY_RESTART_ERROR] Cannot open: "
				<< outputPath
				<< '\n';

		}
		else {

			// ====================================================
			// Header
			// ====================================================
			stateFile
				<< "YEAR,"
				<< "CELL,"
				<< "LAYER,"

				// C pools
				<< "LSC,"
				<< "LMC,"
				<< "BMC,"
				<< "HSC,"
				<< "HPC,"

				// N pools
				<< "LSN,"
				<< "LMN,"
				<< "BMN,"
				<< "HSN,"
				<< "HPN,"

				// litter / structural states
				<< "LM,"
				<< "LS,"
				<< "LSL,"
				<< "LSLC,"
				<< "LSLNC,"

				// mineral N
				<< "NO3,"
				<< "NH4"
				<< '\n';

			stateFile
				<< std::fixed
				<< std::setprecision(8);

			long savedRows = 0;

			for (int i = 0; i < m_nCells; ++i) {

				const int nLayers =
					CVT_INT(m_nSoilLyrs[i]);

				for (int k = 0; k < nLayers; ++k) {

					// --------------------------------------------
					// Make sure all restart states are finite.
					// --------------------------------------------
					const bool valid =
						std::isfinite(m_sol_LSC[i][k]) &&
						std::isfinite(m_sol_LMC[i][k]) &&
						std::isfinite(m_sol_BMC[i][k]) &&
						std::isfinite(m_sol_HSC[i][k]) &&
						std::isfinite(m_sol_HPC[i][k]) &&

						std::isfinite(m_sol_LSN[i][k]) &&
						std::isfinite(m_sol_LMN[i][k]) &&
						std::isfinite(m_sol_BMN[i][k]) &&
						std::isfinite(m_sol_HSN[i][k]) &&
						std::isfinite(m_sol_HPN[i][k]) &&

						std::isfinite(m_sol_LM[i][k]) &&
						std::isfinite(m_sol_LS[i][k]) &&
						std::isfinite(m_sol_LSL[i][k]) &&
						std::isfinite(m_sol_LSLC[i][k]) &&
						std::isfinite(m_sol_LSLNC[i][k]) &&

						std::isfinite(m_soilNO3[i][k]) &&
						std::isfinite(m_soilNH4[i][k]);

					if (!valid) {

						std::cerr
							<< "\n============================================================"
							<< "\n[CENTURY_RESTART_WARNING] INVALID STATE"
							<< "\nCELL=" << i
							<< ", LAYER=" << k

							<< "\n---------------- CARBON ----------------"
							<< "\nLSC=" << m_sol_LSC[i][k]
							<< "\nLMC=" << m_sol_LMC[i][k]
							<< "\nBMC=" << m_sol_BMC[i][k]
							<< "\nHSC=" << m_sol_HSC[i][k]
							<< "\nHPC=" << m_sol_HPC[i][k]

							<< "\n---------------- NITROGEN ----------------"
							<< "\nLSN=" << m_sol_LSN[i][k]
							<< "\nLMN=" << m_sol_LMN[i][k]
							<< "\nBMN=" << m_sol_BMN[i][k]
							<< "\nHSN=" << m_sol_HSN[i][k]
							<< "\nHPN=" << m_sol_HPN[i][k]

							<< "\n---------------- LITTER ----------------"
							<< "\nLM=" << m_sol_LM[i][k]
							<< "\nLS=" << m_sol_LS[i][k]
							<< "\nLSL=" << m_sol_LSL[i][k]
							<< "\nLSLC=" << m_sol_LSLC[i][k]
							<< "\nLSLNC=" << m_sol_LSLNC[i][k]

							<< "\n---------------- MINERAL N ----------------"
							<< "\nNO3=" << m_soilNO3[i][k]
							<< "\nNH4=" << m_soilNH4[i][k]

							<< "\n============================================================"
							<< std::endl;

						continue;
					}

					stateFile
						<< year << ','
						<< i << ','
						<< k << ','

						<< m_sol_LSC[i][k] << ','
						<< m_sol_LMC[i][k] << ','
						<< m_sol_BMC[i][k] << ','
						<< m_sol_HSC[i][k] << ','
						<< m_sol_HPC[i][k] << ','

						<< m_sol_LSN[i][k] << ','
						<< m_sol_LMN[i][k] << ','
						<< m_sol_BMN[i][k] << ','
						<< m_sol_HSN[i][k] << ','
						<< m_sol_HPN[i][k] << ','

						<< m_sol_LM[i][k] << ','
						<< m_sol_LS[i][k] << ','
						<< m_sol_LSL[i][k] << ','
						<< m_sol_LSLC[i][k] << ','
						<< m_sol_LSLNC[i][k] << ','

						<< m_soilNO3[i][k] << ','
						<< m_soilNH4[i][k]
						<< '\n';

					++savedRows;
				}
			}

			stateFile.close();

			std::cout
				<< "[CENTURY_RESTART] Saved "
				<< savedRows
				<< " cell-layer states to "
				<< outputPath
				<< std::endl;
		}
	}
}
}

    // Phase 2: route lateral DOC after all cells have completed their local updates.
    // m_soilIfluCbn is kg C/ha, while LMWC is g C/m2, so divide by 10.
    // This serial pass avoids OpenMP race conditions and cell-order dependence.
    for (int i = 0; i < m_nCells; i++) {
        if (m_landUse[i] == LANDUSE_ID_WATR || m_landUse[i] == LANDUSE_ID_GLC) continue;

        const int id_downstream = CVT_INT(m_flowOutIdxD8[i]);
        if (id_downstream < 0 || id_downstream >= m_nCells) continue;
        if (m_area[i] <= UTIL_ZERO || m_area[id_downstream] <= UTIL_ZERO) continue;

        const int nRouteLyrs = Min(CVT_INT(m_nSoilLyrs[i]),
                                   CVT_INT(m_nSoilLyrs[id_downstream]));

        for (int k = 0; k < nRouteLyrs; k++) {
            const float routedLMWC =
                (m_soilIfluCbn[i][k] / 10.f) *
                m_area[i] / m_area[id_downstream];

            if (routedLMWC > 0.f) {
                LMWC[id_downstream][k] += routedLMWC;
            }
        }
    }

    // Synchronize the final carbon pools and first-layer outputs after routing.
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        const int nSoilLyrs = CVT_INT(m_nSoilLyrs[i]);
        for (int k = 0; k < nSoilLyrs; k++) {
            SOM[i][k] = POM[i][k] + LMWC[i][k] + MAOM[i][k] + MIC[i][k];

            // LMWC is the dynamic dissolved/low-molecular-weight C pool in g C/m2.
            // Convert it back to DOC concentration (mg C/kg soil).
            // DOC = LMWC * 1000 / [BD(Mg/m3) * thickness(mm)].
            const float soilMassFactor = m_soilBD[i][k] * m_soilThk[i][k];
            if (soilMassFactor > UTIL_ZERO) {
                DOC[i][k] = Max(0.f, LMWC[i][k]) * 1000.f / soilMassFactor;
            } else {
                DOC[i][k] = 0.f;
            }
        }

        if (nSoilLyrs > 0) {
            MIC1[i] = MIC[i][0];
            LMWC1[i] = LMWC[i][0];
            POM1[i] = POM[i][0];
            MAOM1[i] = MAOM[i][0];
        }
    }

//	// =============================================================
//    // Daily LMWC, whole-profile SOC, and daily SOILCO2 diagnostics
//    // =============================================================
//	{
//		static std::ofstream lmwcFile;
//		static bool lmwcFileInitialized = false;
//		static long outputDays = 0;
//
//		if (!lmwcFileInitialized) {
//			const char* outputPath =
//				R"(D:\SEIMS_C\data\CW_2\LMWC_seasonality.csv)";
//
//			lmwcFile.open(
//				outputPath,
//				std::ios::out | std::ios::trunc
//			);
//
//			if (!lmwcFile.is_open()) {
//				std::cerr
//					<< "[LMWC_CSV_ERROR] Cannot open: "
//					<< outputPath
//					<< '\n';
//			}
//			else {
//				lmwcFile
//					<< "DATE,CELL,"
//					<< "POM_PROFILE_gCm2,"
//					<< "LMWC_PROFILE_gCm2,"
//					<< "LMWC_0_300_gCm2,"
//					<< "LMWC_SAT90_gCm2,"
//					<< "LMWC_SAT_WEIGHTED_gCm2,"
//					<< "SOC_PROFILE_gCm2,"
//					<< "MIC_PROFILE_gCm2,"
//					<< "MAOM_PROFILE_gCm2,"
//					<< "HETERO_RESP_PROFILE_gCm2d,"
//					<< "RSPC_PROFILE_kgCha_d,"
//					<< "MEAN_SOIL_TEMP_C,"
//					<< "MEAN_SAT_RATIO,"
//					<< "LMWC_LAYER0_gCm2,"
//					<< "DOC_LAYER0_mgkg"
//					<< '\n';
//			}
//
//			lmwcFileInitialized = true;
//		}
//
//		if (lmwcFile.is_open()) {
//			const std::time_t currentDate =
//				static_cast<std::time_t>(m_date);
//
//			std::tm dateInfo = {};
//
//#ifdef _WIN32
//			localtime_s(&dateInfo, &currentDate);
//#else
//			localtime_r(&currentDate, &dateInfo);
//#endif
//
//			char dateText[16] = {};
//
//			std::strftime(
//				dateText,
//				sizeof(dateText),
//				"%Y-%m-%d",
//				&dateInfo
//			);
//
//			const int debugCells[] = {
//				1304, 1305, 1306, 1309, 1310
//			};
//
//			const int nDebugCells =
//				sizeof(debugCells) /
//				sizeof(debugCells[0]);
//
//			for (int p = 0; p < nDebugCells; ++p) {
//				const int i = debugCells[p];
//
//				if (i < 0 || i >= m_nCells) {
//					continue;
//				}
//
//				const int nLayers =
//					CVT_INT(m_nSoilLyrs[i]);
//
//				if (nLayers <= 0) {
//					continue;
//				}
//
//				// 整个剖面的POM，g C m-2
//				double pomProfile = 0.0;
//
//				double lmwcProfile = 0.0;
//				double lmwc0To300 = 0.0;
//				double lmwcSat90 = 0.0;
//				double lmwcSatWeighted = 0.0;
//
//				// 整个土壤剖面所有土层的SOC总量
//				double socProfile = 0.0;
//
//				// 整个土壤剖面所有土层的微生物生物量碳
//                // 单位：g C m-2
//				double micProfile = 0.0;
//
//				// 整个剖面的MAOM，g C m-2
//				double maomProfile = 0.0;
//
//				// 当天整个土壤剖面所有土层的CO2通量总和
//				// Unit: g C m-2 d-1.
//				double soilCO2Profile = 0.0;
//
//				// CENTURY当天整个土壤剖面的异养呼吸总量
//                // m_sol_RSPC原始单位：kg C ha-1 d-1
//				double rspcProfile = 0.0;
//
//				double tempThicknessSum = 0.0;
//				double satThicknessSum = 0.0;
//				double totalThickness = 0.0;
//
//				for (int k = 0; k < nLayers; ++k) {
//					const float layerPOM =
//						Max(0.f, POM[i][k]);
//					const float layerLMWC =
//						Max(0.f, LMWC[i][k]);
//
//					const float layerSOC =
//						Max(0.f, SOM[i][k]);
//
//					const float layerMIC =
//						Max(0.f, MIC[i][k]);
//
//
//					const float layerMAOM =
//						Max(0.f, MAOM[i][k]);
//
//					const float layerSoilCO2 =
//						Max(0.f, soilCO2[i][k]);
//
//					const float thickness =
//						Max(0.f, m_soilThk[i][k]);
//
//					float satRatio = 0.f;
//
//					if (m_soilSat[i][k] > UTIL_ZERO) {
//						satRatio =
//							m_soilWtrSto[i][k] /
//							m_soilSat[i][k];
//
//						satRatio =
//							Max(
//								0.f,
//								Min(1.f, satRatio)
//							);
//					}
//
//					// 所有土层POM总量
//					pomProfile += layerPOM;
//
//					// 所有土层LMWC总量
//					lmwcProfile += layerLMWC;
//
//					// 所有土层SOC总量
//					// SOM = POM + LMWC + MAOM + MIC
//					socProfile += layerSOC;
//
//					// 各土层MIC直接求和
//					micProfile += layerMIC;
//
//					// 所有土层MAOM总量
//					maomProfile += layerMAOM;
//
//					// 当天整个土层剖面的SOILCO2通量
//					soilCO2Profile += layerSoilCO2;
//
//					// 当天整个土壤剖面的CENTURY异养呼吸
//					if (m_cbnModel == 2 && m_sol_RSPC != nullptr) {
//						const float layerRspc = m_sol_RSPC[i][k];
//
//						if (std::isfinite(layerRspc)) {
//							rspcProfile += layerRspc;
//						}
//					}
//
//					const float upperDepth =
//						k == 0
//						? 0.f
//						: m_soilDepth[i][k - 1];
//
//					const float lowerDepth =
//						m_soilDepth[i][k];
//
//					const float overlapThickness =
//						Max(
//							0.f,
//							Min(lowerDepth, 300.f) -
//							upperDepth
//						);
//
//					float shallowFraction = 0.f;
//
//					if (thickness > UTIL_ZERO) {
//						shallowFraction =
//							Min(
//								1.f,
//								overlapThickness / thickness
//							);
//					}
//
//					lmwc0To300 +=
//						layerLMWC * shallowFraction;
//
//					if (satRatio >= 0.9f) {
//						lmwcSat90 += layerLMWC;
//					}
//
//					lmwcSatWeighted +=
//						layerLMWC * satRatio;
//
//					tempThicknessSum +=
//						m_soilTempprofile[i][k] *
//						thickness;
//
//					satThicknessSum +=
//						satRatio *
//						thickness;
//
//					totalThickness += thickness;
//				}
//
//				float meanSoilTemp = NODATA_VALUE;
//				float meanSatRatio = NODATA_VALUE;
//
//				if (totalThickness > UTIL_ZERO) {
//					meanSoilTemp =
//						static_cast<float>(
//							tempThicknessSum /
//							totalThickness
//							);
//
//					meanSatRatio =
//						static_cast<float>(
//							satThicknessSum /
//							totalThickness
//							);
//				}
//
//				float docLayer0MgKg = NODATA_VALUE;
//
//				const float layer0SoilMassFactor =
//					m_soilBD[i][0] *
//					m_soilThk[i][0];
//
//				if (layer0SoilMassFactor > UTIL_ZERO) {
//					docLayer0MgKg =
//						LMWC[i][0] *
//						1000.f /
//						layer0SoilMassFactor;
//				}
//
//				lmwcFile
//					<< dateText << ','
//					<< i << ','
//					<< std::fixed
//					<< std::setprecision(6)
//					<< pomProfile << ','
//					<< lmwcProfile << ','
//					<< lmwc0To300 << ','
//					<< lmwcSat90 << ','
//					<< lmwcSatWeighted << ','
//					<< socProfile << ','
//					<< micProfile << ','
//					<< maomProfile << ','
//					<< soilCO2Profile << ','
//					<< rspcProfile << ','
//					<< meanSoilTemp << ','
//					<< meanSatRatio << ','
//					<< LMWC[i][0] << ','
//					<< docLayer0MgKg
//					<< '\n';
//			}
//
//			++outputDays;
//
//			if (outputDays % 365 == 0) {
//				lmwcFile.flush();
//			}
//		}
//	}


    return 0;
}
float Nutrient_Transformation::fSWC2SWP(float SWC0, float SWCsat) {
    const double const_cm2MPa = 98e-6;  // 1 cm water column
    const float rlim = 1.01f;

    // van Genuchten parameters
    const float SWCres = 0.067f;
    const float alpha = 0.021f;
    const float n = 1.61f;
    const float m = 1.f - 1.f / n;

    // Invalid or degenerate hydraulic parameters: return saturation potential.
    if (!IsFinitePositive(SWCsat) ||
        SWCsat <= SWCres * rlim ||
        !IsFinitePositive(alpha) ||
        !IsFinitePositive(m) ||
        !IsFinitePositive(n)) {
        return 0.f;
    }

    const float SWC = Max(SWC0, SWCres * rlim);
    if (SWC >= SWCsat) {
        return 0.f;
    }

    float eff_sat = SafePositiveDivide(SWC - SWCres,
                                       SWCsat - SWCres,
                                       0.f);
    // Keep effective saturation strictly inside (0, 1] to avoid 1/0.
    eff_sat = Min(1.f, Max(eff_sat, UTIL_ZERO));

    const float inner = Max(
        pow(SafePositiveDivide(1.f, eff_sat, 1.f), 1.f / m) - 1.f,
        0.f);
    const float suctionCm = SafePositiveDivide(pow(inner, 1.f / n), alpha, 0.f);
    const float swp = -suctionCm * static_cast<float>(const_cm2MPa);
    return std::isfinite(swp) ? swp : 0.f;
}
//Compute effect factor of soil temperature with Arrhenius 
float Nutrient_Transformation:: tp_scalar(string sCase, float T0, float Tref) {
    T0 += 273.15f;   // K
    Tref += 273.15f; // K
    const float gas_const = 8.31446f;
    float Ea = 0.f;

    if (sCase == "MR") Ea = 20e3f;
    else if (sCase == "LIG") Ea = 53e3f;
    else if (sCase == "CEL") Ea = 36.3e3f;
    else if (sCase == "POM") Ea = 63909.f;
    else if (sCase == "LMWC") Ea = 57865.f;
    else if (sCase == "MAOM") Ea = 67e3f;
    else if (sCase == "MD") Ea = 47e3f;
    else if (sCase == "KM") Ea = 30e3f;

    if (!IsFinitePositive(T0) || !IsFinitePositive(Tref) ||
        !IsFinitePositive(gas_const)) {
        return 0.f;
    }

    const float exponent = -Ea / gas_const *
        (SafePositiveDivide(1.f, T0, 0.f) -
         SafePositiveDivide(1.f, Tref, 0.f));
    const float scalar = exp(exponent);
    return std::isfinite(scalar) ? scalar : 0.f;
}
void Nutrient_Transformation::ModifiedMillennial(const int i) {
    m_soilSurfCbn[i] = 0.f;
    m_soileroPOC[i] = 0.f;

    float Vg0 = 0.2493207, param_em = 0.9234062;
    //derivs_V2_MM_AD_CO2_Clab
    // float fexud = 0.1f, fexud_oa = 0.5f;
    float fexud = 0.1f, fexud_oa = 0.5f;
	int Tref = 20;
    //constant parameters
    int rootExu = 1;
    int maRco2 = 0; //0-(1-cue)
    int tae_ref = 20; ///< reference  soil temperature (¡æ)
    float kamin = 0.2f; ///< Minimum relative rate in saturated soil (-)
    float lambda = 0.00021f; ///< Dependence of rate on matric potential (1/kPa)
    float rate_leach = 0.0015f;//0.0015f; ///< Leaching rate of LMWC (1/d)
    float param_p1 = 0.12f; ///< Coefficient for estimating the binding affinity for LMWC sorption (-)

    for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        // Reset first so SOILCO2 always represents the current day's flux.
        // If this layer is skipped by any validity check below, today's value remains zero
        // instead of retaining the previous day's result.
        soilCO2[i][k] = 0.f;

        m_soilIfluCbn[i][k] = 0.f;
        m_soilPercoCbn[i][k] = 0.f;

        // These quantities appear repeatedly in denominators.
        if (!IsFinitePositive(m_soilThk[i][k]) ||
            !IsFinitePositive(m_soilPor[i][k]) ||
            !IsFinitePositive(m_soilBD[i][k])) {
            continue;
        }

        float kaff_lm = exp(-param_p1 * m_soilpH[i][k] - param_p2[i]) * kaff_des[i];

        // Equation 11. Convert thickness from mm to m and BD from Mg/m3
        // to kg/m3. The /1000 and *1.e3 factors cancel numerically.
        const float param_qmax =
            m_soilThk[i][k] / 1000.f * m_soilBD[i][k] * 1.e3f *
            (100.f - m_soilSand[i][k]) * param_pc[i];

         //if (!IsFinitePositive(param_qmax) || param_qmax < MAOM[i][k]) {
           // continue;
         //   }

         if (!IsFinitePositive(param_qmax)) {
            continue;
            }

        // NOTE: WTAERSTO does not contain wilting point.
        const float soilWaterDepth = m_soilWtrSto[i][k] + m_soilWP[i][k];
        float forc_sw0 = Min(
            SafePositiveDivide(soilWaterDepth, m_soilThk[i][k], 0.f),
            m_soilPor[i][k]);
        forc_sw0 = Max(0.f, forc_sw0);

        // Equation 4: scalar_wd
        const float relativeWater = Max(
            0.f, SafePositiveDivide(forc_sw0, m_soilPor[i][k], 0.f));
        float scalar_wd = sqrt(relativeWater);

        // Equation 15: scalar_wb
        const float SWP = fSWC2SWP(forc_sw0, m_soilPor[i][k]);
        const float matpot = abs(SWP * 1.e3f);
        const float airFilledFraction = Max(0.f,SafePositiveDivide(m_soilPor[i][k] - forc_sw0,m_soilPor[i][k],0.f));
        float scalar_wb = exp(lambda * -matpot) *(kamin + (1.f - kamin) * sqrt(airFilledFraction)) * scalar_wd;
        // cout << "scalar_wb = " << scalar_wb << "\n";
        
        //ljj++
        float rtresnew =0.f;
        m_forc_litter[i][k] = 0.f;
        if (FloatEqual(m_dormFlag[i], 1.f)){
            if (m_biomass[i] > 0.f) {
                 //rtresnew = m_biomass[i]/10.f * m_frRoot[i];
                rtresnew = m_biomass[i] * m_frRoot[i];
                m_soilRsd[i][k] += rtresnew*tmp_rtfr[i][k];
            }
            if(k==CVT_INT(m_nSoilLyrs[i])-1) m_biomass[i] -= m_biomass[i] * m_frRoot[i] ;
            // C input
            m_forc_litter[i][k] = m_soilRsd[i][k]/10.f; //ka/ha to g/m2
            m_soilRsd[i][k] = 0.f;
        }


		float f_Lit_PO = 0.f;
	    float f_Lit_LM = 0.f;
        f_Lit_PO = m_forc_litter[i][k] * param_pi[i];
        f_Lit_LM = m_forc_litter[i][k] * (1. - param_pi[i]);
        
	    m_forc_npp[i][k] = m_biomassDelta[i]/10.f*tmp_rtfr[i][k];
  
		float forc_exud = m_forc_npp[i][k] * fexud; // total root exudates
		float forc_acid = forc_exud * fexud_oa; // total organic acid
	    float f_Exud_LM = forc_exud - forc_acid;
	    float f_Oa_MA = forc_acid * param_em;
	    float f_Oa_LM = forc_acid * (1. - param_em) ;
        //cout << "f_Oa_MA = " << f_Oa_MA << "\n";
	    
        // Decomposition rates
	    float km_TPscalar = tp_scalar("KM", m_soilTempprofile[i][k], Tref);
	    float vmax_pl = Vpl0[i] * tp_scalar("POM", m_soilTempprofile[i][k], Tref) * scalar_wd;
	    float kaff_pl0 = kaff_pl[i] * km_TPscalar;
        //if(i==33)cout << "km_TPscalar = " << km_TPscalar << "  vmax_pl = " << vmax_pl << "  kaff_pl0 = " << kaff_pl0 << "\n";
		
         // Example decomposition of POM to LMWC
	    float f_PO;
	    const float pomDenom = kaff_pl0 + MIC[i][k];
	    if (POM[i][k] > 0.f && MIC[i][k] > 0.f && pomDenom > UTIL_ZERO) {
	        f_PO = Max(SafePositiveDivide(vmax_pl * POM[i][k] * MIC[i][k],
                                       pomDenom,
                                       0.f), 0.f);
	        f_PO = Min(POM[i][k], f_PO);
	    }else{
      		f_PO = 0.f;
    	}
	    float f_PO_LM = f_PO * param_fpl[i];
	    float f_PO_MA = f_PO * (1. - param_fpl[i]);
	    //cout << "f_PO_LM = " << f_PO_LM << "  f_PO_MA = " << f_PO_MA << "\n";

        //MAOC finish desorption within a few minutes after organic acid input
	    // Conversion factor from mg C/kg soil to g C/m2:
	    // thickness(mm)/1000 * BD(Mg/m3).
	    const float unit_kgTom2 = m_soilThk[i][k] / 1.e3f * m_soilBD[i][k];
	    float acid_des = 0.f;
	    if (MAOM[i][k] > 0.f &&
            IsFinitePositive(unit_kgTom2) &&
            IsFinitePositive(m_soilClay[i][k]) &&
            IsFinitePositive(m_soilpH[i][k])) {
	    //   acid_des = (pow(MAOM[i][k]/unit_kgTom2 *1e-3,0.04f) *pow(forc_acid/unit_kgTom2, 0.86f)
	    //               * pow(m_soilClay[i][k],-0.24f) * pow(m_soilpH[i][k],-2.14f) * exp(3.52f)) * unit_kgTom2;
	    //   acid_des = Min(Max(acid_des,0.f),MAOM[i][k]);
	       float forc_acid_total = fmax(forc_acid, 0.f);
        
	       // Equal-weight assumption for four organic acids
	       float forc_oxalic = forc_acid_total * 0.25f;
	       float forc_citric = forc_acid_total * 0.25f;
	       float forc_malic  = forc_acid_total * 0.25f;
	       float forc_acetic = forc_acid_total * 0.25f;
	       
	       // Relative desorption ability normalized by oxalic acid
	       float fac_oxalic = 1.0f;
	       float fac_citric = 12.11f / 17.3f;
	       float fac_malic  = 6.33f / 17.3f;
	       float fac_acetic = 1.0f / 17.3f;
        
           const float maomConcentration = Max(
               SafePositiveDivide(MAOM[i][k], unit_kgTom2, 0.f) * 1.e-3f,
               0.f);
           const float oxalicConcentration = Max(
               SafePositiveDivide(forc_oxalic, unit_kgTom2, 0.f), 0.f);
           const float citricConcentration = Max(
               SafePositiveDivide(forc_citric, unit_kgTom2, 0.f), 0.f);
           const float malicConcentration = Max(
               SafePositiveDivide(forc_malic, unit_kgTom2, 0.f), 0.f);
           const float aceticConcentration = Max(
               SafePositiveDivide(forc_acetic, unit_kgTom2, 0.f), 0.f);

	       float common_part = fmax((pow(maomConcentration, 0.04f) *
                                 pow(m_soilClay[i][k], -0.24f) *
                                 pow(m_soilpH[i][k], -2.15f) * 34.0f) * unit_kgTom2, 0.f);
	       
	       float acid_des_oxalic = fmax(common_part * fac_oxalic * pow(oxalicConcentration, 0.87f), 0.f);
	       
	       float acid_des_citric = fmax(common_part * fac_citric * pow(citricConcentration, 0.87f), 0.f);
	       
	       float acid_des_malic = fmax(common_part * fac_malic * pow(malicConcentration, 0.87f), 0.f);
	       
	       float acid_des_acetic = fmax(common_part * fac_acetic * pow(aceticConcentration, 0.87f), 0.f);
	       
	       acid_des = acid_des_oxalic + acid_des_citric + acid_des_malic + acid_des_acetic;
	       
	       acid_des = fmin(acid_des, MAOM[i][k]);
	    }else{
	      acid_des = 0.f;
	    }
		
	    LMWC[i][k] = LMWC[i][k]  + f_Oa_LM + acid_des;
	    MAOM[i][k] = MAOM[i][k] + f_Oa_MA - acid_des;
	    //cout << "acid_des = " << acid_des << "  LMWC = " << LMWC[i][k] << "  MAOM = " << MAOM[i][k] <<"\n";

        //the enzymatic decompsition of MAOM to LMWC
	    // vmax_ml = max(alpha_ml * exp(-eact_ml / (gas_const * (forc_st(step.num) + 273.15))),0)
	    float vmax_ml = Vml0[i] * tp_scalar("MAOM", m_soilTempprofile[i][k], Tref) * scalar_wd;
	    float kaff_ml0 = kaff_ml[i] * km_TPscalar;
	    float f_MA_LMe;
	    const float maomDenom = kaff_ml0 + MIC[i][k];
	    if(MAOM[i][k]>0.f && MIC[i][k]>0.f && maomDenom > UTIL_ZERO){
	      f_MA_LMe = Max(SafePositiveDivide(vmax_ml * MAOM[i][k] * MIC[i][k],
                                         maomDenom,
                                         0.f), 0.f);
          f_MA_LMe = Min(MAOM[i][k], f_MA_LMe);
	    }else{
	      f_MA_LMe=0.f;
	    }

	    //cout << "f_MA_LMe = " << f_MA_LMe << "  MA = " << MA[i][k] << "\n";

        //Equation 12: physical desorption of MAOM to LMWC
	    float f_MA_LMm;
	    if(MAOM[i][k] > 0.f){
	      f_MA_LMm = Max(kaff_des[i] * MAOM[i][k] / param_qmax,0.f);
	      f_MA_LMm = Min(MAOM[i][k], f_MA_LMm);
	    }else{
	      f_MA_LMm = 0.f;
	    }

	    //cout << "f_MA_LMm = " << f_MA_LMm  << "\n";
        
        //Equation 14: respiration of MA and MD and CUE process
        //Vg0 = Vlb0; 
	    float Vg = Vlb0[i] * tp_scalar("LMWC", m_soilTempprofile[i][k], Tref)* scalar_wb; // * scalar_wb //the growth respiration of MA is similar to Vd in MEND model
        // Ma must be below 1 because it appears in Ma / (1 - Ma).
        const float maFraction = Min(Max(Ma[i], 0.f), 1.f - 1.e-6f);
	    float Vm = Vlb0[i] * SafePositiveDivide(maFraction,1.f - maFraction,0.f) *
                   tp_scalar("MR", m_soilTempprofile[i][k], Tref) * scalar_wb;
	    // float VmD = Vm * beta[i]; //the maintenance respiration of MD
	  
	    float CUE_max = 0.95f , CUE_min = 0.01f;
	  	float CUE = cue_ref[i] - cue_t[i] * (m_soilTempprofile[i][k] - Tref) ;
// 	    CUE = CUE - acue[i] * forc_acid;     //CUE decline after organic acid input
//         CUE = Min(CUE_max,CUE);
//         CUE = Max(CUE_min,CUE);
	    // if(CUE<CUE_min | CUE>CUE_max){
	    //   //cout << "CUE ERROR EXIT: CUE = " << CUE<<"     ,"<<m_soilTempprofile[i][k] << "\n";
	    //   //return;
	    // }
	    //cout << "CUE = " << CUE  << "\n";

	    //Equation 22
	    //microbial growth flux, but is not used in mass balance
	    // Equation 14.1: MA <--> MD,
	    // float VmA2D = Vm; // * tp_scalar("MR") #* scalar_wd #* wp_scalar_low
	    // float VmD2A = Vm; // * tp_scalar("MR") #* scalar_wd #* wp_scalar
	    float vmax_lb = (Vg + Vm) / CUE;
	    float kaff_lb0 = kaff_lb[i] * km_TPscalar;
	    //float sat = LMWC[i][k]/(kaff_lb[i] + LMWC[i][k]); //Wang 2015
	    //// float sat = pow(f_MA_growth/(f_MA_growth + f_MA_maint_co2 + f_MB_turn), 2); //Huang2022
	    // float f_MA_MD, f_MD_MA;
	    // if(LMWC[i][k] >0.f & MA[i][k] > 0.f){
	    //   f_MA_MD = Max((1-sat) * VmA2D * MA[i][k],0.f);
	    //   f_MA_MD = Min(MA[i][k], f_MA_MD);
	    // }else{
	    //   f_MA_MD = 0.f;
	    // }
	    // if(LMWC[i][k] >0.f & MD[i][k]>0.f){
	    //   f_MD_MA = Max(sat * VmD2A * MD[i][k],0.f);
	    //   f_MD_MA = Min(MD[i][k], f_MD_MA);
	    // }else{
	    //   f_MD_MA = 0;
	    // }
	    //cout << "f_MA_MD = " << f_MA_MD << "  f_MD_MA = " << f_MD_MA  << "\n";
	    
        // MA[i][k] = MA[i][k] + f_MD_MA - f_MA_MD; //MA and MD update immediately£¬because of reacting so quickly£¡
        // MD[i][k] = MD[i][k] + f_MA_MD - f_MD_MA;  
        // MA[i][k] = Max(MA[i][k], 0.f);
        // MD[i][k] = Max(MD[i][k], 0.f);     

	    //Equation 13: MA utilizes LMWC for growth and respiration
	    float f_LM_MB;
	    const float lmwcDenom = kaff_lb0 + LMWC[i][k];
	    if(LMWC[i][k]>0.f && MIC[i][k]>0.f && lmwcDenom > UTIL_ZERO){
	      f_LM_MB = Max(SafePositiveDivide(vmax_lb * MIC[i][k] * LMWC[i][k],lmwcDenom,0.f), 0.f);
	      f_LM_MB = Min(LMWC[i][k], f_LM_MB);
	    }else{
	      f_LM_MB=0.f;
	    }
        f_LM_MB = Min(f_LM_MB, LMWC[i][k]);  
	    //cout << "f_LM_MB = " << f_LM_MB  << "\n";

	    //Equation 21: MIC -> atmosphere
	    float f_MIC_co2;
	    if(maRco2 == 0){
	    	f_MIC_co2 = f_LM_MB * (1.f - CUE);
		}else{
            const float cueInverseMinusOne = SafePositiveDivide(1.f, CUE, 1.f) - 1.f;
			float f_MIC_growth_co2 = Max(
                SafePositiveDivide(Vg * cueInverseMinusOne * MIC[i][k] * LMWC[i][k],
                                   lmwcDenom,
                                   0.f), 0.f);
		    float f_MIC_maint_co2 = Max(
                SafePositiveDivide(Vm * cueInverseMinusOne * MIC[i][k] * LMWC[i][k],
                                   lmwcDenom,
                                   0.f), 0.f);
		    f_MIC_co2 = f_MIC_growth_co2 + f_MIC_maint_co2;
		}
	    LMWC[i][k] = LMWC[i][k] - f_LM_MB; //Update LMWC immediately for adsorption calculations

        float f_MIC_growth = f_LM_MB * CUE;
	    float Tco2 = f_MIC_co2;
        // LMWC[i][k] = Max(LMWC[i][k], 0.f);     
	    // float f_MA_growth = f_LM_MB * CUE; 
	    // //cout << "f_MA_co2 = " << f_MA_co2 << "  f_MA_growth = " << f_MA_growth << "  LMWC = " << LMWC[i][k] << "\n";

		// //the maintenance respiration of MD
		// float f_MD_co2;
	    // if(MD[i][k] > 0.f){
	    //   f_MD_co2 = Max(VmD * MD[i][k],0.f); //0.001 
	    //   f_MD_co2 = Min(MD[i][k], f_MD_co2);
	    // }else{
	    //   f_MD_co2 = 0.f;
	    // }
	    // float Tco2 = f_MA_co2 + f_MD_co2;
	    //cout << "f_MD_co2 = " << f_MD_co2 << "  Tco2 = " << Tco2 << "\n";
	    
        // Equation 8: LMWC -> out of system leaching
        float V = m_surfrunoff[i] + m_soilPerco[i][k] + m_subSurfRf[i][k];
        if(k>0) V = m_soilPerco[i][k] + m_subSurfRf[i][k];

        // Equation 4: scalar_wd. Protect the division by soil saturation storage.
        const float k_leach = m_leach[i];
        float scalar_wd1 = 0.f;
        if (V > UTIL_ZERO && m_soilSat[i][k] > UTIL_ZERO) {
            scalar_wd1 = 1.f - exp(-SafePositiveDivide(V,
                                                       m_soilSat[i][k],
                                                       0.f));
            scalar_wd1 = Min(1.f, Max(0.f, scalar_wd1));
        }

        // FT, FQ and DOC_export were not used by any active calculation, so they
        // are removed here. This also removes their unnecessary division paths.

        //ljj++
	    if(LMWC[i][k] > 0.f &&
           (V+m_soilWtrSto[i][k])>UTIL_ZERO &&
           m_soilSat[i][k] > UTIL_ZERO){
            // //f_LM_leach = Max(rate_leach * scalar_wd * LMWC[i][k],0.f);    
            f_LM_leach[i][k] = Max(k_leach*scalar_wd1*LMWC[i][k],0.f);
            f_LM_leach[i][k] = Min(LMWC[i][k], f_LM_leach[i][k]);
	    
            //DOC[i][k] = fDOC[i][k]*LMWC[i][k] + k_leach * FQ * FT * LMWC[i][k];
            //if(V+m_soilWtrSto[i][k]<=UTIL_ZERO) DOC[i][k]=0.f;

            //f_LM_leach[i][k] = DOC[i][k]/ (V+m_soilWtrSto[i][k])*V;
            //if(V+m_soilWtrSto[i][k]<=UTIL_ZERO) f_LM_leach[i][k]=0.f;
            //f_LM_leach[i][k] = Min(f_LM_leach[i][k],DOC[i][k]);
            
        }else{
	      f_LM_leach[i][k]=0.f;
	    }
        //DOC[i][k] -= f_LM_leach[i][k]; 
        //fDOC[i][k] = DOC[i][k] / LMWC[i][k];
        //if(i==503 && k==4) cout<<LMWC[i][k]<<"   "<<DOC[i][k]<<"  "<<FQ * FT<<"  "<<fDOC[i][k]<<"  "<<f_LM_leach[i][k]<<"  "<<V<<"  "<<m_biomassDelta[i]/10.f<<endl;
        //if(i==66 && k==5) cout<<f_LM_leach[i][0]<<"   "<<f_LM_leach[i][1]<<"  "<<f_LM_leach[i][2]<<"  "<<f_LM_leach[i][3]<<"  "<<f_LM_leach[i][4]<<"  "<<f_LM_leach[i][5]<<"  "<<m_biomassDelta[i]/10.f<<endl;
        //DOC[i][k] = Max(DOC[i][k], 0.f);
        
	    //Equation 9: LMWC -> MAOM
	    //float f_LM_MA;
	   // if(LMWC[i][k] > 0.f && MAOM[i][k] > 0.f){
	    //  f_LM_MA = Max(scalar_wd * kaff_lm * LMWC[i][k] * (1 - MAOM[i][k] / param_qmax),0.f);
       //   //f_MA_LMm = Max(kaff_des[i] * MAOM[i][k] / param_qmax,0.f);
	   //   f_LM_MA = Min(LMWC[i][k], f_LM_MA);
	  //  }else{
	   //   f_LM_MA=0.f;
	 //   }	  

             const float maomCapacityFactor =
             Max(0.f,1.f - MAOM[i][k] / param_qmax);

              float f_LM_MA = 0.f;

            if (LMWC[i][k] > 0.f) { f_LM_MA =Max(scalar_wd *kaff_lm *LMWC[i][k] *maomCapacityFactor,0.f);

             f_LM_MA = Min(LMWC[i][k], f_LM_MA);
              }




	    // Equation 16: MA -> MAOM/LMWC 
	    float rate_bd0 = Max(0.f, rate_bd[i] + rate_Kbd[i] * (m_soilTempprofile[i][k] - Tref));
	    float f_MB_turn;
	    if(MIC[i][k] > 0.f){
	      //f_MB_turn = rate_bd * MIC^2.0; // Millennial
	      f_MB_turn = Max(rate_bd0 * MIC[i][k], 0.f); // Liucq
	      f_MB_turn = Min(MIC[i][k], f_MB_turn);
	    }else{
	      f_MB_turn = 0.f;
	    }

	    float f_MB_LM = f_MB_turn * (1. - param_pb[i]);
	    float f_MB_MA = f_MB_turn * param_pb[i];
	    
        // flux balance
	    if((f_MA_LMe + f_MA_LMm) > MAOM[i][k]){
	      f_MA_LMe = f_MA_LMm = MAOM[i][k]/2;
	    }
	    // float f_MA_LM = acid_des + f_MA_LMe + f_MA_LMm;
	    
        float total_lmwc_out = f_LM_leach[i][k] + f_LM_MA;
	    // if((f_LM_leach[i][k] + f_LM_MA)>LMWC[i][k]){
	    //   f_LM_leach[i][k] = f_LM_MA = LMWC[i][k]/2;
	    // }   
        if (total_lmwc_out > LMWC[i][k] && total_lmwc_out > UTIL_ZERO) {
	        const float scale = LMWC[i][k] / total_lmwc_out;
	        f_LM_leach[i][k] *= scale;
	        f_LM_MA *= scale;
	    } 

	    //Flux each C pool
	    float dPOM = f_Lit_PO - f_PO;
	    float dLMWC = f_Lit_LM + f_Exud_LM + f_PO_LM + f_MB_LM + (f_MA_LMe + f_MA_LMm) - f_LM_leach[i][k] - f_LM_MA;
	    float dMIC = f_MIC_growth - f_MB_turn;
	    //float dMD =  f_MA_MD - f_MD_MA - f_MD_co2;   
	    float dMAOM = f_LM_MA + f_MB_MA + f_PO_MA - (f_MA_LMe + f_MA_LMm);

        // Update C pools
	    POM[i][k] = POM[i][k] + dPOM;
	    LMWC[i][k] = LMWC[i][k] + dLMWC;
	    // MA[i][k] = MA[i][k] + dMA;
	    // MD[i][k] = MD[i][k] + dMD;
	    MAOM[i][k] = MAOM[i][k] + dMAOM;
	    MIC[i][k] = MIC[i][k] + dMIC;
	    SOM[i][k] = POM[i][k] + LMWC[i][k] + MAOM[i][k] + MIC[i][k];
	    // Store only the current day's CO2 flux; do not accumulate across days.
	    soilCO2[i][k] = Tco2;

        //ljj++
        float YEW = 0.f;  
        float enr_poc = 1.0f;
        if(k==0){
            // Check both area and soil mass before calculating erosion fraction.
            if (m_area[i] > UTIL_ZERO && m_soilMass[i][0] > UTIL_ZERO) {
                const float sedimentPerArea = SafePositiveDivide(
                    m_olWtrEroSed[i], m_area[i] * 0.0001f, 0.f);
                YEW = SafePositiveDivide(sedimentPerArea,
                                         m_soilMass[i][0],
                                         0.f);
                YEW = Min(0.9f, Max(0.f, YEW));
            } else {
                YEW = 0.f;
            }
            m_soileroPOC[i] = (POM[i][0]) * YEW * enr_poc * 10.f; //--> kg/ha 
            if(m_surfrunoff[i]<=UTIL_ZERO) m_soileroPOC[i]=0.f;
            POM[i][0] -= m_soileroPOC[i] / 10.f;//--> g/m2
        }

        if(V>UTIL_ZERO) {
            if(k==0) m_soilSurfCbn[i] = f_LM_leach[i][k] * (m_surfrunoff[i] / (V + UTIL_ZERO)) *10.f; //--> kg/ha 
            m_soilIfluCbn[i][k] = f_LM_leach[i][k] * (m_subSurfRf[i][k] / (V + UTIL_ZERO))*10.f;
            m_soilPercoCbn[i][k] = f_LM_leach[i][k] * (m_soilPerco[i][k] / (V + UTIL_ZERO))*10.f; //--> kg/ha       
        }

        
        // Vertical DOC transport to the next soil layer.
        // m_soilPercoCbn is kg C/ha, while LMWC is g C/m2, so divide by 10.
        if (m_soilPercoCbn[i][k] > 0.f &&
            k < CVT_INT(m_nSoilLyrs[i]) - 1) {
            LMWC[i][k + 1] += m_soilPercoCbn[i][k] / 10.f;
        }

    }

    const int nSoilLyrs = CVT_INT(m_nSoilLyrs[i]);
    if (nSoilLyrs > 0) {
        m_soilPercoCbnLowest[i] = m_soilPercoCbn[i][nSoilLyrs - 1];
    } else {
        m_soilPercoCbnLowest[i] = 0.f;
    }

}

void Nutrient_Transformation::RootFraction(const int i, float*& root_fr) {
    
    float cum_rd = 0.f, cum_d = 0.f, cum_rf = 0.f, x1 = 0.f, x2 = 0.f;
    if (m_stoSoilRootD[i] <= UTIL_ZERO) {
        root_fr[0] = 1.f;
        return;
    }
    /// Normalized Root Density = 1.15*exp[-11.7*NRD] + 0.022, where NRD = normalized rooting depth
    /// Parameters of Normalized Root Density Function from Dwyer et al 19xx
    float a = 1.15f;
    float b = 11.7f;
    float c = 0.022f;
    float d = 0.12029f; /// Integral of Normalized Root Distribution Function  from 0 to 1 (normalized depth) = 0.12029
    int k = 0;          /// used as layer identifier
    
    for (int l = 0; l < CVT_INT(m_nSoilLyrs[i]); l++) {
        cum_d += m_soilThk[i][l];
        if (cum_d >= m_stoSoilRootD[i]) cum_rd = m_stoSoilRootD[i];
        if (cum_d < m_stoSoilRootD[i]) cum_rd = cum_d;
        // Use the actual upper boundary of the current layer.
        x1 = Max(0.f, (cum_d - m_soilThk[i][l]) / m_stoSoilRootD[i]);
        x2 = cum_rd / m_stoSoilRootD[i];
        float xx1 = -b * x1;
        if (xx1 > 20.f) xx1 = 20.f;
        float xx2 = -b * x2;
        if (xx2 > 20.f) xx2 = 20.f;
        root_fr[l] = (a / b * (exp(xx1) - exp(xx2)) + c * (x2 - x1)) / d;
        float xx = cum_rf;
        cum_rf += root_fr[l];
        if (cum_rf > 1.f) {
            root_fr[l] = 1.f - xx;
            cum_rf = 1.f;
        }
        k = l;
        if (cum_rd >= m_stoSoilRootD[i]) {
            break;
        }
    }
    
    /// ensures that cumulative fractional root distribution = 1
    if (cum_rf <= UTIL_ZERO || !std::isfinite(cum_rf)) {
        for (int l = 0; l < CVT_INT(m_nSoilLyrs[i]); ++l) root_fr[l] = 0.f;
        root_fr[0] = 1.f;
        return;
    }
    for (int l = 0; l < CVT_INT(m_nSoilLyrs[i]); l++) {
        root_fr[l] = SafePositiveDivide(root_fr[l], cum_rf, 0.f);
        if (l == k) {
            /// exits loop on the same layer as the previous loop
            break;
        }
    }
}

void Nutrient_Transformation::MineralizationStaticCarbonMethod(const int i) {
    const int nSoilLyrs = CVT_INT(m_nSoilLyrs[i]);
    for (int k = 0; k < nSoilLyrs; k++) {
        // Use layer 1 for surface controls only when it exists.
        int kk = (k == 0 && nSoilLyrs > 1) ? 1 : k;
        float sut = 0.f;
        // mineralization can occur only if temp above 0 deg
        //if (m_soilTemp[i] > 0) {
        if (m_soilTempprofile[i][kk] > 0) {
            // compute soil water factor (sut)
            if (m_soilWtrSto[i][kk] < 0) {
                m_soilWtrSto[i][kk] = 0.0000001f;
            }
            //sut = 0.1f + 0.9f * sqrt(m_soilWtrSto[i][kk] / m_soilFC[i][kk]);
            const float awcRatio = Max(0.f, SafePositiveDivide(
                m_soilWtrSto[i][kk], m_soilAWC[i][kk], 0.f));
            sut = 0.1f + 0.9f * sqrt(awcRatio);
            sut = Max(0.05f, sut);

            //compute soil temperature factor
            //variable to hold intermediate calculation result (xx)
            float xx = 0.f;
            //soil temperature factor (cdg)
            float cdg = 0.f;
            //xx = m_soilTemp[i];
            xx = m_soilTempprofile[i][kk];
            const float cdgDenom = xx + exp(9.93f - 0.312f * xx);
            cdg = 0.9f * SafePositiveDivide(xx, cdgDenom, 0.f) + 0.1f;
            cdg = Max(0.1f, cdg);

            // compute combined factor
            xx = 0.f;
            //combined temperature/soil water factor (csf)
            float csf = 0.f;
            xx = cdg * sut;
            if (xx < 0) {
                xx = 0.f;
            }
            if (xx > 1000000) {
                xx = 1000000.f;
            }
            csf = sqrt(xx);

            // compute flow from active to stable pools
            //amount of nitrogen moving from active organic to stable organic pool in layer (rwn)
            float rwn = 0.f;
            rwn = 0.1e-4f * (m_soilActvOrgN[i][k] * (1.f / m_orgNFrActN - 1.f) - m_soilStabOrgN[i][k]);
            if (rwn > 0.f) {
                rwn = Min(rwn, m_soilActvOrgN[i][k]);
            } else {
                rwn = -(Min(Abs(rwn), m_soilStabOrgN[i][k]));
            }
            m_soilStabOrgN[i][k] = Max(1.e-6f, m_soilStabOrgN[i][k] + rwn);
            m_soilActvOrgN[i][k] = Max(1.e-6f, m_soilActvOrgN[i][k] - rwn);

            // compute humus mineralization on active organic n
            //amount of nitrogen moving from active organic nitrogen pool to nitrate pool in layer (hmn)
            float hmn = 0.f;
            hmn = m_minrlCoef * csf * m_soilActvOrgN[i][k];
            hmn = Min(hmn, m_soilActvOrgN[i][k]);
            // compute humus mineralization on active organic p
            xx = 0.f;
            //amount of phosphorus moving from the organic pool to the labile pool in layer (hmp)
            float hmp = 0.f;
            xx = m_soilStabOrgN[i][k] + m_soilActvOrgN[i][k];
            if (xx > 1.e-6f) {
                hmp = 1.4f * hmn * m_soilHumOrgP[i][k] / xx;
            } else {
                hmp = 0.f;
            }
            hmp = Min(hmp, m_soilHumOrgP[i][k]);

            // move mineralized nutrients between pools;
            m_soilActvOrgN[i][k] = Max(1.e-6f, m_soilActvOrgN[i][k] - hmn);
            m_soilNO3[i][k] = m_soilNO3[i][k] + hmn;
            m_soilHumOrgP[i][k] = m_soilHumOrgP[i][k] - hmp;
            m_soilSolP[i][k] = m_soilSolP[i][k] + hmp;

            // compute residue decomp and mineralization of
            // fresh organic n and p (upper two layers only)
            //amount of nitrogen moving from fresh organic to nitrate(80%) and active organic(20%) pools in layer (rmn1)
            float rmn1 = 0.f;
            //amount of phosphorus moving from fresh organic to labile(80%) and organic(20%) pools in layer (rmp)
            float rmp = 0.f;
            if (k <= 2) {
                //the C:N ratio (cnr)
                float cnr = 0.f;
                //the C:P ratio (cnr)
                float cpr = 0.f;
                //the nutrient cycling water factor for layer (ca)
                float ca = 0.f;
                float cnrf = 0.f;
                float cprf = 0.f;
                if (m_soilFrshOrgN[i][k] + m_soilNO3[i][k] > 1e-4f) {
                    //Calculate cnr, equation 3:1.2.5 in SWAT Theory 2009, p189
                    cnr = 0.58f * m_soilRsd[i][k] / (m_soilFrshOrgN[i][k] + m_soilNO3[i][k]);
                    if (cnr > 500) {
                        cnr = 500.f;
                    }
                    cnrf = exp(-0.693f * (cnr - 25.f) * 0.04f);
                } else {
                    cnrf = 1.f;
                }
                if (m_soilFrshOrgP[i][k] + m_soilSolP[i][k] > 1e-4f) {
                    cpr = 0.58f * m_soilRsd[i][k] / (m_soilFrshOrgP[i][k] + m_soilSolP[i][k]);
                    if (cpr > 5000) {
                        cpr = 5000.f;
                    }
                    cprf = 0.f;
                    cprf = exp(-0.693f * (cpr - 200.f) * 0.005f);
                } else {
                    cprf = 1.f;
                }
                //decay rate constant (decr)
                float decr = 0.f;

                float rdc = 0.f;
                //Calculate ca, equation 3:1.2.8 in SWAT Theory 2009, p190
                ca = Min(Min(cnrf, cprf), 1.f);
                //if (m_landcover[i] > 0)
                //{
                //    decr = m_rsdco_pl[(int) m_landcover[i]] * ca * csf;
                //} }else{
                //{
                //    decr = 0.05f;
                //}
                if (m_pltRsdDecCoef[i] < 0.f) {
                    decr = 0.05f;
                } else {
                    decr = m_pltRsdDecCoef[i] * ca * csf;
                }
                decr = Min(decr, 1.f);
                m_soilRsd[i][k] = Max(1.e-6f, m_soilRsd[i][k]);
                rdc = decr * m_soilRsd[i][k];
                m_soilRsd[i][k] = m_soilRsd[i][k] - rdc;
                if (m_soilRsd[i][k] < 0)m_soilRsd[i][k] = 0.f;
                rmn1 = decr * m_soilFrshOrgN[i][k];
                m_soilFrshOrgP[i][k] = Max(1.e-6f, m_soilFrshOrgP[i][k]);
                rmp = decr * m_soilFrshOrgP[i][k];

                m_soilFrshOrgP[i][k] = m_soilFrshOrgP[i][k] - rmp;
                m_soilFrshOrgN[i][k] = Max(1.e-6f, m_soilFrshOrgN[i][k]) - rmn1;

                //Calculate no3, aorgn, solp, orgp, equation 3:1.2.9 in SWAT Theory 2009, p190
                m_soilNO3[i][k] = m_soilNO3[i][k] + 0.8f * rmn1;
                m_soilActvOrgN[i][k] = m_soilActvOrgN[i][k] + 0.2f * rmn1;
                m_soilSolP[i][k] = m_soilSolP[i][k] + 0.8f * rmp;
                m_soilHumOrgP[i][k] = m_soilHumOrgP[i][k] + 0.2f * rmp;
            }
            // compute denitrification
            // amount of nitrogen lost from nitrate pool in layer due to denitrification
            float wdn = 0.f;
            //Calculate wdn, equation 3:1.4.1 and 3:1.4.2 in SWAT Theory 2009, p194
            if (sut >= m_denitThres) {
                wdn = m_soilNO3[i][k] * (1.f - exp(-m_denitCoef * cdg * m_soilCbn[i][k]));
            } else {
                wdn = 0.f;
            }
            m_soilNO3[i][k] = m_soilNO3[i][k] - wdn;

            // Summary calculations
            m_wshd_hmn = m_wshd_hmn + hmn * m_cellAreaFr;
            m_wshd_rwn = m_wshd_rwn + rwn * m_cellAreaFr;
            m_wshd_hmp = m_wshd_hmp + hmp * m_cellAreaFr;
            m_wshd_rmn = m_wshd_rmn + rmn1 * m_cellAreaFr;
            m_wshd_rmp = m_wshd_rmp + rmp * m_cellAreaFr;
            m_wshd_dnit = m_wshd_dnit + wdn * m_cellAreaFr;
            m_hmntl[i] = hmn;
            m_rwntl[i] = rwn;
            m_hmptl[i] = hmp;
            m_rmn2tl[i] = rmn1;
            m_rmptl[i] = rmp;
            m_wdntl[i] = wdn;
        }
    }
}

void Nutrient_Transformation::MineralizationCfarmOneCarbonModel(const int i) {
    /// TODO
}

void Nutrient_Transformation::Volatilization(const int i) {
    float kk = 0.f;
    for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        //nitrification/volatilization temperature factor (nvtf)
        float nvtf = 0.f;
        //Calculate nvtf, equation 3:1.3.1 in SWAT Theory 2009, p192
        //nvtf = 0.41f * (m_soilTemp[i] - 5.f) * 0.1f;
        nvtf = 0.41f * (m_soilTempprofile[i][k] - 5.f) * 0.1f;
        if (m_soilNH4[i][k] > 0.f && nvtf >= 0.001f) {
            float sw25 = 0.f;
            float swwp = 0.f;
            //nitrification soil water factor (swf)
            float swf = 0.f;
            //Calculate nvtf, equation 3:1.3.2 and 3:1.3.3 in SWAT Theory 2009, p192
            //sw25 = m_soilWP[i][k] + 0.25f * m_soilFC[i][k];
            sw25 = m_soilWP[i][k] + 0.25f * m_soilAWC[i][k];
            swwp = m_soilWP[i][k] + m_soilWtrSto[i][k];
            if (swwp < sw25) {
                swf = SafePositiveDivide(swwp - m_soilWP[i][k],
                                         sw25 - m_soilWP[i][k],
                                         0.f);
            } else {
                swf = 1.f;
            }
            kk = k == 0 ? 0.f : m_soilDepth[i][k - 1];
            //depth from the soil surface to the middle of the layer (dmidl/mm)
            float dmidl = 0.f;
            //volatilization depth factor (dpf)
            float dpf = 0.f;
            //nitrification regulator (akn)
            float akn = 0.f;
            //volatilization regulator (akn)
            float akv = 0.f;
            //amount of ammonium converted via nitrification and volatilization in layer (rnv)
            float rnv = 0.f;
            //amount of nitrogen moving from the NH3 to the NO3 pool (nitrification) in the layer (rnit)
            float rnit = 0.f;
            //amount of nitrogen lost from the NH3 pool due to volatilization (rvol)
            float rvol = 0.f;
            //volatilization CEC factor (cecf)
            float cecf = 0.15f;
            //Calculate nvtf, equation 3:1.3.2 and 3:1.3.3 in SWAT Theory 2009, p192
            dmidl = (m_soilDepth[i][k] + kk) * 0.5f;
            dpf = 1.f - dmidl / (dmidl + exp(4.706f - 0.0305f * dmidl));
            //Calculate rnv, equation 3:1.3.6, 3:1.3.7 and 3:1.3.8 in SWAT Theory 2009, p193
            akn = nvtf * swf;
            akv = nvtf * dpf * cecf;
            rnv = m_soilNH4[i][k] * (1.f - exp(-akn - akv));
            //Calculate rnit, equation 3:1.3.9 in SWAT Theory 2009, p193
            rnit = 1.f - exp(-akn);
            //Calculate rvol, equation 3:1.3.10 in SWAT Theory 2009, p193
            rvol = 1.f - exp(-akv);

            //calculate nitrification (NH3 => NO3)
            if (rvol + rnit > 1.e-6f) {
                //Calculate the amount of nitrogen removed from the ammonium pool by nitrification,
                //equation 3:1.3.11 in SWAT Theory 2009, p193
                rvol = rnv * rvol / (rvol + rnit);
                //Calculate the amount of nitrogen removed from the ammonium pool by volatilization,
                //equation 3:1.3.12 in SWAT Theory 2009, p194
                rnit = rnv - rvol;
                if (rnit < 0)rnit = 0.f;
                m_soilNH4[i][k] = Max(1e-6f, m_soilNH4[i][k] - rnit);
            }
            if (m_soilNH4[i][k] < 0.f) {
                rnit = rnit + m_soilNH4[i][k];
                m_soilNH4[i][k] = 0.f;
            }
            //debug
            //if (rnit != rnit)
            //	cout<<"cellid: "<<i<<", lyr: "<<k<<", akn: "<<akn<<", nvtf:"<<
            //	nvtf<<", swf: "<<swf<<", dpf: "<<dpf<<", cecf: "<<cecf<<endl;
            m_soilNO3[i][k] = m_soilNO3[i][k] + rnit;
            //calculate ammonia volatilization
            m_soilNH4[i][k] = Max(1e-6f, m_soilNH4[i][k] - rvol);
            if (m_soilNH4[i][k] < 0.f) {
                rvol = rvol + m_soilNH4[i][k];
                m_soilNH4[i][k] = 0.f;
            }
            //summary calculations
            m_wshd_voln += rvol * m_cellAreaFr;
            m_wshd_nitn += rnit * m_cellAreaFr;
        }
    }
}

void Nutrient_Transformation::CalculatePflux(const int i) {
    for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        // make sure that no zero or negative pool values come in
        if (m_soilSolP[i][k] <= 1.e-6) m_soilSolP[i][k] = 1.e-6f;
        if (m_soilActvMinP[i][k] <= 1.e-6) m_soilActvMinP[i][k] = 1.e-6f;
        if (m_soilStabMinP[i][k] <= 1.e-6) m_soilStabMinP[i][k] = 1.e-6f;

        // Convert kg/ha to ppm so that it is more meaningful to compare between soil layers
        float solp = 0.f;
        float actp = 0.f;
        float stap = 0.f;
        if (m_conv_wt[i][k] > UTIL_ZERO) {
            solp = SafePositiveDivide(m_soilSolP[i][k], m_conv_wt[i][k], 0.f) * 1000000.f;
            actp = SafePositiveDivide(m_soilActvMinP[i][k], m_conv_wt[i][k], 0.f) * 1000000.f;
            stap = SafePositiveDivide(m_soilStabMinP[i][k], m_conv_wt[i][k], 0.f) * 1000000.f;
        }

        // ***************Soluble - Active Transformations***************
        // Dynamic PSP Ratio
        float psp = 0.f;
        //cout << psp << endl;
        //PSP = -0.045*log (% clay) + 0.001*(Solution P, mg kg-1) - 0.035*(% Organic C) + 0.43
        if (m_soilClay[i][k] > 0.f) {
            psp = -0.045f * log(m_soilClay[i][k]) + 0.001f * solp;
            psp = psp - 0.035f * m_soilCbn[i][k] + 0.43f;
        } else {
            psp = 0.4f;
        }
        // Limit PSP range
        if (psp < .1f) psp = 0.1f;
        if (psp > 0.7f) psp = 0.7f;

        // Calculate smoothed PSP average
        if (m_phpSorpIdxBsn > 0.f) psp = (m_phpSorpIdxBsn_temp[i] * 29.f + psp * 1.f) * 0.03333333333333333f; // 1. / 30.
        // Store PSP for tomorrow's smoothing calculation
        m_phpSorpIdxBsn_temp[i]  = psp;

        //***************Dynamic Active/Soluble Transformation Coeff******************
        // Calculate P balance
        float rto = psp / (1.f - psp);
        float rmn1 = 0.f;
        rmn1 = m_soilSolP[i][k] - m_soilActvMinP[i][k] * rto; // P imbalance
        // Move P between the soluble and active pools based on vadas et al., 2006
        if (rmn1 >= 0.f) {
            // Net movement from soluble to active
            rmn1 = Max(rmn1, (-1 * m_soilSolP[i][k]));
            // Calculate dynamic coefficient
            float vara = 0.918f * exp(-4.603f * psp);
            float varb = -0.238f * log(vara) - 1.126f;
            float arate = 0.f;
            if (m_phpApldDays[i] > 0) {
                arate = vara * pow(m_phpApldDays[i], varb);
            } else {
                arate = vara; // ? SWAT code is: arate = vara * (1) ** varb; ?
            }
            // limit rate coefficient from 0.05 to .5 helps on day 1 when a_days is zero
            if (arate > 0.5f) arate = 0.5f;
            if (arate < 0.1f) arate = 0.1f;
            rmn1 = arate * rmn1;
            m_phpApldDays[i] += 1.f; // add a day to the imbalance counter
            m_phpDefDays[i] = 0;
        }

        if (rmn1 < 0.f) {
            // Net movement from Active to Soluble
            rmn1 = Min(rmn1, m_soilActvMinP[i][k]);
            // Calculate dynamic coefficient
            float base = 0.f;
            float varc = 0.f;
            base = -1.08f * psp + 0.79f;
            varc = base * 0.7482635675785653f; // varc = base * exp(-0.29f);
            // limit varc from 0.1 to 1
            if (varc > 1.f) varc = 1.f;
            if (varc < .1f) varc = .1f;
            rmn1 *= varc;
            m_phpApldDays[i] = 0.f;
            m_phpDefDays[i] = m_phpDefDays[i] + 1.f; // add a day to the imbalance counter
        }

        //*************** Active - Stable Transformations ******************
        // Estimate active stable transformation rate coeff
        // original value was .0006
        // based on linear regression rate coeff = 0.005 @ 0% CaCo3 0.05 @ 20% CaCo3
        float as_p_coeff = 0.f;
        float sol_cal = 2.8f;
        as_p_coeff = 0.0023f * sol_cal + 0.005f;
        if (as_p_coeff > 0.05f) as_p_coeff = 0.05f;
        if (as_p_coeff < 0.002f) as_p_coeff = 0.002f;
        // Estimate active/stable pool ratio
        // Generated from sharpley 2003
        float xx = 0.f;
        float ssp = 0.f;
        xx = actp + actp * rto;
        if (xx > 1.e-6f) ssp = 25.044f * pow(xx, -0.3833f);
        // limit ssp to range in measured data
        if (ssp > 10.f) ssp = 10.f;
        if (ssp < 0.7f) ssp = 0.7f;
        // Smooth ssp, no rapid changes
        if (m_ssp_store[i][k] > 0.f) ssp = (ssp + m_ssp_store[i][k] * 99.f) * 0.01f;
        float roc = 0.f;
        roc = ssp * (m_soilActvMinP[i][k] + m_soilActvMinP[i][k] * rto);
        roc = roc - m_soilStabMinP[i][k];
        roc = as_p_coeff * roc;
        // Store todays ssp for tomorrow's calculation
        m_ssp_store[i][k] = ssp;

        // **************** Account for Soil Water content, do not allow movement in dry soil************
        float wetness = 0.f;
        //wetness = m_soilWtrSto[i][k] / m_soilFC[i][k]; // range from 0-1 1 = field cap
        wetness = SafePositiveDivide(m_soilWtrSto[i][k],
                                      m_soilAWC[i][k],
                                      0.25f); // range from 0-1, 1 = field capacity
        if (wetness > 1.f) wetness = 1.f;
        if (wetness < 0.25f) wetness = 0.25f;
        rmn1 *= wetness;
        roc *= wetness;

        // If total P is greater than 10,000 mg/kg do not allow transformations at all
        if (solp + actp + stap < 10000.f) {
            // Allow P Transformations
            m_soilStabMinP[i][k] = m_soilStabMinP[i][k] + roc;
            if (m_soilStabMinP[i][k] < 0.f) m_soilStabMinP[i][k] = 0.f;
            m_soilActvMinP[i][k] = m_soilActvMinP[i][k] - roc + rmn1;
            if (m_soilActvMinP[i][k] < 0.f) m_soilActvMinP[i][k] = 0.f;
            m_soilSolP[i][k] = m_soilSolP[i][k] - rmn1;
            if (m_soilSolP[i][k] < 0.f) m_soilSolP[i][k] = 0.f;

            // Add water soluble P pool assume 1:5 ratio based on sharpley 2005 et al
            m_wshd_pas += roc * m_cellAreaFr;
            m_wshd_pal += rmn1 * m_cellAreaFr;
            m_roctl[i] += roc;
            m_rmp1tl[i] += rmn1;
        }
    }
}

void Nutrient_Transformation::MineralizationCenturyModel(const int i) {
    /// update tillage related variables if stated. Code from subbasin.f of SWAT, line 153-164
    /// if CENTURY model, and tillage operation has been operated
    if (m_tillDays != nullptr && m_tillDays[i] > 0.f) {
        if (m_tillDays[i] >= 30.f) {
            m_tillSwitch[i] = 0.f;
            m_tillDays[i] = 0.f;
        } else {
            m_tillDays[i] += 1.f;
        }
    }
    /******  Local variables  ********/
    // ABCO2   : allocation from biomass to CO2; 0.6 (surface litter), 0.85?.68*(CLAF + SILF) (all other layers) (Parton et al., 1993, 1994)
    // ABL     : carbon allocation from biomass to leaching; ABL = (1-exp(-f/(0.01* SW+ 0.1*(KdBM)*DB)) (Williams, 1995)
    // ABP     : allocation from biomass to passive humus; 0 (surface litter), 0.003 + 0.032*CLAF (all other layers) (Parton et al., 1993, 1994)
    // ALMCO2  : allocation from metabolic litter to CO2; 0.6 (surface litter), 0.55 (all other layers) (Parton et al., 1993, 1994)
    // ALSLCO2 : allocation from lignin of structural litter to CO2; 0.3 (Parton et al., 1993, 1994)
    // ALSLNCO2: allocation from non-lignin of structural litter to CO2; 0.6 (surface litter), 0.55 (all other layers) (Parton et al., 1993, 1994)
    // APCO2   : allocation from passive humus to CO2; 0.55 (Parton et al., 1993, 1994)
    // ASCO2   : allocation from slow humus to CO2; 0.55 (Parton et al., 1993, 1994)
    // ASP     : allocation from slow humus to passive; 0 (surface litter), 0.003-0.009*CLAF (all other layers) (Parton et al., 1993, 1994)
    // BMC     : mass of C in soil microbial biomass and associated products (kg ha-1)
    // BMCTP   : potential transformation of C in microbial biomass (kg ha-1 day-1)
    // BMN     : mass of N in soil microbial biomass and associated products (kg ha-1)
    // BMNTP   : potential transformation of N in microbial biomass (kg ha-1 day-1)
    // CDG     : soil temperature control on biological processes
    // CNR     : C/N ratio of standing dead
    // CS      : combined factor controlling biological processes [CS = sqrt(CDG SUT)* 0.8*OX*X1), CS < 10; CS = 10, CS>=10 (Williams, 1995)]
    // DBp     : soil bulk density of plow layer (Mg m-3) (Not used)
    // HSCTP   : potential transformation of C in slow humus (kg ha-1 day-1)
    // HSNTP   : potential transformation of N in slow humus (kg ha-1 day-1)
    // HPCTP   : potential transformation of C in passive humus (kg ha-1 day-1)
    // HPNTP   : potential transformation of N in passive humus (kg ha-1 day-1)
    // LMF     : fraction of the litter that is metabolic
    // LMNF    : fraction of metabolic litter that is N (kg kg-1)
    // LMCTP   : potential transformation of C in metabolic litter (kg ha-1 day-1)
    // LMNTP   : potential transformation of N in metabolic litter (kg ha-1 day-1)
    // LSCTP   : potential transformation of C in structural litter (kg ha-1 day-1)
    // LSF     : fraction of the litter that is structural
    // LSLF    : fraction of structural litter that is lignin (kg kg-1)
    // LSNF    : fraction of structural litter that is N (kg kg-1)
    // LSLCTP  : potential transformation of C in lignin of structural litter (kg ha-1 day-1)
    // LSLNCTP : potential transformation of C in nonlignin structural litter (kg ha-1 day-1)
    // LSNTP   : potential transformation of N in structural litter (kg ha-1 day-1)
    // NCBM    : N/C ratio of biomass
    // NCHP    : N/C ratio passive humus
    // NCHS    : N/C ratio of the slow humus
    // OX      : oxygen control on biological processes with soil depth
    // SUT     : soil water control on biological processes
    // X1      : tillage control on residue decomposition (Not used)
    // XBMT    : control on transformation of microbial biomass by soil texture and structure.
    //           Its values: surface litter layer = 1; all other layers = 1-0.75*(SILF + CLAF) (Parton et al., 1993, 1994)
    // XLSLF   : control on potential transformation of structural litter by lignin fraction of structural litter [XLSLF = exp(-3* LSLF) (Parton et al., 1993, 1994)]
    float x1 = 0.f, xx = 0.f;
    float LSR = 0.f, BMR = 0.f, HSR = 0.f, HPR = 0.f, RLR = 0.f;
    float LSCTA = 0.f, LSLCTA = 0.f, LSLNCTA = 0.f, LSNTA = 0.f, XBM = 0.f;
    float LMCTA = 0.f, LMNTA = 0.f, BMCTA = 0.f, BMNTA = 0.f, HSCTA = 0.f, HSNTA = 0.f, HPCTA = 0.f, HPNTA = 0.f;
    float LSCTP = 0.f, LSLCTP = 0.f, LSLNCTP = 0.f, LSNTP = 0.f, LMR = 0.f, LMCTP = 0.f;
    float LMNTP = 0.f, BMCTP = 0.f, BMNTP = 0.f, HSCTP = 0.f, HSNTP = 0.f, HPCTP = 0.f, HPNTP = 0.f;
    float NCHP = 0.f, NCBM = 0.f, NCHS = 0.f;
    float ABCO2 = 0.f, A1CO2 = 0.f, APCO2 = 0.f, ASCO2 = 0.f, ABP = 0.f, ASP = 0.f, A1 = 0.f, ASX = 0.f, APX = 0.f;
    float PRMT_51 = 0.f, PRMT_45 = 0.f;
    float DF1 = 0.f, DF2 = 0.f, DF3 = 0.f, DF4 = 0.f, DF5 = 0.f, DF6 = 0.f;
    float ADD = 0.f, ADF1 = 0.f, ADF2 = 0.f, ADF3 = 0.f, ADF4 = 0.f, ADF5 = 0.f;
    float TOT = 0.f, PN1 = 0.f, PN2 = 0.f, PN3 = 0.f, PN5 = 0.f, PN6 = 0.f, PN7 = 0.f, PN8 = 0.f, PN9 = 0.f;
    float SUM = 0.f, CPN1 = 0.f, CPN2 = 0.f, CPN3 = 0.f, CPN4 = 0.f, CPN5 = 0.f, RTO = 0.f;
    /*float wdn = 0.f;*/
    // SNMN = 0.f, PN4 = 0.f, ALMCO2 = 0.f, ALSLCO2 = 0.f, ALSLNCO2 = 0.f, LSLNCAT = 0.f . NOT used in this module
    // XBMT = 0.f, XLSLF = 0.f, LSLF = 0.f, LSF = 0.f, LMF = 0.f, x3 = 0.f
    /// calculate tillage factor using DSSAT
    if (m_tillSwitch[i] == 1 && m_tillDays[i] <= 30.f) {
        m_tillFactor[i] = 1.6f;
    } else {
        m_tillFactor[i] = 1.f;
    }

    /// calculate C/N dynamics for each soil layer
    const int nSoilLyrs = CVT_INT(m_nSoilLyrs[i]);
    for (int k = 0; k < nSoilLyrs; k++) {
		// RSPC is a daily flux, not a persistent carbon pool.
	    // Reset first so an inactive/cold layer does not retain yesterday's RSPC.
		m_sol_RSPC[i][k] = 0.f;
        // If layer 0 exists alone, use itself rather than reading layer 1 out of bounds.
        int kk = (k == 0 && nSoilLyrs > 1) ? 1 : k;
        // mineralization can occur only if temp above 0 deg
        // check sol_st soil water content in each soil ayer mm H2O
        // if (m_soilTemp[i] > 0.f && m_soilWtrSto[i][k] > 0.f) {
        if (m_soilTempprofile[i][k] > 0.f && m_soilWtrSto[i][k] > 0.f) {
            // from Armen
            // compute soil water factor - sut
            // float fc = m_sol_awc[i][k] + m_sol_wpmm[i][k];            // units mm
            float wc = m_soilWtrSto[i][k] + m_soilWP[i][k];    // units mm
            float sat = m_soilSat[i][k] + m_soilWP[i][k];      // units mm
            const float wcSatRatio = SafePositiveDivide(wc, sat, 0.f);
            float voidfr = m_soilPor[i][k] * (1.f - wcSatRatio); // fraction

            float sut = 0.f;
            x1 = wc - m_soilWP[i][k];
            if (x1 < 0.f) {
                const float wpRatio = Max(0.f, SafePositiveDivide(
                    m_soilWtrSto[i][kk], m_soilWP[i][k], 0.f));
                sut = .1f * pow(wpRatio, 2.f);
            } else {
                //sut = .1f + .9f * sqrt(m_soilWtrSto[i][kk] / m_soilFC[i][k]);
                const float awcRatio = Max(0.f, SafePositiveDivide(
                    m_soilWtrSto[i][kk], m_soilAWC[i][k], 0.f));
                sut = .1f + .9f * sqrt(awcRatio);
            }
            sut = Min(1.f, sut);
            sut = Max(.05f, sut);

            //compute tillage factor (x1)
            x1 = 1.f;
            // calculate tillage factor using DSSAT
            if (m_tillSwitch[i] == 1 && m_tillDays[i] <= 30.f) {
                if (k == 1) {
                    x1 = 1.6f;
                } else {
                    if (m_soilDepth[i][k] >= m_tillDepth[i]) {
                        x1 = 1.6f;
                    } else if (k > 0 && m_soilDepth[i][k - 1] > m_tillDepth[i]) {
                        x1 = 1.f + 0.6f * SafePositiveDivide(
                            m_tillDepth[i] - m_soilDepth[i][k - 1],
                            m_soilThk[i][k],
                            0.f);
                    }
                }
            } else {
                x1 = 1.f;
            }
            //compute soil temperature factor
            float cdg = 0.f;
            /// cdg = m_sote[i] / (m_sote[i] + exp(5.058459f - 0.2503591f * m_sote[i]));
            /* cdg is calculated by function fcgd in carbon_new.f of SWAT, by Armen
             * i.e., Function fcgd(xx)
             */
            // cdg = pow(m_soilTemp[i] + 5.f, 8.f/3.f) * (50.f - m_soilTemp[i]) / (pow(40.f, 8.f/3.f) * 15.f);
            //cdg = pow(m_soilTemp[i] + 5.f, _8div3) * (50.f - m_soilTemp[i]) * 3.562449888909787e-06f;
            cdg = pow(m_soilTempprofile[i][kk] + 5.f, _8div3) * (50.f - m_soilTempprofile[i][kk]) * 3.562449888909787e-06f;
            if (cdg < 0.f) cdg = 0.f;

            //compute oxygen (OX)
            float ox = 0.f;
            // Use zero as the upper boundary of the first layer to avoid kk - 1.
            const float upperDepth = kk > 0 ? m_soilDepth[i][kk - 1] : 0.f;
            const float middleDepth = 0.5f * (m_soilDepth[i][kk] + upperDepth);
            const float oxygenDenom = middleDepth +
                exp(18.40961f - 0.023683632f * middleDepth);
            ox = 1.f - 0.8f * SafePositiveDivide(
                middleDepth, oxygenDenom, 0.f);
            // compute combined factor
            float cs = 0.f;
            cs = Min(10.f, sqrt(cdg * sut) * 0.9f * ox * x1);
            // call denitrification (to use void fraction and cdg factor)
            // repetitive computation, commented by LJ
            // cdg = pow((m_soilTemp[i]) + 5.f, 8.f / 3.f) * (50.f - m_soilTemp[i]) / (pow(40.f, 8.f / 3.f) * 15.f);
			// Denitrification is a current-layer flux.
			float wdn = 0.f;
            if (cdg > 0.f && voidfr <= 0.1f) {
                // call ndenit(k,j,cdg,wdn,void);
                // rewrite from ndenit.f of SWAT
                float vof = 1.f / (1.f + pow(voidfr * 25.f, 5.f));
                wdn = m_soilNO3[i][k] * (1.f - exp(-m_denitCoef * cdg * vof * m_soilCbn[i][k]));
                m_soilNO3[i][k] -= wdn;
            }
            /*m_wshd_dnit += wdn * m_cellAreaFr;
            m_wdntl[i] += wdn;*/

            float sol_min_n = m_soilNO3[i][k] + m_soilNH4[i][k];
            //if(i == 100 && k == 0) cout << m_sol_LSL[i][k] << m_sol_LS[i][k] << endl;
            // lignin content in structural litter (fraction)
            RLR = Min(0.8f, m_sol_LSL[i][k] / (m_sol_LS[i][k] + 1.e-5f));
            // HSR=PRMT(47) !CENTURY SLOW HUMUS TRANSFORMATION RATE D^-1(0.00041_0.00068) ORIGINAL VALUE = 0.000548,
            HSR = 5.4799998e-4f; //ljj note note5 years
            // HPR=PRMT(48) !CENTURY PASSIVE HUMUS TRANSFORMATION RATE D^-1(0.0000082_0.000015) ORIGINAL VALUE = 0.000012
            HPR = 1.2e-5f; //ljj note 200+years
            APCO2 = .55f;
            ASCO2 = .60f;
            PRMT_51 = 0.f; // COEF ADJUSTS MICROBIAL ACTIVITY FUNCTION IN TOP SOIL LAYER (0.1_1.),
            PRMT_51 = 1.f;
            //The following codes are calculating of the N:C ration in the newly formed SOM for each pool
            //please note that in the surface layer, no new materials enter Passive pool, therefore, no NCHP is
            //calculated for the first layer.
            if (k == 0) {
                cs = cs * PRMT_51;
                ABCO2 = .55f;
                A1CO2 = .55f;
                BMR = .0164f;
                LMR = .0405f;
                LSR = .0107f;
                NCHP = .1f;
                XBM = 1.f;
                // COMPUTE N/C RATIOS
                // relative nitrogen content in residue (%)
                x1 = 0.1f * (m_sol_LSN[i][k] + m_sol_LMN[i][k]) / (m_soilRsd[i][k] / 1000.f + 1.e-5f);
                if (x1 > 2.f) {
                    NCBM = .1f;
                } else if (x1 > .01f) {
                    NCBM = 1.f / (20.05f - 5.0251f * x1);
                } else {
                    NCBM = .05f;
                }
                NCHS = NCBM / (5.f * NCBM + 1.f);
            } else {
                ABCO2 = 0.17f + 0.0068f * m_soilSand[i][k];
                A1CO2 = .55f;
                BMR = .02f;
                LMR = .0507f;
                LSR = .0132f;
                XBM = .25f + .0075f * m_soilSand[i][k];

                x1 = SafePositiveDivide(1000.f * sol_min_n,
                                        m_soilMass[i][k] * 0.001f,
                                        0.f);
                if (x1 > 7.15f) {
                    NCBM = .33f;
                    NCHS = .083f;
                    NCHP = .143f;
                } else {
                    NCBM = 1.f / (15.f - 1.678f * x1);
                    NCHS = 1.f / (20.f - 1.119f * x1);
                    NCHP = 1.f / (10.f - .42f * x1);
                }
            }
            ABP = .003f + .00032f * m_soilClay[i][k];

            PRMT_45 = 0.f; // COEF IN CENTURY EQ ALLOCATING SLOW TO PASSIVE HUMUS(0.001_0.05) ORIGINAL VALUE = 0.003,
            PRMT_45 = 5.0000001e-2f;
            ASP = Max(.001f, PRMT_45 - .00009f * m_soilClay[i][k]);
            // POTENTIAL TRANSFORMATIONS STRUCTURAL LITTER
            x1 = LSR * cs * exp(-3.f * RLR);
            LSCTP = x1 * m_sol_LSC[i][k];
            LSLCTP = LSCTP * RLR;
            LSLNCTP = LSCTP * (1.f - RLR);
            LSNTP = x1 * m_sol_LSN[i][k];
            // POTENTIAL TRANSFORMATIONS METABOLIC LITTER
            x1 = LMR * cs;
            LMCTP = m_sol_LMC[i][k] * x1;
            LMNTP = m_sol_LMN[i][k] * x1;
            // POTENTIAL TRANSFORMATIONS MICROBIAL BIOMASS
            x1 = BMR * cs * XBM;
            BMCTP = m_sol_BMC[i][k] * x1;
            BMNTP = m_sol_BMN[i][k] * x1;
            // POTENTIAL TRANSFORMATIONS SLOW HUMUS
            x1 = HSR * cs;
            HSCTP = m_sol_HSC[i][k] * x1;
            HSNTP = m_sol_HSN[i][k] * x1;
            // POTENTIAL TRANSFORMATIONS PASSIVE HUMUS
            x1 = cs * HPR;
            HPCTP = m_sol_HPC[i][k] * x1;
            HPNTP = m_sol_HPN[i][k] * x1;
            // ESTIMATE N DEMAND
            A1 = 1.f - A1CO2;
            ASX = 1.f - ASCO2 - ASP;
            APX = 1.f - APCO2;

            PN1 = LSLNCTP * A1 * NCBM; // Structural Litter to Biomass
            PN2 = .7f * LSLCTP * NCHS; // Structural Litter to Slow
            PN3 = LMCTP * A1 * NCBM;   // Metabolic Litter to Biomass
            // PN4 = BMCTP*ABL*NCBM;                // Biomass to Leaching (calculated in NCsed_leach)
            PN5 = BMCTP * ABP * NCHP;                 // Biomass to Passive
            PN6 = BMCTP * (1.f - ABP - ABCO2) * NCHS; // Biomass to Slow
            PN7 = HSCTP * ASX * NCBM;                 // Slow to Biomass
            PN8 = HSCTP * ASP * NCHP;                 // Slow to Passive
            PN9 = HPCTP * APX * NCBM;                 // Passive to Biomass

            // COMPARE SUPPLY AND DEMAND FOR N
            SUM = 0.f;
			// Must be reset for every soil layer.
            // Otherwise the N deficit from the previous layer is retained.
			CPN1 = 0.f;
			CPN2 = 0.f;
			CPN3 = 0.f;
			CPN4 = 0.f;
			CPN5 = 0.f;
            x1 = PN1 + PN2;
            if (LSNTP < x1) { CPN1 = x1 - LSNTP; } else { SUM = SUM + LSNTP - x1; }
            if (LMNTP < PN3) { CPN2 = PN3 - LMNTP; } else { SUM = SUM + LMNTP - PN3; }
            x1 = PN5 + PN6;
            if (BMNTP < x1) { CPN3 = x1 - BMNTP; } else { SUM = SUM + BMNTP - x1; }
            x1 = PN7 + PN8;
            if (HSNTP < x1) { CPN4 = x1 - HSNTP; } else { SUM = SUM + HSNTP - x1; }
            if (HPNTP < PN9) { CPN5 = PN9 - HPNTP; } else { SUM = SUM + HPNTP - PN9; }

            // total available N
            float Wmin = Max(1.e-5f, m_soilNO3[i][k] + m_soilNH4[i][k] + SUM);
            // total demand for potential tranformaiton of SOM;
            float DMDN = CPN1 + CPN2 + CPN3 + CPN4 + CPN5;
            float x3 = 1.f;
            // REDUCE DEMAND if SUPPLY LIMITS
            if (Wmin < DMDN) x3 = Wmin / DMDN;

			// ============================================================
            // DEBUG: Check nitrogen limitation factor x3
            // Only print the five wetland cells when x3 < 0.99
            // ============================================================
			const bool DEBUG_X3 = false;
			const bool debugX3Cell =
				(i == 1366 ||
					i == 1367 ||
					i == 1368 ||
					i == 1370 ||
					i == 1375);

			if (DEBUG_X3 && debugX3Cell && x3 < 0.99f) {

#pragma omp critical(X3_DEBUG_PRINT)
				{
					const std::time_t currentDate =
						static_cast<std::time_t>(m_date);

					std::tm dateInfo = {};

#ifdef _WIN32
					localtime_s(&dateInfo, &currentDate);
#else
					localtime_r(&currentDate, &dateInfo);
#endif

					char dateText[16] = {};

					std::strftime(
						dateText,
						sizeof(dateText),
						"%Y-%m-%d",
						&dateInfo
					);

					std::cout
						<< "[X3_N_LIMIT]"
						<< " DATE=" << dateText
						<< " CELL=" << i
						<< " LAYER=" << k
						<< " x3=" << x3
						<< " Wmin=" << Wmin
						<< " DMDN=" << DMDN
						<< " NO3=" << m_soilNO3[i][k]
						<< " NH4=" << m_soilNH4[i][k]
						<< std::endl;
				}
			}

            // ACTUAL TRANSFORMATIONS

            if (CPN1 > 0.f) {
                LSCTA = LSCTP * x3;
                LSNTA = LSNTP * x3;
                LSLCTA = LSLCTP * x3;
                LSLNCTA = LSLNCTP * x3;
            } else {
                LSCTA = LSCTP;
                LSNTA = LSNTP;
                LSLCTA = LSLCTP;
                //LSLNCAT = LSLNCTP;
				LSLNCTA = LSLNCTP;
            }
            if (CPN2 > 0.f) {
                LMCTA = LMCTP * x3;
                LMNTA = LMNTP * x3;
            } else {
                LMCTA = LMCTP;
                LMNTA = LMNTP;
            }
            if (CPN3 > 0.f) {
                BMCTA = BMCTP * x3;
                BMNTA = BMNTP * x3;
            } else {
                BMCTA = BMCTP;
                BMNTA = BMNTP;
            }
            if (CPN4 > 0.f) {
                HSCTA = HSCTP * x3;
                HSNTA = HSNTP * x3;
            } else {
                HSCTA = HSCTP;
                HSNTA = HSNTP;
            }
            if (CPN5 > 0.f) {
                HPCTA = HPCTP * x3;
                HPNTA = HPNTP * x3;
            } else {
                HPCTA = HPCTP;
                HPNTA = HPNTP;
            }

            // Recalculate demand using actual transformations revised from EPIC code by Zhang
            PN1 = LSLNCTA * A1 * NCBM; // Structural Litter to Biomass
            PN2 = .7f * LSLCTA * NCHS; // Structural Litter to Slow
            PN3 = LMCTA * A1 * NCBM;   // Metabolic Litter to Biomass
            //PN4 = BMCTP * ABL * NCBM;                // Biomass to Leaching (calculated in NCsed_leach)
            PN5 = BMCTA * ABP * NCHP;                 // Biomass to Passive
            PN6 = BMCTA * (1.f - ABP - ABCO2) * NCHS; // Biomass to Slow
            PN7 = HSCTA * ASX * NCBM;                 // Slow to Biomass
            PN8 = HSCTA * ASP * NCHP;                 // Slow to Passive
            PN9 = HPCTA * APX * NCBM;                 // Passive to Biomass
            // COMPARE SUPPLY AND DEMAND FOR N
            SUM = 0.f;
            CPN1 = 0.f;
            CPN2 = 0.f;
            CPN3 = 0.f;
            CPN4 = 0.f;
            CPN5 = 0.f;
            x1 = PN1 + PN2;
            if (LSNTA < x1) { CPN1 = x1 - LSNTA; } else { SUM = SUM + LSNTA - x1; }
            if (LMNTA < PN3) { CPN2 = PN3 - LMNTA; } else { SUM = SUM + LMNTA - PN3; }
            x1 = PN5 + PN6;
            if (BMNTA < x1) { CPN3 = x1 - BMNTA; } else { SUM = SUM + BMNTA - x1; }
            x1 = PN7 + PN8;
            if (HSNTA < x1) { CPN4 = x1 - HSNTA; } else { SUM = SUM + HSNTA - x1; }
            if (HPNTA < PN9) { CPN5 = PN9 - HPNTA; } else { SUM = SUM + HPNTA - PN9; }
            // total available N
            Wmin = Max(1.e-5f, m_soilNO3[i][k] + m_soilNH4[i][k] + SUM);
            // total demand for potential transformation of SOM
            DMDN = CPN1 + CPN2 + CPN3 + CPN4 + CPN5;

            // supply - demand
            m_sol_RNMN[i][k] = SUM - DMDN;
            // UPDATE
            if (m_sol_RNMN[i][k] > 0.f) {
                m_soilNH4[i][k] += m_sol_RNMN[i][k];
            } else {
                x1 = m_soilNO3[i][k] + m_sol_RNMN[i][k];
                if (x1 < 0.f) {
                    m_sol_RNMN[i][k] = -m_soilNO3[i][k];
                    m_soilNO3[i][k] = 1.e-6f;
                } else {
                    m_soilNO3[i][k] = x1;
                }
            }

            DF1 = LSNTA;
            DF2 = LMNTA;
            // DF represents Demand from
            // SNMN=SNMN+sol_RNMN[i][k]

            // calculate P flows
            // compute humus mineralization on active organic p
            float hmp = 0.f;
            float hmp_rate = 0.f;
            hmp_rate = 1.4f * (HSNTA + HPNTA) / (m_sol_HSN[i][k] + m_sol_HPN[i][k] + 1.e-6f);
            // hmp_rate = 1.4f * (HSNTA ) / (m_sol_HSN[i][k] + m_sol_HPN[i][k] + 1.e-6f);
            hmp_rate = Max(hmp_rate,0.f);
            hmp = hmp_rate * m_soilHumOrgP[i][k];
            hmp = Min(hmp, m_soilHumOrgP[i][k]);
            m_soilHumOrgP[i][k] = m_soilHumOrgP[i][k] - hmp;
            m_soilSolP[i][k] = m_soilSolP[i][k] + hmp;

            // compute residue decomposition and mineralization of
            // fresh organic n and p (upper two layers only)
            float rmp = 0.f;
            float decr = 0.f;
            decr = (LSCTA + LMCTA) / (m_sol_LSC[i][k] + m_sol_LMC[i][k] + 1.e-6f);
            decr = Min(1.f, decr);
            rmp = decr * m_soilFrshOrgP[i][k];

            m_soilFrshOrgP[i][k] = m_soilFrshOrgP[i][k] - rmp;
            m_soilSolP[i][k] = m_soilSolP[i][k] + .8f * rmp;
            m_soilHumOrgP[i][k] = m_soilHumOrgP[i][k] + .2f * rmp;
            // end of calculate P flows

            LSCTA = Min(m_sol_LSC[i][k], LSCTA);
            m_sol_LSC[i][k] = Max(1.e-10f, m_sol_LSC[i][k] - LSCTA);
            LSLCTA = Min(m_sol_LSLC[i][k], LSLCTA);
            m_sol_LSLC[i][k] = Max(1.e-10f, m_sol_LSLC[i][k] - LSLCTA);
            m_sol_LSLNC[i][k] = Max(1.e-10f, m_sol_LSLNC[i][k] - LSLNCTA);
            LMCTA = Min(m_sol_LMC[i][k], LMCTA);
            if (m_sol_LM[i][k] > 0.f) {
                RTO = Max(0.42f, m_sol_LMC[i][k] / m_sol_LM[i][k]);
                m_sol_LM[i][k] = m_sol_LM[i][k] - LMCTA / RTO;
                m_sol_LMC[i][k] = m_sol_LMC[i][k] - LMCTA;
            }
            m_sol_LSL[i][k] = Max(1.e-10f, m_sol_LSL[i][k] - LSLCTA * 2.380952380952381f); //  1. / .42f
            m_sol_LS[i][k] = Max(1.e-10f, m_sol_LS[i][k] - LSCTA * 2.380952380952381f);

            x3 = APX * HPCTA + ASX * HSCTA + A1 * (LMCTA + LSLNCTA);
            m_sol_BMC[i][k] = Max(1.e-10f, m_sol_BMC[i][k] - BMCTA + x3);
            DF3 = BMNTA - NCBM * x3;
            DF3 = Max(DF3,0.f);
            // DF3 is the supply of BMNTA - demand of N to meet the Passive, Slow, Metabolic, and Non-lignin Structural
            // C pools transformations into microbiomass pool
            x1 = .7f * LSLCTA + BMCTA * (1.f - ABP - ABCO2);
            m_sol_HSC[i][k] = Max(1.e-10f, m_sol_HSC[i][k] - HSCTA + x1);
            DF4 = HSNTA - NCHS * x1;
            DF4 = Max(DF4,0.f);
            // DF4 Slow pool supply of N - N demand for microbiomass C transformed into slow pool
            x1 = HSCTA * ASP + BMCTA * ABP;
            m_sol_HPC[i][k] = Max(.001f, m_sol_HPC[i][k] - HPCTA + x1);
            DF5 = HPNTA - NCHP * x1;
            DF5 = Max(DF5,0.f);
            // DF5 Passive pool demand of N - N demand for microbiomass C transformed into passive pool
            DF6 = sol_min_n - m_soilNO3[i][k] - m_soilNH4[i][k];
            DF6 = Max(DF6,0.f);
            // DF6 Supply of mineral N - available mineral N = N demanded from mineral pool

            ADD = DF1 + DF2 + DF3 + DF4 + DF5 + DF6;
            ADF1 = Abs(DF1);
            ADF2 = Abs(DF2);
            ADF3 = Abs(DF3);
            ADF4 = Abs(DF4);
            ADF5 = Abs(DF5);
            TOT = ADF1 + ADF2 + ADF3 + ADF4 + ADF5;
            xx = ADD / (TOT + 1.e-10f);

            m_sol_LSN[i][k] = Max(.001f, m_sol_LSN[i][k] - DF1 + xx * ADF1);
            m_sol_LMN[i][k] = Max(.001f, m_sol_LMN[i][k] - DF2 + xx * ADF2);
            m_sol_BMN[i][k] = Max(.001f, m_sol_BMN[i][k] - DF3 + xx * ADF3);
            m_sol_HSN[i][k] = Max(.001f, m_sol_HSN[i][k] - DF4 + xx * ADF4);
            m_sol_HPN[i][k] = Max(.001f, m_sol_HPN[i][k] - DF5 + xx * ADF5);

            m_sol_RSPC[i][k] = .3f * LSLCTA + A1CO2 * (LSLNCTA + LMCTA) +
                    ABCO2 * BMCTA + ASCO2 * HSCTA + APCO2 * HPCTA;


			// ---------------------------------------------------------------------
            // Temporary diagnostics for CENTURY RSPC production.
            // This block only writes diagnostic values and does not modify the model.
            // ---------------------------------------------------------------------
			const bool DEBUG_RSPC_SOURCE = false;

			if (DEBUG_RSPC_SOURCE) {

				const bool debugCell =
					i == 1362 ||
					i == 1363 ||
					i == 1364 ||
					i == 1365 ||
					i == 1366 ||
					i == 1367 ||
					i == 1368 ||
					i == 1370 ||
					i == 1375;

				if (debugCell) {

					const std::time_t currentDate =
						static_cast<std::time_t>(m_date);

					std::tm dateInfo = {};

#ifdef _WIN32
					localtime_s(&dateInfo, &currentDate);
#else
					localtime_r(&currentDate, &dateInfo);
#endif

					const int debugYear = dateInfo.tm_year + 1900;

					// Model can run from 1980, but diagnostics are written only for 2010-2015.
					if (debugYear >= 2010 && debugYear <= 2015) {

						char dateText[16] = {};

						std::strftime(
							dateText,
							sizeof(dateText),
							"%Y-%m-%d",
							&dateInfo
						);

#pragma omp critical(RSPC_SOURCE_DIAG_WRITE)
						{
							static std::ofstream debugFile;
							static bool debugFileInitialized = false;

							if (!debugFileInitialized) {

								const char* outputPath =
									"D:/SEIMS_C/data/CW_2/CW_2_longterm_model/"
									"OUTPUT0/RSPC_SOURCE_DIAG.csv";

								debugFile.open(
									outputPath,
									std::ios::out | std::ios::trunc
								);

								if (!debugFile.is_open()) {
									std::cerr
										<< "[RSPC_DIAG_ERROR] Cannot open: "
										<< outputPath
										<< std::endl;
								}
								else {
									debugFile
										<< "DATE,"
										<< "CELL,"
										<< "LAYER,"
										<< "CONTROL_LAYER,"
										<< "TEMP_LAYER_C,"
										<< "TEMP_CONTROL_C,"
										<< "WATER_LAYER_mm,"
										<< "WATER_CONTROL_mm,"
										<< "WP_mm,"
										<< "AWC_mm,"
										<< "SAT_PLUS_WP_mm,"
										<< "POROSITY,"
										<< "VOIDFR,"
										<< "SUT,"
										<< "CDG,"
										<< "OX,"
										<< "CS,"
										<< "LSC_kgCha,"
										<< "LMC_kgCha,"
										<< "BMC_kgCha,"
										<< "HSC_kgCha,"
										<< "HPC_kgCha,"
										<< "LSLCTA,"
										<< "LSLNCTA,"
										<< "LMCTA,"
										<< "BMCTA,"
										<< "HSCTA,"
										<< "HPCTA,"
										<< "RSPC_STRUCTURAL,"
										<< "RSPC_LITTER,"
										<< "RSPC_BIOMASS,"
										<< "RSPC_SLOW,"
										<< "RSPC_PASSIVE,"
										<< "RSPC_TOTAL_kgCha_d"
										<< '\n';
								}

								debugFileInitialized = true;
							}

							if (debugFile.is_open()) {

								const float rspcStructural =
									0.3f * LSLCTA;

								const float rspcLitter =
									A1CO2 * (LSLNCTA + LMCTA);

								const float rspcBiomass =
									ABCO2 * BMCTA;

								const float rspcSlow =
									ASCO2 * HSCTA;

								const float rspcPassive =
									APCO2 * HPCTA;

								debugFile
									<< std::fixed
									<< std::setprecision(8)
									<< dateText << ','
									<< i << ','
									<< k << ','
									<< kk << ','
									<< m_soilTempprofile[i][k] << ','
									<< m_soilTempprofile[i][kk] << ','
									<< m_soilWtrSto[i][k] << ','
									<< m_soilWtrSto[i][kk] << ','
									<< m_soilWP[i][k] << ','
									<< m_soilAWC[i][k] << ','
									<< sat << ','
									<< m_soilPor[i][k] << ','
									<< voidfr << ','
									<< sut << ','
									<< cdg << ','
									<< ox << ','
									<< cs << ','
									<< m_sol_LSC[i][k] << ','
									<< m_sol_LMC[i][k] << ','
									<< m_sol_BMC[i][k] << ','
									<< m_sol_HSC[i][k] << ','
									<< m_sol_HPC[i][k] << ','
									<< LSLCTA << ','
									<< LSLNCTA << ','
									<< LMCTA << ','
									<< BMCTA << ','
									<< HSCTA << ','
									<< HPCTA << ','
									<< rspcStructural << ','
									<< rspcLitter << ','
									<< rspcBiomass << ','
									<< rspcSlow << ','
									<< rspcPassive << ','
									<< m_sol_RSPC[i][k]
									<< '\n';
							}
						}
					}
				}
			}

            
            m_soilRsd[i][k] = m_sol_LS[i][k] + m_sol_LM[i][k];   //ljj++ for new carbon module should be delete
            m_soilStabOrgN[i][k] = m_sol_HPN[i][k];
                SOL_OUT[i][4] = m_sol_BMC[i][k];
            m_soilActvOrgN[i][k] = m_sol_HSN[i][k];
            m_soilFrshOrgN[i][k] = m_sol_LMN[i][k] + m_sol_LSN[i][k];
            const float centuryCarbon = m_sol_LSC[i][k] + m_sol_LMC[i][k] +
                m_sol_HSC[i][k] + m_sol_HPC[i][k] + m_sol_BMC[i][k];
            m_soilCbn[i][k] = 100.f * SafePositiveDivide(
                centuryCarbon, m_soilMass[i][k], 0.f);
            m_soil_carbon[i][k] = (m_sol_LSC[i][k] + m_sol_LMC[i][k] + m_sol_HSC[i][k] +
                m_sol_HPC[i][k] + m_sol_BMC[i][k]);
            if(i==17){
                SOL_OUT[i][0] = m_sol_LSC[i][k];
                SOL_OUT[i][1] = m_sol_LMC[i][k];
                SOL_OUT[i][2] = m_sol_HSC[i][k];
                SOL_OUT[i][3] = m_sol_HPC[i][k];
                SOL_OUT[i][5] = m_soilRsd[i][k];
            }
            // summary calculations
            float hmn = 0.f;
            hmn = m_sol_RNMN[i][k];
            m_wshd_hmn = m_wshd_hmn + hmn * m_cellAreaFr;
            float rwn = 0.f;
            rwn = HSNTA;
            m_wshd_rwn = m_wshd_rwn + rwn * m_cellAreaFr;

            m_wshd_hmp = m_wshd_hmp + hmp * m_cellAreaFr;
            float rmn1 = 0.f;
            rmn1 = LSNTA + LMNTA;
            m_wshd_rmn = m_wshd_rmn + rmn1 * m_cellAreaFr;
            m_wshd_rmp = m_wshd_rmp + rmp * m_cellAreaFr;
            m_wshd_dnit = m_wshd_dnit + wdn * m_cellAreaFr;
            m_hmntl[i] = m_hmntl[i] + hmn;
            m_rwntl[i] = m_rwntl[i] + rwn;
            m_hmptl[i] = m_hmptl[i] + hmp;
            m_rmn2tl[i] = m_rmn2tl[i] + rmn1;
            m_rmptl[i] = m_rmptl[i] + rmp;
            m_wdntl[i] = m_wdntl[i] + wdn;
        }
    }
}

void Nutrient_Transformation::GetValue(const char* key, float* value) {
    string sk(key);
    if (StringMatch(sk, VAR_WSHD_DNIT)) {
        *value = m_wshd_dnit;
    } else if (StringMatch(sk, VAR_WSHD_HMN)) {
        *value = m_wshd_hmn;
    } else if (StringMatch(sk, VAR_WSHD_HMP)) {
        *value = m_wshd_hmp;
    } else if (StringMatch(sk, VAR_WSHD_RMN)) {
        *value = m_wshd_rmn;
    } else if (StringMatch(sk, VAR_WSHD_RMP)) {
        *value = m_wshd_rmp;
    } else if (StringMatch(sk, VAR_WSHD_RWN)) {
        *value = m_wshd_rwn;
    } else if (StringMatch(sk, VAR_WSHD_NITN)) {
        *value = m_wshd_nitn;
    } else if (StringMatch(sk, VAR_WSHD_VOLN)) {
        *value = m_wshd_voln;
    } else if (StringMatch(sk, VAR_WSHD_PAL)) {
        *value = m_wshd_pal;
    } else if (StringMatch(sk, VAR_WSHD_PAS)) {
        *value = m_wshd_pas;
    } else {
        throw ModelException(MID_NUTR_TF, "GetValue", "Parameter " + sk + " does not exist.");
    }
}

void Nutrient_Transformation::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_HMNTL)) {
        *data = m_hmntl;
    } else if (StringMatch(sk, VAR_HMPTL)) {
        *data = m_hmptl;
    } else if (StringMatch(sk, VAR_RMN2TL)) {
        *data = m_rmn2tl;
    } else if (StringMatch(sk, VAR_RMPTL)) {
        *data = m_rmptl;
    } else if (StringMatch(sk, VAR_RWNTL)) {
        *data = m_rwntl;
    } else if (StringMatch(sk, VAR_WDNTL)) {
        *data = m_wdntl;
    } else if (StringMatch(sk, VAR_RMP1TL)) {
        *data = m_rmp1tl;
    } else if (StringMatch(sk, VAR_ROCTL)) {
        *data = m_roctl;
    } else if (StringMatch(sk, VAR_SOL_COV)) {
        *data = m_rsdCovSoil;
    } else if (StringMatch(sk, VAR_A_DAYS)) {
        *data = m_phpApldDays;
    } else if (StringMatch(sk, VAR_B_DAYS)) {
        *data = m_phpDefDays;
    } else if (StringMatch(sk, "MIC1")) {
        *data = MIC1;
    }else if (StringMatch(sk, "MAOM1")) {
        *data = MAOM1;
    }else if (StringMatch(sk, "LMWC1")) {
        *data = LMWC1;
    }else if (StringMatch(sk, "POM1")) {
        *data = POM1;
    }else if (StringMatch(sk, "SURFDOC")) {
        *data = m_soilSurfCbn;
    }else if (StringMatch(sk, "SURFPOC")) {
        *data = m_soileroPOC;
    }else if (StringMatch(sk, VAR_PERC_LOWEST_DOC)) {
		*data = m_soilPercoCbnLowest;
		*n = m_nCells;
	}else {
        throw ModelException(MID_NUTR_TF, "Get1DData", "Parameter " + sk + " does not exist.");
    }
    *n = m_nCells;
}

void Nutrient_Transformation::Get2DData(const char* key, int* nRows, int* nCols, float*** data) {
    InitialOutputs();
    string sk(key);
    *nRows = m_nCells;
    *nCols = m_maxSoilLyrs;
    if (StringMatch(sk, VAR_SOL_AORGN)) {
        *data = m_soilActvOrgN;
    } else if (StringMatch(sk, VAR_SOL_FORGN)) {
        *data = m_soilFrshOrgN;
    } else if (StringMatch(sk, VAR_SOL_FORGP)) {
        *data = m_soilFrshOrgP;
    } else if (StringMatch(sk, VAR_SOL_NO3)) {
        *data = m_soilNO3;
    } else if (StringMatch(sk, VAR_SOL_NH4)) {
        *data = m_soilNH4;
    } else if (StringMatch(sk, VAR_SOL_SORGN)) {
        *data = m_soilStabOrgN;
    } else if (StringMatch(sk, VAR_SOL_HORGP)) {
        *data = m_soilHumOrgP;
    } else if (StringMatch(sk, VAR_SOL_SOLP)) {
        *data = m_soilSolP;
    } else if (StringMatch(sk, VAR_SOL_ACTP)) {
        *data = m_soilActvMinP;
    } else if (StringMatch(sk, VAR_SOL_STAP)) {
        *data = m_soilStabMinP;
    } else if (StringMatch(sk, VAR_SOL_RSD)) {
        *data = m_soilRsd;
    }
        /// 2-CENTURY C/N cycling related variables
    else if (StringMatch(sk, VAR_SOL_WOC)) {
        *data = m_sol_WOC;
    } else if (StringMatch(sk, VAR_SOL_WON)) {
        *data = m_sol_WON;
    } else if (StringMatch(sk, VAR_SOL_BM)) {
        *data = m_sol_BM;
    } else if (StringMatch(sk, VAR_SOL_BMC)) {
        *data = m_sol_BMC;
    } else if (StringMatch(sk, VAR_SOL_BMN)) {
        *data = m_sol_BMN;
    } else if (StringMatch(sk, VAR_SOL_HP)) {
        *data = m_sol_HP;
    } else if (StringMatch(sk, VAR_SOL_HS)) {
        *data = m_sol_HS;
    } else if (StringMatch(sk, VAR_SOL_HSC)) {
        *data = m_sol_HSC;
    } else if (StringMatch(sk, VAR_SOL_HSN)) {
        *data = m_sol_HSN;
    } else if (StringMatch(sk, VAR_SOL_HPC)) {
        *data = m_sol_HPC;
    } else if (StringMatch(sk, VAR_SOL_HPN)) {
        *data = m_sol_HPN;
    } else if (StringMatch(sk, VAR_SOL_LM)) {
        *data = m_sol_LM;
    } else if (StringMatch(sk, VAR_SOL_LMC)) {
        *data = m_sol_LMC;
    } else if (StringMatch(sk, VAR_SOL_LMN)) {
        *data = m_sol_LMN;
    } else if (StringMatch(sk, VAR_SOL_LSC)) {
        *data = m_sol_LSC;
    } else if (StringMatch(sk, VAR_SOL_LSN)) {
        *data = m_sol_LSN;
    } else if (StringMatch(sk, VAR_SOL_LS)) {
        *data = m_sol_LS;
    } else if (StringMatch(sk, VAR_SOL_LSL)) {
        *data = m_sol_LSL;
    } else if (StringMatch(sk, VAR_SOL_LSLC)) {
        *data = m_sol_LSLC;
    } else if (StringMatch(sk, VAR_SOL_LSLNC)) {
        *data = m_sol_LSLNC;
    } else if (StringMatch(sk, VAR_SOL_RNMN)) {
        *data = m_sol_RNMN;
    } else if (StringMatch(sk, VAR_SOL_RSPC)) {
        *data = m_sol_RSPC;
    } else if (StringMatch(sk, VAR_CONV_WT)) {
        *data = m_conv_wt;
    }
    //ljj++
    else if (StringMatch(sk, VAR_LMC)) {
        *data = m_sol_LMC;
    }
    else if (StringMatch(sk, VAR_LSC)) {
        *data = m_sol_LSC;
    }
    else if (StringMatch(sk, VAR_WOC)) {
        *data = m_soil_carbon;
    }
    else if (StringMatch(sk, VAR_BMC)) {
        *data = m_sol_BMC;
    }
    else if (StringMatch(sk, VAR_HSC)) {
        *data = m_sol_HSC;
    }
    else if (StringMatch(sk, VAR_HPC)) {
        *data = m_sol_HPC;
    } 
    else if (StringMatch(sk, "MIC")) {
        *data = MIC;
    }
    else if (StringMatch(sk, "LMWC")) {
        *data = LMWC;
    }
    else if (StringMatch(sk, "POM")) {
        *data = POM;
    }
    else if (StringMatch(sk, "MAOM")) {
        *data = MAOM;
    }
    else if (StringMatch(sk, "SOILCO2")) {
        *data = soilCO2;
    }
    else if (StringMatch(sk, "SOC")) {
        // SOM is synchronized after all local and routed carbon updates:
        // SOC = POM + LMWC + MAOM + MIC.
        *data = SOM;
    }
    else if (StringMatch(sk, "DOC")) {
        // Dynamic DOC concentration converted from LMWC, unit: mg C/kg soil.
        *data = DOC;
    }
    else if (StringMatch(sk, "SOC_OUT")) {
        *data = SOL_OUT;
        *nCols = 6;
    }
    else if (StringMatch(sk, "SUBSURFDOC")) {
        *data = m_soilIfluCbn;
    }
    else {
        throw ModelException(MID_NUTR_TF, "Get2DData", "Parameter " + sk + " does not exist.");
    }
}
