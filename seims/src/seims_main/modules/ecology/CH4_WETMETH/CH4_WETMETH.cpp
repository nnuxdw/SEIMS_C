#include "CH4_WETMETH.h"
#include "text.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <vector>
#include <array>
#include <unordered_map>
#include <sstream>
#include <string>
#include <ctime>
#include <iostream>


// Soil column constructor
// Initialize all member variables, set pointers to nullptr, set values to 0
SoilCol::SoilCol() :
	z_oxic(0.0f),
	area_Soilcol(0.0f),
	num_layers(0),
	SoilCol_CH4(0.0f),
	SoilCol_CH4_oxidized(0.0f),
	CH4_remaining_fraction(0.0f),
	layer_thickness(nullptr),
	cumulative_depth(nullptr),
	soil_water_storage(nullptr),
	soil_saturated(nullptr),
	soil_saturation(nullptr),
	q10_eff_layer(nullptr),
	temperature_factor_layer(nullptr),
	depth_factor_layer(nullptr),
	CH4_prod_layer(nullptr),
	T_soil(nullptr),
	//Soc(nullptr),
	Rh(nullptr),
	m_soilPor(nullptr),
	m_soilWP(nullptr),
	m_soilFC(nullptr)
{
}


// Release dynamically allocated memory to prevent memory leaks
SoilCol::~SoilCol() {
	if (layer_thickness != nullptr) Release1DArray(layer_thickness);
	if (cumulative_depth != nullptr) Release1DArray(cumulative_depth);
	if (soil_water_storage != nullptr) Release1DArray(soil_water_storage);
	if (soil_saturated != nullptr) Release1DArray(soil_saturated);
	if (soil_saturation != nullptr) Release1DArray(soil_saturation);
	if (T_soil != nullptr) Release1DArray(T_soil);
	//if (Soc != nullptr) Release1DArray(Soc);
	if (Rh != nullptr) Release1DArray(Rh);
	if (m_soilWP != nullptr) Release1DArray(m_soilWP);
	if (m_soilPor != nullptr) Release1DArray(m_soilPor);
	if (q10_eff_layer != nullptr) Release1DArray(q10_eff_layer);
	if (m_soilFC != nullptr) Release1DArray(m_soilFC);
	if (temperature_factor_layer != nullptr) Release1DArray(temperature_factor_layer);
	if (depth_factor_layer != nullptr) Release1DArray(depth_factor_layer);
	if (CH4_prod_layer != nullptr) Release1DArray(CH4_prod_layer);
}


// Initialize soil column with given number of layers
void SoilCol::Initialize(int num_layers) {
	this->num_layers = num_layers;        // Store layer count information
	if (layer_thickness == nullptr) {
		Initialize1DArray(num_layers, layer_thickness, 0.0f);
		Initialize1DArray(num_layers, cumulative_depth, 0.0f);
		Initialize1DArray(num_layers, soil_water_storage, 0.0f);
		Initialize1DArray(num_layers, soil_saturated, 0.0f);
		Initialize1DArray(num_layers, soil_saturation, 0.0f);
		Initialize1DArray(num_layers, T_soil, 0.0f);
		//Initialize1DArray(num_layers, Soc, 0.0f);
		Initialize1DArray(num_layers, Rh, 0.0f);
		Initialize1DArray(num_layers, m_soilWP, 0.0f);
		Initialize1DArray(num_layers, m_soilPor, 0.0f);
		Initialize1DArray(num_layers, q10_eff_layer, 0.0f);
		Initialize1DArray(num_layers, temperature_factor_layer, 0.0f);
		Initialize1DArray(num_layers, depth_factor_layer, 0.0f);
		Initialize1DArray(num_layers, CH4_prod_layer, 0.0f);
		Initialize1DArray(num_layers, m_soilFC, 0.0f);
	}
}


void SoilCol::calculate_soil_saturation(int cell_idx) {

	for (int i = 0; i < num_layers; i++) {
		if (soil_saturated[i] > 0.0f) {

			/*float ratio = (soil_water_storage[i] + m_soilWP[i]) / (layer_thickness[i] * m_soilPor[i]);*/
			float ratio = (soil_water_storage[i] + m_soilWP[i]) / (soil_saturated[i] + m_soilWP[i]);
			// ratio > 1 时截断到 1
			if (ratio > 1.0f) {
				ratio = 1.0f;
			}
			else if (ratio < 0.0f) {
				ratio = 0.0f;
			}
			soil_saturation[i] = ratio;
		}
		else {
			soil_saturation[i] = 0.0f;
		}
	}
}


