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

using namespace std;

// LPJGUESS model constants from paper
//const int NACROTELM = 4;                           // Number of total soil layers in the acrotelm
//const int NCATOTELM = 3;                           // Number of total soil layers in the catotelm
const float ACROTELM_DEPTH_MM = 300.0f;              // Acrotelm: 0-300 mm
//const bool FORCE_CATOTELM_SATURATION_TEST = false;
const float CH4_ROOT_DECAY_COEFF = 199.9995f;        // Exponential decay coefficient (cm)
const float CH4_ROOT_NORM_CONST = 5.94246122f;       // Standardization constant
// LPJ-GUESS peatland root distribution parameters
const float LPJ_PEAT_ROOT_DECAY_COEFF_CM = 25.1695f;
//const float LPJ_PEAT_PROFILE_DEPTH_MM = 1500.0f;
//const float CH4toCO2_PEAT = 0.055f;                  // CH4/CO2(0.001-1.7)    0.085
//const float Fgas = 0.032f;                           // Peat gas fraction,constant,can be set to 0 or 0.08
const float G_PER_KG = 1000.0f;                      // Conversion factor from kilograms to grams
const float PO2 = 2.09e4f;                           // O2 partial pressure (Pa)
const float Scale = 0.00001f;                        // Convert diffusivity coefficients expressed in 10^-9 m2 s-1  // to numerical values in cm2 s-1
const float SECS_PER_DAY = 86400.0f;
const float CM2_PER_M2 = 10000.0f;                   // cm² to m²
const float U10 = 0.0f;                              // Wind speed at 10m height [m s-1]                                           
const float n_coeff = -0.5f;                         // Coefficient for the calculation of the gas transport velocity, given in Riera et al. 1999
const float henry_k_CO2 = 29.41f;                    // Henry's Law constants [L atm mol-1] at 298.15K. Wania et al. (2010), Table 2
const float henry_k_CH4 = 714.29f;
const float henry_k_O2 = 769.23f;
const float henry_C_CO2 = 2400.0f;                   // Constants [K] for CO2, CH4 and O2 for calculation of Henry's coefficient cited by Sander (1999). Wania et al. (2010), Table 2
const float henry_C_CH4 = 1600.0f;
const float henry_C_O2 = 1500.0f;
const float pp_CH4 = 1.7f;                           // micro atm , partial pressure of CH4 above water
const float pp_O2 = 209000.0f;                       // micro atm , partial pressure of O2 above water (value consistent with PO2 in canexch.h)
const float CM_PER_M = 100.0f;
const float MM2_PER_M2 = 1000000.0f;                 // 1 m² = 1,000,000 mm²
const float K2degC = 273.15f;                        // Celsius-to-Kelvin offset [K]
const float atomiccmass = 12.0f;                     // atomic mass of carbon [g/mol]
const float MMOL_PER_MOL = 1000.0f;
const float MM_PER_M = 1000.0f;
const float water_min = 0.1f;                        // a threshold factor for a minimum water content in the layer [unitless]
const float Dt_gas = 0.01f;                          // time step for gas diffusion calculations [day]
const float KG_HA_TO_G_M2 = 0.1f;                    // kgC/ha → gC/m²
const float GC_TO_MG_CH4 = 1000.0f * 16.0f / 12.0f;  // g CH4-C -> mg CH4
const float MAX_ERR = 0.000001f;
//const float oxid_frac = 0.96f; 
const float rho_H2O = 1000.0f;                       // density of water [kg m-3]
const float gravity = 9.81f;                         // acceleration due to gravity [m s-2]
const float atm_press = 101325.0f;                   // standard atmospheric pressure [Pa]
const float R_gas = 8.314472f;                       // universal gas constant [J mol-1 K-1]
const float mr_C = 12.0f;                            // Molecular mass of carbon [g mol-1]
const float SQ_M = 1.0f;                             // 1m²
const float vgc_high = 0.15f;                        // ebullition occurs, when the volumetric gas content (VGC) exceeds this level [unitless, m3/m3]
const float bubble_CH4_frac = 0.57f;                 // CH4 fraction of gas bubbles [unitless]
const float vgc_low = 0.145f;                        // when ebullition occurs, the volumetric gas content (VGC) will drop to this level [unitless]
//const float vgc_high = 0.10f;
//const float bubble_CH4_frac = 0.57f;
//const float vgc_low = 0.095f;

