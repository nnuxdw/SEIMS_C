#include "SoilTemperatureNOAA.h"

#include "text.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

using std::max;
using std::min;
using std::string;
using std::vector;

const double SoilTemperatureNOAA::HC_WATER = 4.188E6;
const double SoilTemperatureNOAA::HC_ICE = 2.094E6;
const double SoilTemperatureNOAA::HC_AIR = 1004.64;
const double SoilTemperatureNOAA::HC_MINERAL = 2.0E6;
const double SoilTemperatureNOAA::HC_ORGANIC = 2.5E6;
const double SoilTemperatureNOAA::GRAVITY = 9.80616;
const double SoilTemperatureNOAA::TFREEZ = 273.15;
const double SoilTemperatureNOAA::WDENSITY = 1000.0;
const float SoilTemperatureNOAA::RAD_WT = 0.01721420632103996f;

namespace {

inline double Clamp(const double value, const double lower, const double upper) {
    return max(lower, min(upper, value));
}

string LayerError(const int cell, const int layer, const string& variable,
                  const double value, const string& expected) {
    std::ostringstream oss;
    oss << "cell=" << cell << ", layer=" << layer << ", variable=" << variable
        << ", value=" << value << ", expected=" << expected;
    return oss.str();
}

}  // namespace

SoilTemperatureNOAA::SoilTemperatureNOAA()
    : m_nCells(-1),
      m_maxSoilLyrs(-1),
      m_maxThermalLyrs(-1),
      m_dt(NODATA_VALUE),
      m_b(4.f),
      m_quartz(0.2f),
      m_satpsi(0.04f),
      m_latentHeatFusion(0.3336E6f),
      m_a0(NODATA_VALUE),
      m_a1(NODATA_VALUE),
      m_a2(NODATA_VALUE),
      m_a3(NODATA_VALUE),
      m_b1(NODATA_VALUE),
      m_b2(NODATA_VALUE),
      m_d1(NODATA_VALUE),
      m_d2(NODATA_VALUE),
      m_kSoil10(NODATA_VALUE),
      m_nSoilLyrs(nullptr),
      m_soilTempRelFactor10(nullptr),
      m_landUse(nullptr),
      m_meanTemp(nullptr),
      m_annMeanTemp(nullptr),
      m_rsdInitSoil(nullptr),
      m_soilAVBD(nullptr),
      m_solsw(nullptr),
      m_snowAcc(nullptr),
      m_soildepth(nullptr),
      m_soilthick(nullptr),
      m_soilPor(nullptr),
      m_soilWP(nullptr),
      m_soilWtrSto(nullptr),
      m_soilSat(nullptr),
      m_soilBD(nullptr),
      m_soilRsd(nullptr),
      m_clay(nullptr),
      m_sand(nullptr),
      m_nThermalLyrs(nullptr),
      m_solthic(nullptr),
      m_solnd(nullptr),
      m_layerThicknessM(nullptr),
      m_layerCenterDepthM(nullptr),
      m_solpormm(nullptr),
      m_solwc(nullptr),
      m_solice(nullptr),
      m_solwcv(nullptr),
      m_solicev(nullptr),
      m_solair(nullptr),
      m_solorgv(nullptr),
      m_solminv(nullptr),
      m_soilTemperature(nullptr),
      m_soilTemperaturePrev(nullptr),
      m_soilLiquidContent(nullptr),
      m_soilIceContent(nullptr),
      m_thermalConductivity(nullptr),
      m_heatCapacity(nullptr),
      m_surfaceBoundaryTemp(nullptr),
      m_groundHeatFlux(nullptr),
      m_bottomHeatFlux(nullptr),
      m_iceFractionSchaake(nullptr),
      m_iceFractionXinanjiang(nullptr),
      m_soilTemp(nullptr),
      m_soilt(nullptr),
      m_meanTempPre1(nullptr),
      m_meanTempPre2(nullptr),
      m_SOTE1(nullptr),
      m_SOTE5(nullptr),
      m_SOTE15(nullptr),
      m_SOTE30(nullptr),
      m_SOTE60(nullptr),
      m_SOTE100(nullptr),
      m_SOTE200(nullptr),
      m_profileInitialized(false),
      m_physicalStateInitialized(false),
      m_temperatureInitialized(false),
      m_lastHistoryDay(-1) {}

SoilTemperatureNOAA::~SoilTemperatureNOAA() {
    Release1DArray(m_nThermalLyrs);
    Release1DArray(m_surfaceBoundaryTemp);
    Release1DArray(m_groundHeatFlux);
    Release1DArray(m_bottomHeatFlux);
    Release1DArray(m_iceFractionSchaake);
    Release1DArray(m_iceFractionXinanjiang);
    Release1DArray(m_soilTemp);
    Release1DArray(m_meanTempPre1);
    Release1DArray(m_meanTempPre2);
    Release1DArray(m_SOTE1);
    Release1DArray(m_SOTE5);
    Release1DArray(m_SOTE15);
    Release1DArray(m_SOTE30);
    Release1DArray(m_SOTE60);
    Release1DArray(m_SOTE100);
    Release1DArray(m_SOTE200);

    Release2DArray(m_nCells, m_solthic);
    Release2DArray(m_nCells, m_solnd);
    Release2DArray(m_nCells, m_layerThicknessM);
    Release2DArray(m_nCells, m_layerCenterDepthM);
    Release2DArray(m_nCells, m_solpormm);
    Release2DArray(m_nCells, m_solwc);
    Release2DArray(m_nCells, m_solice);
    Release2DArray(m_nCells, m_solwcv);
    Release2DArray(m_nCells, m_solicev);
    Release2DArray(m_nCells, m_solair);
    Release2DArray(m_nCells, m_solorgv);
    Release2DArray(m_nCells, m_solminv);
    Release2DArray(m_nCells, m_soilTemperature);
    Release2DArray(m_nCells, m_soilTemperaturePrev);
    Release2DArray(m_nCells, m_soilLiquidContent);
    Release2DArray(m_nCells, m_soilIceContent);
    Release2DArray(m_nCells, m_thermalConductivity);
    Release2DArray(m_nCells, m_heatCapacity);
    Release2DArray(m_nCells, m_soilt);
}