//float SoilCol::SoilColMethane(int cell_idx, float &CH4_before, float handWtrDep, float ch4_rh_ratio,
//	float ch4_tref, float ch4_tau_prod, float ch4_z_oatz, float ch4_tau_oxid) {
//	// Reset CH4 production for current soil column
//	SoilCol_CH4 = 0.0f;
//	float CH4_Soilcol_total = 0.0f;
//	
//	// Calculate soil saturation for all layers 
//	calculate_soil_saturation(cell_idx);
//
//	// Calculate cumulative depth for each layer
//	float cumulative_depth_val = 0.0f;
//
//    for (int i = 0; i < num_layers; i++) {
//        // Update cumulative depth (distance from surface to bottom of layer i)
//        // Convert thickness from mm to m for consistent units
//		cumulative_depth_val += layer_thickness[i] / 1000.0f;
//		cumulative_depth[i] = cumulative_depth_val;     // Store the calculated cumulative depth (m)
//
//		float T_kelvin = T_soil[i] + 273.15f;           // Convert °C to K
//
//        // Calculate CH4 production rate using WETMETH formula
//        // Pi = r * Rh * S(θ) * exp((T - T0)/10 * ln(Q10)) * exp(-z/t_prod)
//        // Consider effects of temperature, depth, heterotrophic respiration, and saturation on methane production
//        // Convert temperature from °C to K: T_soil[i] + 273.15
//		float Pi = 0.0f;            
//		q10_eff_layer[i] = 0.0f;    
//
//		if (T_kelvin > 273.15f) {  
//	       
//			float sat_for_ch4 = soil_saturation[i];
//
//			
//			float q10_eff = 1.7f + 2.5f * std::tanh(0.1f * (ch4_tref - T_kelvin));
//
//			
//			if (q10_eff <= 0.0f) {
//				q10_eff = 1e-3f;
//			}
//
//			q10_eff_layer[i] = q10_eff;
//
//		    
//			Pi = ch4_rh_ratio * Rh[i] * sat_for_ch4 *
//				std::exp((T_kelvin - CH4_T0) / 10.0f * std::log(q10_eff)) *
//				std::exp(-cumulative_depth[i] / ch4_tau_prod);
//		}
//
//		
//		CH4_Soilcol_total += Pi / 24.0f;
//	}
//
//	
//	SoilCol_CH4 = CH4_Soilcol_total;
//	CH4_before = CH4_Soilcol_total;
//
//	
//	if (handWtrDep > 1e-6f) {
//		z_oxic = ch4_z_oatz;
//	}
//	else {
//		z_oxic = calculate_oxic_depth(ch4_z_oatz);    
//	}
//
//	
//	float factor = std::exp(-z_oxic / ch4_tau_oxid);  
//	float CH4_after = SoilCol_CH4 * factor;           
//
//	return CH4_after;
//
//}