// Tiller porosity
// (tiller_por = 0.6) ! Wetland plants book, eds. Cronk and Fennessy, p.90, values for 2 Erioph. spp.
const float tiller_por = 0.7f;                       // Wania et al. (2010) optimal value: 0.7 (see Table 5)
const float tiller_weight = 0.22f;                   // Individual tiller weight [g C]


/// Radius of an average tiller [m]
// (tiller_radius = 0.004)  ! Schimel (1995) - Average over E. angustifolium (diam=7.9mm)
// and C. aquatilis (diam=3.8mm)
// Wania et al. (2010) optimal value: 0.003 m (3 mm)
// McGuire et al (2012), Tang et al (2015) and Zhang et al (2013) use 0.0035, after optimisation
const float tiller_radius = 0.0035f;
//const float SLA_dry = 15.0f;   // m2 kg-1; initial SLA for wetland graminoids (literature-based range: 14–18 m2 kg-1)
//const float leaf_carbon_frac = 0.45f;   // kgC kg-1 dry mass(literature-based range: 0.4–0.5)
// Fixed LPJ-GUESS WetGRS parameters for local Cottonwood test
const float LPJ_WETGRS_LEAFLONG = 0.5f;  // year

// Walter and Heimann (2000) plant-mediated CH4 transport parameters
const float WALTER_KP = 0.01f;                       // Plant transport rate coefficient [h-1]
const float WALTER_AMIN = 0.0f;                      // Minimum plant growth factor
const float WALTER_AMAX = 4.0f;                      // Maximum plant growth factor
//const float WALTER_TGR = 2.0f;                       // Plant growth threshold temperature [deg C]
//const float WALTER_TMAT = 12.0f;                     // Plant maturity temperature [deg C]
//const float WALTER_POX = 0.50f;                    // Fraction oxidized in the rhizosphere
const float WALTER_T50_DEPTH_MM = 500.0f;            // Soil temperature depth [mm]
//const float WALTER_TVEG = 7.5f;                    // Initial vegetation transport parameter (0-15)

const float SNOW_DIFFUSION_DEPTH_LIMIT_MM = 50.0f;

// DEBUGGING BOOLEANS
//const bool DEBUG_METHANE = true;
const bool DEBUG_METHANE = false;
const bool DEBUG_CH4_EBULL = true;



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

	// Initialize LPJ-GUESS peat root fractions for CH4 production
	void InitPeatRootFractions(int i);

	// Method of  calculating the volume fraction of air in the soil
	void InitFracAir(int i);

	// Method for measuring the degree of hypoxia at depth z
	void InitAnoxic(int i);

	// INLINE FUNCTIONS
	/*void tridiag(int n, float* a, float* b, float* c, float* r, float* u);*/
	void tridiag(int n, double* a, double* b, double* c, double* r, double* u);

	// Crank-Nicholson timestepper algorithm for gas diffusion equation.
	/*void Cnstepgas(int i, float* conc, float* Di);*/
	void Cnstepgas(int i, float* conc, float* Di, double dt);

	// Calculate and return diffusivities, in units of m2 d-1
	void Calculate_Gas_Diffusivities(int i);

	// Update gas transport velocities [m d-1] and equilibrium gas concentrations [mmol m-3]
	bool Update_Daily_Gas_Parameters(int i);

	// Generic gas diffusion method that works with CO2, CH4 and O2 (as specified with gastype) 
	float Diffuse_Gas(int i, Gastype gas_type, float& dailyDiff);

	// Calculate the area of tillers
	bool IsWetlandGraminoidActive(const int i);

	// Calculate the area of tillers
	float Calculate_tiller_areas(const int i);

	// Plant transport of O2 or CH4
	float Plant_Gas_Transport(const int i, Gastype gas_type, float& plantTransportToday);

	// Plant-mediated CH4 transport based on Walter and Heimann (2000)
	float Plant_CH4_Transport(const int i, float& plantTransportToday, float& plantOxidToday);

	// Methane of methane ebullition
	bool Calculate_Gas_Ebullition(int i, float& ebull_today);

	// Carbon accounting routine - updates co2_store and ch4_store 
	void Calculate_Carbon_Store(int i, int dy, bool today);

	// Return CH4 content (g CH4-C / m2) 
	float Get_CH4_Content();

	// Return CO2 content (gC / m2) 
	float Get_CO2_Content();

	// Calculate  methane
	bool Methane(int i, int daynum);

	float CalculateSnowDepth(const int i);

