#include "CH4_LPJGUESS.h"
#include "text.h"
#include <cmath>
#include <algorithm>

// Initialize all member variables, set pointers to nullptr, set values to 0
CH4_LPJGUESS::CH4_LPJGUESS() :
	m_nCells(-1),
	m_maxSoilLyrs(-1),
	m_co2Conc(NODATA_VALUE),
	m_area(nullptr),
	m_Tsoil(nullptr),
	m_nSoilLyrs(nullptr),
	m_rootFraction(nullptr),
	m_layer_thickness(nullptr),
	m_soil_water_storage(nullptr),
	m_soilPor(nullptr),
	m_soilWP(nullptr),
	m_Fair(nullptr),
	m_soilIceSto(nullptr),
	m_anoxic(nullptr),
	m_CH4_prod(nullptr),
	m_CH4_oxid(nullptr),
	m_D_CH4_water(nullptr),
	m_D_CO2_water(nullptr),
	m_D_O2_water(nullptr),
	m_D_CH4_air(nullptr),
	m_D_CO2_air(nullptr),
	m_D_O2_air(nullptr),
	m_dCH4(nullptr),
	m_dCO2(nullptr),
	m_dO2(nullptr),
	m_D_CH4(nullptr),
	m_D_CO2(nullptr),
	m_D_O2(nullptr),
	m_k_O2(nullptr),
	m_k_CH4(nullptr),
	m_k_CO2(nullptr),
	m_Ceq_O2(nullptr),
	m_Ceq_CH4(nullptr),
	m_Ceq_CO2(nullptr),
	m_C(nullptr),
	m_Cgas(nullptr),
	m_total_volume_water(nullptr),
	m_Frac_water(nullptr),
	m_Frac_ice(nullptr),
	m_Frac_water_belowpwp(nullptr),
	m_Dz_metre(nullptr),
	m_volume_liquid_water(nullptr),
	m_C_init(nullptr),
	m_C_last(nullptr) {
	
}

CH4_LPJGUESS::~CH4_LPJGUESS() {

	if (m_area != nullptr) Release1DArray(m_area);
	if (m_nSoilLyrs != nullptr) Release1DArray(m_nSoilLyrs);
	if (m_k_O2 != nullptr) Release1DArray(m_k_O2);
	if (m_k_CH4 != nullptr) Release1DArray(m_k_CH4);
	if (m_k_CO2 != nullptr) Release1DArray(m_k_CO2);
	if (m_Ceq_O2 != nullptr) Release1DArray(m_Ceq_O2);
	if (m_Ceq_CH4 != nullptr) Release1DArray(m_Ceq_CH4);
	if (m_Ceq_CO2 != nullptr) Release1DArray(m_Ceq_CO2);


	if (m_Tsoil != nullptr) Release2DArray(m_nCells, m_Tsoil);
	if (m_layer_thickness != nullptr) Release2DArray(m_nCells, m_layer_thickness);
	if (m_soilWP != nullptr) Release2DArray(m_nCells, m_soilWP);
	if (m_soilPor != nullptr) Release2DArray(m_nCells, m_soilPor);
	if (m_rootFraction != nullptr) Release2DArray(m_nCells, m_rootFraction);
	if (m_soilIceSto != nullptr) Release2DArray(m_nCells, m_soilIceSto);
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


	// Initialize a one-dimensional array
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
		m_nSoilLyrs = data;  // Actual number of soil layers for each cell
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


void CH4_LPJGUESS::InitRootFractions(int i) {

	std::cout << "开始计算根生物量比例..." << std::endl;

	// Wania et al. (2010) parameters
	float sumrootfrac = 0.f;         // cumulative root biomass ratio
	float cumulative_depth = 0.f;    // cumulative depth (cm)

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		// Calculate the middle depth of the current layer
		float layer_thickness_cm = m_layer_thickness[i][j] / 10.f;      // mm to cm
		float layermiddepth = cumulative_depth + layer_thickness_cm / 2.f;
		std::cout << "层 " << j << ": 厚度=" << layer_thickness_cm
		 << "cm, 中间深度=" << layermiddepth << "cm" << std::endl;

		if (j == m_nSoilLyrs[i] - 1) {
			// The final layer: Ensure that the total equals 1
			m_rootFraction[i][j] = 1.f - sumrootfrac;
			std::cout << "最后一层，根生物量比例设置为: " << m_rootFraction[i][j] << std::endl;
		}
		else {
			// Calculation of root biomass proportion using the exponential decay model 
			m_rootFraction[i][j] = exp(-layermiddepth / CH4_ROOT_DECAY_COEFF) / CH4_ROOT_NORM_CONST;
			std::cout << "根生物量比例: " << m_rootFraction[i][j] << std::endl;
		}

		sumrootfrac += m_rootFraction[i][j];
		cumulative_depth += layer_thickness_cm;   // Update cumulative depth
		std::cout << "单元格 " << i << " 根生物量比例总和: " << sumrootfrac << std::endl;
	}	
}