void SoilTemperatureNOAA::SetValue(const char* key, const float value) {
    const string sk(key);
    if (StringMatch(sk, Tag_TimeStep) || StringMatch(sk, VAR_SOIL_DT)) m_dt = value;
    else if (StringMatch(sk, VAR_B)) {
        if (!FloatEqual(value, NODATA_VALUE) && value > 0.f) m_b = value;
    } else if (StringMatch(sk, VAR_QUARTZ)) {
        if (!FloatEqual(value, NODATA_VALUE)) m_quartz = value;
    } else if (StringMatch(sk, VAR_SATPSI)) {
        if (!FloatEqual(value, NODATA_VALUE) && value > 0.f) m_satpsi = value;
    } else if (StringMatch(sk, VAR_LATENT_HEAT)) {
        if (!FloatEqual(value, NODATA_VALUE) && value > 0.f) m_latentHeatFusion = value;
    }
    else if (StringMatch(sk, VAR_SOL_TA0)) m_a0 = value;
    else if (StringMatch(sk, VAR_SOL_TA1)) m_a1 = value;
    else if (StringMatch(sk, VAR_SOL_TA2)) m_a2 = value;
    else if (StringMatch(sk, VAR_SOL_TA3)) m_a3 = value;
    else if (StringMatch(sk, VAR_SOL_TB1)) m_b1 = value;
    else if (StringMatch(sk, VAR_SOL_TB2)) m_b2 = value;
    else if (StringMatch(sk, VAR_SOL_TD1)) m_d1 = value;
    else if (StringMatch(sk, VAR_SOL_TD2)) m_d2 = value;
    else if (StringMatch(sk, VAR_K_SOIL10)) m_kSoil10 = value;
    else throw ModelException(MID_STP_NOAA, "SetValue", "Parameter " + sk + " does not exist.");
}

void SoilTemperatureNOAA::Set1DData(const char* key, const int n, float* data) {
    CheckInputSize(MID_STP_NOAA, key, n, m_nCells);
    const string sk(key);
    if (StringMatch(sk, VAR_SOILLAYERS)) m_nSoilLyrs = data;
    else if (StringMatch(sk, VAR_SOL_RSDIN)) m_rsdInitSoil = data;
    else if (StringMatch(sk, VAR_SOL_SW)) m_solsw = data;
    else if (StringMatch(sk, VAR_SOLAVBD)) m_soilAVBD = data;
    else if (StringMatch(sk, VAR_SNAC)) m_snowAcc = data;
    else if (StringMatch(sk, VAR_TMEAN_ANN)) m_annMeanTemp = data;
    else if (StringMatch(sk, VAR_TMEAN)) m_meanTemp = data;
    else if (StringMatch(sk, VAR_LANDUSE)) m_landUse = data;
    else if (StringMatch(sk, VAR_SOIL_T10)) m_soilTempRelFactor10 = data;
    else throw ModelException(MID_STP_NOAA, "Set1DData", "Parameter " + sk + " does not exist.");
}

void SoilTemperatureNOAA::Set2DData(const char* key, const int n, const int col, float** data) {
    CheckInputSize2D(MID_STP_NOAA, key, n, col, m_nCells, m_maxSoilLyrs);
    const string sk(key);
    if (StringMatch(sk, VAR_SOILDEPTH)) m_soildepth = data;
    else if (StringMatch(sk, VAR_SOILTHICK)) m_soilthick = data;
    else if (StringMatch(sk, VAR_POROST)) m_soilPor = data;
    else if (StringMatch(sk, VAR_SOL_ST)) m_soilWtrSto = data;
    else if (StringMatch(sk, VAR_SOL_WPMM)) m_soilWP = data;
    else if (StringMatch(sk, VAR_SOL_BD)) m_soilBD = data;
    else if (StringMatch(sk, VAR_SOL_RSD)) m_soilRsd = data;
    else if (StringMatch(sk, "clay")) m_clay = data;
    else if (StringMatch(sk, "sand")) m_sand = data;
    else if (StringMatch(sk, VAR_SOL_UL)) m_soilSat = data;
    else throw ModelException(MID_STP_NOAA, "Set2DData", "Parameter " + sk + " does not exist.");
}

