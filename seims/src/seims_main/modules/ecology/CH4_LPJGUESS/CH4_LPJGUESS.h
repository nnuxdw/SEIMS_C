/*!
 * \file CH4_LPJGUESS.h
 * \brief The LPJGUESS (Wetland Methane Model) is designed for calculating methane production, oxidation, and transport.
 *
 * Changelog:
 *   - 1. 2025-11-17 - lj - Initial implementation of LPJGUESS model
 *
 * \author Jing Li
 * \date   2025-11-17
 */
#ifndef SEIMS_MODULE_CH4_LPJGUESS_H
#define SEIMS_MODULE_CH4_LPJGUESS_H

#include "SimulationModule.h"

/*!
 * \defgroup CH4_LPJGUESS
 * \ingroup  Ecology
 * \brief    Methods for methane production, oxidation, and transport
 *
 */

 // LPJGUESS model constants from paper
const int NACROTELM = 4;                        // Number of total soil layers in the acrotelm
const int NCATOTELM = 3;                        // number of total soil layers in the catotelm
const float CH4_ROOT_DECAY_COEFF = 199.9995f;   // Exponential decay coefficient (cm)
const float CH4_ROOT_NORM_CONST = 5.94246122f;  // Standardization constant
const float CH4toCO2_PEAT = 0.085f;             // CH4/CO2(0.001-1.7)
const float Fgas = 0.00f;                       // Peat gas fraction,constant,can be set to 0 or 0.08
const float G_PER_KG = 1000.0f;                 // Conversion factor from grams to kilograms
const float PO2 = 2.09e4f;                       // O2 partial pressure (Pa)
const float Scale = 0.00001f;                   // m²/s to  cm²/s
const float SECS_PER_DAY = 86400.0f;
const float CM2_PER_M2 = 10000.0f;              // cm² to m²
const float U10 = 0.0f;                         // Wind speed at 10m height [m s-1]                                           
const float n_coeff = -0.5;                     // coefficient for the calculation of the gas transport velocity, given in Riera et al. 1999
const float henry_k_CO2 = 29.41f;              //  Henry's Law constants [L atm mol-1] at 298.15K. Wania et al. (2010), Table 2
const float henry_k_CH4 = 714.29f;
const float henry_k_O2 = 769.23f;
const float henry_C_CO2 = 2400.0f;              // Constants [K] for CO2, CH4 and O2 for calculation of Henry's coefficient cited by Sander (1999). Wania et al. (2010), Table 2
const float henry_C_CH4 = 1600.0f;
const float henry_C_O2 = 1500.0f;
const float pp_CH4 = 1.7;                       // micro atm , partial pressure of CH4 above water
const float pp_O2 = 209000;                     // micro atm , partial pressure of O2 above water (value consistent with PO2 in canexch.h)
const float CM_PER_M = 100.0f;
const float MM2_PER_M2 = 1000000.0f;            // 1 m² = 1,000,000 mm²
const float K2degC = 273.15f;                   // Baseline temperature (K)
const float atomiccmass = 12.0f;                // atomic mass of carbon [g/mol]
const float MMOL_PER_MOL = 1000.0f;
const float MM_PER_M = 1000.0f;
const float water_min = 0.1f;                   // a threshold factor for a minimum water content in the layer [unitless]
const float Dt_gas = 0.01;                      // time step for gas diffusion calculations [day]
/// Radius of an average tiller [m]
// (tiller_radius = 0.004)  ! Schimel (1995) - Average over E. angustifolium (diam=7.9mm)
// and C. aquatilis (diam=3.8mm)
// Wania et al. (2010) optimal value: 0.003mm (see Table 5). 
// McGuire et al (2012), Tang et al (2015) and Zhang et al (2013) use 0.0035, after optimisation
const float tiller_radius = 0.0035;
/// Tiller porosity
// (tiller_por = 0.6) ! Wetland plants book, eds. Cronk and Fennessy, p.90, values for 2 Erioph. spp.
const float tiller_por = 0.7f;



class CH4_LPJGUESS: public SimulationModule {
public:

	// Define the enumeration for gas types
	enum Gastype {
		O2_gas = 0,
		CO2_gas = 1,
		CH4_gas = 2
	};

	CH4_LPJGUESS();

    ~CH4_LPJGUESS();
	  
	// SetData series functions
	void SetValue(const char* key, float value) OVERRIDE;

	void SetValueByIndex(const char* key, int index, float value) OVERRIDE;

	void Set1DData(const char* key, int n, float* data) OVERRIDE;

	void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

	void SetReaches(clsReaches* rches) OVERRIDE;

	void SetSubbasins(clsSubbasins* subbsns) OVERRIDE;