void CH4_LPJGUESS::InitFracAir(int i) {

	std::cout << "开始计算土壤空气体积分数..." << std::endl;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// 获取土壤层参数
		float porosity = m_soilPor[i][j];                       // Porosity mm/mm
		float water_content = m_soil_water_storage[i][j];       // Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
		float wilting_point = m_soilWP[i][j];                   // water content of soil at -1.5 MPa (wilting point) (mm)
		float layer_thickness = m_layer_thickness[i][j];        // Thickness of each soil layer (mm) - from VAR_SOILTHICK
		std::cout << "  层 " << j << ": 孔隙度=" << porosity
							<< ", 含水量=" << water_content << "mm"
							<< ", 萎蔫点=" << wilting_point << "mm"
							<< ", 厚度=" << layer_thickness << "mm" << std::endl;

		// 计算空气体积分数: Frac_air = 孔隙度 - (当前土壤含水量 + 萎蔫点含水量) / 土壤层厚度
		float water_volume_fraction = (water_content + wilting_point) / layer_thickness;
		m_Fair[i][j] = porosity - water_volume_fraction;

		// 确保结果在合理范围内 [0, porosity]
		if (m_Fair[i][j] < 0.0) {
			m_Fair[i][j] = 0.0;
			std::cout << "  警告: 空气体积分数为负，已调整为0" << std::endl;
		}
		else if (m_Fair[i][j] > porosity) {
			m_Fair[i][j] = porosity;
			std::cout << "  警告: 空气体积分数超过孔隙度，已调整为孔隙度值" << std::endl;
		}
		std::cout << "  空气体积分数: " << m_Fair[i][j] << std::endl;
	}
	std::cout << "单元格 " << i << " Frac_air计算完成" << std::endl;
}

////void CH4_LPJGUESS::InitFracAir() {
////
////	std::cout << "开始计算土壤空气体积分数..." << std::endl;
////	for (int i = 0; i < m_nCells; i++) {
////		if (m_nSoilLyrs[i] <= 0)  continue;
////
////		std::cout << "单元格 " << i << " 有 " << m_nSoilLyrs[i] << " 个土壤层" << std::endl;
////
////		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
////
////			// 获取土壤层参数
////			float porosity = m_soilPor[i][j];                       // Porosity mm/mm
////			float water_content = m_soil_water_storage[i][j];       // Water storage in each soil layer (mm H2O) - from VAR_SOL_ST
////			float ice_content = m_soilIceSto[i][j];
////			float wilting_point = m_soilWP[i][j];                   // water content of soil at -1.5 MPa (wilting point) (mm)
////			float layer_thickness = m_layer_thickness[i][j];        // Thickness of each soil layer (mm) - from VAR_SOILTHICK
////
////			std::cout << "  层 " << j << ": 孔隙度=" << porosity
////				<< ", 含水量=" << water_content << "mm"
////				<< ", 含冰量=" << ice_content << "mm"
////				<< ", 萎蔫点=" << wilting_point << "mm"
////				<< ", 厚度=" << layer_thickness << "mm" << std::endl;
////
////
////			// 计算空气体积分数: Frac_air = 孔隙度 - (当前土壤含水量 + 萎蔫点含水量 + 当前土壤含冰量) / 土壤层厚度
////			float water_volume_fraction = (water_content + wilting_point + ice_content) / layer_thickness;
////			m_Fair[i][j] = porosity - water_volume_fraction;
////
////			// 确保结果在合理范围内 [0, porosity]
////			if (m_Fair[i][j] < 0.0) {
////				m_Fair[i][j] = 0.0;
////				std::cout << "  警告: 空气体积分数为负，已调整为0" << std::endl;
////			}
////			else if (m_Fair[i][j] > porosity) {
////				m_Fair[i][j] = porosity;
////				std::cout << "  警告: 空气体积分数超过孔隙度，已调整为孔隙度值" << std::endl;
////			}
////
////			std::cout << "  空气体积分数: " << m_Fair[i][j] << std::endl;
////		}
////		std::cout << "单元格 " << i << " Frac_air计算完成" << std::endl;
////	}
////
////	std::cout << "土壤空气体积分数计算完成" << std::endl;
////}
//