bool SoilTemperatureNOAA::CheckInputData() {
    CHECK_POSITIVE(MID_STP_NOAA, m_nCells);
    CHECK_POSITIVE(MID_STP_NOAA, m_maxSoilLyrs);
    CHECK_POSITIVE(MID_STP_NOAA, m_dt);
    CHECK_POSITIVE(MID_STP_NOAA, m_b);
    CHECK_POSITIVE(MID_STP_NOAA, m_satpsi);
    CHECK_POSITIVE(MID_STP_NOAA, m_latentHeatFusion);
    CHECK_NODATA(MID_STP_NOAA, m_a0);
    CHECK_NODATA(MID_STP_NOAA, m_a1);
    CHECK_NODATA(MID_STP_NOAA, m_a2);
    CHECK_NODATA(MID_STP_NOAA, m_a3);
    CHECK_NODATA(MID_STP_NOAA, m_b1);
    CHECK_NODATA(MID_STP_NOAA, m_b2);
    CHECK_NODATA(MID_STP_NOAA, m_d1);
    CHECK_NODATA(MID_STP_NOAA, m_d2);
    CHECK_NODATA(MID_STP_NOAA, m_kSoil10);
    CHECK_POINTER(MID_STP_NOAA, m_nSoilLyrs);
    CHECK_POINTER(MID_STP_NOAA, m_soildepth);
    CHECK_POINTER(MID_STP_NOAA, m_soilPor);
    CHECK_POINTER(MID_STP_NOAA, m_soilWP);
    CHECK_POINTER(MID_STP_NOAA, m_soilWtrSto);
    CHECK_POINTER(MID_STP_NOAA, m_soilSat);
    CHECK_POINTER(MID_STP_NOAA, m_soilBD);
    CHECK_POINTER(MID_STP_NOAA, m_clay);
    CHECK_POINTER(MID_STP_NOAA, m_sand);
    CHECK_POINTER(MID_STP_NOAA, m_soilAVBD);
    CHECK_POINTER(MID_STP_NOAA, m_meanTemp);
    CHECK_POINTER(MID_STP_NOAA, m_annMeanTemp);
    CHECK_POINTER(MID_STP_NOAA, m_landUse);
    CHECK_POINTER(MID_STP_NOAA, m_soilTempRelFactor10);
    if (m_rsdInitSoil == nullptr && m_soilRsd == nullptr) {
        throw ModelException(MID_STP_NOAA, "CheckInputData", "SOL_RSDIN or SOL_RSD must be supplied.");
    }

    for (int i = 0; i < m_nCells; ++i) {
        const int nReal = CVT_INT(m_nSoilLyrs[i]);
        if (nReal <= 0 || nReal > m_maxSoilLyrs) {
            throw ModelException(MID_STP_NOAA, "CheckInputData",
                                 LayerError(i, -1, VAR_SOILLAYERS, nReal, "1..max soil layers"));
        }
        float previousBottom = 0.f;
        for (int k = 0; k < nReal; ++k) {
            const float bottom = m_soildepth[i][k];
            if (!(bottom > previousBottom)) {
                throw ModelException(MID_STP_NOAA, "CheckInputData",
                                     LayerError(i, k, VAR_SOILDEPTH, bottom, "strictly increasing and > 0 mm"));
            }
            const float thickness = bottom - previousBottom;
            const float porosity = m_soilPor[i][k];
            const float poreMM = porosity * thickness;
            if (!(porosity > 0.f && porosity <= 1.f)) {
                throw ModelException(MID_STP_NOAA, "CheckInputData",
                                     LayerError(i, k, VAR_POROST, porosity, "(0, 1]"));
            }
            // Water and other non-soil cells may carry zero layer density in
            // legacy SEIMS datasets. They are handled with the profile-average
            // density (or the standard 1.4 g/cm3 fallback) during initialization.
            if (m_soilWP[i][k] < 0.f || m_soilWP[i][k] >= poreMM) {
                throw ModelException(MID_STP_NOAA, "CheckInputData",
                                     LayerError(i, k, VAR_SOL_WPMM, m_soilWP[i][k], "[0, porosity*thickness)"));
            }
            if (m_soilWtrSto[i][k] < 0.f) {
                throw ModelException(MID_STP_NOAA, "CheckInputData",
                                     LayerError(i, k, VAR_SOL_ST, m_soilWtrSto[i][k], ">= 0 mm"));
            }
            previousBottom = bottom;
        }
    }
    return true;
}

