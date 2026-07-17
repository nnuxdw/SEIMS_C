#include "SoilTemperatureFINPL.h"

#include "text.h"
#include "utils_time.h"
#include "ClimateParams.h"

SoilTemperatureFINPL::SoilTemperatureFINPL() :
    m_a0(NODATA_VALUE), m_a1(NODATA_VALUE), m_a2(NODATA_VALUE),
    m_a3(NODATA_VALUE),
    m_b1(NODATA_VALUE), m_b2(NODATA_VALUE), m_d1(NODATA_VALUE),
    m_d2(NODATA_VALUE),
    m_kSoil10(NODATA_VALUE), m_nCells(-1),
    m_soilTempRelFactor10(nullptr),
    m_landUse(nullptr), m_meanTemp(nullptr), m_meanTempPre1(nullptr),
    m_meanTempPre2(nullptr),
    m_soilTemp(nullptr),
    //ljj++
    m_maxSoilLyrs(-1),m_soildepth(nullptr),m_nSoilLyrs(nullptr), m_solthic(nullptr),m_soilthick(nullptr),
    tsoil1(NODATA_VALUE),tsoil2(NODATA_VALUE),tsoil3(NODATA_VALUE),tsoil4(NODATA_VALUE),tsoil5(NODATA_VALUE),
    m_ddepth1(NODATA_VALUE),m_snowCoverMax(NODATA_VALUE),m_snowCover50(NODATA_VALUE),m_tfrozen(NODATA_VALUE),
    m_effcoe(nullptr),m_kcoe(nullptr),m_kscoe(nullptr),m_ccoe(nullptr),
    m_solnd(nullptr),m_soilPor(nullptr),m_soilWP(nullptr),m_soilWtrSto(nullptr),m_solpormm(nullptr),
    m_rsdInitSoil(nullptr),m_soilBD(nullptr),m_snowAcc(nullptr),m_soilAVBD(nullptr), m_solsw(nullptr),
    m_solwc(nullptr),m_solice(nullptr),m_solwcv(nullptr),m_solorg(nullptr),m_solorgv(nullptr),
    m_solminv(nullptr),m_solicev(nullptr),m_solair(nullptr),m_snoden(nullptr),m_snodep(nullptr),
    m_ksno(nullptr),m_casno(nullptr),m_casol(nullptr),ddepth(nullptr),m_dem(nullptr),m_soilRsd(nullptr),
    m_ambtmp(nullptr),m_snoco(nullptr),m_surtmp(nullptr),m_igro(nullptr),m_soilcov(nullptr),
    m_landCoverCls(nullptr),m_lai(nullptr),m_maxLai(nullptr),m_soilALB(nullptr),m_alb(nullptr),
    m_sr(nullptr),m_lightExtCoef(nullptr),m_cellLat(nullptr),m_rhd(nullptr),m_dayLen(nullptr),
    m_tmpmn(nullptr),m_tmpmx(nullptr),m_annMeanTemp(nullptr),m_basedep(nullptr),m_bottmp(nullptr),
    m_solsatu(nullptr),m_solkd(nullptr),m_solke(nullptr),m_solksat(nullptr),m_ksol(nullptr),
    m_kss(nullptr),m_kint(nullptr),m_bsol(nullptr),m_psol(nullptr),m_qsol(nullptr),
    m_csol(nullptr),m_asol(nullptr),m_dsol(nullptr),m_dsols(nullptr),m_dice(nullptr),
    m_asno(nullptr),m_bsno(nullptr),m_csno(nullptr),m_dsno(nullptr),m_psno(nullptr),m_qsno(nullptr),
    m_soltmp1(nullptr),m_snotmp1(nullptr),m_presnotmp(nullptr), m_soilt(nullptr),
    m_smp(nullptr),m_supercool(nullptr),m_sand(nullptr),m_clay(nullptr),m_soilSat(nullptr),m_tMin(nullptr),m_tMax(nullptr),
    m_SOTE1(nullptr),m_SOTE5(nullptr),m_SOTE15(nullptr),m_SOTE30(nullptr),m_SOTE60(nullptr),m_SOTE100(nullptr),m_SOTE200(nullptr)
    {
}

SoilTemperatureFINPL::~SoilTemperatureFINPL() {
    if (m_soilTemp != nullptr) Release1DArray(m_soilTemp);
    if (m_meanTempPre1 != nullptr) Release1DArray(m_meanTempPre1);
    if (m_meanTempPre2 != nullptr) Release1DArray(m_meanTempPre2);
    //ljj++
    if (m_effcoe != nullptr) Release1DArray(m_effcoe);
    if (m_kscoe != nullptr) Release1DArray(m_kscoe);
    if (m_ccoe != nullptr) Release1DArray(m_ccoe);
    if (m_snoden != nullptr) Release1DArray(m_snoden);
    if (m_snodep != nullptr) Release1DArray(m_snodep);
    if (m_ksno != nullptr) Release1DArray(m_ksno);
    if (m_casno != nullptr) Release1DArray(m_casno);
    if (ddepth != nullptr) Release1DArray(ddepth);
    if (m_basedep != nullptr) Release1DArray(m_basedep);
    if (m_bottmp != nullptr) Release1DArray(m_bottmp);
    if (m_ambtmp != nullptr) Release1DArray(m_ambtmp);
    if (m_snoco != nullptr) Release1DArray(m_snoco);
    if (m_surtmp != nullptr) Release1DArray(m_surtmp);
    if (m_alb != nullptr) Release1DArray(m_alb);
    if (m_snotmp1 != nullptr) Release1DArray(m_snotmp1);
    if (m_presnotmp != nullptr) Release1DArray(m_presnotmp);
    if (m_asno != nullptr) Release1DArray(m_asno);
    if (m_bsno != nullptr) Release1DArray(m_bsno);
    if (m_csno != nullptr) Release1DArray(m_csno);
    if (m_dsno != nullptr) Release1DArray(m_dsno);
    if (m_psno != nullptr) Release1DArray(m_psno);
    if (m_qsno != nullptr) Release1DArray(m_qsno);
    if (m_kss != nullptr) Release1DArray(m_kss);
    if (m_kint != nullptr) Release1DArray(m_kint);
    if (m_bsol != nullptr) Release1DArray(m_bsol);
    if (m_psol != nullptr) Release1DArray(m_psol);
    if (m_qsol != nullptr) Release1DArray(m_qsol);
    if (m_csol != nullptr) Release1DArray(m_csol);
    if (m_dsol != nullptr) Release1DArray(m_dsol);
    if (m_asol != nullptr) Release1DArray(m_asol);
    if (m_dsols != nullptr) Release1DArray(m_dsols);

    if (m_SOTE1 != nullptr) Release1DArray(m_SOTE1);
    if (m_SOTE5 != nullptr) Release1DArray(m_SOTE5);
    if (m_SOTE15 != nullptr) Release1DArray(m_SOTE15);
    if (m_SOTE30 != nullptr) Release1DArray(m_SOTE30);
    if (m_SOTE60 != nullptr) Release1DArray(m_SOTE60);
    if (m_SOTE100 != nullptr) Release1DArray(m_SOTE100);
    if (m_SOTE200 != nullptr) Release1DArray(m_SOTE200);

    if (m_kcoe != nullptr) Release2DArray(m_nCells, m_kcoe);
    if (m_solnd != nullptr) Release2DArray(m_nCells, m_solnd);
    if (m_solpormm != nullptr) Release2DArray(m_nCells, m_solpormm);
    if (m_solwc != nullptr) Release2DArray(m_nCells, m_solwc);
    if (m_solice != nullptr) Release2DArray(m_nCells, m_solice);
    if (m_solwcv != nullptr) Release2DArray(m_nCells, m_solwcv);
    if (m_solorg != nullptr) Release2DArray(m_nCells, m_solorg);
    if (m_solorgv != nullptr) Release2DArray(m_nCells, m_solorgv);
    if (m_solminv != nullptr) Release2DArray(m_nCells, m_solminv);
    if (m_solicev != nullptr) Release2DArray(m_nCells, m_solicev);
    if (m_solair != nullptr) Release2DArray(m_nCells, m_solair);
    if (m_casol != nullptr) Release2DArray(m_nCells,m_casol);
    if (m_solsatu != nullptr) Release2DArray(m_nCells, m_solsatu);
    if (m_solkd != nullptr) Release2DArray(m_nCells, m_solkd);
    if (m_solke != nullptr) Release2DArray(m_nCells, m_solke);
    if (m_solksat != nullptr) Release2DArray(m_nCells, m_solksat);
    if (m_ksol != nullptr) Release2DArray(m_nCells, m_ksol);
    if (m_dsols != nullptr) Release2DArray(m_nCells, m_dsols);
    if (m_dice != nullptr) Release2DArray(m_nCells, m_dice);
    if (m_soltmp1 != nullptr) Release2DArray(m_nCells, m_soltmp1);
    if (m_soilt != nullptr) Release2DArray(m_nCells, m_soilt);
    if (m_solthic != nullptr) Release2DArray(m_nCells, m_solthic);
    if (m_smp != nullptr) Release2DArray(m_nCells, m_smp);
    if (m_supercool != nullptr) Release2DArray(m_nCells, m_supercool);
}