void CH4_LPJGUESS::InitAnoxic(int i) {

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// 获取已计算的空气体积分数
		float Frac_air = m_Fair[i][j];
		float porosity = m_soilPor[i][j];

		std::cout << "  层 " << j << ": 空气体积分数=" << Frac_air
			<< ", 孔隙度=" << porosity << std::endl;

		// 计算厌氧条件程度: anoxic = 1.0 - Frac_air - Fgas
		float anoxic = 1.0f - Frac_air - Fgas;

		// 确保结果在合理范围内 [0, 1]
		if (anoxic < 0.0f) {
			anoxic = 0.0f;
			std::cout << "警告: 厌氧程度为负，已调整为0" << std::endl;
		}
		else if (anoxic > 1.0f) {
			anoxic = 1.0f;
			std::cout << "警告: 厌氧程度超过1，已调整为1" << std::endl;
		}

		// 存储结果
		m_anoxic[i][j] = anoxic;

		std::cout << "厌氧程度 (anoxic): " << m_anoxic[i][j]
			<< " (1.0 - " << Frac_air << " - " << Fgas << ")" << std::endl;
	}
}

void CH4_LPJGUESS::MethaneProduction(int i) {

	//std::cout << "开始计算甲烷生产量 (CH4_prod)..." << std::endl;

	//for (int i = 0; i < m_nCells; i++) {
	//	if (m_nSoilLyrs[i] <= 0) continue;

	//	std::cout << "单元格 " << i << " 有 " << m_nSoilLyrs[i] << " 个土壤层" << std::endl;

	//	// 获取每日分解量（假设已计算）
	//	float Rh = m_Rh[i] * G_PER_KG; // 转换为gC/m²

	//	std::cout << "  每日分解量 ( Rh): " << Rh << " gC/m²" << std::endl;

	//	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

	//		// 获取相关参数
	//		float anoxic = m_anoxic[i][j];
	//		float rootfrac = m_rootFraction[i][j];
	//		float Frac_water = m_soil_water_storage[i][j];
	//		float Frac_water_belowpwp = m_soilWP[i][j];

	//		std::cout << "  层 " << j << ": 厌氧程度=" << anoxic
	//			<< ", 根系比例=" << rootfrac
	//			<< ", 可用水=" << Frac_water
	//			<< ", 不可用水=" << Frac_water_belowpwp << std::endl;

	//		// 计算总液态水含量
	//		float total_water = Frac_water + Frac_water_belowpwp;

	//		// 检查水分条件是否满足甲烷产生
	//		float CH4_prod = 0.0f;

	//		if (total_water < 0.1f) {
	//			CH4_prod = 0.0f;
	//			std::cout << "    -> 水分不足，不产生甲烷" << std::endl;
	//		}
	//		else {
	//			// 计算甲烷生产量: CH4_prod = anoxic * CH4toCO2_peat * rootfrac * drh
	//			CH4_prod = anoxic * CH4toCO2_PEAT * rootfrac * drh;

	//			// 确保结果非负
	//			if (CH4_prod < 0.0f) {
	//				CH4_prod = 0.0f;
	//				std::cout << "    -> 警告: 甲烷产量为负，已调整为0" << std::endl;
	//			}

	//			std::cout << "    -> 甲烷产量: " << CH4_prod << " gC/m²day⁻¹"
	//				<< " (计算: " << anoxic << " * " << CH4toCO2_PEAT
	//				<< " * " << rootfrac << " * " << drh << ")" << std::endl;
	//		}

	//		// 存储结果
	//		m_CH4_prod[i][j] = CH4_prod;

	//		// 输出产量评估
	//		if (CH4_prod > 0.1f) {
	//			std::cout << "    -> 高产甲烷条件" << std::endl;
	//		}
	//		else if (CH4_prod > 0.01f) {
	//			std::cout << "    -> 中等甲烷产量" << std::endl;
	//		}
	//		else if (CH4_prod > 0.0f) {
	//			std::cout << "    -> 低甲烷产量" << std::endl;
	//		}
	//		else {
	//			std::cout << "    -> 无甲烷产生" << std::endl;
	//		}
	//	}

	//	// 计算该单元格的总甲烷产量
	//	float TOTAL_CH4_PROD = 0.0f;        //gC/m²day⁻¹
	//	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
	//		TOTAL_CH4_PROD += m_CH4_prod[i][j];
	//	}
	//	std::cout << "单元格 " << i << " 总甲烷产量: " << TOTAL_CH4_PROD << " gC/m²" << std::endl;
	//	std::cout << "单元格 " << i << " CH4_prod计算完成" << std::endl;
	//}

	//std::cout << "甲烷生产量计算完成" << std::endl;
}

