#include "CH4_LPJGUESS.h"
#include "text.h"
#include <cmath>
#include <algorithm>


// Initialize all member variables, set pointers to nullptr, set values to 0
CH4_LPJGUESS::CH4_LPJGUESS() :
	m_nCells(-1),m_maxSoilLyrs(-1),m_co2Conc(NODATA_VALUE),m_area(nullptr),m_Tsoil(nullptr),m_nSoilLyrs(nullptr),m_rootFraction(nullptr), 
	m_layer_thickness(nullptr),m_soil_water_storage(nullptr),m_soilPor(nullptr),m_soilWP(nullptr),m_Fair(nullptr),m_soilIceSto(nullptr),
	m_anoxic(nullptr),m_CH4_prod(nullptr),m_CH4_oxid(nullptr),m_D_CH4_water(nullptr),m_D_CO2_water(nullptr),m_D_O2_water(nullptr),
	m_D_CH4_air(nullptr),m_D_CO2_air(nullptr),m_D_O2_air(nullptr),m_dCH4(nullptr),m_dCO2(nullptr),m_dO2(nullptr),m_D_CH4(nullptr), m_sol_RSPC(nullptr),
	m_D_CO2(nullptr),m_D_O2(nullptr),m_k_O2(nullptr),m_k_CH4(nullptr),m_k_CO2(nullptr),m_Ceq_O2(nullptr),m_Ceq_CH4(nullptr), m_CH4_vgc(nullptr),
	m_Ceq_CO2(nullptr),m_C(nullptr),m_Cgas(nullptr),m_total_volume_water(nullptr),m_Frac_water(nullptr),m_Frac_ice(nullptr), m_CH4_gas_vol(nullptr),
	m_Frac_water_belowpwp(nullptr),m_Dz_metre(nullptr),m_volume_liquid_water(nullptr),m_C_init(nullptr),m_C_last(nullptr), m_CH4_gas(nullptr), m_CH4_ebull_vol(nullptr),
	m_handWtrDep(nullptr),m_CO2_soil(nullptr), m_CO2_soil_yesterday(nullptr), m_CO2_soil_prod(nullptr),m_CH4(nullptr), m_CH4_gas_yesterday(nullptr),
	m_CH4_yesterday(nullptr), m_O2(nullptr), m_soilSaturation(nullptr), m_CH4_ebull_ind(nullptr), m_CH4_diss_yesterday(nullptr), m_CH4_diss(nullptr),
	m_CH4_prod_hour(nullptr), m_CH4_oxid_hour(nullptr), m_CH4_diff_hour(nullptr), m_CH4_ebull_hour(nullptr), m_CH4_flux_hour(nullptr) {
	
}

CH4_LPJGUESS::~CH4_LPJGUESS() {

	if (m_area != nullptr) Release1DArray(m_area);
	if (m_k_O2 != nullptr) Release1DArray(m_k_O2);
	if (m_k_CH4 != nullptr) Release1DArray(m_k_CH4);
	if (m_k_CO2 != nullptr) Release1DArray(m_k_CO2);
	if (m_Ceq_O2 != nullptr) Release1DArray(m_Ceq_O2);
	if (m_Ceq_CH4 != nullptr) Release1DArray(m_Ceq_CH4);
	if (m_Ceq_CO2 != nullptr) Release1DArray(m_Ceq_CO2);
	if (m_nSoilLyrs != nullptr) Release1DArray(m_nSoilLyrs);
	if (m_handWtrDep != nullptr) Release1DArray(m_handWtrDep);

	if (m_CH4_prod_hour != nullptr) Release1DArray(m_CH4_prod_hour);
	if (m_CH4_oxid_hour != nullptr) Release1DArray(m_CH4_oxid_hour);
	if (m_CH4_diff_hour != nullptr) Release1DArray(m_CH4_diff_hour);
	if (m_CH4_ebull_hour != nullptr) Release1DArray(m_CH4_ebull_hour);
	if (m_CH4_flux_hour != nullptr) Release1DArray(m_CH4_flux_hour);


	if (m_Tsoil != nullptr) Release2DArray(m_nCells, m_Tsoil);
	if (m_soilWP != nullptr) Release2DArray(m_nCells, m_soilWP);
	if (m_soilPor != nullptr) Release2DArray(m_nCells, m_soilPor);
	if (m_soilIceSto != nullptr) Release2DArray(m_nCells, m_soilIceSto);
	if (m_rootFraction != nullptr) Release2DArray(m_nCells, m_rootFraction);
	if (m_soilSaturation != nullptr) Release2DArray(m_nCells, m_soilSaturation);
	if (m_layer_thickness != nullptr) Release2DArray(m_nCells, m_layer_thickness);
	if (m_anoxic != nullptr) Release2DArray(m_nCells, m_anoxic);
	if (m_CH4_prod != nullptr) Release2DArray(m_nCells, m_CH4_prod);
	if (m_CH4_oxid != nullptr) Release2DArray(m_nCells, m_CH4_oxid);
	if (m_soil_water_storage != nullptr) Release2DArray(m_nCells, m_soil_water_storage);
	if (m_D_CH4_water != nullptr) Release2DArray(m_nCells, m_D_CH4_water);
	if (m_D_CO2_water != nullptr) Release2DArray(m_nCells, m_D_CO2_water);
	if (m_D_O2_water != nullptr) Release2DArray(m_nCells, m_D_O2_water);
	if (m_D_CH4_air != nullptr) Release2DArray(m_nCells, m_D_CH4_air);
	if (m_D_CO2_air != nullptr) Release2DArray(m_nCells, m_D_CO2_air);
	if (m_D_O2_air != nullptr) Release2DArray(m_nCells, m_D_O2_air);
	if (m_dCH4 != nullptr) Release2DArray(m_nCells, m_dCH4);
	if (m_dCO2 != nullptr) Release2DArray(m_nCells, m_dCO2);
	if (m_dO2 != nullptr) Release2DArray(m_nCells, m_dO2);
	if (m_D_CH4 != nullptr) Release2DArray(m_nCells, m_D_CH4);
	if (m_D_CO2 != nullptr) Release2DArray(m_nCells, m_D_CO2);
	if (m_D_O2 != nullptr) Release2DArray(m_nCells, m_D_O2);
	if (m_C != nullptr) Release2DArray(m_nCells, m_C);
	if (m_Cgas != nullptr) Release2DArray(m_nCells, m_Cgas);
	if (m_total_volume_water != nullptr) Release2DArray(m_nCells, m_total_volume_water);
	if (m_Frac_water != nullptr) Release2DArray(m_nCells, m_Frac_water);
	if (m_Frac_ice != nullptr) Release2DArray(m_nCells, m_Frac_ice);
	if (m_Frac_water_belowpwp != nullptr) Release2DArray(m_nCells, m_Frac_water_belowpwp);
	if (m_Dz_metre != nullptr) Release2DArray(m_nCells, m_Dz_metre);
	if (m_volume_liquid_water != nullptr) Release2DArray(m_nCells, m_volume_liquid_water);
	if (m_C_init != nullptr) Release2DArray(m_nCells, m_C_init);
	if (m_C_last != nullptr) Release2DArray(m_nCells, m_C_last);
	if (m_CO2_soil != nullptr) Release2DArray(m_nCells, m_CO2_soil);
	if (m_CO2_soil_yesterday != nullptr) Release2DArray(m_nCells, m_CO2_soil_yesterday);
	if (m_CO2_soil_prod != nullptr) Release2DArray(m_nCells, m_CO2_soil_prod);
	if (m_CH4 != nullptr) Release2DArray(m_nCells, m_CH4);
	if (m_CH4_yesterday != nullptr) Release2DArray(m_nCells, m_CH4_yesterday);
	if (m_CH4_diss_yesterday != nullptr) Release2DArray(m_nCells, m_CH4_diss_yesterday);
	if (m_CH4_diss != nullptr) Release2DArray(m_nCells, m_CH4_diss);
	if (m_O2 != nullptr) Release2DArray(m_nCells, m_O2);
	if (m_CH4_ebull_ind != nullptr) Release2DArray(m_nCells, m_CH4_ebull_ind);
	if (m_CH4_gas_yesterday != nullptr) Release2DArray(m_nCells, m_CH4_gas_yesterday);
	if (m_CH4_gas != nullptr) Release2DArray(m_nCells, m_CH4_gas);
	if (m_CH4_gas_vol != nullptr) Release2DArray(m_nCells, m_CH4_gas_vol);
	if (m_CH4_vgc != nullptr) Release2DArray(m_nCells, m_CH4_vgc);
	if (m_sol_RSPC != nullptr) Release2DArray(m_nCells, m_sol_RSPC);

	if (m_CH4_ebull_vol != nullptr) Release2DArray(m_nCells, m_CH4_ebull_vol);
}


void CH4_LPJGUESS::InitialOutputs() {
	CHECK_POSITIVE(MID_CH4_LPJGUESS, m_nCells);
	CHECK_POSITIVE(MID_CH4_LPJGUESS, m_maxSoilLyrs);


	// Initialize the two-dimensional array
	if (m_Fair == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_Fair, 0.f);
	if (m_anoxic == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_anoxic, 0.f);
	if (m_CH4_prod == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_prod, 0.f);
	if (m_CH4_oxid == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_oxid, 0.f);
	if (m_rootFraction == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_rootFraction, 0.f);
	if (m_D_CH4_water == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_CH4_water, 0.f);
	if (m_D_CO2_water == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_CO2_water, 0.f);
	if (m_D_O2_water == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_O2_water, 0.f);
	if (m_D_CH4_air == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_CH4_air, 0.f);
	if (m_D_CO2_air == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_CO2_air, 0.f);
	if (m_D_O2_air == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_O2_air, 0.f);
	if (m_dCH4 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_dCH4, 0.f);
	if (m_dCO2 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_dCO2, 0.f);
	if (m_dO2 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_dO2, 0.f);
	if (m_D_CH4 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_CH4, 0.f);
	if (m_D_CO2 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_CO2, 0.f);
	if (m_D_O2 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_D_O2, 0.f);
	if (m_C == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_C, 0.f);
	if (m_Cgas == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_Cgas, 0.f);
	if (m_total_volume_water == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_total_volume_water, 0.f);
	if (m_Frac_water == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_Frac_water, 0.f);
	if (m_Frac_ice == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_Frac_ice, 0.f);
	if (m_Frac_water_belowpwp == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_Frac_water_belowpwp, 0.f);
	if (m_Dz_metre == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_Dz_metre, 0.f);
	if (m_volume_liquid_water == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_volume_liquid_water, 0.f);
	if (m_C_init == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_C_init, 0.f);
	if (m_C_last == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_C_last, 0.f);
	if (m_CO2_soil == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CO2_soil, 0.f);
	if (m_CO2_soil_yesterday == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CO2_soil_yesterday, 0.f);
	if (m_CO2_soil_prod == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CO2_soil_prod, 0.f);
	if (m_CH4 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4, 0.f);
	if (m_CH4_yesterday == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_yesterday, 0.f);
	if (m_O2 == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_O2, 0.f);
	if (m_soilSaturation == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilSaturation, 0.f);
	if (m_CH4_ebull_ind == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_ebull_ind, 0.f);
	if (m_CH4_diss_yesterday == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_diss_yesterday, 0.f);
	if (m_CH4_diss == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_diss, 0.f);
	if (m_CH4_gas_yesterday == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_gas_yesterday, 0.f);
	if (m_CH4_gas == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_gas, 0.f);
	if (m_CH4_gas_vol == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_gas_vol, 0.f);
	if (m_CH4_vgc == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_vgc, 0.f);
	if (m_CH4_ebull_vol == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_CH4_ebull_vol, 0.f);

	// Initialize the one-dimensional array
	if (m_k_O2 == nullptr)
		Initialize1DArray(m_nCells, m_k_O2, 0.f);
	if (m_k_CH4 == nullptr)
		Initialize1DArray(m_nCells, m_k_CH4, 0.f);
	if (m_k_CO2 == nullptr)
		Initialize1DArray(m_nCells, m_k_CO2, 0.f);
	if (m_Ceq_O2 == nullptr)
		Initialize1DArray(m_nCells, m_Ceq_O2, 0.f);
	if (m_Ceq_CH4 == nullptr)
		Initialize1DArray(m_nCells, m_Ceq_CH4, 0.f);
	if (m_Ceq_CO2 == nullptr)
		Initialize1DArray(m_nCells, m_Ceq_CO2, 0.f);


	if (m_CH4_prod_hour == nullptr)
		Initialize1DArray(m_nCells, m_CH4_prod_hour, 0.f);
	if (m_CH4_oxid_hour == nullptr)
		Initialize1DArray(m_nCells, m_CH4_oxid_hour, 0.f);
	if (m_CH4_diff_hour == nullptr)
		Initialize1DArray(m_nCells, m_CH4_diff_hour, 0.f);
	if (m_CH4_ebull_hour == nullptr)
		Initialize1DArray(m_nCells, m_CH4_ebull_hour, 0.f);
	if (m_CH4_flux_hour == nullptr)
		Initialize1DArray(m_nCells, m_CH4_flux_hour, 0.f);

	}