float SoilCol::SoilColMethane(int cell_idx, float &CH4_before, float handWtrDep,  float ch4_rh_ratio,
	float ch4_tref, float ch4_tau_prod, float ch4_z_oatz, float ch4_tau_oxid) {

	// Reset CH4 production for the current soil column
	SoilCol_CH4 = 0.0f;
	float CH4_Soilcol_total = 0.0f;

	SoilCol_CH4_oxidized = 0.0f;
	CH4_remaining_fraction = 0.0f;

	// Calculate soil saturation for all soil layers
	calculate_soil_saturation(cell_idx);

	// Use 0.1 mm as the surface inundation threshold
	// to avoid treating numerical noise as surface water
	//const bool surfaceFlooded = handWtrDep > 1e-3f;     // 1 mm threshold
	const bool surfaceFlooded = handWtrDep > 1e-4f;   // 0.1 mm threshold

	// Initialize the cumulative soil depth
	float cumulative_depth_val = 0.0f;

	for (int i = 0; i < num_layers; i++) {

		// Convert the current layer thickness from mm to m
		const float layer_thickness_m = layer_thickness[i] / 1000.0f;

		// Calculate the depth from the soil surface
		// to the bottom of the current layer
		cumulative_depth_val += layer_thickness_m;
		cumulative_depth[i] = cumulative_depth_val;

		// Use the midpoint depth of the current layer
		// for the depth attenuation term
		const float layer_mid_depth = cumulative_depth[i] - 0.5f * layer_thickness_m;

		// Convert soil temperature from degrees Celsius to Kelvin
		float T_kelvin = T_soil[i] + 273.15f;

		float Pi = 0.0f;

		q10_eff_layer[i] = 0.0f;
		temperature_factor_layer[i] = 0.0f;
		depth_factor_layer[i] = std::exp(-layer_mid_depth / ch4_tau_prod);
		CH4_prod_layer[i] = 0.0f;

		if (T_kelvin > CH4_T0) {

			float sat_for_ch4 = Max(0.0f, Min(soil_saturation[i], 1.0f));

			float q10_eff = 1.7f + 2.5f * std::tanh(0.1f * (ch4_tref - T_kelvin));

			if (q10_eff <= 0.0f) {
				q10_eff = 1.0e-3f;
			}

			q10_eff_layer[i] = q10_eff;

			temperature_factor_layer[i] = std::exp((T_kelvin - CH4_T0) / 10.0f * std::log(q10_eff));

			Pi = ch4_rh_ratio * Rh[i] * sat_for_ch4 * temperature_factor_layer[i] * depth_factor_layer[i];

			CH4_prod_layer[i] = Pi / 24.0f;
		}

		CH4_Soilcol_total += CH4_prod_layer[i];
	}

	SoilCol_CH4 = CH4_Soilcol_total;
	CH4_before = CH4_Soilcol_total;

	// Use the same inundation condition for oxidation
	if (surfaceFlooded) {
		z_oxic = ch4_z_oatz;
	}
	else {
		z_oxic = calculate_oxic_depth(ch4_z_oatz);
	}

	float factor = std::exp(-z_oxic / ch4_tau_oxid);

	factor = Max(0.0f, Min(factor, 1.0f));

	CH4_remaining_fraction = factor;

	float CH4_after = SoilCol_CH4 * CH4_remaining_fraction;

	SoilCol_CH4_oxidized = SoilCol_CH4 - CH4_after;

	return CH4_after;
}


// Calculate oxic zone depth
// Oxic depth is calculated using the equivalent unsaturated soil depth
float SoilCol::calculate_oxic_depth(float ch4_z_oatz) {

	float oxic_depth = ch4_z_oatz;

	// Calculate equivalent unsaturated depth
	for (int i = 0; i < num_layers; i++) {

		// layer_thickness: mm -> m
		oxic_depth += layer_thickness[i] / 1000.0f * (1.0f - soil_saturation[i]);
	}

	// Total soil profile depth has already been calculated
	float profile_depth = cumulative_depth[num_layers - 1];

	// Limit oxic depth to the soil profile
	if (oxic_depth > profile_depth) {
		oxic_depth = profile_depth;
	}
	else if (oxic_depth < 0.0f) {
		oxic_depth = 0.0f;
	}

	return oxic_depth;
}


// Main module constructor
CH4_WETMETH::CH4_WETMETH() :
	m_nCells(-1),
	m_maxSoilLyrs(-1),

	//m_ch4_r(NODATA_VALUE),
	m_ch4_rh_ratio(NODATA_VALUE),    // 新增：CH4-C/CO2-C生产速率比,无量纲
	m_ch4_tref(NODATA_VALUE),
	m_ch4_tau_prod(NODATA_VALUE),
	m_ch4_z_oatz(NODATA_VALUE),
	m_ch4_tau_oxid(NODATA_VALUE),

	m_nSoilLyrs(nullptr),
	m_area(nullptr),
	m_layer_thickness(nullptr),
	m_soil_water_storage(nullptr),
	m_soil_saturated(nullptr),
	m_soilWP(nullptr),
	m_soilPor(nullptr),
	// m_Soc(nullptr), // commented out, using m_Soc_kg_ha instead
	m_soilFC(nullptr),
	//m_Soc_kg_ha(nullptr),
	m_sol_RSPC(nullptr),
	m_Tsoil(nullptr),
	m_SoilCols(nullptr),
	m_P_soilcol(nullptr),
	m_P_soilcol_flux(nullptr),
	m_satL1(nullptr),
	m_satL2(nullptr),
	m_satL3(nullptr),
	m_satL4(nullptr),
	m_satL5(nullptr),
	m_satL6(nullptr),
	m_satL7(nullptr),
	/*m_handWtrDep(nullptr),*/
	/*m_handWtrDepAftInfil(nullptr),*/
	m_handWtrDepAftDep(nullptr),
	m_infil(nullptr),
	m_netPcp(nullptr),
	m_soilPerco(nullptr),
	m_subSurfRf(nullptr),
	m_sd(nullptr),
	m_IntcpET(nullptr),
	m_exsPcp(nullptr),
	//m_ifluQ2Rch(nullptr),
	m_soilET(nullptr),
	m_total_CH4(0.0f)
{

}