void CH4_LPJGUESS::MethaneOxidation(int i) {

	//std::cout << "开始计算甲烷氧化..." << std::endl;

	//for (int i = 0; i < m_nCells; i++) {
	//	if (m_nSoilLyrs[i] <= 0) continue;

	//	std::cout << "单元格 " << i << " 有 " << m_nSoilLyrs[i] << " 个土壤层" << std::endl;

	//	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

	//		// 获取甲烷浓度
	//		float CH4_concentration = m_CH4_prod[i][j];               // 甲烷浓度（gC/m²）
	//		float atomiccmass = 12.0f;                                // 碳原子质量（g/mol）
	//		std::cout << "  层 " << j << ": CH4浓度=" << CH4_concentration << " gC/m²" << std::endl;

	//		// 将甲烷从质量单位转换为物质的量单位
	//		float CH4_mol = CH4_concentration / atomiccmass;           // gC/m² → mol/m²
	//		std::cout << "CH4物质的量: " << CH4_mol << " mol/m²" << std::endl;

	//		// 计算甲烷氧化量
	//		float CH4_oxidized = CH4_mol;
	//		std::cout << "甲烷氧化量: " << CH4_oxidized << " mol/m²" << std::endl;

	//		// 更新甲烷浓度 (减去被氧化的部分)
	//		float remaining_CH4_mol = CH4_mol - CH4_oxidized;
	//		m_CH4_prod[i][j] = remaining_CH4_mol * atomiccmass;         // mol/m² → gC/m²
	//		std::cout << "剩余CH4: " << m_CH4_prod[i][j] << " gC/m²" << std::endl;

	//		// 存储氧化量用于输出或分析
	//		m_CH4_oxid[i][j] = CH4_oxidized * atomiccmass;              // 存储为gC/m²
	//	}
	//}
}

void CH4_LPJGUESS::tridiag(int n, float* a, float* b, float* c, float* r, float* u) {

	// Tridiagonal system solver from Numerical Recipes.
	float* gam = new float[n];

	// 首元素
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
	if (nLyr <= 1) return;

	// 取出二维数组的当前单元指针
	float* dz = m_Dz_metre[i];      // 土层厚度

	// 顶层边界浓度
	float surf_conc = conc[0];

	// 扩散时间步（day）
	float dt = Dt_gas;

	// A fill-in for layers above layer0.
	float MISSING_VALUE = -9999.0f;

	// Layer counters: note that there are two different layer counting
    // schemes used, one for the input and output parameters (vectors of
    // length NLAYERS) and one for the values used in the Crank-Nicholson
    // solver (vectors of length active_layers)
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
	int layer0 = 0;
	active_layers = nLyr - layer0;

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

		if (layer == nLyr - 1)
			dplus = 0.0f;          // BC2 - Bottom layer diffusion clamped to 0
		else
			dplus = 0.5f * (Di[layer] + Di[layer + 1]);

		// D-

		if (layer == layer0)
			dminus = Di[layer];  // top layer
		else
			dminus = 0.5f * (Di[layer] + Di[layer - 1]);   // soil layers	

		// --- HERE ---
		if (layer < nLyr) {
			// all soil layers
			dzhere = dz[layer];
			cohere = conc[layer];
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
		else if (layer < nLyr) {
			// all soil layers and the first padding layer
			dzminus = dz[layer - 1];
			cominus = conc[layer - 1];
		}

		float dz_factor = 0.25f * (dzplus + 2.0f * dzhere + dzminus);
		float Cplus = dplus * dt / dz_factor / (dzplus + dzhere);
		float Cp_minus = dminus * dt / dz_factor / (dzhere + dzminus);

		// Fill in matrix diagonal and off-diagonal elements.

		// DIAG
		if (lidx == 1)
			diag[0] = 1.0f;                 // BC1 - top layer should be (1,0,...,0)
		else
			diag[lidx - 1] = 1.0f + Cplus + Cp_minus;

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
		if (lidx == 1)
			rhs[0] = surf_conc;
		else if (lidx == active_layers) // Cplus == 0 here anyway
			rhs[lidx - 1] = (1.0f - Cp_minus) * cohere + Cp_minus * cominus;
		else
			rhs[lidx - 1] = (1.0f - Cplus - Cp_minus) * cohere + Cplus * coplus + Cp_minus * cominus;
	} // end for

	//   SOLVE TRIDIAGONAL SYSTEM
	tridiag(active_layers, left, diag, right, rhs, solution);

	// FORMAT OUTPUT PARAMETERS

	// Transfer the solution to the concentration array.
	for (int j = 0; j < active_layers; j++) {

		if (j < layer0)
			conc[j] = MISSING_VALUE;
		else
			conc[j] = solution[j - layer0];
	}

	delete[] diag;
	delete[] left;
	delete[] right;
	delete[] rhs;
	delete[] solution;
}