int SoilTemperatureFINPL::Execute() {
    CheckInputData();
    InitialOutputs();
    size_t errCount = 0;
#pragma omp parallel for reduction(+: errCount)
    for (int i = 0; i < m_nCells; i++) {
        float t = m_meanTemp[i];
        float t1 = m_meanTempPre1[i];
        float t2 = m_meanTempPre2[i];
        if ((t > 60.f || t < -90.f) || (t1 > 60.f || t1 < -90.f) || (t2 > 60.f || t2 < -90.f)) {
            cout << "cell index: " << i << ", t1: " << t1 << ", t2: " << t2 << endl;
            errCount++;
        } else {
            if (FloatEqual(CVT_INT(m_landUse[i]), LANDUSE_ID_WATR)) {
                /// if current landuse is water
                m_soilTemp[i] = t;
            } else {
                float t10 = m_a0 + m_a1 * t2 + m_a2 * t1 + m_a3 * t
                        + m_b1 * sin(radWt * m_dayOfYear) + m_d1 * cos(radWt * m_dayOfYear)
                        + m_b2 * sin(2.f * radWt * m_dayOfYear) + m_d2 * cos(2.f * radWt * m_dayOfYear);
                m_soilTemp[i] = t10 * m_kSoil10 + m_soilTempRelFactor10[i];
                if (m_soilTemp[i] > 60.f || m_soilTemp[i] < -90.f) {
                    cout << "The calculated soil temperature at cell (" << i
                            << ") is out of reasonable range: " << m_soilTemp[i]
                            << ". JulianDay: " << m_dayOfYear << ",t: " << t << ", t1: "
                            << t1 << ", t2: " << t2 << ", relativeFactor: " << m_soilTempRelFactor10[i] << endl;
                    errCount++;
                }
            }
            //save the temperature
            m_meanTempPre2[i] = m_meanTempPre1[i];
            m_meanTempPre1[i] = t;
        }
    }
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        if ((FloatEqual(CVT_INT(m_landUse[i]), LANDUSE_ID_WATR)) || (FloatEqual(CVT_INT(m_landUse[i]), 300))|| (FloatEqual(CVT_INT(m_landUse[i]), 400))) {
            m_soilTemp[i] = m_meanTemp[i];
        }else{
            SoilTempTSWAT(i);   //TSWAT
            //SoilTempSWAT(i);   //SWAT
            m_soilTemp[i] = m_soilt[i][0];
        }
    }
    //output
    int nlyr=5; //!!????ljj
    //assume the depth below the deepest soil layer is devided into 5 layer
    int nly1=0;
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        nly1 = m_nSoilLyrs[i]+ nlyr;
        for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
            if (m_solnd[i][k+1]>10. && m_solnd[i][k]<=10.) {
                m_SOTE1[i] = m_soilt[i][k] * ( (m_solnd[i][k+1]-10) / (m_solnd[i][k+1]-m_solnd[i][k]) ) 
                            + m_soilt[i][k+1] * ((10- m_solnd[i][k]) / (m_solnd[i][k+1]-m_solnd[i][k]) );
            } 
            if (m_solnd[i][k+1]>50. && m_solnd[i][k]<=50.) {
                m_SOTE5[i] = m_soilt[i][k] * ( (m_solnd[i][k+1]-50) / (m_solnd[i][k+1]-m_solnd[i][k]) ) 
                            + m_soilt[i][k+1] * ((50- m_solnd[i][k]) / (m_solnd[i][k+1]-m_solnd[i][k]) );
            }
            if (m_solnd[i][k+1]>150. && m_solnd[i][k]<=150.) {
                m_SOTE15[i] = m_soilt[i][k] * ( (m_solnd[i][k+1]-150) / (m_solnd[i][k+1]-m_solnd[i][k]) ) 
                            + m_soilt[i][k+1] * ((150- m_solnd[i][k]) / (m_solnd[i][k+1]-m_solnd[i][k]) );
            } 
            if (m_solnd[i][k+1]>300. && m_solnd[i][k]<=300.) {
                m_SOTE30[i] = m_soilt[i][k] * ( (m_solnd[i][k+1]-300) / (m_solnd[i][k+1]-m_solnd[i][k]) ) 
                            + m_soilt[i][k+1] * ((300- m_solnd[i][k]) / (m_solnd[i][k+1]-m_solnd[i][k]) );
            }
            if (m_solnd[i][k+1]>600. && m_solnd[i][k]<=600.) {
                m_SOTE60[i] = m_soilt[i][k] * ( (m_solnd[i][k+1]-600) / (m_solnd[i][k+1]-m_solnd[i][k]) ) 
                            + m_soilt[i][k+1] * ((600- m_solnd[i][k]) / (m_solnd[i][k+1]-m_solnd[i][k]) );
            } 
            if (m_solnd[i][k+1]>1000. && m_solnd[i][k]<=1000.) {
                m_SOTE100[i] = m_soilt[i][k] * ( (m_solnd[i][k+1]-1000) / (m_solnd[i][k+1]-m_solnd[i][k]) ) 
                            + m_soilt[i][k+1] * ((1000- m_solnd[i][k]) / (m_solnd[i][k+1]-m_solnd[i][k]) );
            }
            if (m_solnd[i][k+1]>2000. && m_solnd[i][k]<=2000.) {
                
                m_SOTE200[i] = m_soilt[i][k] * ( (m_solnd[i][k+1]-2000) / (m_solnd[i][k+1]-m_solnd[i][k]) ) 
                            + m_soilt[i][k+1] * ((2000- m_solnd[i][k]) / (m_solnd[i][k+1]-m_solnd[i][k]) );
            }                   
        }
    }
    if (errCount > 0) {
        throw ModelException(MID_STP_FP, "Execute", "The calculation of soil temperature failed!");
    }
    return 0;
}