bool CH4_LPJGUESS::CheckInputData() {
	// Check basic parameters
	if (m_nCells <= 0) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "The number of cells must be greater than 0.");
	}
	if (m_maxSoilLyrs <= 0) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "The number of soil layers must be greater than 0.");
	}

	// Check input data pointers
	if (m_area == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Cell area data is not set.");
	}
	if (m_layer_thickness == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil layer thickness data is not set.");
	}
	if (m_soil_water_storage == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil water storage data is not set.");
	}
	if (m_soilWP == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil WP data is not set.");
	}
	if (m_soilPor == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil porosity data is not set.");
	}
	if (m_soilIceSto == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil ice storage data is not set.");
	}
	if (m_handWtrDep == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Depth of each hand data is not set.");
	}
	return true;
}


void CH4_LPJGUESS::SetValue(const char* key, float value) {
	string sk(key);
	if (StringMatch(sk, VAR_CO2)) {
		m_co2Conc = value;
	}
	else {
		throw ModelException(MID_CH4_LPJGUESS, "SetValue", "Parameter " + sk + " does not exist.");
	}
}


void CH4_LPJGUESS::SetValueByIndex(const char* key, int index, float value) {

}


void CH4_LPJGUESS::Set1DData(const char* key, int n, float* data) {
	string s(key);
	CheckInputSize(MID_CH4_LPJGUESS, key, n, m_nCells);    // Data size validation (module ID, variable name, data length)
	if (StringMatch(s, VAR_AHRU)) {
		m_area = data;
	}
	else if (StringMatch(s, VAR_SOILLAYERS)) {
		m_nSoilLyrs = data;     // Actual number of soil layers for each cell
	}
	else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
		m_handWtrDep = data;    // Water depth of each hand(m)
	}
	else {
		throw ModelException(MID_CH4_LPJGUESS, "Set1DData", "Parameter " + s + " does not exist.");
	}
}


void CH4_LPJGUESS::Set2DData(const char* key, int n, int col, float** data) {
	string sk(key);
	CheckInputSize2D(MID_CH4_LPJGUESS, key, n, col, m_nCells, m_maxSoilLyrs);

	if (StringMatch(sk, VAR_SOILTHICK)) {
		m_layer_thickness = data;
	}else if (StringMatch(sk, VAR_SOL_ST)) {
		m_soil_water_storage = data;                // Soil layer water storage (mm H2O)
	}else if (StringMatch(sk, VAR_SOILT)) {
		m_Tsoil = data;                             // Soil temperature (°C)
	}else if (StringMatch(sk, VAR_SOL_WPMM)) {
		m_soilWP = data;
	}else if (StringMatch(sk, VAR_POROST)) {
		m_soilPor = data;                           // Porosity mm/mm
	}else if (StringMatch(sk, VAR_SOLICE)) {
		m_soilIceSto = data;
	}else if (StringMatch(sk, VAR_SOL_RSPC)) {
		m_sol_RSPC = data;
	}else {
		throw ModelException(MID_CH4_LPJGUESS, "Set2DData", "Parameter " + sk + " does not exist.");
	}
}


void CH4_LPJGUESS::SetReaches(clsReaches* rches) {

}


void CH4_LPJGUESS::SetSubbasins(clsSubbasins* subbsns) {

}


void CH4_LPJGUESS::SetScenario(Scenario* sce) {

}


TimeStepType CH4_LPJGUESS::GetTimeStepType() {
	return TIMESTEP_HILLSLOPE;
}


void CH4_LPJGUESS::tridiag(int n, float* a, float* b, float* c, float* r, float* u) {

	// Tridiagonal system solver from Numerical Recipes.
	float* gam = new float[n];

	// Header Element
	float bet = b[0];
	u[0] = r[0] / bet;

	for (int j = 1; j < n; j++) {
		gam[j] = c[j - 1] / bet;
		bet = b[j] - a[j] * gam[j];

		u[j] = (r[j] - a[j] * u[j - 1]) / bet;
	}

	for (int j = (n - 2); j >= 0; j--) {
		u[j] -= gam[j + 1] * u[j + 1];
	}
	delete[] gam;
}


// Crank-Nicholson timestepper algorithm for gas diffusion equation.
void CH4_LPJGUESS::Cnstepgas(int i, float* conc, float* Di) {

	// UNITS:
	// Di         m2 d-1
	// dz         m
	// surf_conc  mmol m-3
	// dt         d
	// conc       mmol m-3

	// Only solve for this cell's vertical 1-D diffusion
	int nLyr = m_nSoilLyrs[i];          // number of soil layers for cell i
	if (nLyr <= 1) return;              // It's impossible to achieve diffusion with only one layer.

	// The thickness of the soil layer in this unit (m)
	float* dz = m_Dz_metre[i];         

	// Top boundary concentration
	float surf_conc = conc[0];

	// Diffusion time step (day)
	float dt = Dt_gas;

	// Layer counters: 
	// The values used in the Crank-Nicholson solver (a vector of length active_layers)
	int layer, lidx;
	int active_layers;
	float dplus;
	float dminus;
	float dz_factor;
	float Cplus;
	float Cp_minus;
	float dzhere, dzminus, dzplus;
	float cohere, cominus, coplus;

	// Leading diagonal, left and right subdiagonals for Crank-Nicholson
	// matrix.

	int layer0 = 0;                      // Surface index = 0
	active_layers = nLyr - layer0;       // All the nLyr layers are involved in the diffusion process.

	float* diag = new float[active_layers];
	float* left = new float[active_layers];
	float* right = new float[active_layers];

	// Right hand side vector for Crank-Nicholson scheme equations.
	float* rhs = new float[active_layers];

	// Solution vector for Crank-Nicholson scheme equations.
	float* solution = new float[active_layers];

	// initialise
	dplus = 0.f;
	dminus = 0.f;
	dz_factor = 0.f;
	Cplus = 0.0f;
	Cp_minus = 0.f;
	dzhere = 0.f;
	dzminus = 0.f;
	dzplus = 0.f;
	cohere = 0.f;
	cominus = 0.f;
	coplus = 0.f;

	for (int j = 0; j < active_layers; j++) {
		diag[j] = 0.f;
		left[j] = 0.f;
		right[j] = 0.f;
		rhs[j] = 0.f;
		solution[j] = 0.f;
	}

	// --- CODE STARTS HERE ---
    
	// BUILD TRIDIAGONAL MATRIX AND KNOWN RIGHT HAND SIDE

	// End members for off-diagonal elements.
	left[0] = 0.f;
	right[active_layers - 1] = 0.f;

	// Process the active layers. 
	// The first time (lidx and layer = layer0) corresponds to the surface layer
	for (int lidx = 1; lidx <= active_layers; lidx++) {

		// Deal with different layer counting schemes.
		layer = lidx + layer0 - 1;
		// Minimum is layer0
		// Maximum is nLyr - 1

		// Calculate diffusion constants averaged over adjacent layers.
		// The diffusion constant at the bottom layer is clamped to zero
		// to enforce the no heat flow boundary condition there.  

		// D+

		if (layer == nLyr - 1) {
			dplus = 0.0f;          // BC2 - Bottom layer diffusion clamped to 0
		}
		else {
			dplus = 0.5f * (Di[layer] + Di[layer + 1]);
		}

		// D-

		if (layer == layer0) {
			dminus = Di[layer];  // top layer
		}
		else {
			dminus = 0.5f * (Di[layer] + Di[layer - 1]);   // soil layers
		}

		// --- HERE ---
		if (layer < nLyr) {
			// all soil layers
			dzhere = dz[layer];       // Current layer thickness（m） 
			cohere = conc[layer];     // Current layer gas concentration (mmol/m³)
		}

		// --- PLUS ---
		if (layer < nLyr - 1) {
			// all soil layers apart from the bottom soil layer
			dzplus = dz[layer + 1];
			coplus = conc[layer + 1];
		}

		// --- MINUS ---
		if (layer == layer0) {
			// top layer
			dzminus = dz[layer];
			cominus = conc[layer];
		}
		else {
			dzminus = dz[layer - 1];
			cominus = conc[layer - 1];
		}

		// Crank–Nicholson coefficient
		dz_factor = 0.25f * (dzplus + 2.0f * dzhere + dzminus);
		Cplus = dplus * dt / dz_factor / (dzplus + dzhere);
		Cp_minus = dminus * dt / dz_factor / (dzhere + dzminus);

		// Fill in matrix diagonal and off-diagonal elements.

		// DIAG
		if (lidx == 1) {
			diag[0] = 1.0f;                 // BC1 - top layer should be (1,0,...,0)
		}else {
			diag[lidx - 1] = 1.0f + Cplus + Cp_minus;
		}

		// LEFT & RIGHT
		if (lidx < active_layers) {

			if (lidx > 1)
				right[lidx - 1] = -Cplus;
			else
				right[lidx - 1] = 0.0f;     // i.e. BC1, where top layer == (1,0,..,0)

			// left[0] is set above.
			if (lidx > 1)
				left[lidx - 1] = -Cp_minus;
		}

		if (lidx == active_layers)
			left[lidx - 1] = -Cp_minus;

		// RHS
		// Calculate right hand side vector values.
		if (lidx == 1) {                                                       
			rhs[0] = surf_conc;        // Top layer: Exactly equal to the surface concentration
		} else if (lidx == active_layers) {        // Cplus == 0 here anyway
			rhs[lidx - 1] = (1.0f - Cp_minus) * cohere + Cp_minus * cominus;        // Bottom layer: No flux boundary
		} else {
			rhs[lidx - 1] = (1.0f - Cplus - Cp_minus) * cohere + Cplus * coplus + Cp_minus * cominus;      // Middle Layer: Standard CN Discrete
		}
	} // end for

	//   SOLVE TRIDIAGONAL SYSTEM
	tridiag(active_layers, left, diag, right, rhs, solution);

	if (DEBUG_METHANE) {

		// Test 1:                                                                                                                          
		int testrow = active_layers / 2;
		float checksum = left[testrow] * solution[testrow - 1] + diag[testrow] * solution[testrow] + right[testrow] * solution[testrow + 1] - rhs[testrow];

		const float MAX_ERR = 0.000001;
		                  
		/*if (fabs(checksum) > MAX_ERR) {
				std::cout << "[CH4_DIFF Test1] Bad checksum after cnstepgas - tridiag - test1\n"
					<< "   row = " << testrow                        
					<< "   checksum = " << checksum
					<< "   (should be near 0)"
					<< std::endl;
			}*/

		// Test 2:
		// Every entry in rowsum should == 1.0 
		checksum = 0.0;
		for (int j = 0; j < active_layers; j++) {
			float rowsum = left[j] + diag[j] + right[j];
			checksum += rowsum / float(active_layers);                             
		}

		/*if (fabs(checksum - 1.0) > MAX_ERR) {
			std::cout << "[CH4_DIFF Test2] Bad checksum after cnstepgas - tridiag - test2\n"
				<< "   avg_rowsum = " << checksum
				<< "   (should be near 1.0)"                                                                                                                              
				<< std::endl;
		}*/
	}

	// FORMAT OUTPUT PARAMETERS

	// Transfer the solution to the concentration array.
	for (int j = 0; j < active_layers; j++) {
		conc[j] = solution[j];
	}

	delete[] diag;
	delete[] left;
	delete[] right;
	delete[] rhs;
	delete[] solution;
}