// Calculate and return diffusivities, in units of m2 d-1
// Called each day
// See Wania et al. (2010) - Sec 2.5
void CH4_LPJGUESS::Calculate_Gas_Diffusivities(int i) {

	//std::cout << "开始计算气体扩散系数..." << std::endl;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// 获取当前层的参数
		float layerT = m_Tsoil[i][j];                   // 土壤温度 (°C)
		float frac_air = m_Fair[i][j];                  // 空气体积分数
		float thickness_mm = m_layer_thickness[i][j];   // 土壤层厚度 (mm)
		float thickness_m = thickness_mm / 1000.f;      // 转换为m
		float layer_porosity = m_soilPor[i][j];         // 当前层孔隙度

		std::cout << "  层 " << j << ": 温度=" << layerT << "°C, 空气分数=" << frac_air
			<< ", 厚度=" << thickness_mm << "mm (" << thickness_m << "m)" << std::endl;

		// 计算基础扩散系数 (cm²/s)
		// 空气中扩散系数 - Wania et al. (2010), Eqn. 10
		m_D_CH4_air[i][j] = 0.1875f + 0.00130f * layerT;
		m_D_CO2_air[i][j] = 0.1325f + 0.00090f * layerT;
		m_D_O2_air[i][j] = 0.1759f + 0.00117f * layerT;

		std::cout << "    空气中扩散系数 - CH4: " << m_D_CH4_air[i][j] << ", CO2: " << m_D_CO2_air[i][j] << ", O2: " << m_D_O2_air[i][j] << " cm²/s" << std::endl;

		// 水中扩散系数 - Wania et al. (2010), Eqn. 9
		m_D_CH4_water[i][j] = (0.9798f + 0.02986f * layerT + 0.0004381f * layerT * layerT) * Scale;
		m_D_CO2_water[i][j] = (0.9390f + 0.02671f * layerT + 0.0004095f * layerT * layerT) * Scale;
		m_D_O2_water[i][j] = (1.1720f + 0.03443f * layerT + 0.0005048f * layerT * layerT) * Scale;

		std::cout << "    水中扩散系数 - CH4: " << m_D_CH4_water[i][j] << ", CO2: " << m_D_CO2_water[i][j] << ", O2: " << m_D_O2_water[i][j] << " cm²/s" << std::endl;

		// 确定实际扩散系数
		if (j < NACROTELM) {
			// ACROTELM层（表层）
			if (frac_air > 0.05f) {
				// 有足够空气孔隙，使用空气扩散系数并考虑孔隙度
				float airpow = pow(frac_air, 10.0f / 3.0f) / pow(layer_porosity, 2.0f);
				m_dCH4[i][j] = airpow * m_D_CH4_air[i][j];
				m_dCO2[i][j] = airpow * m_D_CO2_air[i][j];
				m_dO2[i][j] = airpow * m_D_O2_air[i][j];
				std::cout << "    ACROTELM层(有空气) - 孔隙度修正因子: " << airpow << std::endl;
			}
			else {
				// 空气孔隙不足，使用水中扩散系数
				m_dCH4[i][j] = m_D_CH4_water[i][j];
				m_dCO2[i][j] = m_D_CO2_water[i][j];
				m_dO2[i][j] = m_D_O2_water[i][j];
				std::cout << "    ACROTELM层(饱和) - 使用水中扩散系数" << std::endl;
			}
		}
		else {
			// CATOTELM层（深层，假设始终饱和）
			m_dCH4[i][j] = m_D_CH4_water[i][j];
			m_dCO2[i][j] = m_D_CO2_water[i][j];
			m_dO2[i][j] = m_D_O2_water[i][j];
			//std::cout << "    CATOTELM层(饱和) - 使用水中扩散系数" << std::endl;
		}

		// 单位转换: cm²/s → m²/d
		m_dCH4[i][j] *= SECS_PER_DAY / CM2_PER_M2;
		m_dCO2[i][j] *= SECS_PER_DAY / CM2_PER_M2;
		m_dO2[i][j] *= SECS_PER_DAY / CM2_PER_M2;

		// 存储结果
		m_D_CH4[i][j] = m_dCH4[i][j];
		m_D_CO2[i][j] = m_dCO2[i][j];
		m_D_O2[i][j] = m_dO2[i][j];

		std::cout << "    最终扩散系数(m²/d) - CH4: " << m_dCH4[i][j] << ", CO2: " << m_dCO2[i][j] << ", O2: " << m_dO2[i][j] << std::endl;
	}
}
		