CH4_WETMETH::~CH4_WETMETH() {
	if (m_P_soilcol != nullptr) Release1DArray(m_P_soilcol);
	if (m_P_soilcol_flux != nullptr) Release1DArray(m_P_soilcol_flux);
	//if (m_Soc_kg_ha != nullptr) Release2DArray(m_nCells, m_Soc_kg_ha);
	if (m_SoilCols != nullptr) Release1DArray(m_SoilCols);
	/*if (m_handWtrDep != nullptr) Release1DArray(m_handWtrDep);*/
	/*if (m_handWtrDepAftInfil != nullptr) Release1DArray(m_handWtrDepAftInfil);*/
	if (m_satL1 != nullptr) Release1DArray(m_satL1);
	if (m_satL2 != nullptr) Release1DArray(m_satL2);
	if (m_satL3 != nullptr) Release1DArray(m_satL3);
	if (m_satL4 != nullptr) Release1DArray(m_satL4);
	if (m_satL5 != nullptr) Release1DArray(m_satL5);
	if (m_satL6 != nullptr) Release1DArray(m_satL6);
	if (m_satL7 != nullptr) Release1DArray(m_satL7);
	//if (m_ifluQ2Rch != nullptr) Release1DArray(m_ifluQ2Rch);
}

void CH4_WETMETH::SetValue(const char* key, float value) {

	string s(key);
	/*if (StringMatch(s, VAR_CH4_R)) {
		m_ch4_r = value;
	}*/
	if (StringMatch(s, VAR_CH4_RH_RATIO)) {
		m_ch4_rh_ratio = value;
	}
	else if (StringMatch(s, VAR_CH4_TREF)) {
		m_ch4_tref = value;
	}
	else if (StringMatch(s, VAR_CH4_TAU_PROD)) {
		m_ch4_tau_prod = value;
	}
	else if (StringMatch(s, VAR_CH4_Z_OATZ)) {
		m_ch4_z_oatz = value;
	}
	else if (StringMatch(s, VAR_CH4_TAU_OXID)) {
		m_ch4_tau_oxid = value;
	}
	else {
		throw ModelException(MID_CH4_WETMETH, "SetValue", "Parameter " + s + " does not exist.");
	}
}


void CH4_WETMETH::SetValueByIndex(const char* key, int index, float value) {

}

void CH4_WETMETH::Set1DData(const char* key, int n, float* data) {
	string s(key);
	CheckInputSize(MID_CH4_WETMETH, key, n, m_nCells); // Data size validation (module ID, variable name, data length)
	if (StringMatch(s, VAR_AHRU)) {
		m_area = data;
	} else if (StringMatch(s, VAR_SOILLAYERS)) {
		m_nSoilLyrs = data;  // Actual number of soil layers for each cell
	}
	else if (StringMatch(s, VAR_OL_HAND_WTRDEP_AFT_DEP)) {
		m_handWtrDepAftDep = data;
	}
	//else if (StringMatch(s, VAR_OL_HAND_WTRDEP_AFT_INFIL)) {// SUR_MR_HAND计算完成HAND积水入渗后剩余的地表水深，单位m
	//	m_handWtrDepAftInfil = data;
	//}
	//else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
	//    m_handWtrDep = data;    // Water depth of each hand(m)
	//}
	else if (StringMatch(s, VAR_INFIL)) {
		m_infil = data;
	} else if (StringMatch(s, VAR_NEPR)) {
		m_netPcp = data;
	} else if (StringMatch(s, VAR_DPST)) {
		m_sd = data;
	} else if (StringMatch(s, VAR_INET)) {
		m_IntcpET = data;
	} else if (StringMatch(s, VAR_EXCP)) {
		m_exsPcp = data; // excess precipitation
	} /*else if (StringMatch(s, VAR_SBIF)) {
		m_ifluQ2Rch = data;
	}*/ else if (StringMatch(s, VAR_SOET)) {
		m_soilET = data;
	} else {
		throw ModelException(MID_CH4_WETMETH, "Set1DData", "Parameter " + s + " does not exist.");
	}
}