	void SetScenario(Scenario* sce) OVERRIDE;

	// CheckInputData and InitialOutputs 
	bool CheckInputData() OVERRIDE;

	void InitialOutputs() OVERRIDE;

	// Main control structure of execution code
	int Execute() OVERRIDE;

	// GetData series functions
	TimeStepType GetTimeStepType() OVERRIDE;

	void GetValue(const char* key, float* value) OVERRIDE;

	void Get1DData(const char* key, int* n, float** data) OVERRIDE;
	
	void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

	// Method of root biomass ratio
	void InitRootFractions(int i);

	// Method of  calculating the volume fraction of air in the soil
	void InitFracAir(int i);

	// Method for measuring the degree of hypoxia at depth z
	void InitAnoxic(int i);

	// Method of methane production
	void MethaneProduction(int i);

	// Method of methane oxidation
	void MethaneOxidation(int i);

	// Method of methane diffusion
	void MethaneDiffusionCalculation(int i);

	// INLINE FUNCTIONS
	void tridiag(int n, float* a, float* b, float* c, float* r, float* u);

	// Crank-Nicholson timestepper algorithm for gas diffusion equation.
	void Cnstepgas(int i, float* conc, float* Di);

	// Calculate and return diffusivities, in units of m2 d-1
	void Calculate_Gas_Diffusivities(int i);

	// Update gas transport velocities [m d-1] and equilibrium gas concentrations [mmol m-3]
	void Update_Daily_Gas_Parameters(int i);

	// Generic gas diffusion method that works with CO2, CH4 and O2 (as specified with gastype) 
	float Diffuse_Gas(int i, Gastype gas_type, float& dailyDiff);

	// Methane of methane ebullition
	void Calculate_Gas_Ebullition();

private:
    // Basic parameters
    int m_nCells;          ///< Number of valid cells (HRUs)
	int m_maxSoilLyrs;     ///< Maximum number of soil layers or Number of soil layers
	float m_co2Conc;       ///< CO2 concentration,(ppmv)
	float* m_nSoilLyrs;    ///< Actual number of soil layers for each cell

    // Inputs from other modules
	float*  m_area;                  ///< Area of each cell (m²)
	float*  m_Rh;                    ///< Rh of each cell (kgC/m²)
	float** m_Tsoil;                 ///< Temperature of each soil layer (°C) - from VAR_SOTE
	float** m_layer_thickness;       ///< Thickness of each soil layer (mm) - from VAR_SOILTHICK
	float** m_soil_water_storage;    ///< Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
	float** m_soilWP;                ///< water content of soil at -1.5 MPa (wilting point)
	float** m_soilPor;               ///< porosity mm/mm
	float** m_soilIceSto;            ///< Ice storage in each soil layer (mm) 

	// Methane-related variables
	float* m_ch4Concentration;       ///< Concentration of methane(gC/m²)
	float* m_co2Concentration;       ///< Concentrations of carbon dioxide(gC/m²)
	float* m_o2Concentration;        ///< Concentration of oxygen(mol/m²)
	float** m_rootFraction;          ///< Proportion of root biomass at a certain depth z(mol/m²)
	float** m_Fair;                  ///< The proportion of air in the soil layer(0-1)
	float** m_anoxic;                ///< Degree of hypoxia(0-1)
	float** m_CH4_oxid;              ///< Methane oxidation(gC/m²)
	float** m_CH4_diff;              ///< Methane diffusion(gC/m²/day)

    // Output variables
	float* m_k_O2;
	float* m_k_CH4;
	float* m_k_CO2;
	float* m_Ceq_O2;
	float* m_Ceq_CO2;
	float* m_Ceq_CH4;

	float** m_CH4_prod;              ///< Production(gC/m²/day)
	float** m_D_CH4_water;
	float** m_D_CO2_water;
	float** m_D_O2_water;
	float** m_D_CH4_air;
	float** m_D_CO2_air;
	float** m_D_O2_air;
	float** m_dCH4;
	float** m_dCO2;
	float** m_dO2;
	float** m_D_CH4;
	float** m_D_CO2;
	float** m_D_O2;
	float** m_C;                      ///< Concentration of the dissolved gas in question [mmol m-3] 
	float** m_Cgas;                   ///< Concentration of the dissolved gas in question [mmol m-3] 
	float** m_Frac_water;
	float** m_Frac_ice;
	float** m_Frac_water_belowpwp;
	float** m_total_volume_water;
	float** m_Dz_metre;               ///< Thickness of each soil layer (m)
	float** m_volume_liquid_water;

	float** m_C_init;
	float** m_C_last;

};
#endif /* SEIMS_MODULE_CH4_LPJGUESS_H */
