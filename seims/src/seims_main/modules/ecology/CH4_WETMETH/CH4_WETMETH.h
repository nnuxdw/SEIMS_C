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
#define CH4_T0 273.15f        // Baseline temperature (K)


#include "SimulationModule.h"

using namespace std;

class SoilCol {
public:
	// Static variables - WETMETH model parameters
	float z_oxic;               // Oxic-anoxic interface depth (m)
	float area_Soilcol;         // Area of current soil column (m²)
	int num_layers;             // Actual number of soil layers for this column

	// Dynamic variables - soil layer properties
	float *layer_thickness;     // Thickness of each soil layer (mm) - from VAR_SOILTHICK
	float *cumulative_depth;    // Cumulative depth from surface (m) - calculated
	float *soil_water_storage;  // Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
	float *soil_saturated;      // Saturated water capacity of each layer (mm H2O) - from VAR_SOL_UL
	float *soil_saturation;     // Saturation ratio of each soil layer (0-1) - calculated
	float *T_soil;              // Temperature of each soil layer (°C) - from VAR_SOILT
	//float *Soc;                 // Soil organic matter content (%) - from VAR_SOL_OM
	float *Rh;
	float *m_soilWP;            // Water content of soil at -1.5 MPa (wilting point)
	float *m_soilPor;           // porosity mm/mm
	float *q10_eff_layer;
	float *temperature_factor_layer;  // CH4 production temperature factor
	float *depth_factor_layer;        // CH4 production depth factor
	float *CH4_prod_layer;            // Layer CH4 production, g CH4-C m-2 h-1
	float *m_soilFC;            // field capacity m3/m3

	// Output variables
	float SoilCol_CH4;          // Total CH4 production for this soil column (g CH4-C m⁻² h⁻¹)
	float SoilCol_CH4_oxidized;       // Column CH4 oxidation, g CH4-C m-2 h-1
	float CH4_remaining_fraction;     // Fraction remaining after oxidation

public:
	// Constructor and destructor
	SoilCol();
	~SoilCol();

	// Initialize soil column with given number of layers
	void Initialize(int num_layers);

	// Calculate methane production for the current soil column
	//float SoilColMethane(int cell_idx);
	float SoilColMethane(int cell_idx, float &CH4_before, float handWtrDep,
		float ch4_rh_ratio, float ch4_tref, float ch4_tau_prod,
		float ch4_z_oatz, float ch4_tau_oxid);

	// Calculate soil saturation ratio for each layer
	void calculate_soil_saturation(int cell_idx);

	// Calculate oxic zone depth based on soil saturation
	float calculate_oxic_depth(float ch4_z_oatz);
};

// 继承自SimulationModule，需要符合SEIMS模块标准
class CH4_WETMETH : public SimulationModule {

public:
	CH4_WETMETH();

	~CH4_WETMETH();

	///////////// SetData series functions /////////////
	// 数据输入接口
	void SetValue(const char *key, float value) OVERRIDE;

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

	// WETMETH model constants from paper
	//float m_ch4_r;
	float m_ch4_rh_ratio;      // CH4-C / CO2-C production ratio, dimensionless
	float m_ch4_tref;          // Reference temperature used in Q10 equation, K
	float m_ch4_tau_prod;      // CH4 production depth-decay scale, m
	float m_ch4_z_oatz;        // Oxic-anoxic transition-zone thickness, m
	float m_ch4_tau_oxid;      // CH4 oxidation depth-decay scale, m

	// Input data from other modules
	float *m_area;                  // Area of each cell (m²)
	float **m_layer_thickness;      // Thickness of each soil layer (mm) - from VAR_SOILTHICK
	float **m_soil_water_storage;   // Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
	float **m_soil_saturated;       // Saturated water capacity of each layer (mm H2O) - from VAR_SOL_UL
	float **m_soilWP;               // Water content of soil at -1.5 MPa (wilting point) - from VAR_SOL_WPMM
	float **m_soilPor;              // porosity mm/mm
	//float **m_Soc;                // Soil organic matter content (%) - from VAR_SOL_OM (commented out, using VAR_SOL_WOC instead)
	//float **m_Soc_kg_ha;          // Soil organic carbon content (kg/ha) - from VAR_SOL_WOC
	float **m_sol_RSPC;
	float **m_Tsoil;                // Temperature of each soil layer (°C) - from VAR_SOILT

	float *m_infil;                 // m_infil, Infiltration
	float *m_netPcp;                // net precipitation of each cell (mm)
	float *m_sd;                    // depression storage
	float *m_soilET;                // actual soil evaporation
	float *m_IntcpET;               // Evaporation loss from intercepted rainfall, mm
	float *m_exsPcp;                // the excess precipitation (mm) of the total nCells, which could be depressed or generated surface runoff
	//float *m_handWtrDep;            // OL_HAND最终计算的HAND地表水深，m
	//float *m_handWtrDepAftInfil;    // HAND water depth after infiltration from SUR_MR_HAND, m
	float *m_handWtrDepAftDep;     // DEP处理后剩余的自由地表水深，m
	//float *m_ifluQ2Rch;        // subsurface to streams from each subbasin, the first element is the whole watershed, m3/s, VAR_SBIF
	float **m_soilPerco;            // the amount of water percolated from the soil water reservoir
	float **m_subSurfRf;            // subsurface runoff (mm), VAR_SSRU
	float **m_soilFC;


	// Soil column objects
	SoilCol *m_SoilCols;            // Array of soil column objects

	// Output variables
	float *m_P_soilcol;             // CH4 production before oxidation, mg CH4 m-2 h-1
	float *m_P_soilcol_flux;        // CH4 emission after oxidation, mg CH4 m-2 h-1
	float m_total_CH4;              // Total CH4 emission for all cells, mg CH4 h-1 

	float *m_satL1;		            // Soil saturation fraction of layer 1, dimensionless
	float *m_satL2;		            // Soil saturation fraction of layer 2, dimensionless
	float *m_satL3;		            // Soil saturation fraction of layer 3, dimensionless
	float *m_satL4;		            // Soil saturation fraction of layer 4, dimensionless
	float *m_satL5;		            // Soil saturation fraction of layer 5, dimensionless
	float *m_satL6;		            // Soil saturation fraction of layer 6, dimensionless
	float *m_satL7;		            // Soil saturation fraction of layer 7, dimensionless


};





#endif /* SEIMS_MODULE_CH4_WETMETH_H */
