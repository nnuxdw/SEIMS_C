#include "CH4_WETMETH.h"
#include "text.h"
#include <cmath>
#include <algorithm>


// Soil column constructor
// Initialize all member variables, set pointers to nullptr, set values to 0
SoilCol::SoilCol() :
	z_oxic(0.0f),
	area_Soilcol(0.0f),
	num_layers(0),
	SoilCol_CH4(0.0f),
	layer_thickness(nullptr),
	cumulative_depth(nullptr),
	soil_water_storage(nullptr),
	soil_saturated(nullptr),
	soil_saturation(nullptr),
	T_soil(nullptr),
	Soc(nullptr),
	m_soilPor(nullptr),
	m_soilWP(nullptr)
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
	if (Soc != nullptr) Release1DArray(Soc);
	if (m_soilWP != nullptr) Release1DArray(m_soilWP);
	if (m_soilPor != nullptr) Release1DArray(m_soilPor);
}

// Initialize soil column with given number of layers
void SoilCol::Initialize(int num_layers) {
	this->num_layers = num_layers;  // Store layer count information
	if (layer_thickness == nullptr) {
		Initialize1DArray(num_layers, layer_thickness, 0.0f);
		Initialize1DArray(num_layers, cumulative_depth, 0.0f);
		Initialize1DArray(num_layers, soil_water_storage, 0.0f);
		Initialize1DArray(num_layers, soil_saturated, 0.0f);
		Initialize1DArray(num_layers, soil_saturation, 0.0f);
		Initialize1DArray(num_layers, T_soil, 0.0f);
		Initialize1DArray(num_layers, Soc, 0.0f);
		Initialize1DArray(num_layers, m_soilWP, 0.0f);
		Initialize1DArray(num_layers, m_soilPor, 0.0f);
	}
}

// Calculate saturation ratio for each soil layer: saturation 
//void SoilCol::calculate_soil_saturation(int cell_idx) {
//
//	for (int i = 0; i < num_layers; i++) {
//
//		if (soil_saturated[i] > 0.0f) {
//
//			float ratio = (soil_water_storage[i] + m_soilWP[i]) /
//				(layer_thickness[i] * m_soilPor[i]);
//
//			// ratio > 1 时截断到 0.99999
//			if (ratio > 0.99999f) ratio = 0.99999f;
//			else if (ratio < 0.0f) ratio = 0.0f;
//
//			soil_saturation[i] = ratio;
//
//			//// 打印所有土层
//			//std::cout << ">>> Cell " << cell_idx
//			//	<< ", Layer " << i
//			//	<< "  raw_ratio=" << ratio
//			//	<< "  saturation=" << soil_saturation[i]
//			//	<< (soil_saturation[i] >= 0.99999f ? "  [FULL]" : "")
//			//	<< "  [water=" << soil_water_storage[i]
//			//	<< ", WP=" << m_soilWP[i]
//			//	<< ", por=" << m_soilPor[i]
//			//	<< ", thick=" << layer_thickness[i]
//			//	<< ", sat_cap=" << soil_saturated[i]
//			//	<< "]" << std::endl;
//		}
//		else {
//			soil_saturation[i] = 0.0f;
//
//			//// 也打印一下
//			//std::cout << ">>> Cell " << cell_idx
//			//	<< ", Layer " << i
//			//	<< "  sat_cap<=0, set saturation=0"
//			//	<< "  [sat_cap=" << soil_saturated[i] << "]"
//			//	<< std::endl;
//		}
//	}
//}
void SoilCol::calculate_soil_saturation(int cell_idx) {

	for (int i = 0; i < num_layers; i++) {

		if (soil_saturated[i] > 0.0f) {

			float ratio = (soil_water_storage[i] + m_soilWP[i]) /
				(layer_thickness[i] * m_soilPor[i]);

			// ratio > 1 时截断到 0.99999
			if (ratio > 0.99999f) ratio = 0.99999f;
			else if (ratio < 0.0f) ratio = 0.0f;

			soil_saturation[i] = ratio;

		}
		else {
			soil_saturation[i] = 0.0f;
		}
	}

	//// ✅ 只打印 629-644 单元：一行输出所有层饱和度
	//if (cell_idx >= 629 && cell_idx <= 644) {

	//	std::cout << std::fixed << std::setprecision(7);
	//	std::cout << "saturation;" << cell_idx << ";";

	//	for (int i = 0; i < num_layers; i++) {
	//		if (i > 0) std::cout << ",";
	//		std::cout << soil_saturation[i];
	//	}
	//	std::cout << std::endl;
	//}
}