void SoilTemperatureNOAA::InitialOutputs() {
    CHECK_POSITIVE(MID_STP_NOAA, m_nCells);
    CHECK_POSITIVE(MID_STP_NOAA, m_maxSoilLyrs);
    if (m_maxThermalLyrs <= 0) m_maxThermalLyrs = m_maxSoilLyrs + EXTRA_THERMAL_LAYERS;

    if (m_nThermalLyrs == nullptr) Initialize1DArray(m_nCells, m_nThermalLyrs, 0.f);
    if (m_surfaceBoundaryTemp == nullptr) Initialize1DArray(m_nCells, m_surfaceBoundaryTemp, static_cast<float>(TFREEZ));
    if (m_groundHeatFlux == nullptr) Initialize1DArray(m_nCells, m_groundHeatFlux, 0.f);
    if (m_bottomHeatFlux == nullptr) Initialize1DArray(m_nCells, m_bottomHeatFlux, 0.f);
    if (m_iceFractionSchaake == nullptr) Initialize1DArray(m_nCells, m_iceFractionSchaake, 0.f);
    if (m_iceFractionXinanjiang == nullptr) Initialize1DArray(m_nCells, m_iceFractionXinanjiang, 0.f);
    if (m_soilTemp == nullptr) Initialize1DArray(m_nCells, m_soilTemp, 0.f);
    if (m_meanTempPre1 == nullptr && m_meanTemp != nullptr) Initialize1DArray(m_nCells, m_meanTempPre1, m_meanTemp);
    if (m_meanTempPre2 == nullptr && m_meanTemp != nullptr) Initialize1DArray(m_nCells, m_meanTempPre2, m_meanTemp);
    if (m_SOTE1 == nullptr) {
        Initialize1DArray(m_nCells, m_SOTE1, 0.f);
        Initialize1DArray(m_nCells, m_SOTE5, 0.f);
        Initialize1DArray(m_nCells, m_SOTE15, 0.f);
        Initialize1DArray(m_nCells, m_SOTE30, 0.f);
        Initialize1DArray(m_nCells, m_SOTE60, 0.f);
        Initialize1DArray(m_nCells, m_SOTE100, 0.f);
        Initialize1DArray(m_nCells, m_SOTE200, 0.f);
    }

    if (m_solthic == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_solthic, 0.f);
    if (m_solnd == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_solnd, 0.f);
    if (m_layerThicknessM == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_layerThicknessM, 0.f);
    if (m_layerCenterDepthM == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_layerCenterDepthM, 0.f);
    if (m_soilTemperature == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_soilTemperature, NODATA_VALUE);
    if (m_soilTemperaturePrev == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_soilTemperaturePrev, NODATA_VALUE);
    if (m_soilLiquidContent == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_soilLiquidContent, NODATA_VALUE);
    if (m_soilIceContent == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_soilIceContent, NODATA_VALUE);
    if (m_thermalConductivity == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_thermalConductivity, 1.5f);
    if (m_heatCapacity == nullptr) Initialize2DArray(m_nCells, m_maxThermalLyrs, m_heatCapacity, 2.0E6f);

    if (m_solpormm == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solpormm, 0.f);
    if (m_solwc == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solwc, 0.f);
    if (m_solice == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solice, 0.f);
    if (m_solwcv == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solwcv, 0.f);
    if (m_solicev == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solicev, 0.f);
    if (m_solair == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solair, 0.f);
    if (m_solorgv == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solorgv, 0.f);
    if (m_solminv == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_solminv, 0.f);
    if (m_soilt == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilt, 0.f);
}

void SoilTemperatureNOAA::InitializeSoilProfile() {
    for (int i = 0; i < m_nCells; ++i) {
        const int nReal = CVT_INT(m_nSoilLyrs[i]);
        float previousBottom = 0.f;
        for (int k = 0; k < nReal; ++k) {
            const float thickness = m_soildepth[i][k] - previousBottom;
            m_solthic[i][k] = thickness;
            m_solnd[i][k] = previousBottom + 0.5f * thickness;
            previousBottom = m_soildepth[i][k];
        }

        const float bulkDensity = max(m_soilAVBD[i], 0.01f);
        const float factor = bulkDensity /
            (bulkDensity + 686.f * static_cast<float>(exp(-5.63 * bulkDensity)));
        const float dampingDepthMM = 1000.f + 2500.f * factor;
        int nThermal = nReal;
        if (dampingDepthMM > previousBottom) {
            nThermal = nReal + EXTRA_THERMAL_LAYERS;
            const float extraThickness = (dampingDepthMM - previousBottom) / EXTRA_THERMAL_LAYERS;
            for (int e = 0; e < EXTRA_THERMAL_LAYERS; ++e) {
                const int k = nReal + e;
                m_solthic[i][k] = extraThickness;
                m_solnd[i][k] = previousBottom + (e + 0.5f) * extraThickness;
            }
        }
        m_nThermalLyrs[i] = static_cast<float>(nThermal);
        for (int k = 0; k < nThermal; ++k) {
            m_layerThicknessM[i][k] = m_solthic[i][k] * 0.001f;
            m_layerCenterDepthM[i][k] = m_solnd[i][k] * 0.001f;
        }
    }
}

void SoilTemperatureNOAA::InitializeSoilPhysicalState() {
    for (int i = 0; i < m_nCells; ++i) {
        const int nReal = CVT_INT(m_nSoilLyrs[i]);
        for (int k = 0; k < nReal; ++k) {
            const float thickness = m_solthic[i][k];
            m_solpormm[i][k] = m_soilPor[i][k] * thickness;
            m_solice[i][k] = 0.f;
            m_solwc[i][k] = min(m_solpormm[i][k], max(0.f, m_soilWtrSto[i][k] + m_soilWP[i][k]));
            m_solwcv[i][k] = m_solwc[i][k] / thickness;
            m_solicev[i][k] = 0.f;
            m_solair[i][k] = max(0.f, m_solpormm[i][k] - m_solwc[i][k]);

            const float residue = m_soilRsd != nullptr ? m_soilRsd[i][k] : m_rsdInitSoil[i];
            m_solorgv[i][k] = max(0.f, residue / 5000.f / thickness);
            const float layerBulkDensity = m_soilBD[i][k] > 0.f
                ? m_soilBD[i][k]
                : (m_soilAVBD[i] > 0.f ? m_soilAVBD[i] : 1.4f);
            m_solminv[i][k] = max(0.f, layerBulkDensity / 2.65f - m_solorgv[i][k]);
        }
        SyncHydrologyToNOAA(i);
    }
}