void CH4_WETMETH::Set2DData(const char* key, int n, int col, float** data) {
	string sk(key);  
	CheckInputSize2D(MID_CH4_WETMETH, key, n, col, m_nCells, m_maxSoilLyrs);

	if (StringMatch(sk, VAR_SOILTHICK)) {
		m_layer_thickness = data;
	} else if (StringMatch(sk, VAR_SOL_ST)) {
		m_soil_water_storage = data;    // Soil layer water storage (mm H2O)
	} else if (StringMatch(sk, VAR_SOL_UL)) {
		m_soil_saturated = data;        // Soil layer saturated water capacity (mm H2O)
	} else if (StringMatch(sk, VAR_SOILT)) {
		m_Tsoil = data;                 // Soil temperature (°C)
	} else if (StringMatch(sk, VAR_SOL_RSPC)) {
		m_sol_RSPC = data;              // CENTURY逐层异养呼吸,单位：kg C ha-1 d-1
	/*else if (StringMatch(sk, VAR_SOL_WOC)) {
		m_Soc_kg_ha = data;     */        // Soil organic carbon content (kg/ha)
	} else if (StringMatch(sk, VAR_SOL_WPMM)) {
		m_soilWP = data;                // Water content of soil at -1.5 MPa (wilting point)
	} else if (StringMatch(sk, VAR_POROST)) {
		m_soilPor = data;               // Porosity mm/mm
	} else if (StringMatch(sk, VAR_PERCO)) {
		m_soilPerco = data;
	} else if (StringMatch(sk, VAR_SSRU)) {
		m_subSurfRf = data;
	} else if (StringMatch(sk, VAR_FIELDCAP)) {
        m_soilFC = data;
	}
		

	else {
		throw ModelException(MID_CH4_WETMETH, "Set2DData", "Parameter " + sk + " does not exist.");
	}
}


void CH4_WETMETH::SetReaches(clsReaches* rches) {

}

void CH4_WETMETH::SetSubbasins(clsSubbasins* subbsns) {

}

void CH4_WETMETH::SetScenario(Scenario* sce) {

}

bool CH4_WETMETH::CheckInputData() {
	// Check basic parameters
	if (m_nCells <= 0) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "The number of cells must be greater than 0.");
	}
	if (m_maxSoilLyrs <= 0) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "The number of soil layers must be greater than 0.");
	}
	/*if (m_ch4_r <= 0) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "CH4_R Parameter does not exist.");
	}*/
	if (m_ch4_rh_ratio <= 0.f) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "CH4_RH_RATIO Parameter does not exist or is not positive.");
	}
	if (m_ch4_tref <= 0) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "CH4_TREF Parameter does not exist.");
	}
	if (m_ch4_tau_prod <= 0) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "CH4_TAU_PROD Parameter does not exist.");
	}
	if (m_ch4_z_oatz <= 0) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "CH4_Z_OATZ Parameter does not exist.");
	}
	if (m_ch4_tau_oxid <= 0) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "CH4_TAU_OXID Parameter does not exist.");
	}
	
	// Check input data pointers
	if (m_area == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Cell area data is not set.");
	}
	if (m_nSoilLyrs == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil layers number data is not set.");
	}
	if (m_layer_thickness == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil layer thickness data is not set.");
	}
	if (m_soil_water_storage == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil water storage data is not set.");
	}
	if (m_soil_saturated == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil saturated water capacity data is not set.");
	}
	/*if (m_Soc_kg_ha == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil organic carbon data is not set.");
	}*/
	if (m_sol_RSPC == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "CENTURY heterotrophic respiration data is not set.");
	}
	if (m_Tsoil == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil temperature data is not set.");
	}
	if (m_soilWP == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil WP data is not set.");
	}
	if (m_soilPor == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil porosity data is not set.");
	}
	if (m_soilFC == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil field capacity data is not set.");
	}
	/*if (m_handWtrDep == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "OL_HAND final water depth is not set.");
	}*/
	/*if (m_handWtrDepAftInfil == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "HAND water depth after infiltration is not set.");
	}*/
	if (m_handWtrDepAftDep == nullptr) {
        throw ModelException(MID_CH4_WETMETH, "CheckInputData", "HAND water depth after depression processing is not set.");
	}
	if (m_infil == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Infiltration data is not set.");
	}
	if (m_netPcp == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Precipitation data is not set.");
	}
	if (m_soilPerco == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Percolation data is not set.");
	}
	if (m_subSurfRf == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Subsurface runoff data is not set.");
	}
	if (m_sd == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Depression storage data is not set.");
	}
	if (m_IntcpET == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Interception evaporation data is not set.");
	}
	if (m_exsPcp == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Surface runoff data is not set.");
	}
	if (m_soilET == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Actual soil evaporation data is not set.");
	}
	/*if (m_ifluQ2Rch == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "The soil runoff collected from each subbasin and flowing into the river channel data is not set.");
	}*/
	return true;
}