bool SoilTemperatureFINPL::CheckInputData() {
    CHECK_POSITIVE(MID_STP_FP, m_nCells);
    CHECK_NODATA(MID_STP_FP, m_a0);
    CHECK_NODATA(MID_STP_FP, m_a1);
    CHECK_NODATA(MID_STP_FP, m_a2);
    CHECK_NODATA(MID_STP_FP, m_a3);
    CHECK_NODATA(MID_STP_FP, m_b1);
    CHECK_NODATA(MID_STP_FP, m_b2);
    CHECK_NODATA(MID_STP_FP, m_d1);
    CHECK_NODATA(MID_STP_FP, m_d2);
    CHECK_NODATA(MID_STP_FP, m_kSoil10);
    CHECK_POINTER(MID_STP_FP, m_soilTempRelFactor10);
    CHECK_POINTER(MID_STP_FP, m_meanTemp);
    CHECK_POINTER(MID_STP_FP, m_landUse);
    return true;
}

void SoilTemperatureFINPL::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, VAR_SOL_TA0)) m_a0 = value;
    else if (StringMatch(sk, VAR_SOL_TA1)) m_a1 = value;
    else if (StringMatch(sk, VAR_SOL_TA2)) m_a2 = value;
    else if (StringMatch(sk, VAR_SOL_TA3)) m_a3 = value;
    else if (StringMatch(sk, VAR_SOL_TB1)) m_b1 = value;
    else if (StringMatch(sk, VAR_SOL_TB2)) m_b2 = value;
    else if (StringMatch(sk, VAR_SOL_TD1)) m_d1 = value;
    else if (StringMatch(sk, VAR_SOL_TD2)) m_d2 = value;
    else if (StringMatch(sk, VAR_K_SOIL10)) m_kSoil10 = value;
    //ljj++
    else if (StringMatch(sk, VAR_TSOIL1)) tsoil1 = value;
    else if (StringMatch(sk, VAR_TSOIL2)) tsoil2 = value;
    else if (StringMatch(sk, VAR_TSOIL3)) tsoil3 = value;
    else if (StringMatch(sk, VAR_TSOIL4)) tsoil4 = value;
    else if (StringMatch(sk, VAR_TSOIL5)) tsoil5 = value;
    else if (StringMatch(sk, VAR_DDEPTH1)) m_ddepth1 = value;  
    else if (StringMatch(sk, VAR_SNOCOVMX)) m_snowCoverMax = value;
    else if (StringMatch(sk, VAR_SNO50COV)) m_snowCover50 = value; 
    else if (StringMatch(sk, VAR_T_SOIL)) m_tfrozen = value;
}