void SoilTemperatureNOAA::UpdateBoundaryTemperature(const int i) {
    float surfaceC = m_meanTemp[i];
    const int landUse = CVT_INT(m_landUse[i]);
    if (landUse != LANDUSE_ID_WATR && landUse != 300 && landUse != 400) {
        const float t10 = m_a0 + m_a1 * m_meanTempPre2[i] + m_a2 * m_meanTempPre1[i]
            + m_a3 * m_meanTemp[i] + m_b1 * sin(RAD_WT * m_dayOfYear)
            + m_d1 * cos(RAD_WT * m_dayOfYear)
            + m_b2 * sin(2.f * RAD_WT * m_dayOfYear)
            + m_d2 * cos(2.f * RAD_WT * m_dayOfYear);
        surfaceC = t10 * m_kSoil10 + m_soilTempRelFactor10[i];
    }
    m_surfaceBoundaryTemp[i] = static_cast<float>(Clamp(surfaceC, -90., 60.) + TFREEZ);
}

void SoilTemperatureNOAA::InitializeSoilTemperature() {
    for (int i = 0; i < m_nCells; ++i) {
        UpdateBoundaryTemperature(i);
        const int nThermal = CVT_INT(m_nThermalLyrs[i]);
        const float bottomTemp = m_annMeanTemp[i] + static_cast<float>(TFREEZ);
        const float bottomDepth = max(m_layerCenterDepthM[i][nThermal - 1], 1.e-6f);
        for (int k = 0; k < nThermal; ++k) {
            const float ratio = min(1.f, m_layerCenterDepthM[i][k] / bottomDepth);
            m_soilTemperature[i][k] = m_surfaceBoundaryTemp[i] * (1.f - ratio) + bottomTemp * ratio;
            m_soilTemperaturePrev[i][k] = m_soilTemperature[i][k];
        }
    }
}

void SoilTemperatureNOAA::SyncHydrologyToNOAA(const int i) {
    const int nReal = CVT_INT(m_nSoilLyrs[i]);
    const int nThermal = CVT_INT(m_nThermalLyrs[i]);
    for (int k = 0; k < nReal; ++k) {
        const float poreMM = m_solpormm[i][k];
        const float iceMM = min(poreMM, max(0.f, m_solice[i][k]));
        float liquidMM = m_soilWtrSto[i][k];
        if (iceMM <= m_soilWP[i][k]) liquidMM += m_soilWP[i][k] - iceMM;
        liquidMM = min(max(0.f, poreMM - iceMM), max(0.f, liquidMM));
        m_solice[i][k] = iceMM;
        m_solwc[i][k] = liquidMM;
        m_solwcv[i][k] = liquidMM / m_solthic[i][k];
        m_solicev[i][k] = iceMM / m_solthic[i][k];
        m_solair[i][k] = max(0.f, poreMM - liquidMM - iceMM);
        m_soilLiquidContent[i][k] = m_solwcv[i][k];
        m_soilIceContent[i][k] = m_solicev[i][k];
    }
    for (int k = nReal; k < nThermal; ++k) {
        m_soilLiquidContent[i][k] = m_soilLiquidContent[i][nReal - 1];
        m_soilIceContent[i][k] = m_soilIceContent[i][nReal - 1];
    }
}

void SoilTemperatureNOAA::SyncNOAAToLegacy(const int i) {
    const int nReal = CVT_INT(m_nSoilLyrs[i]);
    for (int k = 0; k < nReal; ++k) {
        const float thickness = m_solthic[i][k];
        const float poreMM = m_solpormm[i][k];
        float iceMM = max(0.f, m_soilIceContent[i][k] * thickness);
        float liquidMM = max(0.f, m_soilLiquidContent[i][k] * thickness);
        iceMM = min(iceMM, poreMM);
        liquidMM = min(liquidMM, poreMM - iceMM);
        m_solice[i][k] = iceMM;
        m_solwc[i][k] = liquidMM;
        m_solicev[i][k] = iceMM / thickness;
        m_solwcv[i][k] = liquidMM / thickness;
        m_solair[i][k] = max(0.f, poreMM - liquidMM - iceMM);

        float storage = liquidMM;
        if (iceMM <= m_soilWP[i][k]) storage -= m_soilWP[i][k] - iceMM;
        m_soilWtrSto[i][k] = min(max(0.f, poreMM - m_soilWP[i][k]), max(0.f, storage));
    }
}

float SoilTemperatureNOAA::Porosity(const int i, const int thermalLayer) const {
    const int source = min(thermalLayer, CVT_INT(m_nSoilLyrs[i]) - 1);
    return static_cast<float>(Clamp(m_soilPor[i][source], 1.e-6, 0.999999));
}

float SoilTemperatureNOAA::QuartzFraction(const int i, const int thermalLayer) const {
    const int source = min(thermalLayer, CVT_INT(m_nSoilLyrs[i]) - 1);
    if (m_sand != nullptr) return static_cast<float>(Clamp(m_sand[i][source] * 0.01, 0., 1.));
    return static_cast<float>(Clamp(m_quartz, 0., 1.));
}