void CH4_LPJGUESS::InitRootFractions(int i) {

	//std::cout << "开始计算根生物量比例..." << std::endl;

	// Wania et al. (2010) parameters
	float sumrootfrac = 0.f;         // cumulative root biomass ratio
	float cumulative_depth = 0.f;    // cumulative depth (cm)

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		// Calculate the middle depth of the current layer
		float layer_thickness_cm = m_layer_thickness[i][j] / 10.f;      // mm to cm
		float layermiddepth = cumulative_depth + layer_thickness_cm / 2.f;
		//std::cout << "层 " << j << ": 厚度=" << layer_thickness_cm
		// << "cm, 中间深度=" << layermiddepth << "cm" << std::endl;

		if (j == m_nSoilLyrs[i] - 1) {
			// The final layer: Ensure that the total equals 1
			m_rootFraction[i][j] = 1.f - sumrootfrac;
			//std::cout << "最后一层，根生物量比例设置为: " << m_rootFraction[i][j] << std::endl;
		}
		else {
			// Calculation of root biomass proportion using the exponential decay model 
			m_rootFraction[i][j] = exp(-layermiddepth / CH4_ROOT_DECAY_COEFF) / CH4_ROOT_NORM_CONST;
			//std::cout << "根生物量比例: " << m_rootFraction[i][j] << std::endl;
		}

		sumrootfrac += m_rootFraction[i][j];
		cumulative_depth += layer_thickness_cm;   // Update cumulative depth
		//std::cout << "单元格 " << i << " 根生物量比例总和: " << sumrootfrac << std::endl;
	}	
}


void CH4_LPJGUESS::InitFracAir(int i) {

	//std::cout << "开始计算土壤空气体积分数..." << std::endl;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// 获取土壤层参数
		float porosity = m_soilPor[i][j];                       // Porosity mm/mm
		float water_content = m_soil_water_storage[i][j];       // Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
		float ice_content = m_soilIceSto[i][j];
		float wilting_point = m_soilWP[i][j];                   // water content of soil at -1.5 MPa (wilting point) (mm)
		float layer_thickness = m_layer_thickness[i][j];        // Thickness of each soil layer (mm) - from VAR_SOILTHICK

		/*std::cout << "  层 " << j << ": 孔隙度=" << porosity
			<< ", 含水量=" << water_content << "mm"
			<< ", 含冰量=" << ice_content << "mm"
			<< ", 萎蔫点=" << wilting_point << "mm"
			<< ", 厚度=" << layer_thickness << "mm" << std::endl;*/

		// 计算空气体积分数: m_Fair = 孔隙度 - (当前土壤含水量 + 萎蔫点含水量 + 当前土壤含冰量) / 土壤层厚度
		float water_volume_fraction = (water_content + wilting_point + ice_content) / layer_thickness;;
		m_Fair[i][j] = porosity - water_volume_fraction;

		// 确保结果在合理范围内 [0, porosity]
		if (m_Fair[i][j] < 0.0) {
			m_Fair[i][j] = 0.0;
			//std::cout << "  警告: 空气体积分数为负，已调整为0" << std::endl;
		}
		else if (m_Fair[i][j] > porosity) {
			m_Fair[i][j] = porosity;
			//std::cout << "  警告: 空气体积分数超过孔隙度，已调整为孔隙度值" << std::endl;
		}
		//std::cout << "  空气体积分数: " << m_Fair[i][j] << std::endl;
	}
	//std::cout << "单元格 " << i << " Frac_air计算完成" << std::endl;
}



void CH4_LPJGUESS::InitAnoxic(int i) {

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// 获取已计算的空气体积分数
		float Frac_air = m_Fair[i][j];
		float porosity = m_soilPor[i][j];

		//std::cout << "  层 " << j << ": 空气体积分数=" << Frac_air
			//<< ", 孔隙度=" << porosity << std::endl;

		// 计算厌氧条件程度: anoxic = 1.0 - Frac_air - Fgas
		float anoxic = 1.0f - Frac_air - Fgas;

		// 确保结果在合理范围内 [0, 1]
		if (anoxic < 0.0f) {
			anoxic = 0.0f;
			//std::cout << "警告: 厌氧程度为负，已调整为0" << std::endl;
		}
		else if (anoxic > 1.0f) {
			anoxic = 1.0f;
			//std::cout << "警告: 厌氧程度超过1，已调整为1" << std::endl;
		}

		// 存储结果
		m_anoxic[i][j] = anoxic;

		//std::cout << "厌氧程度 (anoxic): " << m_anoxic[i][j]
			//<< " (1.0 - " << Frac_air << " - " << Fgas << ")" << std::endl;
	}
}



void CH4_LPJGUESS::Calculate_Gas_Diffusivities(int i) {

    // Calculate and return diffusivities, in units of m2 d-1
    // Called each day
    // See Wania et al. (2010) - Sec 2.5

	//std::cout << "开始计算气体扩散系数..." << std::endl;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// AIR - UNITS: 10 - 4 m2 s - 1 == cm2 s - 1
		// Lerman (1979)

		// 获取当前层的参数
		float layerT = m_Tsoil[i][j];                   // 土壤温度 (°C)
		float frac_air = m_Fair[i][j];                  // 空气体积分数
		float layer_porosity = m_soilPor[i][j];         // 当前层孔隙度

		//std::cout << "  层 " << j << ": 温度=" << layerT << "°C, 空气分数=" << frac_air
			//<< ", 厚度=" << thickness_mm << "mm (" << thickness_m << "m)" << std::endl;

		// Wania et al. (2010), Eqn. 10
		m_D_CH4_air[i][j] = 0.1875f + 0.00130f * layerT;
		m_D_CO2_air[i][j] = 0.1325f + 0.00090f * layerT;
		m_D_O2_air[i][j] = 0.1759f + 0.00117f * layerT;

		//std::cout << "    空气中扩散系数 - CH4: " << m_D_CH4_air[i][j] << ", CO2: " << m_D_CO2_air[i][j] << ", O2: " << m_D_O2_air[i][j] << " cm²/s" << std::endl;

		// WATER - UNITS: 10-4 m2 s-1 == cm2 s-1
		// Wania et al. (2010), Eqn. 9 - where the diffusivities are in units of 10**-9 m2 s-1. 
		// * by scale to get 10-4 m2 s-1
		m_D_CH4_water[i][j] = (0.9798f + 0.02986f * layerT + 0.0004381f * layerT * layerT) * Scale;
		m_D_CO2_water[i][j] = (0.9390f + 0.02671f * layerT + 0.0004095f * layerT * layerT) * Scale;
		m_D_O2_water[i][j] = (1.1720f + 0.03443f * layerT + 0.0005048f * layerT * layerT) * Scale;

		//std::cout << "    水中扩散系数 - CH4: " << m_D_CH4_water[i][j] << ", CO2: " << m_D_CO2_water[i][j] << ", O2: " << m_D_O2_water[i][j] << " cm²/s" << std::endl;

		// 确定实际扩散系数
		if (j < NACROTELM) {
			// ACROTELM diffusivities 
			if (frac_air > 0.05f) {
				// Wania et al. (2010), Eqn. 11
			    // 有足够空气孔隙，使用空气扩散系数并考虑孔隙度
				float airpow = pow(frac_air, 10.0f / 3.0f) / pow(layer_porosity, 2.0f);
				m_dCH4[i][j] = airpow * m_D_CH4_air[i][j];
				m_dCO2[i][j] = airpow * m_D_CO2_air[i][j];
				m_dO2[i][j] = airpow * m_D_O2_air[i][j];
				//std::cout << "    ACROTELM层(有空气) - 孔隙度修正因子: " << airpow << std::endl;
			}
			else {
				// 空气孔隙不足，使用水中扩散系数
				m_dCH4[i][j] = m_D_CH4_water[i][j];
				m_dCO2[i][j] = m_D_CO2_water[i][j];
				m_dO2[i][j] = m_D_O2_water[i][j];
				//std::cout << "    ACROTELM层(饱和) - 使用水中扩散系数" << std::endl;
			}
		}  
		else {      
			// CATOTELM - assumed to be always saturated, so use water diffusivities
			m_dCH4[i][j] = m_D_CH4_water[i][j];
			m_dCO2[i][j] = m_D_CO2_water[i][j];
			m_dO2[i][j] = m_D_O2_water[i][j];
			//std::cout << "    CATOTELM层(饱和) - 使用水中扩散系数" << std::endl;
		}

		// Convert from cm2 s-1 to m2 d-1
		m_dCH4[i][j] *= SECS_PER_DAY / CM2_PER_M2;
		m_dCO2[i][j] *= SECS_PER_DAY / CM2_PER_M2;
		m_dO2[i][j] *= SECS_PER_DAY / CM2_PER_M2;

		// 存储结果
		m_D_CH4[i][j] = m_dCH4[i][j];
		m_D_CO2[i][j] = m_dCO2[i][j];
		m_D_O2[i][j] = m_dO2[i][j];

		//std::cout << "    最终扩散系数(m²/d) - CH4: " << m_dCH4[i][j] << ", CO2: " << m_dCO2[i][j] << ", O2: " << m_dO2[i][j] << std::endl;
	}
}
		