void CH4_WETMETH::InitialOutputs() {
	if (m_SoilCols == nullptr) {
		m_SoilCols = new SoilCol[m_nCells];

		Initialize1DArray(m_nCells, m_P_soilcol, 0.0f);
		Initialize1DArray(m_nCells, m_P_soilcol_flux,0.0f);

		Initialize1DArray(m_nCells, m_satL1, 0.0f);
		Initialize1DArray(m_nCells, m_satL2, 0.0f);
		Initialize1DArray(m_nCells, m_satL3, 0.0f);
		Initialize1DArray(m_nCells, m_satL4, 0.0f);
		Initialize1DArray(m_nCells, m_satL5, 0.0f);
		Initialize1DArray(m_nCells, m_satL6, 0.0f);
		Initialize1DArray(m_nCells, m_satL7, 0.0f);
		
		// Initialize each soil column
		for (int i = 0; i < m_nCells; i++) {
			m_SoilCols[i].Initialize(m_maxSoilLyrs);
		}
	}
}


int CH4_WETMETH::Execute() {

	CheckInputData();
	InitialOutputs();

	m_total_CH4 = 0.0f;

	// 是否输出 CH4 调试 CSV
	const bool DEBUG_WRITE_CH4_CSV = false;
	const bool write_ch4_debug_this_step = DEBUG_WRITE_CH4_CSV && m_year >= 2010;

#ifdef _WIN32
	std::string debug_file = "D:/SEIMS_C/data/CW_2/workspace/rundata/sub18_lake_debug_CH4.csv";
#else
	std::string debug_file = "/data/user/lijing/software/SEIMS_C/log/sub18_lake_debug_CH4.csv";
#endif

	static bool debug_header_written = false;
	static int debug_step = 0;

	std::ofstream dbg;

	if (write_ch4_debug_this_step) {

		if (!debug_header_written) {
			dbg.open(debug_file, std::ios::out);

			if (!dbg.is_open()) {
				throw ModelException(
					MID_CH4_WETMETH,
					"Execute",
					"Cannot open debug csv file."
				);
			}

			dbg << "date"
				<< ",year"
				<< ",dayofyear"
				<< ",step"
				<< ",cell"
				<< ",layer"
				<< ",actual_layers"
				<< ",area"
				<< ",CH4_RH_RATIO"
				<< ",CH4_TREF_input"
				<< ",q10_eff_layer"
				<< ",CH4_TAU_PROD"
				<< ",CH4_Z_OATZ"
				<< ",CH4_TAU_OXID"
				<< ",OL_HAND_WTRDEP_AFT_DEP_m"
				<< ",infil"
				<< ",netPcp"
				<< ",sd"
				<< ",exsPcp"
				<< ",IntcpET"
				<< ",soilET"
				<< ",layer_thickness"
				<< ",soil_water_storage"
				<< ",soil_saturated"
				<< ",soilWP"
				<< ",soilPor"
				<< ",soilFC"
				<< ",theta_current"
				<< ",RSPC_kgCha_d"
				<< ",Tsoil"
				<< ",soilPerco"
				<< ",subSurfRf"
				<< ",Rh_gCm2d"
				<< ",Tsoil_copy"
				<< ",cumulative_depth"
				<< ",soil_saturation"
				<< ",effective_sat_for_ch4"
				<< ",temperature_factor"
				<< ",depth_factor"
				<< ",CH4_layer_prod_mgCH4_m2_h"
				<< ",surface_flooded"
				<< ",CH4_column_prod_mgCH4_m2_h"
				<< ",z_oxic"
				<< ",remaining_fraction"
				<< ",oxidized_fraction"
				<< ",CH4_column_oxid_mgCH4_m2_h"
				<< ",CH4_column_emit_mgCH4_m2_h"
				<< ",balance_error_mgCH4_m2_h"
				<< "\n";

			debug_header_written = true;
		}
		else {
			dbg.open(debug_file, std::ios::app);

			if (!dbg.is_open()) {
				throw ModelException(
					MID_CH4_WETMETH,
					"Execute",
					"Cannot open debug csv file."
				);
			}
		}

		dbg << std::fixed << std::setprecision(10);
	}

	// Process each cell
	for (int i = 0; i < m_nCells; i++) {

		// Set soil column properties
		m_SoilCols[i].area_Soilcol = m_area[i];

		// Get actual number of soil layers for this cell
		int actual_layers = (int)m_nSoilLyrs[i];
		m_SoilCols[i].num_layers = actual_layers;

		// Copy soil layer data (only for actual layers)
		for (int j = 0; j < actual_layers; j++) {
			m_SoilCols[i].layer_thickness[j] = m_layer_thickness[i][j];
			m_SoilCols[i].soil_water_storage[j] = m_soil_water_storage[i][j];
			m_SoilCols[i].soil_saturated[j] = m_soil_saturated[i][j];
			m_SoilCols[i].m_soilWP[j] = m_soilWP[i][j];
			m_SoilCols[i].m_soilPor[j] = m_soilPor[i][j];
			m_SoilCols[i].m_soilFC[j] = m_soilFC[i][j];

			// m_SoilCols[i].Soc[j] = m_Soc_kg_ha[i][j] / (thickness_m * 10000.0f);
			// CENTURY layer heterotrophic respiration:
            // kg C ha-1 d-1 -> g C m-2 d-1
			m_SoilCols[i].Rh[j] = m_sol_RSPC[i][j] / 10.0f;
			m_SoilCols[i].T_soil[j] = m_Tsoil[i][j];
		}

		float CH4_before = 0.0f;
		float CH4_after = 0.0f;

		const float surfaceWaterDepth = max(0.0f, m_handWtrDepAftDep[i]) + max(0.0f, m_sd[i]) * 0.001f;

		// HAND surface water depth after infiltration, in metres.
		CH4_after = m_SoilCols[i].SoilColMethane(
			i, CH4_before, surfaceWaterDepth,
			m_ch4_rh_ratio, m_ch4_tref, m_ch4_tau_prod,
			m_ch4_z_oatz, m_ch4_tau_oxid);

		// before (production) -> SoilCol_CH4 在 SoilColMethane() 内已经写好
		CH4_before = m_SoilCols[i].SoilCol_CH4;

		// 只输出关心的 cell，避免文件太大
		if (write_ch4_debug_this_step && (i == 1366 || i == 1367 || i == 1368 || i == 1370 || i == 1375)) {

			const float C_TO_CH4 = 16.0f / 12.0f * 1000.0f;
			const bool surfaceFlooded = surfaceWaterDepth > 1.0e-4f;
			const float columnProduction = CH4_before * C_TO_CH4;
			const float columnOxidation = m_SoilCols[i].SoilCol_CH4_oxidized * C_TO_CH4;
			const float columnEmission = CH4_after * C_TO_CH4;
			const float oxidationFraction = CH4_before > UTIL_ZERO ? m_SoilCols[i].SoilCol_CH4_oxidized / CH4_before : 0.0f;
			const float balanceError = columnProduction - columnOxidation - columnEmission;

			for (int j = 0; j < actual_layers; j++) {
				dbg << m_date << ","
					<< m_year << ","
					<< m_dayOfYear << ","
					<< debug_step << ","
					<< i << ","
					<< j << ","
					<< actual_layers << ","
					<< m_area[i] << ","
					<< m_ch4_rh_ratio << ","
					<< m_ch4_tref << ","
					<< m_SoilCols[i].q10_eff_layer[j] << ","
					<< m_ch4_tau_prod << ","
					<< m_ch4_z_oatz << ","
					<< m_ch4_tau_oxid << ","
					<< m_handWtrDepAftDep[i] << ","
					<< m_infil[i] << ","
					<< m_netPcp[i] << ","
					<< m_sd[i] << ","
					<< m_exsPcp[i] << ","
					<< m_IntcpET[i] << ","
					<< m_soilET[i] << ","
					<< m_layer_thickness[i][j] << ","
					<< m_soil_water_storage[i][j] << ","
					<< m_soil_saturated[i][j] << ","
					<< m_soilWP[i][j] << ","
					<< m_soilPor[i][j] << ","
					<< m_soilFC[i][j] << ","
					<< ((m_soil_water_storage[i][j] + m_soilWP[i][j]) / m_layer_thickness[i][j]) << ","
					/*<< m_sol_RSPC[i][j] << ","*/
					<< m_SoilCols[i].Rh[j] * 10.0f << ","
					<< m_Tsoil[i][j] << ","
					<< m_soilPerco[i][j] << ","
					<< m_subSurfRf[i][j] << ","
					<< m_SoilCols[i].Rh[j] << ","
					<< m_SoilCols[i].T_soil[j] << ","
					<< m_SoilCols[i].cumulative_depth[j] << ","
					<< m_SoilCols[i].soil_saturation[j] << ","
					/*<< ((m_handWtrDep[i] > 1e-6f) ? 1.0f : m_SoilCols[i].soil_saturation[j]) << ","*/
					<< m_SoilCols[i].soil_saturation[j] << ","
					<< m_SoilCols[i].temperature_factor_layer[j] << ","
					<< m_SoilCols[i].depth_factor_layer[j] << ","
					<< m_SoilCols[i].CH4_prod_layer[j] * C_TO_CH4 << ","
					<< (surfaceFlooded ? 1 : 0) << ","
					<< columnProduction << ","
					<< m_SoilCols[i].z_oxic << ","
					<< m_SoilCols[i].CH4_remaining_fraction << ","
					<< oxidationFraction << ","
					<< columnOxidation << ","
					<< columnEmission << ","
					<< balanceError
					<< "\n";
			}
		}

		m_satL1[i] = (actual_layers >= 1) ? m_SoilCols[i].soil_saturation[0] : 0.0f;
		m_satL2[i] = (actual_layers >= 2) ? m_SoilCols[i].soil_saturation[1] : 0.0f;
		m_satL3[i] = (actual_layers >= 3) ? m_SoilCols[i].soil_saturation[2] : 0.0f;
		m_satL4[i] = (actual_layers >= 4) ? m_SoilCols[i].soil_saturation[3] : 0.0f;
		m_satL5[i] = (actual_layers >= 5) ? m_SoilCols[i].soil_saturation[4] : 0.0f;
		m_satL6[i] = (actual_layers >= 6) ? m_SoilCols[i].soil_saturation[5] : 0.0f;
		m_satL7[i] = (actual_layers >= 7) ? m_SoilCols[i].soil_saturation[6] : 0.0f;


		m_P_soilcol[i] = CH4_before * 16.0f / 12.0f * 1000.0f;            // g CH4-C m^-2 h^-1 -> mg CH4 m^-2 h^-1
		m_P_soilcol_flux[i] = CH4_after * 16.0f / 12.0f * 1000.0f;        // g CH4-C m^-2 h^-1 -> mg CH4 m^-2 h^-1

		m_total_CH4 += CH4_after * 16.0f / 12.0f * 1000.0f * m_area[i];   // g CH4-C m^-2 h^-1 to mg CH4 h^-1
	}

	if (write_ch4_debug_this_step) {
		dbg.close();
		debug_step++;
	}


	return 0;
}