void SoilTemperatureNOAA::ThermalConductivity(const int i) {
    const int nThermal = CVT_INT(m_nThermalLyrs[i]);
    for (int k = 0; k < nThermal; ++k) {
        const double porosity = Porosity(i, k);
        const double quartz = QuartzFraction(i, k);
        const double liquid = max(0.f, m_soilLiquidContent[i][k]);
        const double ice = max(0.f, m_soilIceContent[i][k]);
        const double saturation = Clamp((liquid + ice) / porosity, 0., 1.);
        const double tcOtherMineral = quartz > 0.2 ? 2.0 : 3.0;
        const double tcSolid = pow(7.7, quartz) * pow(tcOtherMineral, 1. - quartz);
        const double unfrozenFraction = liquid + ice > 0. ? liquid / (liquid + ice) : 1.;
        const double unfrozenVolume = unfrozenFraction * porosity;
        const double tcSaturated = pow(tcSolid, 1. - porosity)
            * pow(2.2, porosity - unfrozenVolume) * pow(0.57, unfrozenVolume);
        const double dryDensity = (1. - porosity) * 2700.;
        const double tcDry = (0.135 * dryDensity + 64.7) / (2700. - 0.947 * dryDensity);
        double kersten = 0.;
        if (ice > 0.0005) kersten = saturation;
        else if (saturation > 0.1) kersten = log10(saturation) + 1.;
        else if (saturation > 0.05) kersten = 0.7 * log10(saturation) + 1.;
        kersten = Clamp(kersten, 0., 1.);
        m_thermalConductivity[i][k] = static_cast<float>(max(0.05, tcDry + kersten * (tcSaturated - tcDry)));
    }
}

void SoilTemperatureNOAA::SoilHeatCapacity(const int i) {
    const int nReal = CVT_INT(m_nSoilLyrs[i]);
    const int nThermal = CVT_INT(m_nThermalLyrs[i]);
    for (int k = 0; k < nThermal; ++k) {
        const int source = min(k, nReal - 1);
        const double porosity = Porosity(i, k);
        const double liquid = Clamp(m_soilLiquidContent[i][k], 0., porosity);
        const double ice = Clamp(m_soilIceContent[i][k], 0., porosity - liquid);
        const double air = max(0., porosity - liquid - ice);
        const double solid = 1. - porosity;
        const double organic = min(solid, max(0.f, m_solorgv[i][source]));
        const double mineral = solid - organic;
        m_heatCapacity[i][k] = static_cast<float>(liquid * HC_WATER + ice * HC_ICE
            + air * HC_AIR + mineral * HC_MINERAL + organic * HC_ORGANIC);
    }
}

void SoilTemperatureNOAA::SolveDiffusionEquation(const int i) {
    const int nThermal = CVT_INT(m_nThermalLyrs[i]);
    vector<double> lower(nThermal, 0.);
    vector<double> diagonal(nThermal, 1.);
    vector<double> upper(nThermal, 0.);
    vector<double> rhs(nThermal, 0.);
    vector<double> solution(nThermal, 0.);

    for (int k = 0; k < nThermal; ++k) {
        m_soilTemperaturePrev[i][k] = m_soilTemperature[i][k];
        rhs[k] = m_soilTemperaturePrev[i][k];
        const double dz = m_layerThicknessM[i][k];
        const double arealHeatCapacity = max(1., static_cast<double>(m_heatCapacity[i][k]) * dz);

        if (k == 0) {
            const double conductance = m_thermalConductivity[i][k] / max(0.5 * dz, 1.e-9);
            const double ratio = m_dt * conductance / arealHeatCapacity;
            diagonal[k] += ratio;
            rhs[k] += ratio * m_surfaceBoundaryTemp[i];
            m_groundHeatFlux[i] = static_cast<float>(conductance
                * (m_surfaceBoundaryTemp[i] - m_soilTemperaturePrev[i][k]));
        } else {
            const double resistance = 0.5 * m_layerThicknessM[i][k - 1]
                    / m_thermalConductivity[i][k - 1]
                + 0.5 * dz / m_thermalConductivity[i][k];
            const double ratio = m_dt / max(resistance, 1.e-12) / arealHeatCapacity;
            lower[k] = -ratio;
            diagonal[k] += ratio;
        }

        if (k == nThermal - 1) {
            const double conductance = m_thermalConductivity[i][k] / max(0.5 * dz, 1.e-9);
            const double ratio = m_dt * conductance / arealHeatCapacity;
            const double bottomTemperature = m_annMeanTemp[i] + TFREEZ;
            diagonal[k] += ratio;
            rhs[k] += ratio * bottomTemperature;
            m_bottomHeatFlux[i] = static_cast<float>(conductance
                * (bottomTemperature - m_soilTemperaturePrev[i][k]));
        } else {
            const double resistance = 0.5 * dz / m_thermalConductivity[i][k]
                + 0.5 * m_layerThicknessM[i][k + 1] / m_thermalConductivity[i][k + 1];
            const double ratio = m_dt / max(resistance, 1.e-12) / arealHeatCapacity;
            upper[k] = -ratio;
            diagonal[k] += ratio;
        }
    }

    if (!SolverTDMA(lower, diagonal, upper, rhs, solution)) {
        throw ModelException(MID_STP_NOAA, "SolveDiffusionEquation", "Tridiagonal solver failed.");
    }
    for (int k = 0; k < nThermal; ++k) m_soilTemperature[i][k] = static_cast<float>(solution[k]);
}