float SoilCol::SoilColMethane(int cell_idx, float &CH4_before)
{
	// Reset CH4 production for current soil column
	SoilCol_CH4 = 0.0f;
	float CH4_Soilcol_total = 0.0f;
	
	// Calculate soil saturation for all layers first
	calculate_soil_saturation(cell_idx);

	// Calculate cumulative depth for each layer
    float cumulative_depth_val = 0.0f;
    for (int i = 0; i < num_layers; i++) {
        // Update cumulative depth (distance from surface to bottom of layer i)
        // Convert thickness from mm to m for consistent units
        cumulative_depth_val += layer_thickness[i] / 1000.0f;
        cumulative_depth[i] = cumulative_depth_val; // Store the calculated cumulative depth (m)

        // Calculate CH4 production rate using WETMETH formula
        // Pi = r * SOC * S(θ) * exp((T - T0)/T_ref * ln(Q10)) * exp(-z/t_prod)
        // Consider effects of temperature, depth, organic carbon, and saturation on methane production
        // Convert temperature from °C to K: T_soil[i] + 273.15
        // Convert thickness from mm to m: layer_thickness[i] / 1000.0
        float T_kelvin = T_soil[i] + 273.15f;  // Convert °C to K
		//std::cout << "T: " << T_soil[i] << std::endl; 
        float Pi = CH4_R * Soc[i] * soil_saturation[i] *                            // Pi: kg C m⁻³ s⁻¹ → g C m⁻³ s⁻¹
                   std::exp((T_kelvin - CH4_T0) / 10 * std::log(CH4_Q10)) *
                   std::exp(-cumulative_depth[i] / CH4_TAU_PROD) * 1e3f;            // Use stored cumulative_depth

        // Total CH4 production = rate * layer thickness
        // Pi: g C m⁻³ s⁻¹, layer_thickness: mm → m, result: g C m⁻² h⁻¹  
        CH4_Soilcol_total += (float)(Pi * (layer_thickness[i] / 1000.0f) * 3600.0f);
	}

	//// ===== 新增：打印 CH4_Soilcol_total =====
	//// 只打印你关心的 cell，避免输出太多；不需要就删掉 if
	//if (cell_idx >= 629 && cell_idx <= 644) {
	//	std::cout << std::fixed << std::setprecision(7)
	//		<< "[CH4_PROD] cell=" << cell_idx
	//		<< " CH4_Soilcol_total(gC m^-2 h^-1)=" << CH4_Soilcol_total
	//		<< std::endl;
	//}

	// ------ BEFORE oxidation ------
	SoilCol_CH4 = CH4_Soilcol_total;
	CH4_before = CH4_Soilcol_total;

	/*if (cell_idx >= 629 && cell_idx <= 644) {
		std::cout << std::fixed << std::setprecision(7)
			<< "[CH4_DEBUG] cell=" << cell_idx
			<< " total=" << CH4_Soilcol_total
			<< " SoilCol_CH4=" << SoilCol_CH4
			<< std::endl;
	}*/

	// Calculate oxic zone depth
	z_oxic = calculate_oxic_depth();

	/*std::cout << "cell=" << cell_idx
		<< " oxic_deep=" << z_oxic
		<< " CH4_before_oxid=" << SoilCol_CH4
		<< std::endl;

	std::cout << "tau_runtime=" << CH4_TAU_OXID << std::endl;*/

	// Apply oxidation reduction based on oxic zone depth
	// area_Soilcol: m², CH4_Soilcol_total: g C m⁻² h⁻¹
	//CH4_Soilcol_total = (float)(CH4_Soilcol_total * std::exp(-z_oxic/CH4_TAU_OXID));

	//std::cout << "[WETMETH] cell=" << cell_idx
		//<< " CH4_after_oxid=" << CH4_Soilcol_total
		//<< std::endl;

	//// Calculate the flux after oxidation
	//float expo = -z_oxic / CH4_TAU_OXID;
	//float factor = std::exp(expo);
	//float before = CH4_Soilcol_total;
	//float after = before * factor;
	//float delta = before - after;

	//std::cout << std::fixed << std::setprecision(7)
	//	<< "[WETMETH][OXID_DEBUG] cell=" << cell_idx
	//	//<< " z=" << z
	//	//<< " tau=" << tau
	//	<< " expo=" << expo
	//	<< " factor=" << factor
	//	<< " before=" << before
	//	<< " after=" << after
	//	<< " delta=" << delta
	//	<< std::endl;

	//// Apply oxidation reduction
	//CH4_Soilcol_total = after;

	// ------ AFTER oxidation ------
	float factor = std::exp(-z_oxic / CH4_TAU_OXID);
	float CH4_after = SoilCol_CH4 * factor;  // 通量（氧化后）

	//// ---- DEBUG PRINT (AFTER OXIDATION) ----
	//std::cout << std::fixed << std::setprecision(7)
	//	<< "[WETMETH] cell=" << cell_idx
	//	<< " CH4_after_oxid=" << CH4_after
	//	<< std::endl;

	return CH4_after;

	//// ---- DEBUG PRINT (AFTER OXIDATION) ----
	//std::cout << std::fixed << std::setprecision(7)
	//	<< "[WETMETH] cell=" << cell_idx
	//	<< " CH4_after_oxid=" << CH4_Soilcol_total
	//	<< std::endl;

	/*return CH4_Soilcol_total;*/
}