// Update gas transport velocities [m d-1] and equilibrium gas concentrations [mmol m-3]
// for CH4, CO2 and O2
// See Wania et al. (2010) - Sec 2.5
void CH4_LPJGUESS::Update_Daily_Gas_Parameters(int i) {

	std::cout << "开始更新每日气体传输参数..." << std::endl;

	/* GAS TRANSPORT VELOCITIES */
	// gas transport velocity of SF6.
	// Wania et al. (2010) - Eqn. 6
	float k_600 = 2.07 + 0.215 * pow(U10, 1.7);   // cm h-1
	float deg25 = K2degC + 25.0;                  // 25°C for Henry constant
	float surfT = m_Tsoil[i][0];                  // 每个单元的表层温度

	// Schmidt number of O2
	// Wania et al. (2010) - Eqn. 7
	float ScO2 =1800.6 - 120.1 * surfT + 3.7818 * pow(surfT, 2) - pow(surfT, 3);
			
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

	// Schmidt number of CO2
	// Wania et al. (2010) - Eqn. 7
	float ScCO2 =1911.0 - 113.7 * surfT + 2.967 * pow(surfT, 2) - 0.02943 * pow(surfT, 3);
			
	// gas transport velocity of CO2 [cm h-1]
	// Wania et al. (2010) - Eqn. 5
	m_k_CO2[i] = k_600 * pow(ScCO2 / 600.0, n_coeff);
	m_k_CO2[i] *= 24.0 / CM_PER_M;

	// 打印传输速度
	std::cout << "Cell " << i
		<< " - O2传输速度: " << m_k_O2[i] << " m d-1"
		<< ", CH4传输速度: " << m_k_CH4[i] << " m d-1"
		<< ", CO2传输速度: " << m_k_CO2[i] << " m d-1"
		<< ", 表层温度: " << surfT << " °C"
		<< std::endl;

	/* EQUILIBRIUM GAS CONCENTRATIONS */
	// See Wania et al. (2010) - Eqn. 8
	// Henry coefficient for O2
	float henry_coeff_O2 =henry_k_O2 * exp(-1.0 * henry_C_O2 * (1.0 / (surfT + K2degC) - 1.0 / deg25));      // [L atm mol-1]
			
	// pp_gas/MM2_PER_M2 converts to atm units
	m_Ceq_O2[i] = pp_O2 / MM2_PER_M2 / henry_coeff_O2;    // mol/L
	m_Ceq_O2[i] *= MM2_PER_M2;                            // mol/L to mmol/m3

	// Henry coefficient for CO2
	float henry_coeff_CO2 =henry_k_CO2 * exp(-1.0 * henry_C_CO2 * (1.0 / (surfT + K2degC) - 1.0 / deg25));    // [L atm mol-1]
			
	// Use this gridcell's CO2 concentration, not a fixed value as in Wan  ia et al. (2010).
	// CO₂ mixing ratio (ppmv), already stored in class variable
	float pp_CO2 = m_co2Conc;      // ppmv = micro atm

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
	std::cout << "Cell " << i
		<< " - O2平衡浓度: " << m_Ceq_O2[i] << " mmol m-3"
		<< ", CO2平衡浓度: " << m_Ceq_CO2[i] << " mmol m-3"
		<< ", CH4平衡浓度: " << m_Ceq_CH4[i] << " mmol m-3"
		<< ", 表层温度: " << surfT << " °C"
		<< std::endl;
}