bool CH4_LPJGUESS::Update_Daily_Gas_Parameters(int i) {

	// Update gas transport velocities [m d-1] and equilibrium gas concentrations [mmol m-3]
	// for CH4, CO2 and O2
	// Called daily by Soil::methane()
	// See Wania et al. (2010) - Sec 2.5

	//std::cout << "开始更新每日气体传输参数..." << std::endl;

	float surfT = m_Tsoil[i][0];                  // The surface temperature of each unit

	/* GAS TRANSPORT VELOCITIES */

	// gas transport velocity of SF6.
	// Wania et al. (2010) - Eqn. 6
	float k_600 = 2.07 + 0.215 * pow(U10, 1.7);   // cm h-1

	// Schmidt number of O2
	// Wania et al. (2010) - Eqn. 7
	float ScO2 =1800.6 - 120.1 * surfT + 3.7818 * pow(surfT, 2) - 0.047608 * pow(surfT, 3);
			
	// gas transport velocity of O2 [cm h-1]
	// Wania et al. (2010) - Eqn. 5
	m_k_O2[i] = k_600 * pow(ScO2 / 600.0, n_coeff);
	m_k_O2[i] *= 24.0 / CM_PER_M;              // cm h-1 → m d-1

	// Schmidt number of CH4
	// Wania et al. (2010) - Eqn. 7
	float ScCH4 =1898.0 - 110.1 * surfT + 2.834 * pow(surfT, 2) - 0.02791 * pow(surfT, 3);
		
	// gas transport velocity of CH4 [cm h-1]
	// Wania et al. (2010) - Eqn. 5
	m_k_CH4[i] = k_600 * pow(ScCH4 / 600.0, n_coeff);
	m_k_CH4[i] *= 24.0 / CM_PER_M;

	/*if (i == 629) {
		std::cout << " surfT=" << surfT
			<< " U10=" << U10
			<< " k_600(cm/h)=" << k_600
			<< " k_CH4(m/d)=" << m_k_CH4[i]
			<< std::endl;
	}*/
	

	// Schmidt number of CO2
	// Wania et al. (2010) - Eqn. 7
	float ScCO2 =1911.0 - 113.7 * surfT + 2.967 * pow(surfT, 2) - 0.02943 * pow(surfT, 3);
			
	// gas transport velocity of CO2 [cm h-1]
	// Wania et al. (2010) - Eqn. 5
	m_k_CO2[i] = k_600 * pow(ScCO2 / 600.0, n_coeff);
	m_k_CO2[i] *= 24.0 / CM_PER_M;

	// 打印传输速度
	//std::cout << "Cell " << i
		//<< " - O2传输速度: " << m_k_O2[i] << " m d-1"
		//<< ", CH4传输速度: " << m_k_CH4[i] << " m d-1"
		//<< ", CO2传输速度: " << m_k_CO2[i] << " m d-1"
		//<< ", 表层温度: " << surfT << " °C"
		//<< std::endl;

	/* EQUILIBRIUM GAS CONCENTRATIONS */
	// See Wania et al. (2010) - Eqn. 8

	float deg25 = K2degC + 25.0;                  // 25 degrees C

	// Henry coefficient for O2
	float henry_coeff_O2 =henry_k_O2 * exp(-1.0 * henry_C_O2 * (1.0 / (surfT + K2degC) - 1.0 / deg25));      // [L atm mol-1]
			
	// pp_gas/MM2_PER_M2 converts to atm units
	m_Ceq_O2[i] = pp_O2 / MM2_PER_M2 / henry_coeff_O2;    // mol/L
	m_Ceq_O2[i] *= MM2_PER_M2;                            // mol/L to mmol/m3

	// Henry coefficient for CO2
	float henry_coeff_CO2 =henry_k_CO2 * exp(-1.0 * henry_C_CO2 * (1.0 / (surfT + K2degC) - 1.0 / deg25));    // [L atm mol-1]
			
	// Use this gridcell's CO2 concentration, not a fixed value as in Wan  ia et al. (2010).
	// CO₂ mixing ratio (ppmv), already stored in class variable
	float pp_CO2 = m_co2Conc;      // ppmv → micro atm

	//std::cout << "Cell " << i
		//<< " pp_CO2(micro atm)=" << pp_CO2
		//<< std::endl;

	// Convert Pa → atm using original code style: Pa / MM2_PER_M2 = atm
	m_Ceq_CO2[i] = pp_CO2 / MM2_PER_M2 / henry_coeff_CO2; // mol L-1

	// mol L-1 → mmol m-3
	m_Ceq_CO2[i] *= MM2_PER_M2;

	// Henry coefficient for CH4
	float henry_coeff_CH4 =henry_k_CH4 * exp(-1.0 * henry_C_CH4 * (1.0 / (surfT + K2degC) - 1.0 / deg25));
			
	m_Ceq_CH4[i] = pp_CH4 / MM2_PER_M2 / henry_coeff_CH4;
	m_Ceq_CH4[i] *= MM2_PER_M2;

	// 打印平衡气体浓度
	//std::cout << "Cell " << i
		//<< " - O2平衡浓度: " << m_Ceq_O2[i] << " mmol m-3"
		//<< ", CO2平衡浓度: " << m_Ceq_CO2[i] << " mmol m-3"
		//<< ", CH4平衡浓度: " << m_Ceq_CH4[i] << " mmol m-3"
		//<< ", 表层温度: " << surfT << " °C"
		//<< std::endl;

	return true;
}


float CH4_LPJGUESS::Diffuse_Gas(int i, Gastype gas_type, float& dailyDiff) {

	// Generic gas diffusion method that works with CO2, CH4 and O2 (as specified with gastype) 
    // See Wania et al. (2010) - Sec 2.5 - for a full description

	// Called like this (e.g. for O2): 
	// diffuse_gas(O2, D_O2, O2gas, Ceq_O2, k_O2, Dz_metre, dailyO2diffusion);
	//std::cout << "开始执行每日气体扩散..." << std::endl;

	//float* Cgas = m_Cgas[i];
	float* Cgas = nullptr;
	if (gas_type == CH4_gas) {
		Cgas = m_CH4[i];            // gC layer-1
	}
	else if (gas_type == CO2_gas) {
		Cgas = m_CO2_soil[i];       // gC layer-1
	}
	else { // O2_gas
		Cgas = m_O2[i];             // mol layer-1
	}

	// Atomic mass of the gas [gC/mol]
	float atomic_mass;              
	if (gas_type != O2_gas)
		atomic_mass = atomiccmass;   // i.e. CO2 or CH4 - 12 gC/mol
	else
		atomic_mass = 1.0;           // O2 already in mol layer-1

	float initialAmount = 0.0;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		//initialAmount += m_Cgas[i][j];
		initialAmount += Cgas[j];

		m_Frac_water[i][j] = m_soil_water_storage[i][j] / m_layer_thickness[i][j];
		m_Frac_ice[i][j] = m_soilIceSto[i][j] / m_layer_thickness[i][j];
		m_Frac_water_belowpwp[i][j] = m_soilWP[i][j] / m_layer_thickness[i][j];
		m_Dz_metre[i][j] = m_layer_thickness[i][j] / MM_PER_M;
		m_total_volume_water[i][j] = (m_Frac_water[i][j] + m_Frac_ice[i][j] + m_Frac_water_belowpwp[i][j]) * m_Dz_metre[i][j];
		m_volume_liquid_water[i][j] = (m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) * m_Dz_metre[i][j];

		// CO2 & CH4 - g layer-1 to mmol m-3 
		// O2 - mol layer-1 to mmol m-3
		//m_C[i][j] = m_Cgas[i][j] / atomic_mass / m_total_volume_water[i][j] * MMOL_PER_MOL;
		m_C[i][j] = Cgas[j] / atomic_mass / m_total_volume_water[i][j] * MMOL_PER_MOL;

		//std::cout << "  Soil layer " << j
			//<< "  Cgas=" << m_Cgas[i][j]
			//<< "  volume=" << m_total_volume_water[i][j]
			//<< "  C溶解(mmol/m3)=" << m_C[i][j]
			//<< std::endl;
	}

	// Set the BC, i.e. equilibrium gas concentrations in the top layer depending on atmospheric concentrations
	// and using Henry's law. See Wania et al. (2010), Eqs. 4-8

	// New surface concentration [mmol m-3]
	float C_old = m_C[i][0];     // 原始表层浓度（mmol/m³）
	float Cnew = C_old;          // 新浓度
	float Ceq = 0.0f;            // Henry 平衡浓度（mmol/m³）
	float kgas = 0.0f;           // 气体交换速度（m/d）
	if (gas_type == CH4_gas) {
		Ceq = m_Ceq_CH4[i];     // mmol/m³
		kgas = m_k_CH4[i];      // m/d
	}
	else if (gas_type == CO2_gas) {
		Ceq = m_Ceq_CO2[i];
		kgas = m_k_CO2[i];
	}
	else { // O2_gas
		Ceq = m_Ceq_O2[i];
		kgas = m_k_O2[i];
	}


	// 若是 O2 或 CH4 才需要 Henry 边界条件
	if (gas_type == O2_gas || gas_type == CH4_gas) {     // Could also run for CH4 here below.

		if ((m_Frac_water[i][0] + m_Frac_water_belowpwp[i][0]) < water_min) {    // Could add a snow restriction
		// No diffusion if there's too little liquid water in the top layer
			Cnew = C_old;                  // Unchanged surface concentration
			dailyDiff = 0.0f;              // dailyDiff
		}
		else {

			// Analytical solution to determine Csurf - see Wania et al. Sec 2.5
			Cnew = Ceq + (m_C[i][0] - Ceq) * exp(-kgas / m_volume_liquid_water[i][0]); // mmol m-3
			dailyDiff = (m_C[i][0] - Cnew) * atomic_mass * m_volume_liquid_water[i][0] / MMOL_PER_MOL; // mol layer-1 d-1 (O2)
		}

		m_C[i][0] = Cnew;                  // mmol m-3 - The new surface concentration
	}

	float surf_conc = m_C[i][0];
	int   layer0 = 0;

	// Diffusion of gas today?
	if (dailyDiff != 0) {

		// 选择对应气体的扩散系数数组 D
		float* m_D = nullptr;
		if (gas_type == CH4_gas)
			m_D = m_D_CH4[i];
		else if (gas_type == CO2_gas)
			m_D = m_D_CO2[i];
		else
			m_D = m_D_O2[i];

		// Determine the timestep to use
		float maxD = -0.01f;
		// Maximum diffusivity today below the surface layer [m2 d-1]
		for (int j = 1; j < m_nSoilLyrs[i]; j++) {
			// exclude top layer as we've already considered in the top layer above
			if (m_D[j] > maxD) {
				maxD = m_D[j]; // [m2 d-1]
			}
		}

		float max_timestep = (0.1 * 0.1) / (2 * maxD);       // [day]

		if (max_timestep < Dt_gas) {
			std::cout << "[Warning] Max gas diffusion timestep exceeded: "
				<< maxD << std::endl;
		}

		// Assume a tiny diffusivity if there is very little liquid water in the layer
		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			if ((m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) < water_min) {
				m_D[j] = 1e-9;                               // [m2 d-1]
			}
		}


		// Determine initial and total concentrations
		float totalConc = 0.0f; // mmol m-3

		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			m_C_init[i][j] = m_C[i][j];
			totalConc += m_C_init[i][j];
		}

		float total_diff = 0.0f;
		totalConc = 0.0;
		float ratio = 0.0f;

		int cncount = 0;

		// Loop until stable concentrations are found, or until 100 iterations have been performed
		int gasdiffix = int(1 / Dt_gas);

		bool stable = false;

		do {
			totalConc = 0;
			for (int j = 0; j < m_nSoilLyrs[i]; j++) {
				m_C_init[i][j] = m_C[i][j];
				totalConc += m_C_init[i][j];
			}

			// Use the CN scheme
			Cnstepgas(i, m_C[i], m_D);

			total_diff = 0.0f;
			for (int j = 0; j < m_nSoilLyrs[i]; j++) {
				m_C_last[i][j] = m_C[i][j];
				total_diff += fabs(m_C_last[i][j] - m_C_init[i][j]);
			}

			cncount++;

			ratio = total_diff / totalConc;

			// stable if the ratio is < 0.01 and we've passed a third of the max iterations 
			if (ratio < 0.01 && cncount > int(gasdiffix / 3))
				stable = true;

		} while (cncount < gasdiffix && !stable);     // until max 1% variation between iterations, or 100 loops
	}

	// Unit conversions - allocate concentrations to g layer-1 or mole layer-1
	float finalAmount = 0.0f;
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		//m_Cgas[i][j] = m_C[i][j] * atomic_mass * m_total_volume_water[i][j] / MMOL_PER_MOL;
		//finalAmount += m_Cgas[i][j];
		Cgas[j] = m_C[i][j] * atomic_mass * m_total_volume_water[i][j] / MMOL_PER_MOL;
		finalAmount += Cgas[j];
	}

	// Return the amount of gas that has diffused INTO the soil
	// CO2 & CH4 - g
	// O2 - mol
	float gasIntoSoil = finalAmount - initialAmount;

	if (gas_type == CH4_gas)
		dailyDiff = gasIntoSoil;         // gC/m2/d

	return gasIntoSoil;
}		