void SoilTemperatureFINPL::Set1DData(const char* key, const int n, float* data) {
    CheckInputSize(MID_STP_FP, key, n, m_nCells);
    string sk(key);
    if (StringMatch(sk, VAR_SOIL_T10)) m_soilTempRelFactor10 = data;
    else if (StringMatch(sk, VAR_TMEAN)) m_meanTemp = data;
    else if (StringMatch(sk, VAR_LANDUSE)) m_landUse = data;
    //ljj++
    else if (StringMatch(sk, VAR_SOILLAYERS)) m_nSoilLyrs = data;
    else if (StringMatch(sk, VAR_SOL_RSDIN)) m_rsdInitSoil = data;
    else if (StringMatch(sk, VAR_SNAC)) m_snowAcc = data;
    else if (StringMatch(sk, VAR_SOL_SW)) m_solsw = data;
	else if (StringMatch(sk, VAR_SOLAVBD)) m_soilAVBD = data;
    else if (StringMatch(sk, VAR_TMEAN_ANN)) m_annMeanTemp = data;
    else if (StringMatch(sk, VAR_TMAX)) m_tMax = data;
	else if (StringMatch(sk, VAR_TMIN)) m_tMin = data;
    else if (StringMatch(sk, DataType_MaximumMonthlyTemperature)) m_tmpmx = data;
    else if (StringMatch(sk, DataType_MinimumMonthlyTemperature)) m_tmpmn = data;
    else if (StringMatch(sk, VAR_DEM)) m_dem = data;
    else if (StringMatch(sk, VAR_IGRO)) m_igro = data;
    else if (StringMatch(sk, VAR_IDC)) m_landCoverCls = data;
    else if (StringMatch(sk, VAR_SOL_ALB)) m_soilALB = data;
    else if (StringMatch(sk, VAR_BLAI)) m_maxLai = data;
    else if (StringMatch(sk, VAR_LAIDAY)) m_lai = data;
    else if (StringMatch(sk, VAR_SOL_COV)) m_soilcov = data;
    else if (StringMatch(sk, DataType_SolarRadiation)) m_sr = data;
    else if (StringMatch(sk, VAR_EXT_COEF)) m_lightExtCoef = data;
    else if (StringMatch(sk, DataType_RelativeAirMoisture)) m_rhd = data;
    else if (StringMatch(sk, VAR_DAYLEN)) m_dayLen = data;
    else if (StringMatch(sk, VAR_CELL_LAT)) m_cellLat = data;
    else {
        throw ModelException(MID_STP_FP, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void SoilTemperatureFINPL::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    *n = m_nCells;
    if (StringMatch(sk, VAR_SOTE)) *data = m_soilTemp;
    else if (StringMatch(sk, VAR_TMEAN1)) *data = m_meanTempPre1;
    else if (StringMatch(sk, VAR_TMEAN2)) *data = m_meanTempPre2;
    //ljj++
    else if (StringMatch(sk, VAR_SOTE1)) {
		*data = m_SOTE1;
	}
    else if (StringMatch(sk, VAR_SOTE5)) {
		*data = m_SOTE5;
	}
    else if (StringMatch(sk, VAR_SOTE15)) {
		*data = m_SOTE15;
	}
    else if (StringMatch(sk, VAR_SOTE30)) {
		*data = m_SOTE30;
	}
    else if (StringMatch(sk, VAR_SOTE60)) {
		*data = m_SOTE60;
	}
    else if (StringMatch(sk, VAR_SOTE100)) {
		*data = m_SOTE100;
	}
    else if (StringMatch(sk, VAR_SOTE200)) {
		*data = m_SOTE200;
	}
    else {
        throw ModelException(MID_STP_FP, "Get1DData", "Parameter " + sk + " does not exist in current module.");
    }
}

void SoilTemperatureFINPL::InitialOutputs() {
    CHECK_POSITIVE(MID_STP_FP, m_nCells);
    // initialize m_t1 and m_t2 as m_tMean
    if (nullptr == m_meanTempPre1 && m_meanTemp != nullptr) {
        Initialize1DArray(m_nCells, m_meanTempPre1, m_meanTemp);
    }
    if (nullptr == m_meanTempPre2 && m_meanTemp != nullptr) {
        Initialize1DArray(m_nCells, m_meanTempPre2, m_meanTemp);
    }
    if (nullptr == m_soilTemp) {
        Initialize1DArray(m_nCells, m_soilTemp, 0.f);
    }
    //ljj++
    if (nullptr == m_SOTE1) {
        Initialize1DArray(m_nCells, m_SOTE1, 0.f);
        Initialize1DArray(m_nCells, m_SOTE5, 0.f);
        Initialize1DArray(m_nCells, m_SOTE15, 0.f);
        Initialize1DArray(m_nCells, m_SOTE30, 0.f);
        Initialize1DArray(m_nCells, m_SOTE60, 0.f);
        Initialize1DArray(m_nCells, m_SOTE100, 0.f);
        Initialize1DArray(m_nCells, m_SOTE200, 0.f);
    }
    if (nullptr == m_effcoe) {
        Initialize1DArray(m_nCells, m_effcoe, 0.f);
        Initialize1DArray(m_nCells, m_kscoe, 0.f);
        Initialize1DArray(m_nCells, m_ccoe, 0.f);
        Initialize1DArray(m_nCells, m_basedep, 0.f);
        
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solpormm, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solwc, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solwcv, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solice, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solicev, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solorg, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solorgv, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solminv, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solair, 0.f);
        
        int newlayer = m_maxSoilLyrs+10; //ljj++
        Initialize2DArray(m_nCells, newlayer, m_kcoe, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_solnd, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_casol, 1.f);
        Initialize2DArray(m_nCells, newlayer, m_solsatu, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_solkd, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_solke, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_solksat, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_ksol, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_solthic, 0.f);
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            m_basedep[i] = m_ddepth1;
            m_effcoe[i] = tsoil1; //50; //calibration air to surface
            m_kcoe[i][0] = tsoil2; //10; //calibration 土壤热导率的乘数
            for (int k = 1; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                m_kcoe[i][k] = tsoil3;//10; //calibration
            }
            m_kscoe[i] = tsoil4; //1; calibration snow heat conduc.
            m_ccoe[i] = tsoil5;  //1; calibration soil heat capacity
            //call soltphys_in  !initialization
            //depth to point k from soil surface mm
            m_solthic[i][0]=m_soildepth[i][0];    
            for (int k = 1; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                m_solthic[i][k]= m_soildepth[i][k] - m_soildepth[i][k-1];
                m_solthic[i][k]= Abs(m_solthic[i][k]);
            }         
            m_solnd[i][0]=m_soildepth[i][0] / 2;  
            for (int k = 1; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                m_solnd[i][k]=m_soildepth[i][k-1] + m_solthic[i][k]/2;
            }

            for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
                m_solpormm[i][k]=m_soilPor[i][k]*m_solthic[i][k];
                if(m_solpormm[i][k]-m_soilWP[i][k] <= 0.f) m_solpormm[i][k] = m_soilWP[i][k]/0.25f;
                m_solwc[i][k] = m_soilWP[i][k] - m_solice[i][k]; //ljj change for bug
                m_solwcv[i][k]  = m_solwc[i][k] / m_solthic[i][k];
                m_solorg[i][k]  = (m_rsdInitSoil[i]) / (0.5*10000); //note:m_solhum is zero
                m_solorgv[i][k] = m_solorg[i][k] / (m_solthic[i][k]);
                m_solminv[i][k] = m_soilBD[i][k] / 2.65 - m_solorgv[i][k];
                m_solicev[i][k] = m_solice[i][k] / m_solthic[i][k];
                m_solair[i][k]  = m_solpormm[i][k]- m_solwc[i][k]-m_solice[i][k];
            }
        }
    }
    if (nullptr == m_snoden) {
        Initialize1DArray(m_nCells, m_snoden, 0.f);
        Initialize1DArray(m_nCells, m_snodep, 0.f);
    }
    if (nullptr == m_ksno)  Initialize1DArray(m_nCells, m_ksno, 0.f);
    if (nullptr == m_casno)  Initialize1DArray(m_nCells, m_casno, 0.f);
    if (nullptr == ddepth) Initialize1DArray(m_nCells, ddepth, 0.f);
    if (nullptr == m_bottmp) Initialize1DArray(m_nCells, m_bottmp, 0.f);
    if (nullptr == m_ambtmp) Initialize1DArray(m_nCells, m_ambtmp, 0.f);
    if (nullptr == m_snoco) Initialize1DArray(m_nCells, m_snoco, 0.f);
    if (nullptr == m_surtmp) Initialize1DArray(m_nCells, m_surtmp, 0.f);
    if (nullptr == m_alb) Initialize1DArray(m_nCells, m_alb, 0.f);
    if (nullptr == m_asno ) Initialize1DArray(m_nCells, m_asno, 0.f);
    if (nullptr == m_bsno ) Initialize1DArray(m_nCells, m_bsno, 0.f);
    if (nullptr == m_csno ) Initialize1DArray(m_nCells, m_csno, 0.f);
    if (nullptr == m_dsno ) Initialize1DArray(m_nCells, m_dsno, 0.f);
    if (nullptr == m_psno ) Initialize1DArray(m_nCells, m_psno, 0.f);
    if (nullptr == m_qsno ) Initialize1DArray(m_nCells, m_qsno, 0.f);
    if (nullptr == m_kss ) Initialize1DArray(m_nCells, m_kss, 0.f);
    int newlayer = m_maxSoilLyrs+6; //ljj++
    if (nullptr == m_kint ) Initialize2DArray(m_nCells,newlayer, m_kint, 0.f);
    if (nullptr == m_bsol ) Initialize2DArray(m_nCells,newlayer, m_bsol, 0.f);
    if (nullptr == m_csol ) Initialize2DArray(m_nCells,newlayer, m_csol, 0.f);
    if (nullptr == m_asol ) Initialize2DArray(m_nCells,newlayer, m_asol, 0.f);
    if (nullptr == m_dsol ) Initialize2DArray(m_nCells,newlayer, m_dsol, 0.f);
    if (nullptr == m_psol ) Initialize2DArray(m_nCells,newlayer, m_psol, 0.f);
    if (nullptr == m_qsol ) Initialize2DArray(m_nCells,newlayer, m_qsol, 0.f);
    if (nullptr == m_dice) Initialize2DArray(m_nCells, newlayer, m_dice, 0.f);
    if (nullptr == m_dsols) Initialize2DArray(m_nCells, newlayer, m_dsols, 0.f);
    if (nullptr == m_smp) Initialize2DArray(m_nCells, newlayer, m_smp, 0.f);
    if (nullptr == m_supercool) Initialize2DArray(m_nCells, newlayer, m_supercool, 0.f);

    if (nullptr == m_soilt) {
        int newlayer = m_maxSoilLyrs+6; //ljj++
        Initialize1DArray(m_nCells, m_snotmp1, 0.f);
        Initialize1DArray(m_nCells, m_presnotmp, 0.f);
        Initialize2DArray(m_nCells, newlayer, m_soltmp1, 0.f);
        Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilt, 0.f);
    }
}