TimeStepType CH4_WETMETH::GetTimeStepType() {
    return TIMESTEP_HILLSLOPE;
}


void CH4_WETMETH::GetValue(const char* key, float* value) {
	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, VAR_CH4_TOTAL)) {
		*value = m_total_CH4;
	}
	else {
		throw ModelException(MID_CH4_WETMETH, "GetValue", "Result " + sk + " does not exist.");
	}  
}

void CH4_WETMETH::Get1DData(const char* key, int* n, float** data) {
	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, VAR_CH4_PRODUCTION)) {
		*data = m_P_soilcol; 
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_CH4_EMISSION_FLUX)) {
		*data = m_P_soilcol_flux;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SOILSAT_L1)) {
		*data = m_satL1;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SOILSAT_L2)) {
		*data = m_satL2;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SOILSAT_L3)) {
		*data = m_satL3;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SOILSAT_L4)) {
		*data = m_satL4;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SOILSAT_L5)) {
		*data = m_satL5;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SOILSAT_L6)) {
		*data = m_satL6;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SOILSAT_L7)) {
		*data = m_satL7;
		*n = m_nCells;
	}
	else {
		throw ModelException(MID_CH4_WETMETH, "Get1DData", "Result " + sk + " does not exist.");
	}
}
void CH4_WETMETH::Get2DData(const char* key, int* n, int* col, float*** data) {

}


