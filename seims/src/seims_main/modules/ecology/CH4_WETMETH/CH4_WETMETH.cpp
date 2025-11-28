#include "CH4_WETMETH.h"
#include "text.h"
#include <cmath>

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
	m_soilWP(nullptr){
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

// Calculate saturation ratio for each soil layer: saturation =
void SoilCol::calculate_soil_saturation() {
	// Use actual layer count from member variable for calculation
	for (int i = 0; i < num_layers; i++) {
		//std::cout << "Debug - Layer " << i << ": m_soilPor[" << i << "] = " << m_soilPor[i]
			//<< ", layer_thickness[" << i << "] = " << layer_thickness[i]
			//<< ", soil_water_storage[" << i << "] = " << soil_water_storage[i]
			//<< ", m_soilWP[" << i << "] = " << m_soilWP[i] << std::endl;
		if (soil_saturated[i] > 0.0f) {
			// Calculate saturation ratio, limit between 0-1
//            soil_saturation[i] = std::min(1.0f, std::max(0.0f, soil_water_storage[i] / soil_saturated[i]));
			//float ratio = soil_water_storage[i] / soil_saturated[i];
			//float ratio = ((soil_water_storage[i] + m_soilWP[i]) / layer_thickness[i]) / m_soilPor[i];
			//float ratio = soil_water_storage[i] / (layer_thickness[i] * m_soilPor[i]);
			float ratio = (soil_water_storage[i] + m_soilWP[i]) / (layer_thickness[i] * m_soilPor[i]);
			soil_saturation[i] = (ratio > 1.0f) ? 1.0f : ((ratio < 0.0f) ? 0.0f : ratio);
			// 打印最终的饱和度值
			//std::cout << "Layer " << i << " final saturation: " << soil_saturation[i] << std::endl;
			// 只打印饱和度为1的值
			/*if (soil_saturation[i] >= 0.9f) {
				std::cout << "Layer " << i << " is fully saturated: " << soil_saturation[i]
					<< " (ratio: " << ratio << ")" << std::endl;
			}*/
		} else {
			soil_saturation[i] = 0.0f;
		}
	}
}

float SoilCol::SoilColMethane()
{
	// Reset CH4 production for current soil column
	SoilCol_CH4 = 0.0f;
	float CH4_Soilcol_total = 0.0f;
	
	// Calculate soil saturation for all layers first
	calculate_soil_saturation();
	
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
        float Pi = CH4_R * Soc[i] * soil_saturation[i] *
                   std::exp((T_kelvin - CH4_T0) / 10 * std::log(CH4_Q10)) *
                   std::exp(-cumulative_depth[i] / CH4_TAU_PROD)*1e9f; // Use stored cumulative_depth

        // Total CH4 production = rate * layer thickness
        // Pi: kg C m⁻³ s⁻¹, layer_thickness: mm → m, result: μg C m⁻² s⁻¹
        CH4_Soilcol_total += (float)(Pi * (layer_thickness[i] / 1000.0f));
		//std::cout << "CH4: " << CH4_Soilcol_total << std::endl;

	}


	// Calculate oxic zone depth
	z_oxic = calculate_oxic_depth();
	//std::cout << "oxic deep = " <<  z_oxic << std::endl;

	// Apply oxidation reduction based on oxic zone depth
	// area_Soilcol: m², CH4_Soilcol_total: μg C m⁻² s⁻¹, result: μg C s⁻¹
	CH4_Soilcol_total = CH4_Soilcol_total * exp(-z_oxic/CH4_TAU_OXID);
	//std::cout << "E = " << CH4_Soilcol_total << std::endl;
	//std::cout << "SOIL: " << CH4_TAU_OXID << std::endl;
	//std::cout << "After oxidation: " << CH4_Soilcol_total << std::endl;
	return CH4_Soilcol_total;
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
	m_total_CH4(0.0f)
{

}

CH4_WETMETH::~CH4_WETMETH() {
	if (m_P_soilcol != nullptr) Release1DArray(m_P_soilcol);
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
	}
	else if (StringMatch(sk, VAR_SOL_WOC)) {
		m_Soc_kg_ha = data;         // Soil organic carbon content (kg/ha)
	}
	else if (StringMatch(sk, VAR_SOL_WPMM)) {
		m_soilWP = data;          //  Water content of soil at -1.5 MPa (wilting point)
	}
	else if (StringMatch(sk, VAR_POROST)) {
		m_soilPor = data;       // Porosity mm/mm
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
	
	return true;
}

void CH4_WETMETH::InitialOutputs() {
	if (m_SoilCols == nullptr) {
		m_SoilCols = new SoilCol[m_nCells];
		Initialize1DArray(m_nCells, m_P_soilcol, 0.0f);
		
		// Initialize each soil column
		for (int i = 0; i < m_nCells; i++) {
			m_SoilCols[i].Initialize(m_maxSoilLyrs);
		}
	}
	m_total_CH4 = 0.0f;
}

int CH4_WETMETH::Execute() {
	// Validate input data
	CheckInputData();

	// Initialize outputs if not already done
	InitialOutputs();

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
			// Convert kg/ha to kg C/m³ for WETMETH model
			// kg/ha / (thickness_m * 10000) = kg C/m³
			float thickness_m = m_layer_thickness[i][j] / 1000.0f; // Convert mm to m
			m_SoilCols[i].Soc[j] = m_Soc_kg_ha[i][j] / (thickness_m * 10000.0f);    // kg/ha ÷ (thick_m × 10000) = kg C/m³
			m_SoilCols[i].T_soil[j] = m_Tsoil[i][j];
		}

		// Calculate CH4 production for this soil column
		m_P_soilcol[i] = m_SoilCols[i].SoilColMethane();
		m_total_CH4 += m_P_soilcol[i];
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
	} else {
		throw ModelException(MID_CH4_WETMETH, "GetValue", "Result " + sk + " does not exist.");
	}  
}

void CH4_WETMETH::Get1DData(const char* key, int* n, float** data) {
	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, VAR_CH4_PRODUCTION)) {
		*data = m_P_soilcol; 
		*n = m_nCells;
	} else {
		throw ModelException(MID_CH4_WETMETH, "Get1DData", "Result " + sk + " does not exist.");
	}
}

void CH4_WETMETH::Get2DData(const char* key, int* n, int* col, float*** data) {

}