// Calculate oxic zone depth based on soil saturation
// According to WETMETH model, oxic depth is calculated as the depth where
// soil saturation drops below a certain threshold, plus transition zone
float SoilCol::calculate_oxic_depth() {

	float oxic_depth = 0.0f;
	bool found_saturated_layer = false;
	
	// Calculate oxic depth from surface downward
	// Use 1.0 as threshold for soil saturation
	for (int i = 0; i < num_layers; i++) {

		// If this layer is saturated, stop here
		if (soil_saturation[i] >= 0.99999) { // Saturation >= 1.0 is considered saturated
			found_saturated_layer = true;
			break;
		}
		// Convert thickness from mm to m for consistent units
		oxic_depth += layer_thickness[i] / 1000.0f; 
	}
	
	// Add transition zone thickness only if saturated layer was found
	// If all layers are non-saturated, oxic depth is just the total soil thickness
	if (found_saturated_layer) {
		oxic_depth += CH4_Z_OATZ;
	}

	
	//float total_depth = 0.0f;  // total soil column thickness (m)
	//float unsat_sum = 0.0f;    // sum of UNSAT layer thickness (m)

	//// Traverse all layers
	//for (int i = 0; i < num_layers; i++) {

	//	float thk_m = layer_thickness[i] / 1000.0f;   // mm -> m
	//	total_depth += thk_m;

	//	// UNSAT thickness accumulation
	//	if (soil_saturation[i] < 0.99999) {
	//		unsat_sum += thk_m;
	//	}
	//}

	//// Case 1: all layers saturated -> only transition zone
	//if (unsat_sum <= 0.0f) {
	//	oxic_depth = CH4_Z_OATZ;
	//}
	//// Case 2: all layers unsaturated -> total soil thickness (no extra 0.05 m)
	//else if (fabs(unsat_sum - total_depth) < 1e-7f) {
	//	oxic_depth = total_depth;
	//}
	//// Case 3: mixed -> sum of UNSAT thickness + transition zone
	//else {
	//	oxic_depth = unsat_sum + CH4_Z_OATZ;

	//	// optional safety cap (uncomment if you want to prevent exceeding total depth)
	//	// if (oxic_depth > total_depth) oxic_depth = total_depth;
	//}

	
	return oxic_depth;
}