void SoilTemperatureNOAA::PhaseChange(const int i) {
    const int nReal = CVT_INT(m_nSoilLyrs[i]);
    for (int k = 0; k < nReal; ++k) {
        const double dz = m_layerThicknessM[i][k];
        const double porosity = Porosity(i, k);
        double liquid = Clamp(m_soilLiquidContent[i][k], 0., porosity);
        double ice = Clamp(m_soilIceContent[i][k], 0., porosity - liquid);
        const double total = liquid + ice;
        const double arealHeatCapacity = max(1., static_cast<double>(m_heatCapacity[i][k]) * dz);
        const double sensibleEnergy = (m_soilTemperature[i][k] - TFREEZ) * arealHeatCapacity;

        if (sensibleEnergy > 0. && ice > 0.) {
            const double availableIceMass = ice * dz * WDENSITY;
            const double meltedMass = min(availableIceMass, sensibleEnergy / m_latentHeatFusion);
            const double volumeChange = meltedMass / (WDENSITY * dz);
            ice -= volumeChange;
            liquid += volumeChange;
            const double residualEnergy = sensibleEnergy - meltedMass * m_latentHeatFusion;
            m_soilTemperature[i][k] = static_cast<float>(TFREEZ + residualEnergy / arealHeatCapacity);
        } else if (sensibleEnergy < 0. && liquid > 0.) {
            const double temperature = max(1., static_cast<double>(m_soilTemperature[i][k]));
            const double suction = m_latentHeatFusion * (TFREEZ - temperature)
                / (GRAVITY * temperature);
            double unfrozen = 0.;
            if (suction > 0.) unfrozen = porosity * pow(suction / m_satpsi, -1. / m_b);
            unfrozen = Clamp(unfrozen, 0., total);
            const double freezableMass = max(0., liquid - unfrozen) * dz * WDENSITY;
            const double frozenMass = min(freezableMass, -sensibleEnergy / m_latentHeatFusion);
            const double volumeChange = frozenMass / (WDENSITY * dz);
            liquid -= volumeChange;
            ice += volumeChange;
            const double residualColdEnergy = -sensibleEnergy - frozenMass * m_latentHeatFusion;
            m_soilTemperature[i][k] = static_cast<float>(TFREEZ - residualColdEnergy / arealHeatCapacity);
        }

        m_soilLiquidContent[i][k] = static_cast<float>(max(0., liquid));
        m_soilIceContent[i][k] = static_cast<float>(max(0., total - liquid));
    }
}

void SoilTemperatureNOAA::ComputeIceFraction(const int i) {
    const int nReal = CVT_INT(m_nSoilLyrs[i]);
    double iceDepthMM = 0.;
    for (int k = 0; k < nReal; ++k) {
        iceDepthMM += m_soilIceContent[i][k] * m_solthic[i][k];
    }
    m_iceFractionSchaake[i] = static_cast<float>(iceDepthMM * 0.001);
    const double topPorosity = Porosity(i, 0);
    const double iceSaturation = Clamp(m_soilIceContent[i][0] / topPorosity, 0., 1.);
    const double coefficient = 4.;
    m_iceFractionXinanjiang[i] = static_cast<float>(max(0.,
        (exp(-coefficient * (1. - iceSaturation)) - exp(-coefficient))
        / (1. - exp(-coefficient))));
}

float SoilTemperatureNOAA::InterpolateTemperatureC(const int i, const float depthMM) const {
    const int nThermal = CVT_INT(m_nThermalLyrs[i]);
    if (depthMM <= m_solnd[i][0]) return m_soilTemperature[i][0] - static_cast<float>(TFREEZ);
    for (int k = 0; k < nThermal - 1; ++k) {
        if (depthMM <= m_solnd[i][k + 1]) {
            const float fraction = (depthMM - m_solnd[i][k])
                / (m_solnd[i][k + 1] - m_solnd[i][k]);
            const float valueK = m_soilTemperature[i][k]
                + fraction * (m_soilTemperature[i][k + 1] - m_soilTemperature[i][k]);
            return valueK - static_cast<float>(TFREEZ);
        }
    }
    return m_soilTemperature[i][nThermal - 1] - static_cast<float>(TFREEZ);
}

void SoilTemperatureNOAA::UpdateLegacyOutputs(const int i) {
    const int nReal = CVT_INT(m_nSoilLyrs[i]);
    for (int k = 0; k < nReal; ++k) {
        m_soilt[i][k] = m_soilTemperature[i][k] - static_cast<float>(TFREEZ);
    }
    m_soilTemp[i] = m_soilt[i][0];
    m_SOTE1[i] = InterpolateTemperatureC(i, 10.f);
    m_SOTE5[i] = InterpolateTemperatureC(i, 50.f);
    m_SOTE15[i] = InterpolateTemperatureC(i, 150.f);
    m_SOTE30[i] = InterpolateTemperatureC(i, 300.f);
    m_SOTE60[i] = InterpolateTemperatureC(i, 600.f);
    m_SOTE100[i] = InterpolateTemperatureC(i, 1000.f);
    m_SOTE200[i] = InterpolateTemperatureC(i, 2000.f);
}

void SoilTemperatureNOAA::UpdateTemperatureHistory() {
    if (m_lastHistoryDay == m_dayOfYear) return;
    for (int i = 0; i < m_nCells; ++i) {
        m_meanTempPre2[i] = m_meanTempPre1[i];
        m_meanTempPre1[i] = m_meanTemp[i];
    }
    m_lastHistoryDay = m_dayOfYear;
}