//ljj++
void SoilTemperatureFINPL::Set2DData(const char* key, int n, int col, float** data) {
	CheckInputSize2D(MID_STP_FP, key, n, col, m_nCells, m_maxSoilLyrs);
	string sk(key);
	if (StringMatch(sk, VAR_SOILDEPTH)) m_soildepth = data;
    //ljj++
    else if (StringMatch(sk, VAR_SOILTHICK)) m_soilthick = data;
    else if (StringMatch(sk, VAR_POROST)) m_soilPor = data;
	else if (StringMatch(sk, VAR_SOL_ST)) {
		m_soilWtrSto = data;

	}
	else if (StringMatch(sk, VAR_SOL_WPMM)) m_soilWP = data;
    else if (StringMatch(sk, VAR_SOL_BD)) m_soilBD = data;
    else if (StringMatch(sk, VAR_SOL_RSD)) m_soilRsd = data;
    else if (StringMatch(sk, "clay")) m_clay = data;
    else if (StringMatch(sk, "sand"))  m_sand = data;
    else if (StringMatch(sk, VAR_SOL_UL)) m_soilSat = data;
}

void SoilTemperatureFINPL::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	InitialOutputs();
	string sk(key);
    int newlayer = m_maxSoilLyrs+6; //ljj++
	if (StringMatch(sk, VAR_SOILT)) {
        *data = m_soilt;
        *nrows = m_nCells;
        *ncols = m_maxSoilLyrs;
    }
    if (StringMatch(sk, VAR_SOLICE)) {
        *data = m_solice;
        *nrows = m_nCells;
        *ncols = m_maxSoilLyrs;
    }
    if (StringMatch(sk, VAR_SOLWC)) {
        *data = m_solwc;
        *nrows = m_nCells;
        *ncols = m_maxSoilLyrs;
    }
}

