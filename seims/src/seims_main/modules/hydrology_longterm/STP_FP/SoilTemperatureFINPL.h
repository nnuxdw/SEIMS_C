/*!
 * \file SoilTemperatureFINPL.h
 * \brief Finn Plauborg Method to Compute Soil Temperature
 *
 * Changelog:
 *   - 1. 2011-01-05 - jz - Initial implementation.
 *   - 2. 2016-05-27 - lj - Code review and reformat.
 *
 * \author Junzhi Liu, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_STP_FP_H
#define SEIMS_MODULE_STP_FP_H

#include "SimulationModule.h"

/*!
 * \defgroup STP_FP
 * \ingroup Hydrology_longterm
 * \brief Finn Plauborg Method to Compute Soil Temperature
 *
 */

const float radWt = 0.01721420632103996f; /// PI * 2.f / 365.f;

/*!
 * \class SoilTemperatureFINPL
 * \ingroup STP_FP
 * \brief Soil temperature
 *
 */
class SoilTemperatureFINPL: public SimulationModule {
public:
    SoilTemperatureFINPL();

    ~SoilTemperatureFINPL();

    void SetValue(const char* key, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

private:
    void SoilTempTSWAT(int i);

    void SoilTempSWAT(int i);

private:
    /// from parameter database
    /// coefficients in the Equation
    float m_a0, m_a1, m_a2, m_a3, m_b1, m_b2, m_d1, m_d2;
    /// ratio between soil temperature at 10 cm and the mean
    float m_kSoil10;

    /// count of cells
    int m_nCells;
    /// factor of soil temperature relative to short grass (degree)
    float* m_soilTempRelFactor10;
    /// landuse type, for distinguish calculation, such as water body.
    float* m_landUse;
    /// from interpolation module
    /// mean air temperature of the current day
    float* m_meanTemp;
    ///// mean air temperature of the day(d-1)
    float* m_meanTempPre1;
    ///// mean air temperature of the day(d-2)
    float* m_meanTempPre2;

    /// output soil temperature
    float* m_soilTemp;

    float* m_tMax;
    float* m_tMin;

    //ljj++
    /// max number of soil layers
    int m_maxSoilLyrs;
    
    float tsoil1;
    float tsoil2;
    float tsoil3;
    float tsoil4;
    float tsoil5;
    float m_ddepth1;

    float m_snowCoverMax;
    float m_snowCover50;
    float m_tfrozen;

    float** m_soildepth;
    float** m_solthic;
    float** m_soilthick;
    float** m_soilPor;
    float** m_soilWP;
    float** m_soilWtrSto;
    float** m_solpormm;
    float** m_soilBD;
    float** m_soilRsd;
    float** m_clay;
    float** m_sand;
    float** m_soilSat;

    float** m_solwc;
    float** m_solice;
    float** m_solwcv;
    float** m_solorg;
    float** m_solorgv;
    float** m_solminv;
    float** m_solicev;
    float** m_solair;

    float* m_effcoe;
    float* m_kscoe;
    float* m_ccoe;
    float* m_nSoilLyrs;	/// soil layers
    float* m_rsdInitSoil;
    float* m_snowAcc;
    float* m_solsw;
	float* m_soilAVBD;
    float* m_landCoverCls;
    float* m_lai;
    float* m_maxLai;
    float* m_soilALB;
    float* m_igro;
    float* m_soilcov;
    float* m_annMeanTemp;
    float* m_tmpmx;
    float* m_tmpmn;
    float* m_cellLat;
    float* m_dayLen;
    float* m_rhd;
    float* m_sr;
    float* m_lightExtCoef;

    float* m_snoden;
    float* m_snodep;
    float* m_ksno;
    float* m_casno;
    float* ddepth;
    float* m_dem;
    float* m_bottmp;
    float* m_ambtmp;
    float* m_snoco;
    float* m_surtmp;
    float* m_basedep;
    float* m_alb;

    float* m_kss;
    float** m_kint;
    float** m_bsol;
    float** m_csol;
    float** m_asol;
    float** m_dsol;
    float** m_psol;
    float** m_qsol;
    float* m_asno;
    float* m_bsno;
    float* m_csno;
    float* m_dsno;
    float* m_psno;
    float* m_qsno;
    
    float** m_kcoe;
    float** m_casol;
    float** m_solsatu;
    float** m_solkd;
    float** m_solke;
    float** m_solksat;
    float** m_ksol;
    float** m_dice;
    float** m_dsols;
    float** m_smp;
    float** m_supercool;

    float** m_solnd;

    float* m_snotmp1;
    float* m_presnotmp;
    float** m_soltmp1;
    float** m_soilt;

    float* m_SOTE1;
    float* m_SOTE5;
    float* m_SOTE15;
    float* m_SOTE30;
    float* m_SOTE60;
    float* m_SOTE100;
    float* m_SOTE200;
};
#endif /* SEIMS_MODULE_STP_FP_H */