// Generic gas diffusion method that works with CO2, CH4 and O2 (as specified with gastype) 
// See Wania et al. (2010) - Sec 2.5 - for a full description
float CH4_LPJGUESS::Diffuse_Gas(int i, Gastype gas_type, float& dailyDiff) {

	// Called like this (e.g. for O2): 
	// diffuse_gas(O2, D_O2, O2gas, Ceq_O2, k_O2, Dz_metre, dailyO2diffusion);
	std::cout << "开始执行每日气体扩散..." << std::endl;

	float* Cgas = m_Cgas[i];

	float atomic_mass;               // Atomic mass of the gas [gC/mol]
	if (gas_type != O2_gas)
		atomic_mass = atomiccmass;   // CO2或CH4 - 12 gC/mol
	else
		atomic_mass = 1.0;           // O2已经是 mol layer-1

	float initialAmount = 0.0;
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		initialAmount += m_Cgas[i][j];
		m_Frac_water[i][j] = m_soil_water_storage[i][j] / m_layer_thickness[i][j];
		m_Frac_ice[i][j] = m_soilIceSto[i][j] / m_layer_thickness[i][j];
		m_Frac_water_belowpwp[i][j] = m_soilWP[i][j] / m_layer_thickness[i][j];
		m_Dz_metre[i][j] = m_layer_thickness[i][j] / MM_PER_M;
		m_total_volume_water[i][j] = (m_Frac_water[i][j] + m_Frac_ice[i][j] + m_Frac_water_belowpwp[i][j]) * m_Dz_metre[i][j];
		m_volume_liquid_water[i][j] = (m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) * m_Dz_metre[i][j];

		// CO2 & CH4 - g layer-1 to mmol m-3 
		// O2 - mol layer-1 to mmol m-3
		m_C[i][j] = m_Cgas[i][j] / atomic_mass / m_total_volume_water[i][j] * MMOL_PER_MOL;
		std::cout << "  Soil layer " << j
			<< "  Cgas=" << m_Cgas[i][j]
			<< "  volume=" << m_total_volume_water[i][j]
			<< "  C溶解(mmol/m3)=" << m_C[i][j]
			<< std::endl;
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

	// dailyDiff：当天因表层气体交换导致的通量（仅用于“是否需要做扩散”的判断）
	float m_dailyDiff = 0.0f;

	// 若是 O2 或 CH4 才需要 Henry 边界条件
	if (gas_type == O2_gas || gas_type == CH4_gas) {     // Could also run for CH4 here below.

		if ((m_Frac_water[i][0] + m_Frac_water_belowpwp[i][0]) < water_min) {    // Could add a snow restriction
		// No diffusion if there's too little liquid water in the top layer
			Cnew = C_old;                  // Unchanged surface concentration
			m_dailyDiff = 0.0f;            // dailyDiff
		}
		else {

			// Analytical solution to determine Csurf - see Wania et al. Sec 2.5
			Cnew = Ceq + (m_C[i][0] - Ceq) * exp(-kgas / m_volume_liquid_water[i][0]); // mmol m-3
			m_dailyDiff = (m_C[i][0] - Cnew) * atomic_mass * m_volume_liquid_water[i][0] / MMOL_PER_MOL; // mol layer-1 d-1 (O2)
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

		/*for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			m_C_init[i][j] = m_C[i][j];
			totalConc += m_C_init[i][j];
		}*/

		float total_diff = 0.0f;
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
		m_Cgas[i][j] = m_C[i][j] * atomic_mass * m_total_volume_water[i][j] / MMOL_PER_MOL;
		finalAmount += m_Cgas[i][j];
	}

	// Return the amount of gas that has diffused INTO the soil
	// CO2 & CH4 - g
	// O2 - mol
	float gasIntoSoil = finalAmount - initialAmount;

	if (gas_type == CH4_gas)
		dailyDiff = gasIntoSoil;         // gC/m2/d

	return gasIntoSoil;
}		


void CH4_LPJGUESS::Calculate_Gas_Ebullition() {


}

void CH4_LPJGUESS::MethaneDiffusionCalculation(int i) {
			//
			//	std::cout << "开始计算甲烷扩散..." << std::endl;
			//  
			//	for (int i = 0; i < m_nCells; i++) {
			//		if (m_nSoilLyrs[i] <= 0) continue;
			//
			//		std::cout << "单元格 " << i << " 有 " << m_nSoilLyrs[i] << " 个土壤层" << std::endl;
			//
			//		// 固定分层：前4层为水位波动层，后面的层均为永久饱和层
			//		const int nAcrotelm = 4;    // 水位波动层数
			//		int nCatotelm = m_nSoilLyrs[i] - nAcrotelm;  // 永久饱和层数
			//
			//		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			//
			//			// 获取当前层的甲烷浓度
			//			float CH4_concentration = m_CH4_prod[i][j];               // 甲烷浓度（gC/m²）
			//			float atomiccmass = 12.0f;                                // 碳原子质量（g/mol）
			//			std::cout << "  层 " << j << ": CH4浓度=" << CH4_concentration << " gC/m²" << std::endl;
			//
			//			// 将甲烷从质量单位转换为物质的量单位
			//			float CH4_mol = CH4_concentration / atomiccmass;           // gC/m² → mol/m²
			//			std::cout << "CH4物质的量: " << CH4_mol << " mol/m²" << std::endl;
			//
			//			// 获取土壤参数
			//			float temperature = m_Tsoil[i][j];                        // 土壤温度（°C）
			//			float water_content = m_soil_water_storage[i][j];         // 土壤水分含量（体积分数）  ?
			//			float porosity = m_soilPor[i][j];                         // 土壤孔隙度（体积分数）
			//			float air_content = m_Fair[i][j];                         // 土壤空气含量
			//
			//			// 计算水中的扩散系数（基于Wania et al. 2010, Eqn. 9）
			//			float D_CH4_water = (0.9798f + 0.02986f * temperature +
			//				0.0004381f * temperature * temperature) * 0.00001f;
			//
			//			// 计算空气中的扩散系数（基于Wania et al. 2010, Eqn. 10）
			//			float D_CH4_air = 0.1875f + 0.00130f * temperature;        // cm²/s
			//			std::cout << "水中扩散系数: " << D_CH4_water << " cm²/s" << std::endl;
			//			std::cout << "空气中扩散系数: " << D_CH4_air << " cm²/s" << std::endl;
			//
			//			// 根据土层类型选择扩散机制
			//			float D_CH4 = 0.0f;
			//
			//			// 判断是否为水位波动层（前四层，如果总层数少于4则全部视为水位波动层）
			//			bool isAcrotelm = (j < nAcrotelm) && (j < m_nSoilLyrs[i]);
			//
			//			if (isAcrotelm) {
			//				// 水位波动层（acrotelm）: 根据空气含量选择扩散机制
			//				if (air_content > 0.05f) {
			//					// 空气含量高时，以气体扩散为主，考虑孔隙度校正（Wania et al. 2010, Eqn. 11）
			//					float airpow = pow(air_content, 10.0f / 3.0f) / pow(porosity, 2.0f);
			//					D_CH4 = airpow * D_CH4_air;
			//					std::cout << "水位波动层 - 使用气体扩散，校正因子: " << airpow << std::endl;
			//				}
			//				else {
			//					// 水分饱和时，以水中扩散为主
			//					D_CH4 = D_CH4_water;
			//					std::cout << "水位波动层 - 使用水中扩散" << std::endl;
			//				}
			//			}
			//			else {
			//				// 永久饱和层（catotelm）: 总是使用水中扩散
			//				D_CH4 = D_CH4_water;
			//				std::cout << "永久饱和层 - 使用水中扩散" << std::endl;
			//			}
			//
			//			// 转换为 m²/d
			//			D_CH4 = D_CH4 * 86400.0f / 10000.0f;
			//			std::cout << "有效扩散系数: " << D_CH4 << " m²/d" << std::endl;
			//
			//			// 计算扩散通量（简化Fick定律）
			//			float concentration_gradient = 0.0f;
			//			if (j > 0) {
			//				// 与上层浓度差
			//				float upper_CH4_mol = m_CH4_prod[i][j - 1] / atomiccmass;
			//				concentration_gradient = CH4_mol - upper_CH4_mol;
			//			}
			//			else {
			//				// 表层与大气浓度差（假设大气浓度为0）
			//				concentration_gradient = CH4_mol;
			//			}
			//
			//			// 将土壤厚度从毫米转换为米
			//			float layer_thickness_mm = m_layer_thickness[i][j];              // 层厚度（mm）
			//			float layer_thickness = layer_thickness_mm / 1000.0f;            // 转换为（m）
			//			std::cout << "层厚度: " << layer_thickness_mm << " mm = " << layer_thickness << " m" << std::endl;
			//
			//			float diffusion_flux = D_CH4 * concentration_gradient / layer_thickness; // mol/m²/d
			//			std::cout << "扩散通量: " << diffusion_flux << " mol/m²/d" << std::endl;
			//
			//			// 更新甲烷浓度（减去扩散损失）
			//			float CH4_loss_mol = diffusion_flux;
			//			float remaining_CH4_mol = CH4_mol - CH4_loss_mol;
			//			m_CH4_prod[i][j] = remaining_CH4_mol * atomiccmass;              // mol/m² → gC/m²
			//			std::cout << "扩散后剩余CH4: " << m_CH4_prod[i][j] << " gC/m²" << std::endl;
			//
			//			// 存储扩散通量用于输出或分析
			//			m_CH4_diff[i][j] = CH4_loss_mol * atomiccmass;              // 存储为gC/m²
			//		}
			//	}
}

int CH4_LPJGUESS::Execute() {

	CheckInputData();
	InitialOutputs();

#pragma omp parallel for
	for (int i = 0; i < m_nCells; i++) {

		if (m_nSoilLyrs[i] <= 0) continue;

		InitRootFractions(i);
		InitFracAir(i);
		InitAnoxic(i);
		MethaneProduction(i);
		MethaneOxidation(i);
		Calculate_Gas_Diffusivities(i);
		Update_Daily_Gas_Parameters(i);
		
		float dailyCH4diff = 0.0f;
		Diffuse_Gas(i, CH4_gas, dailyCH4diff);

	}
	
	return 0;
}

void CH4_LPJGUESS::GetValue(const char* key, float* value) {
	
}

void CH4_LPJGUESS::Get1DData(const char* key, int* n, float** data) {
	
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
