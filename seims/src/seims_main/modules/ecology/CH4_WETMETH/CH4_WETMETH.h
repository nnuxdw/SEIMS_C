/*!
 * \file CH4_WETMETH.h
 * \brief WETMETH (Wetland CH4 Model) module for methane production and oxidation
 *        Implementation of WETMETH model for calculating methane emissions from wetlands
 * \author long ping; li jing; zheng xin
 * \date 2025-09-09
 *
 * Changelog:
 *   - 1. 2024-01-01 - Initial implementation of WETMETH model
 */
#ifndef SEIMS_MODULE_CH4_WETMETH_H
#define SEIMS_MODULE_CH4_WETMETH_H

// Module ID definition
#define MID_CH4_WETMETH "CH4_WETMETH"

// WETMETH model constants from paper
#define CH4_R 2.6e-10f        // Specific CH4 production rate (kg kg^-1 s^-1)
#define CH4_Q10 4.2f          // Temperature coefficient for CH4 production
#define CH4_T0 273.15f        // Baseline temperature (K)
#define CH4_T_REF 308.15f     // Reference temperature for CH4 production (K)
#define CH4_TAU_PROD 0.75f    // Scaling parameter for CH4 production (m)
#define CH4_Z_OATZ 0.05f      // Thickness of oxic-anoxic transition zone (m)
#define CH4_TAU_OXID 0.0146f  // Scaling parameter for CH4 oxidation (m)


#include "SimulationModule.h"

using namespace std;

class SoilCol {
public:
	// Static variables - WETMETH model parameters
	float z_oxic;           // Oxic-anoxic interface depth (m)
	float area_Soilcol;     // Area of current soil column (m²)
	int num_layers;         // Actual number of soil layers for this column
	
	// Dynamic variables - soil layer properties
	float *layer_thickness;    // Thickness of each soil layer (mm) - from VAR_SOILTHICK
	float *cumulative_depth;   // Cumulative depth from surface (m) - calculated
	float *soil_water_storage; // Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
	float *soil_saturated;     // Saturated water capacity of each layer (mm H2O) - from VAR_SOL_UL
	float *soil_saturation;    // Saturation ratio of each soil layer (0-1) - calculated
	float *T_soil;            // Temperature of each soil layer (°C) - from VAR_SOTE
	float *Soc;               // Soil organic matter content (%) - from VAR_SOL_OM
	
	// Output variables
	float SoilCol_CH4;        // Total CH4 production for this soil column (kg C/s)
	
public:
	// Constructor and destructor
	SoilCol();
	~SoilCol();
	
	// Initialize soil column with given number of layers
	void Initialize(int num_layers);
	
	// Calculate methane production for the current soil column
	float SoilColMethane();
	
	// Calculate soil saturation ratio for each layer
	void calculate_soil_saturation();
	
	// Calculate oxic zone depth based on soil saturation
	float calculate_oxic_depth();
};

// 继承自SimulationModule，需要符合SEIMS模块标准
class CH4_WETMETH : public SimulationModule {

public:
	CH4_WETMETH();

    ~CH4_WETMETH();

    ///////////// SetData series functions /////////////
	// 数据输入接口
    void SetValue(const char* key, float value) OVERRIDE;

    void SetValueByIndex(const char* key, int index, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

    void SetReaches(clsReaches* rches) OVERRIDE;

    void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;

    void SetScenario(Scenario* sce) OVERRIDE;

    ///////////// CheckInputData and InitialOutputs /////////////
	// 执行函数
    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    ///////////// Main control structure of execution code /////////////

    int Execute() OVERRIDE;

    ///////////// GetData series functions /////////////
	// 数据输出接口
    TimeStepType GetTimeStepType() OVERRIDE;

    void GetValue(const char* key, float* value) OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

private:
	// Basic parameters
	int m_nCells;              // Number of valid cells (HRUs)
	int m_maxSoilLyrs;         // Maximum number of soil layers
	float *m_nSoilLyrs;        // Actual number of soil layers for each cell
	
	// Input data from other modules
	float *m_area;             // Area of each cell (m²)
	float **m_layer_thickness; // Thickness of each soil layer (mm) - from VAR_SOILTHICK
	float **m_soil_water_storage; // Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
	float **m_soil_saturated;  // Saturated water capacity of each layer (mm H2O) - from VAR_SOL_UL
	// float **m_Soc;             // Soil organic matter content (%) - from VAR_SOL_OM (commented out, using VAR_SOL_WOC instead)
	float **m_Soc_kg_ha;       // Soil organic carbon content (kg/ha) - from VAR_SOL_WOC
	float **m_Tsoil;           // Temperature of each soil layer (°C) - from VAR_SOTE
	
	// Soil column objects
	SoilCol *m_SoilCols;       // Array of soil column objects
	
	// Output variables
	float *m_P_soilcol;        // CH4 production for each soil column (kg C/s)
	float m_total_CH4;         // Total CH4 production for all cells (kg C/s)
};






#endif /* SEIMS_MODULE_CH4_WETMETH_H */