float CH4_LPJGUESS::Calculate_tiller_areas() {

	// Calculate the area of tillers
	// See Wania et al. (2010) - Sec 2.6

	float tiller_density = 0.0;           // to return [number m-2]

	float graminoid_leafcmass = 0.0;      // kgC/m2
	float graminoid_dphen = 0.0;
	float graminoid_anpp_red = 0.0;

	// Loop through this patch object's vegetation and pick out graminoids

	return tiller_density;

}


float CH4_LPJGUESS::Plant_Gas_Transport() {


	// Plant transport of O2 or CH4
	// See Wania et al. (2010) - Sec 2.6

	// The limiting factor to plant transport are the number of tillers as
	// they represent the cross-sectional area available to gas transport.
	// The biomass is related to tiller density based on Schimel 1995. The
	// average biomass was 185g/m2 and the average number of tillers was 380/m2.
	// This gives us 2.05 tillers per g biomass and 0.48 g per tiller, which
	// corresponds to 0.22 g C per tiller

	return true;

}


bool CH4_LPJGUESS::Calculate_Gas_Ebullition(int i, float& ebull_today) {

	// Gas ebullition
	// Returns the gas ebullition today
	// See Wania et al. (2010) - Sec 2.7

	float waterheight = 0.0;
	ebull_today = 0.0;

	// Calculate the saturation of each layer
	for (int j = 0; j < m_nSoilLyrs[i]; j++)
	{
		if (m_soil_water_storage[i][j] > 0.0f)
		{
			float ratio =(m_soil_water_storage[i][j] + m_soilWP[i][j]) / (m_layer_thickness[i][j] * m_soilPor[i][j]);

			m_soilSaturation[i][j] = (ratio > 1.0f ? 1.0f : (ratio < 0.0f ? 0.0f : ratio));
		}
		else
		{
			m_soilSaturation[i][j] = 0.0f;
		}
	}

	bool emitToAtmosphere = true;       // Whether to emit CH4 directly to the atmosphere (true by default)
	int bubbleToLayer = -1;

	if (m_handWtrDep[i] <= 0.0f) {    // m

		const float sat_eps = 1e-3f;   // Slightly loosen the "fully saturated" criterion

		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			if (m_soilSaturation[i][j] < 1.0f - sat_eps) {
				bubbleToLayer = j;          // 第一个非饱和层
				break;
			}
		}

		if (bubbleToLayer >= 0) {
			// 找到了非饱和层 → 气泡停在这个土层里，不直接出大气
			emitToAtmosphere = false;
		}
		else {
			// 整个剖面都接近饱和 → 仍然视为冒到大气
			emitToAtmosphere = true;
			bubbleToLayer = -1;
		}
	}
	else {
		// waterDepth > 0：地表有积水 → 冒泡直接出到水面/大气
		emitToAtmosphere = true;
		bubbleToLayer = -1;
	}

	// Ensure ebullition from all layers
	bubbleToLayer = -1;     // The soil layer to bubble to.

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		float Tsoil = m_Tsoil[i][j];
		float TsoilK = Tsoil + K2degC;

		waterheight += m_total_volume_water[i][j];   // m

		// Bubble formation below the WT only
		if (j > bubbleToLayer) {

			// Max CH4 that can be dissolved (Wania et al. (2010), Eqn 15)
			float CH4_diss_max = 0.05708 - 0.001545 * std::fmax(0.0f, Tsoil) + 0.00002069 * std::fmax(0.0f, Tsoil * Tsoil);
			//float tpos = 0.0f;
			//if (Tsoil > 0.0f)   tpos = Tsoil;
			//float CH4_diss_max = 0.05708f - 0.001545f  * tpos + 0.00002069f * (Tsoil * Tsoil);

			// Water pressure
			float hydro_press = rho_H2O * gravity * waterheight;

			// Maximum volume that can be dissolved
			float CH4_diss_max_m3 = CH4_diss_max * m_volume_liquid_water[i][j];     // m3 CH4 layer-1

			// Use ideal gas law to convert to mol CH4 layer-1
			float CH4_diss_max_mol = CH4_diss_max_m3 * (hydro_press + atm_press) / (R_gas * TsoilK);   // mol CH4 layer-1

			// Convert to g CH4-C layer-1
			float CH4_diss_max_g = CH4_diss_max_mol * mr_C;       // g CH4-C layer-1

			// Restrict ebullition to cases when there is enough liquid water and when soil T > 0. 
			if (m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j] > water_min && Tsoil > 0.0) {

				float henry_k_cc_CH4 = TsoilK / (12.2 * henry_k_CH4);
				m_CH4_diss[i][j] = std::fmin(CH4_diss_max_g, henry_k_cc_CH4 *  m_CH4[i][j]);
				//std::cout << "henry_k_cc_CH4=" << henry_k_cc_CH4 << "\n";

				//// Override (a la Wania et al. 2010) with this new, simpler ebullition
				//m_CH4_diss[i][j] = min(CH4_diss_max_g, m_CH4[i][j]);

				m_CH4_gas[i][j] = m_CH4[i][j] - m_CH4_diss[i][j];              // g CH4-C layer-1 - gas
				m_CH4_gas_vol[i][j] = m_CH4_gas[i][j] / atomiccmass * R_gas * TsoilK / (hydro_press + atm_press);      // [m3]
				m_CH4_vgc[i][j] = m_CH4_gas_vol[i][j] / (m_layer_thickness[i][j] / MM_PER_M * SQ_M);            // m3/m3

				/*if (i == 629) {
					float thr = vgc_high * bubble_CH4_frac;

					std::cout << std::fixed << std::setprecision(6)
						<< "[Ebull] cell=" << i << " lyr=" << j
						<< " T=" << Tsoil
						<< " waterheight=" << waterheight
						<< " Ptot=" << (hydro_press + atm_press)
						<< " CH4_total=" << m_CH4[i][j]
						<< " CH4_diss_max_g=" << CH4_diss_max_g
						<< " henry_k_cc_CH4=" << henry_k_cc_CH4
						<< " henry_cap=" << (henry_k_cc_CH4 * m_CH4[i][j])
						<< " CH4_diss=" << m_CH4_diss[i][j]
						<< " CH4_gas=" << m_CH4_gas[i][j]
						<< " Vgas=" << m_CH4_gas_vol[i][j]
						<< " VGC=" << m_CH4_vgc[i][j]
						<< " thr=" << thr
						<< " trig=" << (m_CH4_vgc[i][j] > thr ? 1 : 0)
						<< std::endl;                                                                                                                                                                                                                                                                                                 
				}*/

				// Ebullition/bubble formation if the volumetric gas content (VGC) exceeds vgc_high * bubble_CH4_frac, 
				// where bubble_CH4_frac id the CH4 fraction of gas bubbles
				if (m_CH4_vgc[i][j] > vgc_high * bubble_CH4_frac) {

					m_CH4_ebull_vol[i][j] = (m_CH4_vgc[i][j] - vgc_low * bubble_CH4_frac) * m_layer_thickness[i][j] / MM_PER_M * SQ_M; // m3/m3
					m_CH4_vgc[i][j] = vgc_low * bubble_CH4_frac;
					m_CH4_ebull_ind[i][j] = m_CH4_ebull_vol[i][j] * (hydro_press + atm_press) / (R_gas * TsoilK); // mol
					m_CH4_gas[i][j] = m_CH4_vgc[i][j] * m_layer_thickness[i][j] / MM_PER_M * SQ_M * (hydro_press + atm_press) / (R_gas * TsoilK); // mol
					m_CH4_ebull_ind[i][j] *= atomiccmass;     // mol to g
					m_CH4_gas[i][j] *= atomiccmass;           // mol to g 

					// Update the amount of CH4 to emit today, from this layer
					ebull_today += m_CH4_ebull_ind[i][j]; // g m-2 d-1
				}

				// Now recalculate the total CH4 amount in this layer, both dissolved and gaseous
				m_CH4[i][j] = m_CH4_diss[i][j] + m_CH4_gas[i][j];
			}
			else {

				// No change in the dissolved CH4 amount
				m_CH4_diss[i][j] = m_CH4_diss_yesterday[i][j];
			} // Frac_water
		}
		else {

			// No change in the dissolved CH4 amount
			m_CH4_diss[i][j] = m_CH4_diss_yesterday[i][j];
		} // bubble layer check
	} // for ii


	// If the water table is below the surface, then put the bubbled methane into the first unsaturated layer
	if (!emitToAtmosphere) {

		m_CH4[i][bubbleToLayer] += ebull_today;
		ebull_today = 0.0;                    // No emission today.

		// O2 is in [mol layer-1]
		m_O2[i][bubbleToLayer] *= oxid_frac;       // since 25% of O2 used by roots themselves...  
		m_CH4[i][bubbleToLayer] /= atomiccmass;    // mol layer-1
		m_CH4_oxid[i][bubbleToLayer] = std::fmin(m_CH4[i][bubbleToLayer], 0.5f * m_O2[i][bubbleToLayer]); // usually 75%
		m_CH4[i][bubbleToLayer] = (m_CH4[i][bubbleToLayer] - m_CH4_oxid[i][bubbleToLayer]) * atomiccmass; // gC layer again	
		m_O2[i][bubbleToLayer] -= 2.0 * m_CH4_oxid[i][bubbleToLayer]; // subtract the moles used in oxidation

		m_CO2_soil[i][bubbleToLayer] += m_CH4_oxid[i][bubbleToLayer] * atomiccmass; // Oxidised CH4 becomes CO2 (and water!)
	}

	return true;
}

float CH4_LPJGUESS::Get_CH4_Content() {
	// Return dissolved CH4 content. Units: g CH4-C / m2
	return m_CH4_store;
}

float CH4_LPJGUESS::Get_CO2_Content() {
	// Return dissolved CO2 content. Units: g C / m2
	return m_CO2_store;
}