//ljj++
void SoilTemperatureFINPL::SoilTempTSWAT(const int i) {
    for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        if(m_solice[i][k] <= m_soilWP[i][k]){
            m_solwc[i][k] = m_soilWtrSto[i][k] + m_soilWP[i][k] - m_solice[i][k];
        } else{
            m_solwc[i][k] = m_soilWtrSto[i][k];
        }
        
        //limit the wc+ice !> pormm
        float rto = m_solwc[i][k]/(m_solwc[i][k]+m_solice[i][k]);
        if((m_solwc[i][k]+m_solice[i][k]) >  m_solpormm[i][k]) {
            m_solwc[i][k] = m_solpormm[i][k]*rto;
            m_solice[i][k] = m_solpormm[i][k] -  m_solwc[i][k];
        }
        m_solwcv[i][k]  = m_solwc[i][k] / m_solthic[i][k];
        m_solorg[i][k]  = (m_soilRsd[i][k]) / (0.5*10000); //note:m_solhum is zero
        m_solorgv[i][k] = m_solorg[i][k] / (m_solthic[i][k]);
        m_solminv[i][k] = m_soilBD[i][k] / 2.65 - m_solorgv[i][k];
        m_solicev[i][k] = m_solice[i][k] / m_solthic[i][k];
        m_solair[i][k]  = m_solpormm[i][k]- m_solwc[i][k]-m_solice[i][k];

    }   
    //subroutine snom1
    //Snow depth
    float a=1;
	float b=0.3;
    //Effective density of snow
    m_snoden[i]=(b+a*m_snowAcc[i]/1000);

    m_snodep[i]=m_snowAcc[i]/ m_snoden[i];

    if(m_snowAcc[i] <= 0.f) {
        m_snodep[i] = 0.f;
        m_snoden[i] = 0.f;
    }

    //call solthermal   !thermal parameters
    //snow thermal parameters
    if( m_ksno[i] >= 864*2 ) {
	    m_ksno[i] = 864*2;
    } else {
	    if ( m_snoden[i] < 0.156 ) {
	        m_ksno[i] = 0.023+0.234*m_snoden[i];
        }else{
	        m_ksno[i] = 0.138 - 1.01*m_snoden[i] + 3.233*pow(m_snoden[i],2.);
	    }
        m_ksno[i]=m_ksno[i] * 864. * m_kscoe[i];	 
	} 
    m_casno[i]=1.9 * pow(m_snoden[i], 2.) / 0.917;  //??
    
    //heat capacities of soil layers
    for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        m_casol[i][k]=4.1868*( 0.48*m_solminv[i][k] + 0.6*m_solorgv[i][k] + 1*m_solwcv[i][k]+0.45*m_solicev[i][k]);
	    m_casol[i][k]=m_casol[i][k]*m_ccoe[i];
    //}
        
    //soil thermal conductivity  in the frozen and unfrozen states. 
    //for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        m_solsatu[i][k]= Max((m_solwc[i][k]+m_solice[i][k])/m_solpormm[i][k], 0.0001);
        m_solkd[i][k]=864*((0.135*m_soilBD[i][k]*1000+64.7) / (2700-0.947*m_soilBD[i][k]*1000));
    //}
    //for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        if ((m_solice[i][k] /m_solwc[i][k]) > 1) {
            m_solke[i][k]= m_solsatu[i][k];
            m_solksat[i][k]=pow((864*2.9),(1-m_soilPor[i][k])) * pow((864*2.2),(m_soilPor[i][k]-m_solwcv[i][k])) * pow((0.57*864),m_solwcv[i][k]);
        }else{
            if (m_solsatu[i][k]>0.1){
	            m_solke[i][k]=log10(m_solsatu[i][k])+1;
	        }else {
                m_solke[i][k]=0;
	        }
            m_solksat[i][k]=pow((864*0.57),m_soilPor[i][k]) * pow((2.9*864),(1-m_soilPor[i][k]));
        }
        m_ksol[i][k]=(m_solksat[i][k]-m_solkd[i][k])*m_solke[i][k]+m_solkd[i][k] ;
        m_ksol[i][k]=Max(m_ksol[i][k],m_solkd[i][k]);
        m_ksol[i][k]= m_ksol[i][k]* m_kcoe[i][k];  //Calibration
    }

    // call soltly       !阻尼深度：在一定的周期上土壤温度的振幅减少至表层土壤温度振幅1/e倍的深度(m)
    //calculate maximum damping depth
    //SWAT manual equation 2.3.6
    float f = 0.;
    float d = 0.;
    float dp = 0.;
    f = m_soilAVBD[i] / (m_soilAVBD[i] + 686. * exp(-5.63 * m_soilAVBD[i]));
    dp = m_basedep[i] + 2500. * f; //ljj base depth =1000
    //dp = m_basedep[i] + 2500. * f; 

    //calculate scaling factor for soil water
    //SWAT manual equation 2.3.7
	float ww = 0.f;
	float wc = 0.f;
	ww = 0.356f - 0.144f * m_soilAVBD[i];
	wc = m_solsw[i] / (ww * Max(m_soildepth[i][m_maxSoilLyrs-1],1.e-20f));

    //! calculate daily value for damping depth
    // SWAT manual equation 2.3.8
	b = 0.f;
	f = 0.f;
	d = 0.f;
	b = log(500.f / dp);
	f = exp(b * pow(((1.f - wc) / (1.f + wc)), 2.f));
	d = f * dp;

    ddepth[i]=d;
    ddepth[i]=dp;

    int nlyr=5; //!!????ljj
    //assume the depth below the deepest soil layer is devided into 5 layer
    int nly1=0;
    int maxk = m_nSoilLyrs[i];
    if (ddepth[i]> (m_soildepth[i][maxk-1]) ){
        nly1 = m_nSoilLyrs[i]+ nlyr;
        for (int k = 0; k < nlyr; k++) {
            int kk = m_nSoilLyrs[i]+k;
            m_solthic[i][kk] = (ddepth[i] - m_soildepth[i][maxk-1])/nlyr;
            m_ksol[i][kk] = m_ksol[i][maxk-1];
	        m_casol[i][kk] = m_casol[i][maxk-1];
        }
        m_solnd[i][maxk]= m_soildepth[i][maxk-1] + m_solthic[i][maxk]/2;
        m_solnd[i][maxk+1]= m_solnd[i][maxk] + m_solthic[i][maxk];
        m_solnd[i][maxk+2]= m_solnd[i][maxk+1] + m_solthic[i][maxk];
        m_solnd[i][maxk+3]= m_solnd[i][maxk+2] + m_solthic[i][maxk];
        m_solnd[i][maxk+4]= m_solnd[i][maxk+3] + m_solthic[i][maxk];
    }else{
        for(int k = m_nSoilLyrs[i]-1; k > -1; k--){
	        if(ddepth[i]>m_soildepth[i][k-1] && ddepth[i]<=m_soildepth[i][k] ) nly1=k;
        }
	}

    //call soltbott     !土壤底层温度计算
    f=0;
    float tamp=0;
    f=0.00032*60*60*24*365.25/3.14159265/0.2;
    f=sqrt(f);
	float ansnodep=1200;

	tamp = (m_tmpmx[i] - m_tmpmn[i]) / 2;
    m_bottmp[i]= m_annMeanTemp[i]+(0.5*tamp)*(1-exp(-ansnodep/10/f));
    
    //call soltsurf  !地表温度计算，考虑了冠层和雪盖作用
    if (m_igro[i] == 1) {
	int idc = CVT_INT(m_landCoverCls[i]);
        if (idc == CROP_IDC_TREES ) {
            float t_c = -0.11+0.96*m_meanTemp[i]-0.00008*pow(m_meanTemp[i],3);
            m_ambtmp[i]=m_meanTemp[i]+(t_c-m_meanTemp[i]) *(log(1+Min(m_maxLai[i],m_lai[i])) / log(1+ m_maxLai[i]));
        } else{
            m_ambtmp[i]=m_meanTemp[i];
        }
    }else{ 
        m_ambtmp[i]=m_meanTemp[i];
    }
    //adjust for areal extent of snow cover
    float snocov1 = 0.f;
    float snocov2 = 0.f;
    if (m_snowAcc[i]  < m_snowCoverMax) {
        float xx = 0.f;
        xx = log(m_snowCover50 / 0.5f - m_snowCover50);
        snocov2 = (xx - log(0.95f / 0.95f - 0.95f)) / (0.95f - m_snowCover50);
        snocov1 = xx + m_snowCover50 * (snocov2);
        xx = m_snowAcc[i]  / m_snowCoverMax;
        m_snoco[i] = xx / (xx + exp(snocov1 - snocov2 * xx));
    }else{
        m_snoco[i] = 1.;
    }
    float cej = -5.e-5;
    float eaj = 0.;
    eaj = exp(cej * (m_soilcov[i] + .1));

    if (m_snoco[i] <= 0.5) {
        m_alb[i] = m_soilALB[i];
    }else{
        m_alb[i] = 0.8;
    }

    //net radiation
    float ralb = 0.;
    ralb = m_sr[i] *(1.0 - m_alb[i]);  

    if (m_igro[i] == 1){   
        ralb = m_sr[i]*(1.0 -  m_alb[i])*exp(-m_lightExtCoef[i]*m_lai[i]);
	}else{
        ralb =m_sr[i]*(1.0 - m_alb[i]);
	}
    
    //calculate net long-wave radiation--
    //calculate the max solar radiation
    float srMax = 0.f;
    if(m_cellLat[i]>90.f){
        MaxSolarRadiation(m_dayOfYear, m_cellLat[i], m_dayLen[i], srMax);
    }else{
        MaxSolarRadiation(m_dayOfYear, 30.f, m_dayLen[i], srMax);
    }
    //calculate net long-wave radiation
    //net emissivity  equation 2.2.20 in SWAT manual
    float satVaporPressure = SaturationVaporPressure(m_meanTemp[i]); //kPa
    float actualVaporPressure = 0.f;
    if (m_rhd[i] > 1) {
        /// IF percent unit.
        actualVaporPressure = m_rhd[i] * satVaporPressure * 0.01f;
    } else {
        actualVaporPressure = m_rhd[i] * satVaporPressure;
    }
    //m_vpd[j] = satVaporPressure - actualVaporPressure;
    float rbo = -(0.34f - 0.139f * sqrt(actualVaporPressure)); //P37 1:1.2.22
    //cloud cover factor equation 2.2.19
    float rto = 0.0f;
    srMax = Max(srMax,m_sr[i]);
    if (srMax >= 1.0e-4f) {
        rto = 0.9f * (m_sr[i] / srMax) + 0.1f;
    }
    //net long-wave radiation equation 2.2.21
    float tk = m_meanTemp[i] + 273.15f;
    float rout = rbo * rto * 4.9e-9f * pow(tk, 4.f);
    
    // calculate net radiation
    float rn_pet = 0.;
    rn_pet = ralb + rout;
    //why *100??? MJ/M2 to J/CM2 
    rn_pet = 100*rn_pet;
    ralb=100*ralb;

    float eff_conr=8.1*ralb*(1 - exp(Min(m_maxLai[i],m_lai[i])-6.8))/86400;
    eff_conr=eff_conr*m_effcoe[i];  //calibration
    eff_conr = Max(eff_conr,0.f);
   
	if (m_snowAcc[i] <=0) {
        m_surtmp[i]=m_ambtmp[i]/(1+eff_conr)+eff_conr/(1+eff_conr)*(m_soltmp1[i][0]+rn_pet/(m_ksol[i][0]/((m_solthic[i][0]/10)/2)));
    }else{
        m_surtmp[i]=m_ambtmp[i]/(1+eff_conr)+eff_conr/(1+eff_conr)*(m_snotmp1[i]+rn_pet/(m_ksno[i]/((m_snodep[i]/10)/2)));
	}

    //call soltcal      !土壤内部温度计算
    if (m_snoco[i]>0.5 ){
        m_kss[i]=m_ksno[i]*m_ksol[i][0]*(m_snodep[i]/10/2+m_solthic[i][0]/10/2)/(m_ksno[i]*m_solthic[i][0]/10/2+m_ksol[i][0]*m_snodep[i]/10/2);
    }else{
	    m_kss[i]=m_ksol[i][0];
    }

    for (int k = 0; k < nly1-1; k++) {
        m_kint[i][k]=m_ksol[i][k]*m_ksol[i][k+1]*(m_solthic[i][k]/10/2 + m_solthic[i][k+1]/10/2) / (m_ksol[i][k]*m_solthic[i][k+1] / 10/2+m_ksol[i][k+1]*m_solthic[i][k]/10/2);
        m_bsol[i][k]=m_kint[i][k] /(m_solthic[i][k]/10/2+m_solthic[i][k+1]/10/2);
        m_csol[i][k+1]=m_kint[i][k] /(m_solthic[i][k]/10/2+m_solthic[i][k+1]/10/2) ;
    }
    
    m_kint[i][nly1-1]=m_ksol[i][nly1-1]/(m_solthic[i][nly1-1]/10/2);
    m_bsol[i][nly1-1]=m_kint[i][nly1-1]/(m_solthic[i][nly1-1]/10/2) ;

    if (m_snoco[i]>0.5 ){
        m_csol[i][0]=m_kss[i]/(m_snodep[i]/10/2+m_solthic[i][0]/10/2);
    }else{
	    m_csol[i][0]=m_kint[i][0]/(m_solthic[i][0]/10/2);
    }
    int solft=1; //solft=0 no freeze and thaw; solft=1 with freeze and thaw
    int supercool = 0; //ljj++
    if(solft==0) {
       for (int k = 0; k < nly1; k++) {
          m_asol[i][k]=m_bsol[i][k]+m_csol[i][k]+m_casol[i][k]*(m_solthic[i][k]/10)/1 ;
          m_dsol[i][k]=m_casol[i][k]*(m_solthic[i][k]/10)*m_soltmp1[i][k]/1;
       }
	}
   
    if(solft==1) { 
        for (int k = 0; k <nly1; k++) {
            m_supercool[i][k]= 0.f;
            m_asol[i][k]=m_bsol[i][k]+m_csol[i][k]+m_casol[i][k]*(m_solthic[i][k]/10)/1 ;
            //call soltfretha(k)!! latent heat source  
            if (k<=m_nSoilLyrs[i]-1) {
                if (m_soltmp1[i][k]<=0.f) { //---------------freeziing---------------
                    a=0;
	                b=0;
	                a=m_casol[i][k]*(m_solthic[i][k]/10)*m_soltmp1[i][k] ;
                    b=334*m_solwc[i][k]/10;                             
	                m_dsols[i][k]=a+b;
                    //ljj++ supercooled soil water
                    // SMP = HFUS*(TFRZ-STC(J))/(GRAV*STC(J))             
                    // SUPERCOOL(J) = SMCMAX(J)*(SMP/PSISAT(J))**(-1./BEXP(J))
                    // SUPERCOOL(J) = SUPERCOOL(J)*DZSNSO(J)*1000. 
                    float HFUS = 0.3336e6;
                    float GRAV = 9.80616;
                    m_smp[i][k]= HFUS*(0.f-m_soltmp1[i][k])/(GRAV*(m_soltmp1[i][k]+273.15));   //m
                    
                    //m_smp[i][k] = Max(0.f,m_smp[i][k]);
                    //BEXP(I)   = 2.91+0.159*myClay(I)
                    float BEXP = 2.91+0.159*m_clay[i][k];
                    //float BEXP = -3.140-0.00222*m_clay[i][k]*m_clay[i][k]-0.000003484*m_clay[i][k]*m_sand[i][k]*m_sand[i][k];
                    //SMCMAX is porosity of liquid water
                    //PSISAT is saturated soil matric potential 0.01*10**( 1.88-0.0131*mySand(I) )
                    float PSISAT = 0.01*pow(10,( 1.88-0.0131*m_sand[i][k] ));
                    //float PSISAT =(exp(-4.396-0.0715*m_clay[i][k]-0.0004880*m_sand[i][k]*m_sand[i][k]-0.000004285*m_clay[i][k]*m_sand[i][k]*m_sand[i][k]));
                    PSISAT = Min(PSISAT,1.f);
                   // m_supercool[i][k]= (m_soilSat[i][k])*(pow((m_smp[i][k]/PSISAT),(-1./BEXP))); 
                    m_supercool[i][k]= (m_solpormm[i][k])*(pow((m_smp[i][k]/PSISAT),(-1./BEXP))); 
                    //m_supercool[i][k] = Min(m_supercool[i][k],0.05f*m_soilthick[i][k]*m_soilBD[i][k]);
                    
                    if(m_soildepth[i][k]>=ddepth[i]) m_supercool[i][k] =0.f;

                    //
                    if (m_dsols[i][k]>=0) {
                        m_dice[i][k] = -a/334*10;
                        if(supercool==1 && m_solwc[i][k] - m_dice[i][k] <m_supercool[i][k]) m_dice[i][k] = m_solwc[i][k]-m_supercool[i][k]; 
                        m_dice[i][k] = Max(m_dice[i][k],0.f);
                        m_dice[i][k] = Min(m_dice[i][k], m_solwc[i][k]);
                        m_solwc[i][k] =m_solwc[i][k] - m_dice[i][k];
                        m_solice[i][k]=m_solice[i][k]+ m_dice[i][k];
		                m_dsol[i][k]=0; 
                        //if(supercool==1 && m_solwc[i][k] - m_dice[i][k] <m_supercool[i][k]) m_dsol[k]= (a+334*m_solwc[i][k]/10)/1;  // m_dsol[k]= (a+b)/1;
                    }else{
                        m_dsol[i][k]= (a+b)/1;    
                        m_dice[i][k]=m_solwc[i][k];
                        if(supercool==1 && m_solwc[i][k] - m_dice[i][k] <m_supercool[i][k]) m_dice[i][k] = m_solwc[i][k]-m_supercool[i][k];
                        m_dice[i][k] = Max(m_dice[i][k],0.f);
                        m_dice[i][k] = Min(m_dice[i][k], m_solwc[i][k]);
                        m_solice[i][k]=m_solice[i][k]+ m_dice[i][k]; 
                        m_solwc[i][k]=m_solwc[i][k] - m_dice[i][k];         
                        //if(supercool==1 && m_solwc[i][k] - m_dice[i][k] <m_supercool[i][k]) m_dsol[k]= (a+334*m_solwc[i][k]/10)/1;
                    }
                }else{ //-------------------------thawing--------------------------------
                    a=0;
	                b=0;
	                a=m_solthic[i][k]/10*m_casol[i][k]*m_soltmp1[i][k];
                    b=334*m_solice[i][k];                          
	                m_dsols[i][k]=a-b;
                    if (m_dsols[i][k]<=0) {
                        m_dice[i][k]=-a/334*10;
                        m_solwc[i][k] = m_solwc[i][k] - m_dice[i][k];
                        m_solice[i][k]=m_solice[i][k]+ m_dice[i][k];
		                m_dsol[i][k]=0;
                    }else{
                        m_dsol[i][k]=(a-b) /1;
		                m_dice[i][k]=m_solice[i][k];
                        m_solwc[i][k]=m_solwc[i][k]+m_dice[i][k];
		                m_solice[i][k]=0;
                        
                    }
                }
                if(m_solice[i][k]<=0.f){
                    m_solwc[i][k] += m_solice[i][k];
                    m_solice[i][k] = 0.f;
                }
            }else{
                //added soil layers without freezing and thaw
                m_dsol[i][k]=m_casol[i][k]*(m_solthic[i][k]/10)*m_soltmp1[i][k];
            } 
        }
    }
    
    if ( m_snoco[i]>0.5 ) {
        m_bsno[i]=m_kss[i]/(m_snodep[i]/10/2+m_solthic[i][0]/10/2); 
        m_csno[i]=m_ksno[i]/(m_snodep[i]/10/2);            
		m_asno[i]=m_bsno[i]+m_csno[i]+m_casno[i]*(m_snodep[i]/10);
		m_dsno[i]=m_casno[i]*(m_snodep[i]/10)*m_snotmp1[i];
        m_psno[i]=m_bsno[i] / (m_asno[i]);
        m_qsno[i]=(m_dsno[i]+m_csno[i]*m_surtmp[i])/ m_asno[i];
    }else{
	    m_bsno[i]=0;
        m_csno[i]=0;
        m_asno[i]=0;
        m_dsno[i]=0;
        m_psno[i]=0;
        m_qsno[i]=0;
    }
    
    if (m_snoco[i]>0.5 ) {
        m_psol[i][0]=m_bsol[i][0]/(m_asol[i][0]-m_csol[i][0]*m_psno[i]);
        m_qsol[i][0]=(m_dsol[i][0]+m_csol[i][0]*m_qsno[i])/ (m_asol[i][0]-m_csol[i][0]*m_psno[i]);
    }else{
	    m_psol[i][0]=m_bsol[i][0]/(m_asol[i][0]);
        m_qsol[i][0]=(m_dsol[i][0]+m_csol[i][0]*m_surtmp[i])/ (m_asol[i][0]);
    }

    for (int k = 1; k < nly1; k++) {
        m_psol[i][k]=m_bsol[i][k] / (m_asol[i][k]-m_csol[i][k]*m_psol[i][k-1]);
        m_qsol[i][k]=(m_dsol[i][k]+m_csol[i][k]*m_qsol[i][k-1])/(m_asol[i][k]-m_csol[i][k]*m_psol[i][k-1]);
    }
    m_soltmp1[i][nly1-1]=m_psol[i][nly1-1]*m_bottmp[i]+m_qsol[i][nly1-1];

    for (int k = nly1-2; k > -1; k--) {
        m_soltmp1[i][k]=m_psol[i][k]*m_soltmp1[i][k+1]+m_qsol[i][k];
    }

    for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
		m_soilt[i][k]=m_soltmp1[i][k]; 
    }
    //------------------Snowpackage temperature update--------------------------
    m_presnotmp[i]=m_soltmp1[i][0];
	float snot=0;
    if ( m_snoco[i]>0.5 ){
		snot=m_psno[i]*m_soltmp1[i][0]+m_qsno[i] ;
    }else{ 
	    m_snotmp1[i]=0;
    } 
    m_snotmp1[i]=Min(0.,snot);
    
    //call soltphys_out 
    for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
        m_solair[i][k] = m_solpormm[i][k]- m_solwc[i][k]-m_solice[i][k];
        m_solair[i][k]= Max(m_solair[i][k] , 0.0);
        m_solair[i][k]= Min(m_solpormm[i][k] , m_solair[i][k]) ;

        m_solice[i][k]= Max(m_solice[i][k] , 0.0);
        m_solice[i][k]= Min(m_solpormm[i][k] , m_solice[i][k]);

        m_solwc[i][k] = Max(m_solwc[i][k] , 0.0);
        m_solwc[i][k] = Min(m_solpormm[i][k] , m_solwc[i][k]); 
        if( m_solice[i][k] <= m_soilWP[i][k] ){
            m_soilWtrSto[i][k] = m_solwc[i][k]-(m_soilWP[i][k] - m_solice[i][k]); 
        }else{  
            m_soilWtrSto[i][k] = m_solwc[i][k];   
        }
        m_soilWtrSto[i][k] = Max(m_soilWtrSto[i][k],0.0); 
        m_soilWtrSto[i][k] = Min(m_solpormm[i][k]-m_soilWP[i][k] , m_soilWtrSto[i][k]) ;  
    } 

}
void SoilTemperatureFINPL::SoilTempSWAT(const int i) {
	float tlag = 0.8f;  ///default is 0.8
	float xx, zd, df;

	float f = 0.f;
	float dp = 0.f;
	f = m_soilAVBD[i] / (m_soilAVBD[i] + 686. * exp(-5.63 * m_soilAVBD[i]));
	dp = 1000.f + 2500.f * f;
    //dp = m_basedep[i];

	float ww = 0.f;
	float wc = 0.f;
	ww = 0.356f - 0.144f * m_soilAVBD[i];
	wc = m_solsw[i] / (ww * Max(m_soildepth[i][m_maxSoilLyrs-1],1.e-20f));

	float b = 0.f;
	f = 0.f;
	float dd = 0.f;
	b = log(500.f / dp);
	f = exp(b * pow(((1.f - wc) / (1.f + wc)), 2.f));
	dd = f * dp;

	float bcv = 0.f;
	bcv = m_soilcov[i] / (m_soilcov[i] + exp(7.563 - 1.297e-4 * m_soilcov[i]));
	if (m_snowAcc[i] > 0.0001f) {
		if (m_snowAcc[i] <= 120.) {
			xx = 0.f;
			xx = m_snowAcc[i] / (m_snowAcc[i] + exp(6.055 - .3002 * m_snowAcc[i]));
		}
		else {
			xx = 1.f;
		}
		bcv = Max(xx, bcv);
	}

	//calculate temperature at soil surface
	float st0 = 0.f;
	float tbare = 0.f;
	float tcov = 0.f;
	float tmp_srf = 0.f; 
	st0 = (m_sr[i] * (1. - m_alb[i]) - 14.f) / 20.f;
	tbare = m_meanTemp[i] + 0.5f * (m_tMax[i] - m_tMin[i]) * st0;
	tcov = bcv * m_soilt[i][1] + (1.f - bcv) * tbare;

	tmp_srf = 0.5f * (tbare + tcov);

	//calculate temperature for each layer on current day
	xx = 0.;
	for (int k = 0; k < CVT_INT(m_nSoilLyrs[i]); k++) {
		zd = 0.;
		df = 0.;
		zd = (xx + m_soildepth[i][k]) / 2.;
		zd = zd / dd;
		df = zd / (zd + exp(-.8669 - 2.0775 * zd));
		
		m_soilt[i][k] = tlag * m_soilt[i][k] + (1. - tlag) *(df * (m_meanTemp[i] - tmp_srf) + tmp_srf);
		xx = m_soildepth[i][k];
		//if (isep_opt(j) /= 0.and.iyr >= isep_iyr(j).and.k >= i_sep(j)) {
		//	if (m_soilt[i][k] < 10.f) {
		//		m_soilt[i][k] = 10.f - (10.f - m_soilt[i][k]) * 0.1f;
		//}
		//}
	}
}