// Main module constructor
CH4_WETMETH::CH4_WETMETH() :
	m_nCells(-1),
	m_maxSoilLyrs(-1),
	m_nSoilLyrs(nullptr),
	m_area(nullptr),
	m_layer_thickness(nullptr),
	m_soil_water_storage(nullptr),
	m_soil_saturated(nullptr),
	m_soilWP(nullptr),
	m_soilPor(nullptr),
	// m_Soc(nullptr), // commented out, using m_Soc_kg_ha instead
	m_Soc_kg_ha(nullptr),
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
	m_handWtrDep(nullptr),
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
	if (m_area != nullptr) Release1DArray(m_area);
	if (m_nSoilLyrs != nullptr) Release1DArray(m_nSoilLyrs);
	if (m_layer_thickness != nullptr) Release2DArray(m_nCells, m_layer_thickness);
	if (m_soil_water_storage != nullptr) Release2DArray(m_nCells, m_soil_water_storage);
	if (m_soil_saturated != nullptr) Release2DArray(m_nCells, m_soil_saturated);
	if (m_soilWP != nullptr) Release2DArray(m_nCells, m_soilWP);
	if (m_soilPor != nullptr) Release2DArray(m_nCells, m_soilPor);
	// if (m_Soc != nullptr) Release2DArray(m_nCells, m_Soc); // commented out, using m_Soc_kg_ha instead
	if (m_Soc_kg_ha != nullptr) Release2DArray(m_nCells, m_Soc_kg_ha);
	if (m_Tsoil != nullptr) Release2DArray(m_nCells, m_Tsoil);
	if (m_SoilCols != nullptr) Release1DArray(m_SoilCols);
	if (m_handWtrDep != nullptr) Release1DArray(m_handWtrDep);
	if (m_satL1 != nullptr) Release1DArray(m_satL1);
	if (m_satL2 != nullptr) Release1DArray(m_satL2);
	if (m_satL3 != nullptr) Release1DArray(m_satL3);
	if (m_satL4 != nullptr) Release1DArray(m_satL4);
	if (m_satL5 != nullptr) Release1DArray(m_satL5);
	if (m_satL6 != nullptr) Release1DArray(m_satL6);
	if (m_satL7 != nullptr) Release1DArray(m_satL7);
	if (m_handWtrDep != nullptr) Release1DArray(m_handWtrDep);
	if (m_infil != nullptr) Release1DArray(m_infil);
	if (m_netPcp != nullptr) Release1DArray(m_netPcp);
	if (m_soilPerco != nullptr) Release2DArray(m_nCells, m_soilPerco);
	if (m_subSurfRf != nullptr) Release2DArray(m_nCells, m_subSurfRf);
	if (m_sd != nullptr) Release1DArray(m_sd);
	if (m_IntcpET != nullptr) Release1DArray(m_IntcpET);
	if (m_exsPcp != nullptr) Release1DArray(m_exsPcp);
	//if (m_ifluQ2Rch != nullptr) Release1DArray(m_ifluQ2Rch);
	if (m_soilET != nullptr) Release1DArray(m_soilET);
}