void CH4_LPJGUESS::Calculate_Carbon_Store(int i, int dy, bool today) {

	// Carbon accounting routine - updates co2_store and ch4_store 
	// See Wania et al. (2010)
	int day = 0;

	// Reset the stores
	m_CO2_store = 0.0;
	m_CH4_store = 0.0;

	if (today)
		day = dy;
	else
		day = dy - 1;

	if (dy > 0 || today) {
		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			m_CO2_store += m_CO2_soil[i][j];
			m_CH4_store += m_CH4[i][j];
		}
	}
	else {
		// Jan 1 or yesterday
		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			m_CO2_store += m_CO2_soil_yesterday[i][j];
			m_CH4_store += m_CH4_yesterday[i][j];
		}
	}
}

bool CH4_LPJGUESS::Methane(int i, int daynum) {

	// Main methane routine, called daily. 
	// Calculates daily methane fluxes and updates methane concentrations in peatland soil layers.
	// Algorithm etc. from Wania et al. 2010
	// This version coded by Paul Miller, based on Rita Wania's F90 code

	// Max allowed error in checks
	const float MAX_ERR = 0.000001;
	const float MAX_ERR_BALANCE = 0.0001;
	const float LARGE_ERR = 0.01;

	// Variables for debugging
	// bool allow_planttransport = true;
	bool allow_Ebullition = true;
	bool allow_CH4diffusion = true;
	bool allow_O2diffusion = true;

	// Daily diffusion 
	float O2_diff_today;  
	float CO2_diff_today;
	float CH4_diff_today;

	// Total daily ebullition [g CH4-C d-1]
	float CH4_ebull_today;

	// Budget/conservation variables [gC/m2]
	float CH4_C_in = 0.0;
	float CO2_C_in = 0.0;

	float m_CH4_C_store_init = 0.0;
	float m_CO2_C_store_init = 0.0;

	float m_CH4_C_store_now = 0.0;
	float m_CO2_C_store_now = 0.0;

	// Initialise components of total_C_flux, CO2_flux and CH4_flux
	O2_diff_today = 0.0;
	CO2_diff_today = 0.0;
	CH4_diff_today = 0.0;
	CH4_ebull_today = 0.0;

	//CH4_plant_today = 0.0;
	//CO2_plant_today = 0.0;
	//O2_plant_today = 0.0;

	// Initialise the peatland root fractions.
	if (daynum == 0)
		InitRootFractions(i);



	// *** STEP 0 ***

	// C budget before today's methane calculations

	Calculate_Carbon_Store(i, daynum, false);
	m_CH4_C_store_init = m_CH4_store;
	m_CO2_C_store_init = m_CO2_store;

	/*if (i == 629) {
		float sum_y = 0.f;
		std::cout << "\n=== Day " << daynum << " Cell 629: CH4_yesterday per layer (gC/layer) ===\n";
		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			std::cout << "  lyr " << j << " CH4_yesterday=" << m_CH4_yesterday[i][j]
				<< " CH4_diss_yesterday=" << m_CH4_diss_yesterday[i][j]
				<< " CH4_gas_yesterday=" << m_CH4_gas_yesterday[i][j]
				<< "\n";
			sum_y += m_CH4_yesterday[i][j];
		}
		std::cout << "  sum CH4_yesterday=" << sum_y << "\n";
	}*/



	// *** STEP 1 ***
	// Diffusivities and layer depths, and update of gas constants

	// Set layer gas diffusivities
	Calculate_Gas_Diffusivities(i);
	// Units: m2 d-1

	// Layer depths in m, and volume of water in m3

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		m_Frac_water[i][j] = m_soil_water_storage[i][j] / m_layer_thickness[i][j];
		m_Frac_ice[i][j] = m_soilIceSto[i][j] / m_layer_thickness[i][j];
		m_Frac_water_belowpwp[i][j] = m_soilWP[i][j] / m_layer_thickness[i][j];
		m_Dz_metre[i][j] = m_layer_thickness[i][j] / MM_PER_M;
		m_volume_liquid_water[i][j] = (m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) * m_Dz_metre[i][j];                       // m3 water 
		m_total_volume_water[i][j] = (m_Frac_water[i][j] + m_Frac_ice[i][j] + m_Frac_water_belowpwp[i][j]) * m_Dz_metre[i][j];     // m3 water + ice
	}

	// If we have standing water...
	if (m_handWtrDep[i] > 0.0) {
		// m_handWtrDep in m
		m_Dz_metre[i][0] += m_handWtrDep[i];
		m_volume_liquid_water[i][0] += (m_Frac_water[i][0] + m_Frac_water_belowpwp[i][0]) * m_handWtrDep[i];
		m_total_volume_water[i][0] += (m_Frac_water[i][0] + m_Frac_ice[i][0] + m_Frac_water_belowpwp[i][0]) * m_handWtrDep[i];
	}

	// Update the temperature-dependent gas parameters
	bool gasParamsOK = Update_Daily_Gas_Parameters(i);
	// Units: 
	// k_O2 etc: [m d-1]
	// Ceq_O2: [mmol m-3]

	if (!gasParamsOK)
		return false;



	// *** STEP 2 ***
	// Calculate CH4 & CO2 production

	float CH4_daily_total = 0.0f;
	
	float total_Rh = 0.0f;             // The total soil respiration (Rh) generated today, gC/m2/day.
	
	// Today's decomposition of litter and soil C pools
	float C_input = 0.0;               // Conservation check - C in methane produced  记录今天产生的所有 C（CH₄ + CO₂）
	float CH4_Init_Prod = 0.0;         // The total CH₄ originally generated on that day (before oxidation, diffusion, and bubbling)
	float Total_CH4 = 0.0;             // Calculate the total amount of CH₄ in all layers

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// Daily decomposition, converted to gC/m2 from kgC/ha
		float m_dRh = m_sol_RSPC[i][j] * KG_HA_TO_G_M2;
		total_Rh += m_dRh;

		//// 打印每一层土壤呼吸（gC/m2/day）
		//std::cout << "Cell " << i
		//	<< " Layer " << j
		//	<< " Soil respiration (m_dRh) = "
		//	<< m_dRh << " gC/m2/day"
		//	<< std::endl;

		float anoxic = m_anoxic[i][j];
		float rootfrac = m_rootFraction[i][j];
		float C_input_old = C_input;

		// *** CH4 PRODUCTION IN THIS LAYER ***
		if ((m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) < water_min) {
			m_CH4_prod[i][j] = 0.0;
		}
		else {  
			//m_CH4_prod[i][j] = anoxic * CH4toCO2_PEAT * rootfrac * m_dRh;     // gC/m2
			//It is already the respiratory volume for each layer, so there is no need to redistribute the root system proportions at each layer.
			m_CH4_prod[i][j] = anoxic * CH4toCO2_PEAT * m_dRh;       // gC/m2
		}

