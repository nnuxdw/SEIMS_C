/*!
 * \file SoilTemperatureNOAA.h
 * \brief Layered soil heat conduction and freeze/thaw module for SEIMS.
 */

#ifndef SEIMS_MODULE_STP_NOAA_H
#define SEIMS_MODULE_STP_NOAA_H

#include "SimulationModule.h"

#include <vector>

class SoilTemperatureNOAA : public SimulationModule {
public:
    SoilTemperatureNOAA();
    ~SoilTemperatureNOAA();

    void SetValue(const char* key, float value) OVERRIDE;
    void SetValueByIndex(const char* key, int index, float value) OVERRIDE;
    void Set1DData(const char* key, int n, float* data) OVERRIDE;
    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;
    void SetReaches(clsReaches* rches) OVERRIDE;
    void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;
    void SetScenario(Scenario* sce) OVERRIDE;

    bool CheckInputData() OVERRIDE;
    void InitialOutputs() OVERRIDE;
    int Execute() OVERRIDE;

    void GetValue(const char* key, float* value) OVERRIDE;
    void Get1DData(const char* key, int* n, float** data) OVERRIDE;
    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

private:
    static const int EXTRA_THERMAL_LAYERS = 5;

    // Scalar parameters.
    int m_nCells;
    int m_maxSoilLyrs;
    int m_maxThermalLyrs;
    float m_dt;
    float m_b;
    float m_quartz;
    float m_satpsi;
    float m_latentHeatFusion;
    float m_a0;
    float m_a1;
    float m_a2;
    float m_a3;
    float m_b1;
    float m_b2;
    float m_d1;
    float m_d2;
    float m_kSoil10;

    // Arrays supplied by SEIMS. This module never releases these pointers.
    float* m_nSoilLyrs;
    float* m_soilTempRelFactor10;
    float* m_landUse;
    float* m_meanTemp;
    float* m_annMeanTemp;
    float* m_rsdInitSoil;
    float* m_soilAVBD;
    float* m_solsw;
    float* m_snowAcc;
    float** m_soildepth;
    float** m_soilthick;
    float** m_soilPor;
    float** m_soilWP;
    float** m_soilWtrSto;
    float** m_soilSat;
    float** m_soilBD;
    float** m_soilRsd;
    float** m_clay;
    float** m_sand;

    // Geometry owned by NOAA. Legacy geometry is in mm; solver geometry is SI.
    float* m_nThermalLyrs;
    float** m_solthic;
    float** m_solnd;
    float** m_layerThicknessM;
    float** m_layerCenterDepthM;

    // FINPL-compatible state for real soil layers.
    float** m_solpormm;
    float** m_solwc;
    float** m_solice;
    float** m_solwcv;
    float** m_solicev;
    float** m_solair;
    float** m_solorgv;
    float** m_solminv;

    // NOAA state for all thermal layers.
    float** m_soilTemperature;
    float** m_soilTemperaturePrev;
    float** m_soilLiquidContent;
    float** m_soilIceContent;
    float** m_thermalConductivity;
    float** m_heatCapacity;

    // Boundary and diagnostic arrays.
    float* m_surfaceBoundaryTemp;
    float* m_groundHeatFlux;
    float* m_bottomHeatFlux;
    float* m_iceFractionSchaake;
    float* m_iceFractionXinanjiang;

    // FINPL-compatible outputs.
    float* m_soilTemp;
    float** m_soilt;
    float* m_meanTempPre1;
    float* m_meanTempPre2;
    float* m_SOTE1;
    float* m_SOTE5;
    float* m_SOTE15;
    float* m_SOTE30;
    float* m_SOTE60;
    float* m_SOTE100;
    float* m_SOTE200;

    bool m_profileInitialized;
    bool m_physicalStateInitialized;
    bool m_temperatureInitialized;
    int m_lastHistoryDay;

    void InitializeSoilProfile();
    void InitializeSoilPhysicalState();
    void InitializeSoilTemperature();
    void SyncHydrologyToNOAA(int i);
    void SyncNOAAToLegacy(int i);
    void UpdateBoundaryTemperature(int i);
    void UpdateTemperatureHistory();
    void UpdateLegacyOutputs(int i);

    void ThermalConductivity(int i);
    void SoilHeatCapacity(int i);
    void SolveDiffusionEquation(int i);
    void PhaseChange(int i);
    void ComputeIceFraction(int i);

    float Porosity(int i, int thermalLayer) const;
    float QuartzFraction(int i, int thermalLayer) const;
    float InterpolateTemperatureC(int i, float depthMM) const;
    bool SolverTDMA(const std::vector<double>& lower,
                    const std::vector<double>& diagonal,
                    const std::vector<double>& upper,
                    const std::vector<double>& rhs,
                    std::vector<double>& solution) const;

    static const double HC_WATER;
    static const double HC_ICE;
    static const double HC_AIR;
    static const double HC_MINERAL;
    static const double HC_ORGANIC;
    static const double GRAVITY;
    static const double TFREEZ;
    static const double WDENSITY;
    static const float RAD_WT;
};

#endif  // SEIMS_MODULE_STP_NOAA_H