bool SoilTemperatureNOAA::SolverTDMA(const vector<double>& lower,
                                     const vector<double>& diagonal,
                                     const vector<double>& upper,
                                     const vector<double>& rhs,
                                     vector<double>& solution) const {
    const int n = static_cast<int>(rhs.size());
    if (n <= 0) return false;
    vector<double> cPrime(n, 0.);
    vector<double> dPrime(n, 0.);
    if (fabs(diagonal[0]) < 1.e-20) return false;
    cPrime[0] = upper[0] / diagonal[0];
    dPrime[0] = rhs[0] / diagonal[0];
    for (int k = 1; k < n; ++k) {
        const double denominator = diagonal[k] - lower[k] * cPrime[k - 1];
        if (fabs(denominator) < 1.e-20) return false;
        cPrime[k] = k < n - 1 ? upper[k] / denominator : 0.;
        dPrime[k] = (rhs[k] - lower[k] * dPrime[k - 1]) / denominator;
    }
    solution[n - 1] = dPrime[n - 1];
    for (int k = n - 2; k >= 0; --k) solution[k] = dPrime[k] - cPrime[k] * solution[k + 1];
    return true;
}

int SoilTemperatureNOAA::Execute() {
    CheckInputData();
    InitialOutputs();
    if (!m_profileInitialized) {
        InitializeSoilProfile();
        m_profileInitialized = true;
    }
    if (!m_physicalStateInitialized) {
        InitializeSoilPhysicalState();
        m_physicalStateInitialized = true;
    }
    if (!m_temperatureInitialized) {
        InitializeSoilTemperature();
        m_temperatureInitialized = true;
    }

#pragma omp parallel for
    for (int i = 0; i < m_nCells; ++i) {
        UpdateBoundaryTemperature(i);
        SyncHydrologyToNOAA(i);
        ThermalConductivity(i);
        SoilHeatCapacity(i);
        SolveDiffusionEquation(i);
        PhaseChange(i);
        SyncNOAAToLegacy(i);
        ComputeIceFraction(i);
        UpdateLegacyOutputs(i);
    }
    UpdateTemperatureHistory();
    return 0;
}

void SoilTemperatureNOAA::GetValue(const char* key, float* value) {
    InitialOutputs();
    const string sk(key);
    if (StringMatch(sk, VAR_ICE_FRACTION_SCHAAKE)) *value = m_iceFractionSchaake[0];
    else if (StringMatch(sk, VAR_ICE_FRACTION_XINAN)) *value = m_iceFractionXinanjiang[0];
    else throw ModelException(MID_STP_NOAA, "GetValue", "Output variable " + sk + " does not exist.");
}

void SoilTemperatureNOAA::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    const string sk(key);
    *n = m_nCells;
    if (StringMatch(sk, VAR_SOTE)) *data = m_soilTemp;
    else if (StringMatch(sk, VAR_TMEAN1)) *data = m_meanTempPre1;
    else if (StringMatch(sk, VAR_TMEAN2)) *data = m_meanTempPre2;
    else if (StringMatch(sk, VAR_SOTE1)) *data = m_SOTE1;
    else if (StringMatch(sk, VAR_SOTE5)) *data = m_SOTE5;
    else if (StringMatch(sk, VAR_SOTE15)) *data = m_SOTE15;
    else if (StringMatch(sk, VAR_SOTE30)) *data = m_SOTE30;
    else if (StringMatch(sk, VAR_SOTE60)) *data = m_SOTE60;
    else if (StringMatch(sk, VAR_SOTE100)) *data = m_SOTE100;
    else if (StringMatch(sk, VAR_SOTE200)) *data = m_SOTE200;
    else if (StringMatch(sk, VAR_ICE_FRACTION_SCHAAKE)) *data = m_iceFractionSchaake;
    else if (StringMatch(sk, VAR_ICE_FRACTION_XINAN)) *data = m_iceFractionXinanjiang;
    else if (StringMatch(sk, VAR_GROUND_HEAT_FLUX)) *data = m_groundHeatFlux;
    else throw ModelException(MID_STP_NOAA, "Get1DData", "Output variable " + sk + " does not exist.");
}

void SoilTemperatureNOAA::Get2DData(const char* key, int* n, int* col, float*** data) {
    InitialOutputs();
    const string sk(key);
    *n = m_nCells;
    if (StringMatch(sk, VAR_SOILT)) {
        *col = m_maxSoilLyrs;
        *data = m_soilt;
    } else if (StringMatch(sk, VAR_SOLWC)) {
        *col = m_maxSoilLyrs;
        *data = m_solwc;
    } else if (StringMatch(sk, VAR_SOLICE)) {
        *col = m_maxSoilLyrs;
        *data = m_solice;
    } else if (StringMatch(sk, VAR_SOIL_TEMPERATURE)) {
        *col = m_maxThermalLyrs;
        *data = m_soilTemperature;
    } else if (StringMatch(sk, VAR_SOIL_LIQUID_CONTENT)) {
        *col = m_maxThermalLyrs;
        *data = m_soilLiquidContent;
    } else if (StringMatch(sk, VAR_SOIL_ICE_CONTENT)) {
        *col = m_maxThermalLyrs;
        *data = m_soilIceContent;
    } else {
        throw ModelException(MID_STP_NOAA, "Get2DData", "Output variable " + sk + " does not exist.");
    }
}

void SoilTemperatureNOAA::SetValueByIndex(const char*, int, float) {}
void SoilTemperatureNOAA::SetReaches(clsReaches*) {}
void SoilTemperatureNOAA::SetSubbasins(clsSubbasins*) {}
void SoilTemperatureNOAA::SetScenario(Scenario*) {}