//		// ===== DEBUG PRINT (cells 629-644): what controls CH4 production =====
//		if (i >= 629 && i <= 644) {
//			const float FracLiq = m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j];
//			const bool  passWater = (FracLiq >= water_min);
//			const float ch4_formula = passWater ? (anoxic * CH4toCO2_PEAT * m_dRh) : 0.0f;
//
//#pragma omp critical(CH4DBG_PRINT)
//			{
//				std::cout << std::fixed << std::setprecision(6)
//					<< "[CH4CTRL] day=" << daynum
//					<< " cell=" << i
//					<< " lyr=" << j
//					<< " | sol_RSPC=" << m_sol_RSPC[i][j]                 // kgC/ha/day
//					<< " m_dRh=" << m_dRh                                // gC/m2/day
//					<< " | anoxic=" << anoxic
//					<< " Fair=" << m_Fair[i][j]
//					<< " por=" << m_soilPor[i][j]
//					<< " | FracLiq=" << FracLiq
//					<< " (water=" << m_Frac_water[i][j]
//					<< " WP=" << m_Frac_water_belowpwp[i][j]
//					<< ") water_min=" << water_min
//					<< " pass=" << (passWater ? 1 : 0)
//					<< " | T=" << m_Tsoil[i][j]
//					<< " | water_mm=" << m_soil_water_storage[i][j]
//					<< " WP_mm=" << m_soilWP[i][j]
//					<< " ice_mm=" << m_soilIceSto[i][j]
//					<< " thk_mm=" << m_layer_thickness[i][j]
//					<< " | CH4toCO2=" << CH4toCO2_PEAT
//					<< " | CH4_prod=" << m_CH4_prod[i][j]
//					<< " (calc=" << ch4_formula << ")"
//					<< "\n";
//			}
//		}

		// Cumulative daily CH4
		CH4_daily_total += m_CH4_prod[i][j];

		C_input += m_CH4_prod[i][j];
		CH4_Init_Prod += m_CH4_prod[i][j];

		// Add CH4 production to CH4 pool
		m_CH4[i][j] = m_CH4_yesterday[i][j] + m_CH4_prod[i][j];
		Total_CH4 += m_CH4[i][j];

		// *** CO2 PRODUCTION ***
		//m_CO2_soil_prod[i][j] = rootfrac * m_dRh - m_CH4_prod[i][j];
		//It is already the respiratory volume for each layer, so there is no need to redistribute the root system proportions at each layer.
		m_CO2_soil_prod[i][j] = m_dRh - m_CH4_prod[i][j];
		C_input += m_CO2_soil_prod[i][j];

		// CO2 pool
		// Add CO2 production to CO2 pool 
		m_CO2_soil[i][j] = m_CO2_soil_yesterday[i][j] + m_CO2_soil_prod[i][j];

		float C_Added = C_input - C_input_old;            // The actual current carbon input (CH₄ + CO₂) added to the soil layer
		float Ought_to_have_been = rootfrac * m_dRh;      // Theoretically, the total amount of carbon that this layer should have produced today

		// Error (used to detect whether the model has correctly allocated carbon)
		float Er = Ought_to_have_been - C_Added;          // Theoretical production quantity - The actual quantity added to the system,
	}

	//// C conservation test:
	//if (fabs(total_Rh - C_input) > MAX_ERR) {
	//	std::cout << "[CH4] C conservation error: |dRh - C_input| = "
	//		<< fabs(total_Rh - C_input)
	//		<< "  at Cell = " << i
	//		<< std::endl;
	//	//return false;
	//}

	//// debugging:
	//if (Total_CH4 < 0.0000000 || Total_CH4 > 10000000) {
	//	std::cout << "Bad Total_CH4 at the start of Soil::methane()" << i
	//		<< ": Total_CH4 = " << Total_CH4
	//		<< std::endl;
	//}

	// C conservation test:
	Calculate_Carbon_Store(i, daynum, true);
	m_CH4_C_store_now = m_CH4_store;
	m_CO2_C_store_now = m_CO2_store;
	float check = m_CH4_C_store_now + m_CO2_C_store_now - m_CH4_C_store_init - m_CO2_C_store_init - total_Rh;
	/*if (fabs(check) > MAX_ERR) {
		std::cout << "[CH4] Carbon imbalance detected!"
			<< "  Cell=" << i
			<< "  CH4_store_now=" << m_CH4_C_store_now
			<< "  CO2_store_now=" << m_CO2_C_store_now
			<< "  CH4_store_init=" << m_CH4_C_store_init
			<< "  CO2_store_init=" << m_CO2_C_store_init
			<< "  total_Rh=" << total_Rh
			<< "  check=" << check
			<< std::endl;
	}*/



	// *** STEP 3 ***
	// Diffusion of O2
	float DailyO2diffusion = 0.0;     // 当日扩散总量
	float MolesO2IntoSoil = 0.0;      // 进入土壤顶层的 O₂ 摩尔数

	if (allow_O2diffusion)
		MolesO2IntoSoil = Diffuse_Gas(i, O2_gas, DailyO2diffusion);
	O2_diff_today = DailyO2diffusion;                   // mol O2 into the soil (and then diffused downwards) 
	// Should be negative, i.e. O2 diffuses INTO the soil



	//// *** STEP 4 ***
	//// Plant transport of oxygen

	//// Tiller set-up
	//// Loop through the individuals present and sum the leaf carbon mass for C3-graminoids 
	//float tillers = calculate_tiller_areas(rootfrac, tiller_area); // m-2

	//// O2 plant transport
	//float plantTransportToday_O2 = 0.0;

	//if (allow_planttransport && tillers > 0) // Only allow plant transport if there are graminoid tillers present
	//	plant_gas_transport(O2, Ceq_O2, k_O2, O2gas, plantTransportToday_O2);

	//O2_plant_today = plantTransportToday_O2; // mol O2 into soil through plants



	// *** STEP 5 ***
	// Diffusion of CH4
	float CH4_before_diffusion = m_CH4_store;   // The content of CH₄ before diffusion
	float DailyCH4diffusion = 0.0;
	float GramCH4IntoSoil = 0.0;

	if (allow_CH4diffusion)
		GramCH4IntoSoil = Diffuse_Gas(i, CH4_gas, DailyCH4diffusion);
	CH4_diff_today = -DailyCH4diffusion;   // SHOULD BE 0 - gC m-2 d-1
	// Should be positive, i.e. upward flux
	

	if (fabs(CH4_diff_today) < MAX_ERR)
		CH4_diff_today = 0.0;              // remove tiny values

	//// C conservation test:
	//if (CH4_diff_today < -0.1 || CH4_diff_today > 10000000) {
	//	std::cout << "Bad CH4 diffusion detected: "
	//		<< CH4_diff_today
	//		<< " (cell=" << i << ")"
	//		<< std::endl;
	//}

	// C conserved?
	Calculate_Carbon_Store(i, daynum, true);
	check = m_CH4_store - CH4_before_diffusion + CH4_diff_today;
	/*if (fabs(check) > MAX_ERR) {
		std::cout << "[C Conservation Error] After CH4 diffusion: "
			<< check
			<< "  (cell=" << i << ")"
			<< std::endl;
	}*/



	// *** STEP 6 ***
	// CH4 oxidation

	// CH4 units: gC layer-1 
	// O2 units: mol layer-1

	// Assume that 1/2 of the O2 is utilized by other electron
	// acceptors and you need 2 moles of O2 to oxidise 1 mole of CH4.
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// O2 is in [mol layer-1]
		m_O2[i][j] *= oxid_frac;             // since ((1-oxid_frac)*100)% of O2 used by roots themselves... [mol layer-1] 
		m_CH4[i][j] /= atomiccmass;          // mol layer-1
		m_CH4_oxid[i][j] = std::fmin(m_CH4[i][j], 0.5f * m_O2[i][j]);                // usually 75%
		m_CH4[i][j] = (m_CH4[i][j] - m_CH4_oxid[i][j]) * atomiccmass;         // gC layer again	
		m_O2[i][j] -= 2.0 * m_CH4_oxid[i][j];                                 // subtract the moles used in oxidation

		m_CO2_soil[i][j] += m_CH4_oxid[i][j] * atomiccmass;                   // Oxidised CH4 becomes CO2 (and water!)
	}

	// Daily CH4 oxidation (sum over layers)
    // Units: gC m-2 day-1
	float CH4_oxid_daily = 0.0f;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		CH4_oxid_daily += m_CH4_oxid[i][j] * atomiccmass;  // mol → gC
	}



	// *** STEP 6b ***
	// CH4 diffusion from top layer AFTER diffusion and oxidation
	// Now done after CN diffusion

	float CH4_diff_before_BC = CH4_diff_today;

	// Check C budget
	float CH4_before_oxid_and_diffusion = m_CH4_store;
	float CO2_before_oxid_and_diffusion = m_CO2_store;

	float checkbefore = CH4_before_oxid_and_diffusion + CO2_before_oxid_and_diffusion + CH4_diff_before_BC;

	Calculate_Carbon_Store(i, daynum, true);
	check = m_CH4_store + m_CO2_store + CH4_diff_today;

	float checkafter1 = check - checkbefore;

	/*if (fabs(checkafter1) > MAX_ERR) {
		std::cout << "[CH4] Carbon imbalance after oxidation and CH4 diffusion: "
			<< checkafter1
			<< "  (Cell " << i << ")"
			<< std::endl;
	}*/

	checkbefore = check;



	// *** STEP 7 ***
	// Plant transport of CH4

	//double plantTransportToday_CH4 = 0.0;

	//if (allow_planttransport && tillers > 0) // Only allow plant transport of there are gramoinoid tillers present
	//	plant_gas_transport(CH4, Ceq_CH4, k_CH4, CH4gas, plantTransportToday_CH4);

	//CH4_plant_today = plantTransportToday_CH4; // CH4 into soil through plants (gC m-2)

	//// Check C budget
	//Calculate_Carbon_Ctore(i, daynum, true);
	//check = ch4_store + co2_store + CH4_diff_today + CH4_plant_today;
	//double checkafter2 = check - checkbefore;
	//if (fabs(checkafter2) > MAX_ERR && verbosity >= WARNING)
	//	dprintf("%s%g\n", "Soil::methane() - after plant transport of CH4: ", checkafter2);
	//checkbefore = check;



	// *** STEP 8 ***
	// Ebullition of CH4
	if (allow_Ebullition)
		Calculate_Gas_Ebullition(i, CH4_ebull_today);
	// reduces CH4 and updates CH4_ebull_today

	// Check C budget
	Calculate_Carbon_Store(i, daynum, true);
	//check = m_CH4_store + m_CO2_store + CH4_diff_today + CH4_plant_today + CH4_ebull_today;
	check = m_CH4_store + m_CO2_store + CH4_diff_today  + CH4_ebull_today;     // Ignoring the transmission by plants

	float checkafter3 = check - checkbefore;

	/*if (fabs(checkafter3) > MAX_ERR) {
		std::cout << "[CH4] Carbon imbalance after ebullition: "
			<< checkafter3
			<< "  (Cell " << i << ")"
			<< std::endl;
	}

	if (i == 629) {
		std::cout << "CH4 budget: prod=" << CH4_daily_total
			<< " oxid=" << CH4_oxid_daily
			<< " diff=" << CH4_diff_today
			<< " ebull=" << CH4_ebull_today
			<< " store=" << m_CH4_store
			<< "\n";
	}*/


	// STEPS 9 & 10
    // Instead of calling diffusion and plant transport for CO2, I simply diffuse ALL the CO2. 
	// This should improve C conservation
	// In future one could look at how much IS emitted, especially as CO2 could be saved and emitted in 
	// bursts during spring thaw

	CO2_diff_today = m_CO2_store;

    // Correct & record Dec 31 data
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		m_CO2_soil[i][j] = 0.0;

	}

	// Recalculate ch4_store and co2_store now, after CO2 diffusion
	Calculate_Carbon_Store(i, daynum, true);

	//CO2_plant_today = 0.0;

	// *** STEP 9 ***
	// Conservation steps

	// Total C flux out of the soil today [gC m-2 d-1]
	//float total_C_flux = CO2_diff_today + CH4_diff_today + CH4_ebull_today + CH4_plant_today + CO2_plant_today;
	float total_C_flux = CO2_diff_today + CH4_diff_today + CH4_ebull_today;       // Ignoring the transmission by plants

	// CO2_flux_today = CO2_diff_today + CO2_plant_today;
	// Calculate daily CH4 flux [gCH4-C m-2 d-1]
	//float CH4_flux_today = CH4_diff_today + CH4_plant_today + CH4_ebull_today;     
	float CH4_flux_today = CH4_diff_today + CH4_ebull_today;         // Ignoring the transmission by plants

	// Reduce heterotrophic respiration by this CH4-C amount

	// Total C content of soil
	float C_soil = 0.0;
	float CO2_soil = 0.0;

	Calculate_Carbon_Store(i, daynum, true);
	C_soil += m_CH4_store + m_CO2_store;

	/*if (total_C_flux < -LARGE_ERR) {
		std::cout << "[CH4 Warning] Large negative C flux in CH4_LPJGUESS::Methane() "
			<< " at Cell = " << i
			<< "  total_C_flux = " << total_C_flux
			<< std::endl;
	}*/

	float final_C_budget = (m_CH4_C_store_init + m_CO2_C_store_init + C_input) - (C_soil + total_C_flux);
	/*if (fabs(final_C_budget) > MAX_ERR_BALANCE) {
		std::cout << " Unbalanced C budget in CH4_LPJGUESS::Methane() "
			<< " at Cell = " << i
			<< "  total_C_flux = " << total_C_flux
			<< std::endl;
	}*/
	

	// Record today's values
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		m_CO2_soil_yesterday[i][j] = m_CO2_soil[i][j];
		m_CH4_yesterday[i][j] = m_CH4[i][j];
		m_CH4_diss_yesterday[i][j] = m_CH4_diss[i][j];
		m_CH4_gas_yesterday[i][j] = m_CH4_gas[i][j];
	}  // generatemethane?


	/*if (i == 629) {
#pragma omp critical
		{
			float sum_y2 = 0.f;
			std::cout << "\n[Record END] day=" << daynum << " cell=629\n";
			for (int j = 0; j < m_nSoilLyrs[i]; j++) {
				std::cout << "  lyr " << j
					<< " CH4(today)=" << m_CH4[i][j]
					<< " -> CH4_yesterday=" << m_CH4_yesterday[i][j]
					<< " diss_y=" << m_CH4_diss_yesterday[i][j]
					<< " gas_y=" << m_CH4_gas_yesterday[i][j]
					<< "\n";
				sum_y2 += m_CH4_yesterday[i][j];
			}
			std::cout << "  sum CH4_yesterday(after record)=" << sum_y2 << "\n";
		}
	}*/



	// Hourly methane fluxes (carbon-based)    gC/m2/day  →  gC/m2/h
	float CH4_hourly = CH4_daily_total / 24.0f;         // Daily methane production
	float CH4_oxid_hourly = CH4_oxid_daily / 24.0f;     // Daily methane oxidation  -> hourly
	float CH4_diff_hourly = CH4_diff_today / 24.0f;     // Diffusive flux
	float CH4_ebull_hourly = CH4_ebull_today / 24.0f;   // Ebullition flux
	float CH4_flux_hourly = CH4_flux_today / 24.0f;     // Total CH4 flux

	// (Optional) Hourly CO2 diffusion (also carbon-based)
	float CO2_diff_hourly = CO2_diff_today / 24.0f;

	/*std::cout << "Cell " << i
		<< " Daily CH4 Production = " << CH4_daily_total << " gC/m2/day"
		<< " (" << CH4_hourly << " gC/m2/h)"
		<< " | CH4 oxidation = " << CH4_oxid_hourly << " gC/m2/h"
		<< " | CH4 diffusion = " << CH4_diff_hourly << " gC/m2/h"
		<< " | CH4 ebullition = " << CH4_ebull_hourly << " gC/m2/h"
		<< " | CH4 total flux = " << CH4_flux_hourly << " gC/m2/h"
		<< std::endl;*/

	m_CH4_prod_hour[i] = CH4_daily_total / 24.0f;      // Daily methane production  gC/m2/hour
	m_CH4_oxid_hour[i] = CH4_oxid_daily / 24.0f;       // Daily methane oxidation   gC/m2/hour
	m_CH4_diff_hour[i] = CH4_diff_today / 24.0f;       // Diffusive flux            gC/m2/hour
	m_CH4_ebull_hour[i] = CH4_ebull_today / 24.0f;     // Ebullition flux           gC/m2/hour
	m_CH4_flux_hour[i] = CH4_flux_today / 24.0f;       // Total CH4 flux            gC/m2/hour

	//// ======= 单条 std::cout 输出（只在 cell=629）=======
	//if (i == 629)
	//	std::cout << std::fixed << std::setprecision(6)
	//	<< "[CH4DBG] day=" << daynum
 //   << " handWtrDep=" << m_handWtrDep[i]
 //   << " Rh=" << total_Rh
 //   << " CH4prod_tot=" << CH4_daily_total
 //   << " CH4diff=" << CH4_diff_today
 //   << " CH4oxid_tot=" << CH4_oxid_daily
 //   << " CH4ebull=" << CH4_ebull_today
 //   << " CH4store_end=" << m_CH4_store
 //   << " thr=" << (vgc_high * bubble_CH4_frac)

 //   << " | L0:prod=" << (m_nSoilLyrs[i] > 0 ? m_CH4_prod[i][0] : -999.f)
 //   << " oxid=" << (m_nSoilLyrs[i] > 0 ? m_CH4_oxid[i][0] * atomiccmass : -999.f)
 //   << " CH4=" << (m_nSoilLyrs[i] > 0 ? m_CH4[i][0] : -999.f)
 //   << " diss=" << (m_nSoilLyrs[i] > 0 ? m_CH4_diss[i][0] : -999.f)
 //   << " gas=" << (m_nSoilLyrs[i] > 0 ? m_CH4_gas[i][0] : -999.f)
 //   << " VGC=" << (m_nSoilLyrs[i] > 0 ? m_CH4_vgc[i][0] : -999.f)
 //   << " anoxic=" << (m_nSoilLyrs[i] > 0 ? m_anoxic[i][0] : -999.f)
 //   << " FracLiq=" << (m_nSoilLyrs[i] > 0 ? (m_Frac_water[i][0] + m_Frac_water_belowpwp[i][0]) : -999.f)

 //   << " | L1:prod=" << (m_nSoilLyrs[i] > 1 ? m_CH4_prod[i][1] : -999.f)
 //   << " oxid=" << (m_nSoilLyrs[i] > 1 ? m_CH4_oxid[i][1] * atomiccmass : -999.f)
 //   << " CH4=" << (m_nSoilLyrs[i] > 1 ? m_CH4[i][1] : -999.f)
 //   << " diss=" << (m_nSoilLyrs[i] > 1 ? m_CH4_diss[i][1] : -999.f)
 //   << " gas=" << (m_nSoilLyrs[i] > 1 ? m_CH4_gas[i][1] : -999.f)
 //   << " VGC=" << (m_nSoilLyrs[i] > 1 ? m_CH4_vgc[i][1] : -999.f)
 //   << " anoxic=" << (m_nSoilLyrs[i] > 1 ? m_anoxic[i][1] : -999.f)
 //   << " FracLiq=" << (m_nSoilLyrs[i] > 1 ? (m_Frac_water[i][1] + m_Frac_water_belowpwp[i][1]) : -999.f)

 //   << " | L2:prod=" << (m_nSoilLyrs[i] > 2 ? m_CH4_prod[i][2] : -999.f)
 //   << " oxid=" << (m_nSoilLyrs[i] > 2 ? m_CH4_oxid[i][2] * atomiccmass : -999.f)
 //   << " CH4=" << (m_nSoilLyrs[i] > 2 ? m_CH4[i][2] : -999.f)
 //   << " diss=" << (m_nSoilLyrs[i] > 2 ? m_CH4_diss[i][2] : -999.f)
 //   << " gas=" << (m_nSoilLyrs[i] > 2 ? m_CH4_gas[i][2] : -999.f)
 //   << " VGC=" << (m_nSoilLyrs[i] > 2 ? m_CH4_vgc[i][2] : -999.f)
 //   << " anoxic=" << (m_nSoilLyrs[i] > 2 ? m_anoxic[i][2] : -999.f)
 //   << " FracLiq=" << (m_nSoilLyrs[i] > 2 ? (m_Frac_water[i][2] + m_Frac_water_belowpwp[i][2]) : -999.f)

 //   << " | L3:prod=" << (m_nSoilLyrs[i] > 3 ? m_CH4_prod[i][3] : -999.f)
 //   << " oxid=" << (m_nSoilLyrs[i] > 3 ? m_CH4_oxid[i][3] * atomiccmass : -999.f)
 //   << " CH4=" << (m_nSoilLyrs[i] > 3 ? m_CH4[i][3] : -999.f)
 //   << " diss=" << (m_nSoilLyrs[i] > 3 ? m_CH4_diss[i][3] : -999.f)
 //   << " gas=" << (m_nSoilLyrs[i] > 3 ? m_CH4_gas[i][3] : -999.f)
 //   << " VGC=" << (m_nSoilLyrs[i] > 3 ? m_CH4_vgc[i][3] : -999.f)
 //   << " anoxic=" << (m_nSoilLyrs[i] > 3 ? m_anoxic[i][3] : -999.f)
 //   << " FracLiq=" << (m_nSoilLyrs[i] > 3 ? (m_Frac_water[i][3] + m_Frac_water_belowpwp[i][3]) : -999.f)

 //   << " | L4:prod=" << (m_nSoilLyrs[i] > 4 ? m_CH4_prod[i][4] : -999.f)
 //   << " oxid=" << (m_nSoilLyrs[i] > 4 ? m_CH4_oxid[i][4] * atomiccmass : -999.f)
 //   << " CH4=" << (m_nSoilLyrs[i] > 4 ? m_CH4[i][4] : -999.f)
 //   << " diss=" << (m_nSoilLyrs[i] > 4 ? m_CH4_diss[i][4] : -999.f)
 //   << " gas=" << (m_nSoilLyrs[i] > 4 ? m_CH4_gas[i][4] : -999.f)
 //   << " VGC=" << (m_nSoilLyrs[i] > 4 ? m_CH4_vgc[i][4] : -999.f)
 //   << " anoxic=" << (m_nSoilLyrs[i] > 4 ? m_anoxic[i][4] : -999.f)
 //   << " FracLiq=" << (m_nSoilLyrs[i] > 4 ? (m_Frac_water[i][4] + m_Frac_water_belowpwp[i][4]) : -999.f)

 //   << " | L5:prod=" << (m_nSoilLyrs[i] > 5 ? m_CH4_prod[i][5] : -999.f)
 //   << " oxid=" << (m_nSoilLyrs[i] > 5 ? m_CH4_oxid[i][5] * atomiccmass : -999.f)
 //   << " CH4=" << (m_nSoilLyrs[i] > 5 ? m_CH4[i][5] : -999.f)
 //   << " diss=" << (m_nSoilLyrs[i] > 5 ? m_CH4_diss[i][5] : -999.f)
 //   << " gas=" << (m_nSoilLyrs[i] > 5 ? m_CH4_gas[i][5] : -999.f)
 //   << " VGC=" << (m_nSoilLyrs[i] > 5 ? m_CH4_vgc[i][5] : -999.f)
 //   << " anoxic=" << (m_nSoilLyrs[i] > 5 ? m_anoxic[i][5] : -999.f)
 //   << " FracLiq=" << (m_nSoilLyrs[i] > 5 ? (m_Frac_water[i][5] + m_Frac_water_belowpwp[i][5]) : -999.f)

 //   << " | L6:prod=" << (m_nSoilLyrs[i] > 6 ? m_CH4_prod[i][6] : -999.f)
 //   << " oxid=" << (m_nSoilLyrs[i] > 6 ? m_CH4_oxid[i][6] * atomiccmass : -999.f)
 //   << " CH4=" << (m_nSoilLyrs[i] > 6 ? m_CH4[i][6] : -999.f)
 //   << " diss=" << (m_nSoilLyrs[i] > 6 ? m_CH4_diss[i][6] : -999.f)
 //   << " gas=" << (m_nSoilLyrs[i] > 6 ? m_CH4_gas[i][6] : -999.f)
 //   << " VGC=" << (m_nSoilLyrs[i] > 6 ? m_CH4_vgc[i][6] : -999.f)
 //   << " anoxic=" << (m_nSoilLyrs[i] > 6 ? m_anoxic[i][6] : -999.f)
 //   << " FracLiq=" << (m_nSoilLyrs[i] > 6 ? (m_Frac_water[i][6] + m_Frac_water_belowpwp[i][6]) : -999.f)

 //   << "\n";

	return true;
}