private:
	enum MethaneMethod {
		CH4_METHOD_NONE = 0,
		CH4_METHOD_SPAHNI = 1,
		CH4_METHOD_WANIA = 2
	};

	bool IsMethaneWetland(const int i) const;
	MethaneMethod SelectMethaneMethod(const int i) const;
	bool MethaneSpahni(const int i);

    // Basic parameters
    int m_nCells;          ///< Number of valid cells (HRUs)
	int m_maxSoilLyrs;     ///< Maximum number of soil layers or Number of soil layers
	float m_co2Conc;       ///< CO2 concentration,(ppmv)
	float m_walterTveg;    ///< Walter vegetation transport quality factor [0-15]
	float m_walterPox;     ///< Rhizospheric oxidation fraction [0-1]
	float m_ch4Co2Ratio;   ///< Ratio of CH4 production to anaerobic respiration
	float m_fGas;          ///< Constant gas fraction in peat soil
	float m_oxidFrac;      ///< Fraction of oxygen used for CH4 oxidation
	float m_walterTgr;     ///< Walter plant growth threshold temperature [deg C]

	float* m_nSoilLyrs;    ///< Actual number of soil layers for each cell
	float* m_landuse;
	/*float* m_icnum;*/

    // Inputs from other modules
	float*  m_area;                  ///< Area of each cell (m²)
	float*  m_Rh;                    ///< Rh of each cell (kgC/m²)
	float*  m_handWtrDep;            ///< HAND surface-water depth after SUR_MR_HAND infiltration (m)
	float*  m_lai;                   ///< the leaf area indices for day i
	float*  m_maxLai;                ///< maximum (potential) leaf area index (BLAI in cropLookup db)
	float*  m_biomass;               ///< land cover/crop biomass (dry weight), bio_ms in SWAT

	float** m_Tsoil;                 ///< Temperature of each soil layer (°C) - from VAR_SOILT
	float** m_layer_thickness;       ///< Thickness of each soil layer (mm) - from VAR_SOILTHICK
	float** m_soil_water_storage;    ///< Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
	float** m_soilWP;                ///< water content of soil at -1.5 MPa (wilting point)
	float** m_soilPor;               ///< porosity mm/mm
	float** m_soilIceSto;            ///< Ice storage in each soil layer (mm)
	float** m_sol_RSPC;              ///< soil heterotrophic respiration (kgC/ha/d)
	
	float** m_soilSaturation;        ///< Saturation ratio of each soil layer (0-1) - calculated

	float*  m_snowAccum;             ///< Snow water equivalent from snow module [mm]
	float*  m_cellLat;


	// Plant-related variables
	float*  m_rootDepth;             ///< Root depth from PG_EPIC (mm)
	float** m_tiller_area;

	// Methane-related variables
	/*float** m_rootFraction;          ///< Proportion of root biomass at a certain depth z(mol/m²)*/
	float** m_rootFraction;          ///< Walter root distribution fraction in each soil layer
	float** m_peatRootFraction;      ///< LPJ-GUESS peat root fraction used to redistribute profile-total Rh
	float** m_Fair;                  ///< The proportion of air in the soil layer(0-1)
	float** m_anoxic;                ///< Degree of hypoxia(0-1)
	float** m_CH4_diff;              ///< Methane diffusion(gC/m²/day)

	
	float m_CO2_store;             ///< CO2 stores in the soil layers - updated daily 
	float m_CH4_store;             ///< CH4 stores in the soil layers - updated daily

	float** m_CO2_soil;              ///< Dissolved CO2 concentration in each layer [g CO2-C layer-1 d-1]
	float** m_CO2_soil_yesterday;    ///< Dissolved CO2 concentration in each layer yesterday [g CO2-C layer-1 d-1]
	float** m_CO2_soil_prod;         ///< Daily CO2 production in each layer [g CO2-C layer-1 d-1]

	float** m_CH4;                   ///< Dissolved CH4 concentration in each layer [g CH4-C layer-1 d-1]
	float** m_CH4_yesterday;         ///< Dissolved CH4 concentration in each layer yesterday [g CH4-C layer-1 d-1]
	float** m_CH4_prod;              ///< Daily CH4 production in each layer [g CH4-C layer-1 d-1]
	float** m_CH4_oxid;              ///< Daily CH4 oxidation in each layer [g CH4-C layer-1 d-1]

	float** m_O2;                    ///< Dissolved O2 concentration in each layer [mol O2 layer-1 d-1]


	float** m_CH4_ebull_ind;           ///< CH4 which bubbles out [g CH4-C layer-1]
	float** m_CH4_ebull_vol;           ///< Volume of CH4 which bubbles out [m3]
	float** m_CH4_gas;                 ///< gaseous CH4 concentration in each layer [g CH4-C layer-1 d-1]
	float** m_CH4_gas_yesterday;       ///< gaseous CH4 concentration in each layer [g CH4-C layer-1 d-1]
	float** m_CH4_diss;                ///< dissolved CH4 concentration in each layer [g CH4-C layer-1 d-1]
	float** m_CH4_diss_yesterday;      ///< dissolved CH4 concentration in each layer [g CH4-C layer-1] yesterday
	float** m_CH4_gas_vol;             ///< gaseous CH4 volume in each layer [m3]
	float** m_CH4_vgc;                 ///< volumetric CH4 content [unitless]


    // Output variables
	float* m_k_O2;            ///< O2 surface exchange velocity [m d-1]
	float* m_k_CH4;           ///< CH4 surface exchange velocity [m d-1]
	float* m_k_CO2;           ///< CO2 surface exchange velocity [m d-1]
	float* m_Ceq_O2;          ///< O2 equilibrium concentration [mmol m-3]
	float* m_Ceq_CO2;         ///< CO2 equilibrium concentration [mmol m-3]
	float* m_Ceq_CH4;         ///< CH4 equilibrium concentration [mmol m-3]


	//float** m_CH4_prod;              ///< Production(gC/m²/day)
	float** m_D_CH4_water;     ///< CH4 molecular diffusivity in water [cm2 s-1]
	float** m_D_CO2_water;     ///< CO2 molecular diffusivity in water [cm2 s-1]
	float** m_D_O2_water;      ///< O2 molecular diffusivity in water [cm2 s-1]
	float** m_D_CH4_air;       ///< CH4 molecular diffusivity in air [cm2 s-1]
	float** m_D_CO2_air;       ///< CO2 molecular diffusivity in air [cm2 s-1]
	float** m_D_O2_air;        ///< O2 molecular diffusivity in air [cm2 s-1]
	float** m_dCH4;            ///< Effective CH4 diffusivity [m2 d-1]
	float** m_dCO2;            ///< Effective CO2 diffusivity [m2 d-1]
	float** m_dO2;             ///< Effective O2 diffusivity [m2 d-1]
	float** m_D_CH4;           ///< Working CH4 diffusivity used by solver [m2 d-1]
	float** m_D_CO2;           ///< Working CO2 diffusivity used by solver [m2 d-1]
	float** m_D_O2;            ///< Working O2 diffusivity used by solver [m2 d-1]
	float** m_C;               ///< Concentration of the dissolved gas in question [mmol m-3] 
	float** m_Cgas;            ///< Concentration of the dissolved gas in question [mmol m-3] 
	float** m_Frac_water;      ///< Liquid-water fraction above wilting point [-]
	float** m_Frac_ice;        ///< Ice volume fraction [-]
	float** m_Frac_water_belowpwp;  ///< Remaining liquid-water fraction below wilting point [-]
	float** m_total_volume_water;   ///< Liquid-water-plus-ice volume per unit ground area [m3 m-2]
	float** m_Dz_metre;             ///< Thickness of each soil layer (m)
	float** m_volume_liquid_water;  ///< Liquid-water volume per unit ground area [m3 m-2]

	float** m_C_init;          ///< Concentration before one CN substep [mmol m-3]
	float** m_C_last;          ///< Concentration after one CN substep [mmol m-3]

	// daily outputs (per cell)
	float* m_CH4_prod_hour;     // mg CH4 m-2 h-1
	float* m_CH4_oxid_hour;     // mg CH4 m-2 h-1
	float* m_CH4_diff_hour;     // mg CH4 m-2 h-1
	float* m_CH4_ebull_hour;    // mg CH4 m-2 h-1
	float* m_CH4_plant_hour;    // mg CH4 m-2 h-1
	float* m_CH4_flux_hour;     // mg CH4 m-2 h-1 (diff + ebull + plant)

};
#endif /* SEIMS_MODULE_CH4_LPJGUESS_H */