void CH4_WETMETH::SetValue(const char* key, float value) {

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
	} else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
		m_handWtrDep = data;    // Water depth of each hand(m)
	} else if (StringMatch(s, VAR_INFIL)) {
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
		m_soil_saturated = data;       // Soil layer saturated water capacity (mm H2O)
	} else if (StringMatch(sk, VAR_SOILT)) {
		m_Tsoil = data;               // Soil temperature (°C)
	// } else if (StringMatch(sk, VAR_SOL_OM)) {
	//	m_Soc = data;                // Soil organic matter percentage (%) (commented out, using VAR_SOL_WOC instead)
	} else if (StringMatch(sk, VAR_SOL_WOC)) {
		m_Soc_kg_ha = data;         // Soil organic carbon content (kg/ha)
	} else if (StringMatch(sk, VAR_SOL_WPMM)) {
		m_soilWP = data;          //  Water content of soil at -1.5 MPa (wilting point)
	} else if (StringMatch(sk, VAR_POROST)) {
		m_soilPor = data;       // Porosity mm/mm
	} else if (StringMatch(sk, VAR_PERCO)) {
		m_soilPerco = data;
	} else if (StringMatch(sk, VAR_SSRU)) {
		m_subSurfRf = data;
	} else {
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
	// if (m_Soc == nullptr) {
	//	throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil organic matter data is not set.");
	// } // commented out, using m_Soc_kg_ha instead
	if (m_Soc_kg_ha == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Soil organic carbon data is not set.");
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
	if (m_handWtrDep == nullptr) {
		throw ModelException(MID_CH4_WETMETH, "CheckInputData", "Depth of each hand data is not set.");
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

	// Process each cell
	for (int i = 0; i < m_nCells; i++) {

		// Set soil column properties
		m_SoilCols[i].area_Soilcol = m_area[i];

		// Get actual number of soil layers for this cell
		int actual_layers = (int)m_nSoilLyrs[i];
		m_SoilCols[i].num_layers = actual_layers;

		//// ✅✅✅ 你的打印代码就放在这儿（拷贝 j 循环之前）
		//if (i == 0) { // 或者 (i>=629 && i<=644)
		//	std::cout << "cell=" << i << " layers=" << actual_layers << "\n";
		//	for (int j = 0; j < actual_layers; j++) {
		//		std::cout << "  L" << j
		//			<< " thk=" << m_layer_thickness[i][j]
		//			<< " por=" << m_soilPor[i][j]
		//			<< " st=" << m_soil_water_storage[i][j]
		//			<< " wp=" << m_soilWP[i][j]
		//			<< " satcap=" << m_soil_saturated[i][j]
		//			<< "\n";
		//	}
		//}

		// Copy soil layer data (only for actual layers)
		for (int j = 0; j < actual_layers; j++) {
			m_SoilCols[i].layer_thickness[j] = m_layer_thickness[i][j];
			m_SoilCols[i].soil_water_storage[j] = m_soil_water_storage[i][j];
			m_SoilCols[i].soil_saturated[j] = m_soil_saturated[i][j];
			m_SoilCols[i].m_soilWP[j] = m_soilWP[i][j];
			m_SoilCols[i].m_soilPor[j] = m_soilPor[i][j];

			// Convert kg/ha to kg C/m³ for WETMETH model
			// kg/ha / (thickness_m * 10000) = kg C/m³
			float thickness_m = m_layer_thickness[i][j] / 1000.0f;  // Convert mm to m
			m_SoilCols[i].Soc[j] = m_Soc_kg_ha[i][j] / (thickness_m * 10000.0f);
			m_SoilCols[i].T_soil[j] = m_Tsoil[i][j];
		}

		//// ✅ 在这里加 debug（拷贝完输入后、算甲烷前）
		//if (i == 0 || (i >= 629 && i <= 644)) {
		//	std::cout << std::fixed << std::setprecision(7);
		//	std::cout << "cell=" << i << " layers=" << actual_layers << "\n";
		//	for (int j = 0; j < actual_layers; j++) {
		//		std::cout << "  L" << j
		//			<< " thk=" << m_layer_thickness[i][j]
		//			<< " por=" << m_soilPor[i][j]
		//			<< " st=" << m_soil_water_storage[i][j]
		//			<< " wp=" << m_soilWP[i][j]
		//			<< " satcap=" << m_soil_saturated[i][j]
		//			<< " soc_kg_ha=" << m_Soc_kg_ha[i][j]
		//			<< " T=" << m_Tsoil[i][j]
		//			<< "\n";
		//	}
		//}

		//// ✅ DEBUG: 打印 m_handWtrDep（只打印 629-644）
		//if (i >= 629 && i <= 644) {
		//	std::cout << std::fixed << std::setprecision(7)
		//		<< " Cell=" << i
		//		<< " handWtrDep(m)=" << (m_handWtrDep ? m_handWtrDep[i] : NAN)
		//		<< " handWtrDep(mm)=" << (m_handWtrDep ? (m_handWtrDep[i] * 1000.0f) : NAN)
		//		<< " infil(mm)=" << (m_infil ? m_infil[i] : NAN)
		//		<< " netPcp(mm)=" << (m_netPcp ? m_netPcp[i] : NAN)
		//		<< " sd(mm)=" << (m_sd ? m_sd[i] : NAN)
		//		<< " exsPcp(mm)=" << (m_exsPcp ? m_exsPcp[i] : NAN)
		//		<< " IntcpET(mm)=" << (m_IntcpET ? m_IntcpET[i] : NAN)
		//		<< " soilET(mm)=" << (m_soilET ? m_soilET[i] : NAN)
		//		<< std::endl;
		//}

		//// ✅ DEBUG: PERCO 渗漏量（mm）按 "PERCO;cell;v1,v2,..." 输出
		//if (i >= 629 && i <= 644) {
		//	std::cout << std::fixed << std::setprecision(7);
		//	std::cout << "soilPerco;" << i << ";";
		//	for (int j = 0; j < actual_layers; j++) {
		//		if (j > 0) std::cout << ",";
		//		float v = (m_soilPerco ? m_soilPerco[i][j] : NAN);
		//		std::cout << v;
		//	}
		//	std::cout << std::endl;
		//}

		//// ✅ DEBUG: SSRF 侧向壤中流（mm）按 "SSRF;cell;v1,v2,..." 输出
		//if (i >= 629 && i <= 644) {
		//	std::cout << std::fixed << std::setprecision(7);
		//	std::cout << "subSurfRf;" << i << ";";
		//	for (int j = 0; j < actual_layers; j++) {
		//		if (j > 0) std::cout << ",";
		//		float v = (m_subSurfRf ? m_subSurfRf[i][j] : NAN);
		//		std::cout << v;
		//	}
		//	std::cout << std::endl;
		//}

		float CH4_before = 0.0f;
		float CH4_after = 0.0f;

		// after (flux)
		CH4_after = m_SoilCols[i].SoilColMethane(i, CH4_before);

		// before (production) -> SoilCol_CH4 在 SoilColMethane() 内已经写好
		CH4_before = m_SoilCols[i].SoilCol_CH4;

		//// ===== DEBUG: 打印 CH4_before / CH4_after =====
		//if (i >= 629 && i <= 644) {
		//	std::cout << std::fixed << std::setprecision(7)
		//		<< "CH4;" << i
		//		<< ";before=" << CH4_before
		//		<< ";after=" << CH4_after
		//		<< std::endl;
		//}

		m_satL1[i] = (actual_layers >= 1) ? m_SoilCols[i].soil_saturation[0] : 0.0f;
		m_satL2[i] = (actual_layers >= 2) ? m_SoilCols[i].soil_saturation[1] : 0.0f;
		m_satL3[i] = (actual_layers >= 3) ? m_SoilCols[i].soil_saturation[2] : 0.0f;
		m_satL4[i] = (actual_layers >= 4) ? m_SoilCols[i].soil_saturation[3] : 0.0f;
		m_satL5[i] = (actual_layers >= 5) ? m_SoilCols[i].soil_saturation[4] : 0.0f;
		m_satL6[i] = (actual_layers >= 6) ? m_SoilCols[i].soil_saturation[5] : 0.0f;
		m_satL7[i] = (actual_layers >= 7) ? m_SoilCols[i].soil_saturation[6] : 0.0f;


		m_P_soilcol[i] = CH4_before;
		m_P_soilcol_flux[i] = CH4_after;

		m_total_CH4 += CH4_after;
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