int CH4_LPJGUESS::Execute() {

	CheckInputData();
	InitialOutputs();

	//// ===== DEBUG: print once before parallel region =====
	//const int DBG = 629;
	//if (DBG >= 0 && DBG < m_nCells && m_nSoilLyrs[DBG] > 0) {
	//	float sum_y = 0.f, sum_ch4 = 0.f;
	//	for (int j = 0; j < m_nSoilLyrs[DBG]; j++) {
	//		sum_y += m_CH4_yesterday[DBG][j];
	//		sum_ch4 += m_CH4[DBG][j];
	//	}
	//	std::cout << "\n[CH4 Execute START] day=" << m_day
	//		<< " cell=" << DBG
	//		<< " sum(CH4_yesterday)=" << sum_y
	//		<< " sum(CH4)=" << sum_ch4
	//		<< " handWtrDep=" << m_handWtrDep[DBG]
	//		<< "\n";
	//}


#pragma omp parallel for
	for (int i = 0; i < m_nCells; i++) {

		if (m_nSoilLyrs[i] <= 0)  continue;

		InitRootFractions(i);

		InitFracAir(i);

		InitAnoxic(i);

		Methane(i, m_day);

	}
	return 0;
}




void CH4_LPJGUESS::GetValue(const char* key, float* value) {
	
}

void CH4_LPJGUESS::Get1DData(const char* key, int* n, float** data) {

	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, VAR_CH4_PROD)) {
		*data = m_CH4_prod_hour;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_CH4_DIFF)) {
		*data = m_CH4_diff_hour;
		*n = m_nCells;
	}  
	else if (StringMatch(sk, VAR_CH4_OXID)) {
		*data = m_CH4_oxid_hour;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_CH4_EBUL)) {
		*data = m_CH4_ebull_hour;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_CH4_FLUX)) {
		*data = m_CH4_flux_hour;
		*n = m_nCells;
	}
	else {
		throw ModelException(MID_CH4_LPJGUESS, "Get1DData", "Result " + sk + " does not exist.");
	}
}

void CH4_LPJGUESS::Get2DData(const char* key, int* n, int* col, float*** data) {

	InitialOutputs();
	string sk(key);
	*n = m_nCells;
	*col = m_maxSoilLyrs;
	if (StringMatch(sk, VAR_SOL_ANOXIC)) {
		*data = m_anoxic;
	}
	else {
		throw ModelException(MID_CH4_LPJGUESS, "Get2DData", "Result " + sk + " does not exist.");
	}
}
