#include "CH4_LPJGUESS.h"
#include "text.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <fstream>
#include <iomanip>
#include <ctime>


// Initialize all member variables, set pointers to nullptr, set values to 0
CH4_LPJGUESS::CH4_LPJGUESS() :
	m_nCells(-1),m_maxSoilLyrs(-1),m_co2Conc(NODATA_VALUE), m_walterTveg(NODATA_VALUE),m_walterPox(NODATA_VALUE),
	m_ch4Co2Ratio(NODATA_VALUE), m_fGas(NODATA_VALUE),m_oxidFrac(NODATA_VALUE), m_walterTgr(NODATA_VALUE),
	m_area(nullptr),m_Tsoil(nullptr),m_nSoilLyrs(nullptr),m_rootFraction(nullptr), m_peatRootFraction(nullptr),
	m_layer_thickness(nullptr),m_soil_water_storage(nullptr),m_soilPor(nullptr),m_soilWP(nullptr),m_Fair(nullptr),
	m_soilIceSto(nullptr),m_anoxic(nullptr),m_CH4_prod(nullptr),m_CH4_oxid(nullptr),m_D_CH4_water(nullptr),
	m_D_CO2_water(nullptr),m_D_O2_water(nullptr),m_D_CH4_air(nullptr),m_D_CO2_air(nullptr),m_D_O2_air(nullptr),
	m_dCH4(nullptr),m_dCO2(nullptr),m_dO2(nullptr),m_D_CH4(nullptr), m_sol_RSPC(nullptr),m_D_CO2(nullptr),
	m_D_O2(nullptr),m_k_O2(nullptr),m_k_CH4(nullptr),m_k_CO2(nullptr),m_Ceq_O2(nullptr),m_Ceq_CH4(nullptr),
	m_CH4_vgc(nullptr),m_Ceq_CO2(nullptr),m_C(nullptr),m_Cgas(nullptr),m_total_volume_water(nullptr),
	m_Frac_water(nullptr),m_Frac_ice(nullptr), m_CH4_gas_vol(nullptr),m_Frac_water_belowpwp(nullptr),
	m_Dz_metre(nullptr),m_volume_liquid_water(nullptr),m_C_init(nullptr),m_C_last(nullptr), m_CH4_gas(nullptr),
	m_CH4_ebull_vol(nullptr),m_handWtrDep(nullptr),m_CO2_soil(nullptr), m_CO2_soil_yesterday(nullptr),
	m_CO2_soil_prod(nullptr),m_CH4(nullptr), m_CH4_gas_yesterday(nullptr),m_CH4_yesterday(nullptr), m_O2(nullptr),
	m_soilSaturation(nullptr), m_CH4_ebull_ind(nullptr),m_CH4_diss_yesterday(nullptr), m_CH4_diss(nullptr),
	m_CH4_prod_hour(nullptr), m_CH4_oxid_hour(nullptr), m_CH4_diff_hour(nullptr), m_CH4_ebull_hour(nullptr),
	m_CH4_flux_hour(nullptr), m_landuse(nullptr), m_lai(nullptr),m_maxLai(nullptr), m_biomass(nullptr),
	m_tiller_area(nullptr), m_rootDepth(nullptr),m_CH4_plant_hour(nullptr), m_snowAccum(nullptr), m_cellLat(nullptr)
{
	
}

CH4_LPJGUESS::~CH4_LPJGUESS() {

	// One-dimensional arrays allocated by this module.
	/*if (m_area != nullptr) Release1DArray(m_area);*/
	if (m_k_O2 != nullptr) Release1DArray(m_k_O2);
	if (m_k_CH4 != nullptr) Release1DArray(m_k_CH4);
	if (m_k_CO2 != nullptr) Release1DArray(m_k_CO2);
	if (m_Ceq_O2 != nullptr) Release1DArray(m_Ceq_O2);
	if (m_Ceq_CH4 != nullptr) Release1DArray(m_Ceq_CH4);
	if (m_Ceq_CO2 != nullptr) Release1DArray(m_Ceq_CO2);
	/*if (m_nSoilLyrs != nullptr) Release1DArray(m_nSoilLyrs);*/
	/*if (m_handWtrDep != nullptr) Release1DArray(m_handWtrDep);*/

	/*if (m_icnum != nullptr) Release1DArray(m_icnum);*/
	/*if (m_lai != nullptr) Release1DArray(m_lai);
	if (m_maxLai != nullptr) Release1DArray(m_maxLai);
	if (m_landuse != nullptr) Release1DArray(m_landuse);
	if (m_biomass != nullptr) Release1DArray(m_biomass);*/

	if (m_CH4_prod_hour != nullptr) Release1DArray(m_CH4_prod_hour);
	if (m_CH4_oxid_hour != nullptr) Release1DArray(m_CH4_oxid_hour);
	if (m_CH4_diff_hour != nullptr) Release1DArray(m_CH4_diff_hour);
	if (m_CH4_ebull_hour != nullptr) Release1DArray(m_CH4_ebull_hour);
	if (m_CH4_flux_hour != nullptr) Release1DArray(m_CH4_flux_hour);
	/*if (m_rootDepth != nullptr) Release1DArray(m_rootDepth);*/
	/*if (m_Tveg != nullptr) Release1DArray(m_Tveg);*/
	if (m_CH4_plant_hour != nullptr) Release1DArray(m_CH4_plant_hour);

	// Two-dimensional arrays allocated by this module.
	/*if (m_Tsoil != nullptr) Release2DArray(m_nCells, m_Tsoil);
	if (m_soilWP != nullptr) Release2DArray(m_nCells, m_soilWP);
	if (m_soilPor != nullptr) Release2DArray(m_nCells, m_soilPor);
	if (m_soilIceSto != nullptr) Release2DArray(m_nCells, m_soilIceSto);*/
	if (m_Fair != nullptr) Release2DArray(m_nCells, m_Fair);
	if (m_rootFraction != nullptr) Release2DArray(m_nCells, m_rootFraction);
	if (m_peatRootFraction != nullptr)Release2DArray(m_nCells, m_peatRootFraction);
	if (m_soilSaturation != nullptr) Release2DArray(m_nCells, m_soilSaturation);
	/*if (m_layer_thickness != nullptr) Release2DArray(m_nCells, m_layer_thickness);*/
	if (m_anoxic != nullptr) Release2DArray(m_nCells, m_anoxic);
	if (m_CH4_prod != nullptr) Release2DArray(m_nCells, m_CH4_prod);
	if (m_CH4_oxid != nullptr) Release2DArray(m_nCells, m_CH4_oxid);
	/*if (m_soil_water_storage != nullptr) Release2DArray(m_nCells, m_soil_water_storage);*/
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
	/*if (m_sol_RSPC != nullptr) Release2DArray(m_nCells, m_sol_RSPC);*/

	if (m_CH4_ebull_vol != nullptr) Release2DArray(m_nCells, m_CH4_ebull_vol);
	if (m_tiller_area != nullptr) Release2DArray(m_nCells, m_tiller_area);
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
	if (m_peatRootFraction == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_peatRootFraction, 0.f);
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
	if (m_tiller_area == nullptr) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_tiller_area, 0.f);

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
	if (m_CH4_plant_hour == nullptr)
		Initialize1DArray(m_nCells, m_CH4_plant_hour, 0.f);
	}


bool CH4_LPJGUESS::CheckInputData() {
	// Check basic parameters
	if (m_nCells <= 0) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "The number of cells must be greater than 0.");
	}
	if (m_maxSoilLyrs <= 0) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "The number of soil layers must be greater than 0.");
	}
	if (m_nSoilLyrs == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil layer number data is not set.");
	}
	if (!std::isfinite(m_co2Conc) || m_co2Conc <= 0.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Atmospheric CO2 concentration must be greater than 0.");
	}
	if (m_walterTveg < 0.0f || m_walterTveg > 15.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "CH4_TVEG must be within [0, 15].");
	}
	if (m_walterPox < 0.0f || m_walterPox > 1.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "CH4_POX must be within [0, 1].");
	}
	if (m_ch4Co2Ratio < 0.001f || m_ch4Co2Ratio > 1.70f) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData","CH4_CO2_RATIO must be within [0.001, 1.70].");
	}
	if (m_fGas < 0.0f || m_fGas > 0.08f) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "CH4_FGAS must be within [0, 0.08].");
	}
	if (m_oxidFrac < 0.30f || m_oxidFrac > 1.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "CH4_OXID_FRAC must be within [0.30, 1.00].");
	}
	if (m_walterTgr != 2.0f && m_walterTgr != 7.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "CH4_TGR must be either 2 or 7 deg C.");
	}
	if (m_cellLat == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Latitude data is not set.");
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
	/*if (m_icnum == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "ICNUM data is not set.");
	}*/
	if (m_lai == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "LAI data is not set.");
	}
	if (m_maxLai == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "MAXLAI data is not set.");
	}
	if (m_biomass == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Biomass data is not set.");
	}
	if (m_landuse == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Landuse data is not set.");
	}
	if (m_Tsoil == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil temperature data is not set.");
	}
	if (m_rootDepth == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Plant root depth from PG_EPIC is not set.");
	}
	if (m_sol_RSPC == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Soil heterotrophic respiration data is not set.");
	}
	if (m_snowAccum == nullptr) {
		throw ModelException(MID_CH4_LPJGUESS, "CheckInputData", "Snow accumulation data is not set.");
	}
	return true;
}


void CH4_LPJGUESS::SetValue(const char* key, float value) {
	string sk(key);
	if (StringMatch(sk, VAR_CO2)) {
		m_co2Conc = value;
	}
	else if (StringMatch(sk, VAR_CH4_TVEG)) {
		m_walterTveg = value;
	}
	else if (StringMatch(sk, VAR_CH4_POX)) {
		m_walterPox = value;
	}
	else if (StringMatch(sk, VAR_CH4_CO2_RATIO)) {
		m_ch4Co2Ratio = value;
	}
	else if (StringMatch(sk, VAR_CH4_FGAS)) {
		m_fGas = value;
	}
	else if (StringMatch(sk, VAR_CH4_OXID_FRAC)) {
		m_oxidFrac = value;
	}
	else if (StringMatch(sk, VAR_CH4_TGR)) {
		m_walterTgr = value;
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
	//else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
	//	m_handWtrDep = data;    // Water depth of each hand(m)
	//}
	else if (StringMatch(s, VAR_OL_HAND_WTRDEP_AFT_DEP)) {
		m_handWtrDep = data;
	}
	else if (StringMatch(s, VAR_OL_HAND_WTRDEP_AFT_INFIL)) {
		m_handWtrDep = data;
	}
	else if (StringMatch(s, VAR_BLAI)) {
		m_maxLai = data;
	}
	else if (StringMatch(s, VAR_LAIDAY)) {
		m_lai = data;
	}
	else if (StringMatch(s, VAR_LANDUSE)) {
		m_landuse = data;
	}
	else if (StringMatch(s, VAR_BIOMASS)) {
		m_biomass = data;
	}
	/*else if (StringMatch(s, VAR_ICNUM)) {
        m_icnum = data;
	}*/
	else if (StringMatch(s, VAR_ROOTDEPTH)) {
		m_rootDepth = data;
	}
	else if (StringMatch(s, VAR_SNAC)) {
		m_snowAccum = data;
	}
	else if (StringMatch(s, VAR_CELL_LAT)) {
		m_cellLat = data;
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


bool CH4_LPJGUESS::IsMethaneWetland(const int i) const {

	const int landuse = static_cast<int>(m_landuse[i]);

	if (landuse == LANDUSE_ID_WATR ||
		landuse == LANDUSE_ID_GLC) {
		return false;
	}

	if (landuse == 9 || landuse == 10 || landuse == 11 || landuse == 15) {
		return true;
	}

	return false;
}

CH4_LPJGUESS::MethaneMethod
CH4_LPJGUESS::SelectMethaneMethod(const int i) const {

	if (!IsMethaneWetland(i)) {
		return CH4_METHOD_NONE;
	}

	if (!std::isfinite(m_cellLat[i])) {
		return CH4_METHOD_NONE;
	}

	if (m_cellLat[i] < 40.0f) {
		return CH4_METHOD_SPAHNI;
	}

	return CH4_METHOD_WANIA;
}


bool CH4_LPJGUESS::MethaneSpahni(const int i) {

	// Original LPJ-GUESS low-latitude inundated-wetland ratio.
	const float CH4toCO2_inundated = 0.027f;

	// Profile-total heterotrophic respiration.
	// m_sol_RSPC: kg C ha-1 d-1
	// totalRh:    g C m-2 d-1
	float totalRh = 0.0f;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		if (!std::isfinite(m_sol_RSPC[i][j])) {
			throw ModelException(
				MID_CH4_LPJGUESS,
				"MethaneSpahni",
				"SOL_RSPC contains a non-finite value at cell " +
				std::to_string(i) +
				", layer " +
				std::to_string(j) + ".");
		}

		// Negative respiration is not used to produce methane.
		if (m_sol_RSPC[i][j] > 0.0f) {
			totalRh +=
				m_sol_RSPC[i][j] *
				KG_HA_TO_G_M2;
		}
	}

	// Spahni simplified methane flux:
	// a fixed fraction of daily heterotrophic respiration
	// is emitted directly as CH4-C.
	const float CH4_daily =
		totalRh * CH4toCO2_inundated;

	// Internal unit:
	// g CH4-C m-2 d-1
	//
	// Output unit:
	// mg CH4 m-2 h-1
	const float CH4_hour =
		CH4_daily /
		24.0f *
		GC_TO_MG_CH4;

	// In the simplified Spahni method, methane is emitted directly.
	m_CH4_prod_hour[i] = CH4_hour;
	m_CH4_flux_hour[i] = CH4_hour;

	// No explicit layered transport pathways in this method.
	m_CH4_oxid_hour[i] = 0.0f;
	m_CH4_diff_hour[i] = 0.0f;
	m_CH4_ebull_hour[i] = 0.0f;
	m_CH4_plant_hour[i] = 0.0f;

	return true;
}


//void CH4_LPJGUESS::tridiag(int n, float* a, float* b, float* c, float* r, float* u) {
//
//	// Tridiagonal system solver from Numerical Recipes.
//	float* gam = new float[n];
//
//	// Header Element
//	float bet = b[0];
//	u[0] = r[0] / bet;
//
//	for (int j = 1; j < n; j++) {
//		gam[j] = c[j - 1] / bet;
//		bet = b[j] - a[j] * gam[j];
//
//		u[j] = (r[j] - a[j] * u[j - 1]) / bet;
//	}
//
//	for (int j = (n - 2); j >= 0; j--) {
//		u[j] -= gam[j + 1] * u[j + 1];
//	}
//	delete[] gam;
//}
void CH4_LPJGUESS::tridiag(int n, double* a, double* b, double* c, double* r, double* u) {

	if (n <= 0) {
		return;
	}

	double* gam = new double[n];

	for (int j = 0; j < n; j++) {
		gam[j] = 0.0;
	}

	double bet = b[0];

	if (!std::isfinite(bet) || fabs(bet) < 1.0e-20) {
		delete[] gam;
		throw ModelException(MID_CH4_LPJGUESS, "tridiag", "Invalid first diagonal element in gas diffusion solver.");
	}

	u[0] = r[0] / bet;

	if (!std::isfinite(u[0])) {
		delete[] gam;
		throw ModelException(MID_CH4_LPJGUESS, "tridiag", "Non-finite first solution value in gas diffusion solver.");
	}

	for (int j = 1; j < n; j++) {

		gam[j] = c[j - 1] / bet;
		bet = b[j] - a[j] * gam[j];

		if (!std::isfinite(bet) || fabs(bet) < 1.0e-20) {
			delete[] gam;
			throw ModelException(MID_CH4_LPJGUESS, "tridiag", "Zero or non-finite diagonal element in gas diffusion solver.");
		}

		u[j] = (r[j] - a[j] * u[j - 1]) / bet;

		if (!std::isfinite(u[j])) {
			delete[] gam;
			throw ModelException(MID_CH4_LPJGUESS, "tridiag", "Non-finite solution value in gas diffusion solver.");
		}
	}

	for (int j = n - 2; j >= 0; j--) {

		u[j] -= gam[j + 1] * u[j + 1];

		if (!std::isfinite(u[j])) {
			delete[] gam;
			throw ModelException(MID_CH4_LPJGUESS, "tridiag", "Non-finite value during tridiagonal back substitution.");
		}
	}

	delete[] gam;
}

// Crank-Nicholson timestepper algorithm for gas diffusion equation.
//void CH4_LPJGUESS::Cnstepgas(int i, float* conc, float* Di) {
void CH4_LPJGUESS::Cnstepgas(int i, float* conc, float* Di, double dt) {

	// UNITS:
	// Di         m2 d-1
	// dz         m
	// surf_conc  mmol m-3
	// dt         d
	// conc       mmol m-3

	// Only solve for this cell's vertical 1-D diffusion
	int nLyr = static_cast<int>(m_nSoilLyrs[i]);          // number of soil layers for cell i
	if (nLyr <= 1) return;                                // It's impossible to achieve diffusion with only one layer.

	if (!std::isfinite(dt) || dt <= 0.0) {
		throw ModelException(MID_CH4_LPJGUESS, "Cnstepgas", "Gas diffusion timestep must be finite and greater than zero.");
	}

	// The thickness of the soil layer in this unit (m)
	float* dz = m_Dz_metre[i];         

	// Top boundary concentration
	/*float surf_conc = conc[0];*/
	double surf_conc = conc[0];

	// Diffusion time step (day)
	/*float dt = Dt_gas;*/

	// Layer counters: 
	// The values used in the Crank-Nicholson solver (a vector of length active_layers)
	int layer;
	int active_layers;
	/*float dplus;
	float dminus;
	float dz_factor;
	float Cplus;
	float Cp_minus;
	float dzhere, dzminus, dzplus;
	float cohere, cominus, coplus;*/
	double dplus;
	double dminus;
	double dz_factor;
	double Cplus;
	double Cp_minus;
	double dzhere, dzminus, dzplus;
	double cohere, cominus, coplus;

	// Leading diagonal, left and right subdiagonals for Crank-Nicholson
	// matrix.

	int layer0 = 0;                      // Surface index = 0
	active_layers = nLyr - layer0;       // All the nLyr layers are involved in the diffusion process.

	/*float* diag = new float[active_layers];
	float* left = new float[active_layers];
	float* right = new float[active_layers];*/
	double* diag = new double[active_layers];
	double* left = new double[active_layers];
	double* right = new double[active_layers];

	// Right hand side vector for Crank-Nicholson scheme equations.
	/*float* rhs = new float[active_layers];*/
	double* rhs = new double[active_layers];

	// Solution vector for Crank-Nicholson scheme equations.
	/*float* solution = new float[active_layers];*/
	double* solution = new double[active_layers];

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
		/*diag[j] = 0.f;
		left[j] = 0.f;
		right[j] = 0.f;
		rhs[j] = 0.f;
		solution[j] = 0.f;*/
		diag[j] = 0.0;
		left[j] = 0.0;
		right[j] = 0.0;
		rhs[j] = 0.0;
		solution[j] = 0.0;
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
		// to enforce the no-gas-flux bottom boundary condition.  

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
		//if (layer < nLyr - 1) {
		//	// all soil layers apart from the bottom soil layer
		//	dzplus = dz[layer + 1];
		//	coplus = conc[layer + 1];
		//}
		if (layer < nLyr - 1) {
			dzplus = dz[layer + 1];
			coplus = conc[layer + 1];
		}
		else {
			// Bottom no-flux boundary:
			// use the bottom-layer thickness itself as the virtual lower layer thickness.
			dzplus = dzhere;
			coplus = cohere;
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
		if (!std::isfinite(dzhere) || !std::isfinite(dzminus) || !std::isfinite(dzplus) || dzhere <= 0.0 || dzminus <= 0.0 || dzplus <= 0.0) {
			throw ModelException(MID_CH4_LPJGUESS, "Cnstepgas", "Invalid soil-layer thickness in gas diffusion solver.");
		}

		if (!std::isfinite(dplus) || !std::isfinite(dminus) || dplus < 0.0 || dminus < 0.0) {
			throw ModelException(MID_CH4_LPJGUESS, "Cnstepgas", "Invalid gas diffusivity in gas diffusion solver.");
		}
		/*dz_factor = 0.25f * (dzplus + 2.0f * dzhere + dzminus);*/
		/*dz_factor = 0.25 * (dzplus + 2.0 * dzhere + dzminus);
		Cplus = dplus * dt / dz_factor / (dzplus + dzhere);
		Cp_minus = dminus * dt / dz_factor / (dzhere + dzminus);*/

		// Gas-storage volume of the current layer [m3 m-2 = m]
		const double storage_volume = static_cast<double>(m_total_volume_water[i][layer]);

		if (!std::isfinite(storage_volume) || storage_volume <= 1.0e-12) {
			throw ModelException(MID_CH4_LPJGUESS, "Cnstepgas", "Invalid gas-storage volume.");
		}

		// Distance between adjacent layer centres [m]
		const double distance_plus = 0.5 * (dzhere + dzplus);

		const double distance_minus = 0.5 * (dzhere + dzminus);

		if (!std::isfinite(distance_plus) || !std::isfinite(distance_minus) || distance_plus <= 0.0 || distance_minus <= 0.0) {
			throw ModelException(MID_CH4_LPJGUESS, "Cnstepgas", "Invalid distance between soil-layer centres.");
		}

		// Interface conductance [m d-1]
		const double conductance_plus = dplus / distance_plus;

		const double conductance_minus = dminus / distance_minus;

		// Conservative Crank–Nicolson coefficients
		Cplus = 0.5 * dt * conductance_plus / storage_volume;

		Cp_minus = 0.5 * dt * conductance_minus / storage_volume;

		if (!std::isfinite(Cplus) ||!std::isfinite(Cp_minus) || Cplus < 0.0 || Cp_minus < 0.0) {
			throw ModelException(MID_CH4_LPJGUESS, "Cnstepgas", "Invalid Crank-Nicolson coefficient.");
		}

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
		double checksum = left[testrow] * solution[testrow - 1] + diag[testrow] * solution[testrow] + right[testrow] * solution[testrow + 1] - rhs[testrow];

		const double MAX_ERR_CN = 0.000001;
		                  
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
			double rowsum = left[j] + diag[j] + right[j];
			checksum += rowsum / static_cast<double>(active_layers);
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

		if (!std::isfinite(solution[j])) {
			throw ModelException(MID_CH4_LPJGUESS, "Cnstepgas", "Non-finite gas concentration after diffusion.");
		}

		conc[j] = static_cast<float>(solution[j]);
	}

	delete[] diag;
	delete[] left;
	delete[] right;
	delete[] rhs;
	delete[] solution;
}

float CH4_LPJGUESS::CalculateSnowDepth(const int i) {

	if (m_snowAccum == nullptr || m_snowAccum[i] <= 0.0f) {
		return 0.0f;
	}

	// Keep consistent with the snow-density formulation in STP_FP
	float snowDensityRatio = 0.3f + m_snowAccum[i] / 1000.0f;

	if (snowDensityRatio <= 0.0f) {
		return 0.0f;
	}

	// Physical snow depth [mm]
	return m_snowAccum[i] / snowDensityRatio;
}


void CH4_LPJGUESS::InitRootFractions(int i) {

	for (int l = 0; l < CVT_INT(m_nSoilLyrs[i]); l++) {
		m_rootFraction[i][l] = 0.0f;
	}

	float cum_rd = 0.f;
	float cum_d = 0.f;
	float cum_rf = 0.f;
	float x1 = 0.f;
	float x2 = 0.f;

	if (m_rootDepth[i] <= UTIL_ZERO) {
		m_rootFraction[i][0] = 1.f;
		return;
	}

	/// Normalized Root Density =
	/// 1.15 * exp(-11.7 * NRD) + 0.022
	float a = 1.15f;
	float b = 11.7f;
	float c = 0.022f;
	float d = 0.12029f;

	int k = 0;

	for (int l = 0; l < CVT_INT(m_nSoilLyrs[i]); l++) {

		cum_d += m_layer_thickness[i][l];

		if (cum_d >= m_rootDepth[i]) {
			cum_rd = m_rootDepth[i];
		}

		if (cum_d < m_rootDepth[i]) {
			cum_rd = cum_d;
		}

		x1 = Max(0.f, (cum_d - m_layer_thickness[i][l]) / m_rootDepth[i]);

		x2 = cum_rd / m_rootDepth[i];

		float xx1 = -b * x1;

		if (xx1 > 20.f) {
			xx1 = 20.f;
		}

		float xx2 = -b * x2;

		if (xx2 > 20.f) {
			xx2 = 20.f;
		}

		m_rootFraction[i][l] = (a / b * (exp(xx1) - exp(xx2)) + c * (x2 - x1)) / d;

		float xx = cum_rf;

		cum_rf += m_rootFraction[i][l];

		if (cum_rf > 1.f) {
			m_rootFraction[i][l] = 1.f - xx;
			cum_rf = 1.f;
		}

		k = l;

		if (cum_rd >= m_rootDepth[i]) {
			break;
		}
	}

	if (cum_rf <= UTIL_ZERO || !std::isfinite(cum_rf)) {

		for (int l = 0; l < CVT_INT(m_nSoilLyrs[i]); l++) {
			m_rootFraction[i][l] = 0.f;
		}

		m_rootFraction[i][0] = 1.f;
		return;
	}

	for (int l = 0; l < CVT_INT(m_nSoilLyrs[i]); l++) {

		m_rootFraction[i][l] /= cum_rf;

		if (l == k) {
			break;
		}
	}
}


void CH4_LPJGUESS::InitPeatRootFractions(int i) {

	float profiledepth = 0.0f;
	float peatdepth = 0.0f;
	float cumulative_depth = 0.0f;
	float sumrootfrac = 0.0f;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		m_peatRootFraction[i][j] = 0.0f;

		if (m_layer_thickness[i][j] > 0.0f) {
			profiledepth += m_layer_thickness[i][j];
		}
	}

	if (profiledepth <= 0.0f) {
		return;
	}

	// Use the actual WISE soil-profile depth while retaining
    // the LPJ-GUESS exponential root-distribution function.
	peatdepth = profiledepth;

	const float decay_depth_mm = LPJ_PEAT_ROOT_DECAY_COEFF_CM * 10.0f;

	const float normalization = 1.0f - exp(-peatdepth / decay_depth_mm);

	if (normalization <= 1.0e-12f) {
		return;
	}

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		float topdepth = cumulative_depth;

		float bottomdepth = cumulative_depth + m_layer_thickness[i][j];

		if (topdepth < peatdepth) {

			float effective_bottom = bottomdepth;

			if (effective_bottom > peatdepth) {
				effective_bottom = peatdepth;
			}

			m_peatRootFraction[i][j] = (exp(-topdepth / decay_depth_mm) -exp(-effective_bottom / decay_depth_mm)) /
				normalization;

			if (m_peatRootFraction[i][j] < 0.0f) {
				m_peatRootFraction[i][j] = 0.0f;
			}

			sumrootfrac += m_peatRootFraction[i][j];
		}

		cumulative_depth = bottomdepth;
	}

	// Remove floating-point normalization error.
	if (sumrootfrac > 1.0e-12f) {

		for (int j = 0; j < m_nSoilLyrs[i]; j++) {
			m_peatRootFraction[i][j] /= sumrootfrac;
		}
	}
}


//void CH4_LPJGUESS::InitFracAir(int i) {
//
//	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
//
//		float porosity = m_soilPor[i][j];
//		float water_content = m_soil_water_storage[i][j];
//		float ice_content = m_soilIceSto[i][j];
//		float wilting_point = m_soilWP[i][j];
//		float layer_thickness = m_layer_thickness[i][j];
//
//		// SEIMS 的 SOL_ST 不包含萎蔫点以下的土壤水
//		float liquid_below_pwp =
//			Max(0.0f, wilting_point - ice_content);
//
//		// 液态水和冰占据的总体积，单位为 mm
//		float occupied_pore_water =
//			water_content +
//			liquid_below_pwp +
//			ice_content;
//
//		// 将水量除以土层厚度，转换为体积分数
//		float water_volume_fraction =
//			occupied_pore_water / layer_thickness;
//
//		// 空气体积分数；这里不要减 m_fGas
//		m_Fair[i][j] =
//			porosity - water_volume_fraction;
//
//		// 限制在 [0, porosity]
//		if (m_Fair[i][j] < 0.0f) {
//			m_Fair[i][j] = 0.0f;
//		}
//		else if (m_Fair[i][j] > porosity) {
//			m_Fair[i][j] = porosity;
//		}
//	}
//}


void CH4_LPJGUESS::InitFracAir(int i) {

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		float porosity = m_soilPor[i][j];
		float water_content = Max(0.0f, m_soil_water_storage[i][j]);
		float ice_content = Max(0.0f, m_soilIceSto[i][j]);
		float wilting_point = m_soilWP[i][j];
		float layer_thickness = m_layer_thickness[i][j];

		float liquid_below_pwp = Max(0.0f, wilting_point - ice_content);

		float occupied_pore_water = water_content + liquid_below_pwp + ice_content;

		float water_volume_fraction = occupied_pore_water / layer_thickness;

		m_Fair[i][j] = porosity - water_volume_fraction;

		if (m_Fair[i][j] < 0.0f) {
			m_Fair[i][j] = 0.0f;
		}
		else if (m_Fair[i][j] > porosity) {
			m_Fair[i][j] = porosity;
		}
	}
}


void CH4_LPJGUESS::InitAnoxic(int i) {

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		float porosity = m_soilPor[i][j];
		float anoxic = 0.0f;

		// anoxic = 1 - air-filled porosity / total porosity
		// m_Fair already accounts for liquid water and soil ice.
		if (porosity > UTIL_ZERO) {
			anoxic = 1.0f - m_Fair[i][j] / porosity;
		}

		// Limit the anaerobic fraction to [0, 1].
		if (anoxic < 0.0f) {
			anoxic = 0.0f;
		}
		else if (anoxic > 1.0f) {
			anoxic = 1.0f;
		}

		m_anoxic[i][j] = anoxic;
	}
}


void CH4_LPJGUESS::Calculate_Gas_Diffusivities(int i) {

    // Calculate and return diffusivities, in units of m2 d-1
    // Called each day
    // See Wania et al. (2010) - Sec 2.5

	/*float cumulative_depth = 0.0f;*/

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		/*float layer_bottom_depth = cumulative_depth + m_layer_thickness[i][j];

		bool is_acrotelm = layer_bottom_depth <= ACROTELM_DEPTH_MM;*/

		// AIR - UNITS: 10 - 4 m2 s - 1 == cm2 s - 1
		// Lerman (1979)

		// 获取当前层的参数
		float layerT = m_Tsoil[i][j];                   // 土壤温度 (°C)
		float frac_air = m_Fair[i][j];                  // 空气体积分数
		float layer_porosity = m_soilPor[i][j];         // 当前层孔隙度

		if (!std::isfinite(layerT) || !std::isfinite(frac_air) || !std::isfinite(layer_porosity) || layer_porosity <= 0.0f) {
			throw ModelException(MID_CH4_LPJGUESS, "Calculate_Gas_Diffusivities", "Invalid temperature, air fraction or soil porosity.");
		}

		// Wania et al. (2010), Eqn. 10
		m_D_CH4_air[i][j] = 0.1875f + 0.00130f * layerT;
		m_D_CO2_air[i][j] = 0.1325f + 0.00090f * layerT;
		m_D_O2_air[i][j] = 0.1759f + 0.00117f * layerT;

		// WATER - UNITS: 10-4 m2 s-1 == cm2 s-1
		// Wania et al. (2010), Eqn. 9 - where the diffusivities are in units of 10**-9 m2 s-1. 
		// * by scale to get 10-4 m2 s-1
		m_D_CH4_water[i][j] = (0.9798f + 0.02986f * layerT + 0.0004381f * layerT * layerT) * Scale;
		m_D_CO2_water[i][j] = (0.9390f + 0.02671f * layerT + 0.0004095f * layerT * layerT) * Scale;
		m_D_O2_water[i][j] = (1.1720f + 0.03443f * layerT + 0.0005048f * layerT * layerT) * Scale;

		// Apply the original LPJ-GUESS acrotelm diffusivity logic
        // to every WISE soil layer using its actual daily air fraction.
		if (frac_air > 0.05f) {

			// Wania et al. (2010), Eq. 11
			float airpow = pow(frac_air, 10.0f / 3.0f) / pow(layer_porosity, 2.0f);

			m_dCH4[i][j] = airpow * m_D_CH4_air[i][j];
			m_dCO2[i][j] = airpow * m_D_CO2_air[i][j];
			m_dO2[i][j] = airpow * m_D_O2_air[i][j];
		}
		else {

			// Water diffusivities dominate
			m_dCH4[i][j] = m_D_CH4_water[i][j];
			m_dCO2[i][j] = m_D_CO2_water[i][j];
			m_dO2[i][j] = m_D_O2_water[i][j];
		}

		// Convert from cm2 s-1 to m2 d-1
		m_dCH4[i][j] *= SECS_PER_DAY / CM2_PER_M2;
		m_dCO2[i][j] *= SECS_PER_DAY / CM2_PER_M2;
		m_dO2[i][j] *= SECS_PER_DAY / CM2_PER_M2;

		// Check the calculated diffusivities after unit conversion
        // and before storing them in the diffusion arrays.
		if (!std::isfinite(m_dCH4[i][j]) || !std::isfinite(m_dCO2[i][j]) || !std::isfinite(m_dO2[i][j]) ||
			m_dCH4[i][j] < 0.0f || m_dCO2[i][j] < 0.0f || m_dO2[i][j] < 0.0f) {
			throw ModelException(MID_CH4_LPJGUESS, "Calculate_Gas_Diffusivities", "Invalid gas diffusivity after unit conversion.");
		}

		// Store diffusivities in units of m2 d-1
		m_D_CH4[i][j] = m_dCH4[i][j];
		m_D_CO2[i][j] = m_dCO2[i][j];
		m_D_O2[i][j] = m_dO2[i][j];
	}
}
		


bool CH4_LPJGUESS::Update_Daily_Gas_Parameters(int i) {

	// Update gas transport velocities [m d-1] and equilibrium gas concentrations [mmol m-3]
	// for CH4, CO2 and O2
	// Called daily by Soil::methane()
	// See Wania et al. (2010) - Sec 2.5

	float surfT = m_Tsoil[i][0];                  // The surface temperature of each unit

	if (!std::isfinite(surfT)) {
		throw ModelException(MID_CH4_LPJGUESS, "Update_Daily_Gas_Parameters", "Surface soil temperature is not finite.");
	}

	// Schmidt数和Henry系数经验公式采用的有效水温
	// 将其限制在经验公式适用范围0—30°C。
	// 这里只限制气体参数计算，不修改真实土壤温度。
	float gasTemp = surfT;

	if (gasTemp < 0.0f) {
		gasTemp = 0.0f;
	}
	else if (gasTemp > 30.0f) {
		gasTemp = 30.0f;
	}

	/* GAS TRANSPORT VELOCITIES */

	// gas transport velocity of SF6.
	// Wania et al. (2010) - Eqn. 6
	float k_600 = 2.07 + 0.215 * pow(U10, 1.7);   // cm h-1

	// Schmidt number of O2
	// Wania et al. (2010) - Eqn. 7
	/*float ScO2 =1800.6 - 120.1 * surfT + 3.7818 * pow(surfT, 2) - 0.047608 * pow(surfT, 3);*/
	float ScO2 = 1800.6f - 120.1f * gasTemp + 3.7818f * gasTemp * gasTemp - 0.047608f * gasTemp * gasTemp * gasTemp;

	if (!std::isfinite(ScO2) || ScO2 <= 0.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "Update_Daily_Gas_Parameters", "Invalid O2 Schmidt number.");
	}
			
	// gas transport velocity of O2 [cm h-1]
	// Wania et al. (2010) - Eqn. 5
	m_k_O2[i] = k_600 * pow(ScO2 / 600.0, n_coeff);
	m_k_O2[i] *= 24.0 / CM_PER_M;              // cm h-1 → m d-1

	// Schmidt number of CH4
	// Wania et al. (2010) - Eqn. 7
	/*float ScCH4 =1898.0 - 110.1 * surfT + 2.834 * pow(surfT, 2) - 0.02791 * pow(surfT, 3);*/
	float ScCH4 = 1898.0f - 110.1f * gasTemp + 2.834f * gasTemp * gasTemp - 0.02791f * gasTemp * gasTemp * gasTemp;

	if (!std::isfinite(ScCH4) || ScCH4 <= 0.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "Update_Daily_Gas_Parameters", "Invalid CH4 Schmidt number.");
	}
		
	// gas transport velocity of CH4 [cm h-1]
	// Wania et al. (2010) - Eqn. 5
	m_k_CH4[i] = k_600 * pow(ScCH4 / 600.0, n_coeff);
	m_k_CH4[i] *= 24.0 / CM_PER_M;

	// Schmidt number of CO2
	// Wania et al. (2010) - Eqn. 7
	/*float ScCO2 =1911.0 - 113.7 * surfT + 2.967 * pow(surfT, 2) - 0.02943 * pow(surfT, 3);*/
	float ScCO2 = 1911.0f - 113.7f * gasTemp + 2.967f * gasTemp * gasTemp - 0.02943f * gasTemp * gasTemp * gasTemp;

	if (!std::isfinite(ScCO2) || ScCO2 <= 0.0f) {
		throw ModelException(MID_CH4_LPJGUESS, "Update_Daily_Gas_Parameters", "Invalid CO2 Schmidt number.");
	}
			
	// gas transport velocity of CO2 [cm h-1]
	// Wania et al. (2010) - Eqn. 5
	m_k_CO2[i] = k_600 * pow(ScCO2 / 600.0, n_coeff);
	m_k_CO2[i] *= 24.0 / CM_PER_M;

	/* EQUILIBRIUM GAS CONCENTRATIONS */
	// See Wania et al. (2010) - Eqn. 8

	float deg25 = K2degC + 25.0;                  // 25 degrees C

	// Henry coefficient for O2
	//float henry_coeff_O2 = henry_k_O2 * exp(-1.0 * henry_C_O2 * (1.0 / (surfT + K2degC) - 1.0 / deg25));      // [L atm mol-1]
	float henry_coeff_O2 = henry_k_O2 * exp(-henry_C_O2 * (1.0f / (gasTemp + K2degC) - 1.0f / deg25));     // [L atm mol-1]
			
	// pp_gas/MM2_PER_M2 converts to atm units
	m_Ceq_O2[i] = pp_O2 / MM2_PER_M2 / henry_coeff_O2;    // mol/L
	m_Ceq_O2[i] *= MM2_PER_M2;                            // mol/L to mmol/m3

	// Henry coefficient for CO2
	//float henry_coeff_CO2 = henry_k_CO2 * exp(-1.0 * henry_C_CO2 * (1.0 / (surfT + K2degC) - 1.0 / deg25));    // [L atm mol-1]
	float henry_coeff_CO2 = henry_k_CO2 * exp(-henry_C_CO2 * (1.0f / (gasTemp + K2degC) - 1.0f / deg25));      // [L atm mol-1]
			
	// Use this gridcell's CO2 concentration, not a fixed value as in Wan  ia et al. (2010).
	// CO₂ mixing ratio (ppmv), already stored in class variable
	float pp_CO2 = m_co2Conc;      // ppmv → micro atm

	// Convert atmospheric CO2 mixing ratio from ppmv
    // (numerically equivalent to micro-atm) to atm 
	m_Ceq_CO2[i] = pp_CO2 / MM2_PER_M2 / henry_coeff_CO2; // mol L-1

	// mol L-1 → mmol m-3
	m_Ceq_CO2[i] *= MM2_PER_M2;

	// Henry coefficient for CH4
	/*float henry_coeff_CH4 = henry_k_CH4 * exp(-1.0 * henry_C_CH4 * (1.0 / (surfT + K2degC) - 1.0 / deg25));*/
	float henry_coeff_CH4 = henry_k_CH4 * exp(-henry_C_CH4 * (1.0f / (gasTemp + K2degC) - 1.0f / deg25));
			
	m_Ceq_CH4[i] = pp_CH4 / MM2_PER_M2 / henry_coeff_CH4;
	m_Ceq_CH4[i] *= MM2_PER_M2;

	return true;
}


float CH4_LPJGUESS::Diffuse_Gas(int i, Gastype gas_type, float& dailyDiff) {

	// Generic gas diffusion method that works with CO2, CH4 and O2 (as specified with gastype) 
    // See Wania et al. (2010) - Sec 2.5 - for a full description

	// Called like this (e.g. for O2): 
	// diffuse_gas(O2, D_O2, O2gas, Ceq_O2, k_O2, Dz_metre, dailyO2diffusion);

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

	const char* gasName = (gas_type == CH4_gas) ? "CH4" : (gas_type == CO2_gas) ? "CO2" : "O2";

	// Atomic mass of the gas [gC/mol]
	float atomic_mass;              
	if (gas_type != O2_gas)
		atomic_mass = atomiccmass;   // i.e. CO2 or CH4 - 12 gC/mol
	else
		atomic_mass = 1.0;           // O2 already in mol layer-1

	double initialAmount = 0.0;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		if (!std::isfinite(Cgas[j]) || !std::isfinite(m_total_volume_water[i][j]) || m_total_volume_water[i][j] <= 1.0e-12f) {
			throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Invalid gas amount or total water volume.");
		}

		// Remove only tiny negative values caused by floating-point roundoff.
		if (Cgas[j] < 0.0f) {

			if (Cgas[j] >= -MAX_ERR) {
				Cgas[j] = 0.0f;
			}
			else {
				throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas",
					std::string("Substantial negative gas pool: gas=") + gasName +
					", cell=" + std::to_string(i) +
					", layer=" + std::to_string(j) +
					", pool=" + std::to_string(Cgas[j]));
			}
		}

		// Add the corrected value to the initial total.
		initialAmount += static_cast<double>(Cgas[j]);

		// CO2 & CH4 - g layer-1 to mmol m-3 
		// O2 - mol layer-1 to mmol m-3
		//m_C[i][j] = m_Cgas[i][j] / atomic_mass / m_total_volume_water[i][j] * MMOL_PER_MOL;
		/*m_C[i][j] = Cgas[j] / atomic_mass / m_total_volume_water[i][j] * MMOL_PER_MOL;*/
		double concentration = static_cast<double>(Cgas[j]) / static_cast<double>(atomic_mass) /static_cast<double>(m_total_volume_water[i][j]) * static_cast<double>(MMOL_PER_MOL);

		if (!std::isfinite(concentration) || concentration < 0.0 || concentration > static_cast<double>((std::numeric_limits<float>::max)())) {

			throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas",
				std::string("Invalid converted concentration: gas=") + gasName +
				", cell=" + std::to_string(i) +
				", layer=" + std::to_string(j) +
				", pool=" + std::to_string(Cgas[j]) +
				", volume=" +
				std::to_string(m_total_volume_water[i][j]) +
				", concentration=" + std::to_string(concentration));
		}

		m_C[i][j] = static_cast<float>(concentration);
	}

	// Set the BC, i.e. equilibrium gas concentrations in the top layer depending on atmospheric concentrations
	// and using Henry's law. See Wania et al. (2010), Eqs. 4-8

	// New surface concentration [mmol m-3]
    // Surface-layer gas concentration before atmospheric exchange [mmol m-3]
	float C_old = m_C[i][0];
	float Cnew = C_old;

	// Henry equilibrium concentration [mmol m-3]
	float Ceq = 0.0f;

	// Surface gas exchange velocity [m d-1]
	float kgas = 0.0f;

	if (gas_type == CH4_gas) {
		Ceq = m_Ceq_CH4[i];
		kgas = m_k_CH4[i];
	}
	else if (gas_type == CO2_gas) {
		Ceq = m_Ceq_CO2[i];
		kgas = m_k_CO2[i];
	}
	else {
		Ceq = m_Ceq_O2[i];
		kgas = m_k_O2[i];
	}

	// Check gas parameters before using them in the boundary equation.
	if (!std::isfinite(C_old)) {
		throw ModelException(
			MID_CH4_LPJGUESS,
			"Diffuse_Gas",
			std::string("Invalid C_old: gas=") + gasName +
			", cell=" + std::to_string(i) +
			", C_old=" + std::to_string(C_old) +
			", pool=" + std::to_string(Cgas[0]) +
			", total_volume=" +
			std::to_string(m_total_volume_water[i][0]));
	}

	if (!std::isfinite(Ceq)) {
		throw ModelException(
			MID_CH4_LPJGUESS,
			"Diffuse_Gas",
			std::string("Invalid Ceq: gas=") + gasName +
			", cell=" + std::to_string(i) +
			", Ceq=" + std::to_string(Ceq) +
			", Tsoil=" + std::to_string(m_Tsoil[i][0]));
	}

	if (!std::isfinite(kgas) || kgas < 0.0f) {
		throw ModelException(
			MID_CH4_LPJGUESS,
			"Diffuse_Gas",
			std::string("Invalid kgas: gas=") + gasName +
			", cell=" + std::to_string(i) +
			", kgas=" + std::to_string(kgas) +
			", Tsoil=" + std::to_string(m_Tsoil[i][0]));
	}

	// Whether surface ponding exists.
	bool has_surface_water = m_handWtrDep[i] > 1.0e-8f;

	// Whether the top soil layer contains sufficient liquid water.
	bool has_topsoil_liquid = (m_Frac_water[i][0] + m_Frac_water_belowpwp[i][0]) >= water_min;

	// Apply the atmospheric boundary condition to O2 and CH4.
	if (gas_type == O2_gas || gas_type == CH4_gas) {

		// Disable surface exchange only when both surface water
		// and topsoil liquid water are absent.
		if (!has_surface_water && !has_topsoil_liquid) {

			Cnew = C_old;
			dailyDiff = 0.0f;
		}
		else {

			if (!std::isfinite(m_volume_liquid_water[i][0]) || m_volume_liquid_water[i][0] <= 1.0e-12f) {

				throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Invalid top-layer liquid-water volume.");
			}

			// Calculate with double precision to reduce numerical error.
			double boundary_exponent = -static_cast<double>(kgas) / static_cast<double>(m_volume_liquid_water[i][0]);

			double Cnew_double = static_cast<double>(Ceq) + (static_cast<double>(C_old) - static_cast<double>(Ceq)) * std::exp(boundary_exponent);

			// Check the result immediately after applying
			// the atmospheric boundary condition.
			if (!std::isfinite(Cnew_double)) {
				throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Non-finite surface gas concentration after boundary exchange.");
			}

			Cnew = static_cast<float>(Cnew_double);

			dailyDiff = (C_old - Cnew) * atomic_mass * m_volume_liquid_water[i][0] / MMOL_PER_MOL;

			if (!std::isfinite(dailyDiff)) {
				throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Non-finite daily surface gas diffusion flux.");
			}
		}

		m_C[i][0] = Cnew;
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

		// Assume a tiny diffusivity if there is very little liquid water
        // This must be done before determining the diffusion timestep.
		for (int j = 0; j < m_nSoilLyrs[i]; j++) {

			bool is_surface_layer_with_water = (j == 0 && has_surface_water);

			bool has_layer_liquid = (m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) >= water_min;

			// Do not classify the first layer as dry when it contains
			// HAND surface water.
			if (!is_surface_layer_with_water && !has_layer_liquid) {
				m_D[j] = 1.0e-9f;
			}
		}


		// Determine a stable diffusion timestep using the actual
        // WISE layer thicknesses, interface diffusivities and
        // gas-storage volume of each layer.
		double timestep_limit = static_cast<double>(Dt_gas);

		for (int j = 1; j < m_nSoilLyrs[i]; j++) {

			// Thickness of the current, upper and lower layers [m]
			const double dzhere = static_cast<double>(m_Dz_metre[i][j]);

			const double dzminus = static_cast<double>(m_Dz_metre[i][j - 1]);

			const double dzplus = j < m_nSoilLyrs[i] - 1 ? static_cast<double>(m_Dz_metre[i][j + 1]) : dzhere;

			// Interface diffusivity [m2 d-1]
			const double dminus = 0.5 * (static_cast<double>(m_D[j]) + static_cast<double>(m_D[j - 1]));

			const double dplus = j < m_nSoilLyrs[i] - 1 ? 0.5 * (static_cast<double>(m_D[j]) + static_cast<double>(m_D[j + 1])) : 0.0;

			// Gas-storage volume per unit ground area
			// [m3 m-2 = m]
			const double storage_volume = static_cast<double>(m_total_volume_water[i][j]);

			if (!std::isfinite(dzhere) ||
				!std::isfinite(dzminus) ||
				!std::isfinite(dzplus) ||
				!std::isfinite(dminus) ||
				!std::isfinite(dplus) ||
				!std::isfinite(storage_volume) ||
				dzhere <= 0.0 ||
				dzminus <= 0.0 ||
				dzplus <= 0.0 ||
				dminus < 0.0 ||
				dplus < 0.0 ||
				storage_volume <= 1.0e-12) {

				throw ModelException(MID_CH4_LPJGUESS,"Diffuse_Gas","Invalid layer geometry, diffusivity or ""gas-storage volume.");
			}

			// Distance between adjacent layer centres [m]
			const double distance_minus = 0.5 * (dzhere + dzminus);

			const double distance_plus = 0.5 * (dzhere + dzplus);

			// Interface conductance [m d-1]
			const double conductance_minus = dminus / distance_minus;

			const double conductance_plus = dplus / distance_plus;

			// Conservative diffusion rate [d-1]
			const double rate = (conductance_minus + conductance_plus) / storage_volume;

			if (!std::isfinite(rate) || rate < 0.0) {
				throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Invalid conservative diffusion rate.");
			}

			if (rate > 0.0) {

				const double layer_timestep = 0.8 / rate;

				if (!std::isfinite(layer_timestep) || layer_timestep <= 0.0) {
					throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Invalid conservative diffusion timestep.");
				}

				if (layer_timestep < timestep_limit) {
					timestep_limit = layer_timestep;
				}
			}
		}

		if (!std::isfinite(timestep_limit) || timestep_limit <= 0.0) {
			throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Invalid gas diffusion timestep.");
		}

		// Determine the maximum number of equal substeps for one day.
        // The iteration may stop early after the convergence criterion is met.
		int gasdiffix = static_cast<int>(ceil(1.0 / timestep_limit));

		if (gasdiffix < 1) {
			gasdiffix = 1;
		}

		if (gasdiffix > 100000) {
			throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Required number of gas diffusion substeps is too large.");
		}

		double actual_dt = 1.0 / static_cast<double>(gasdiffix);

		double totalConc = 0.0;
		double total_diff = 0.0;
		double ratio = 0.0;

		int cncount = 0;
		bool stable = false;

		do {
			totalConc = 0.0;

			for (int j = 0; j < m_nSoilLyrs[i]; j++) {

				if (!std::isfinite(m_C[i][j])) {
					throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Non-finite gas concentration before CN diffusion.");
				}

				m_C_init[i][j] = m_C[i][j];

				// Absolute value avoids cancellation if a small numerical
				// negative concentration is present.
				totalConc += fabs(static_cast<double>(m_C_init[i][j]));
			}

			Cnstepgas(i, m_C[i], m_D, actual_dt);

			total_diff = 0.0;

			for (int j = 0; j < m_nSoilLyrs[i]; j++) {

				if (!std::isfinite(m_C[i][j])) {
					throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Non-finite gas concentration after CN diffusion.");
				}

				// CN diffusion may produce a tiny negative concentration
				// because its double solution is stored back into float arrays.
				if (m_C[i][j] < 0.0f) {

					if (m_C[i][j] >= -MAX_ERR) {
						m_C[i][j] = 0.0f;
					}
					else {
						throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas",
							std::string("Substantial negative concentration after CN diffusion: gas=") +
							gasName +
							", cell=" + std::to_string(i) +
							", layer=" + std::to_string(j) +
							", concentration=" + std::to_string(m_C[i][j]));
					}
				}

				m_C_last[i][j] = m_C[i][j];

				total_diff += fabs(static_cast<double>(m_C_last[i][j]) - static_cast<double>(m_C_init[i][j]));
			}

			cncount++;

			if (totalConc > 1.0e-20) {
				ratio = total_diff / totalConc;
			}
			else {
				ratio = total_diff <= 1.0e-20 ? 0.0 : 1.0;
			}

			if (!std::isfinite(ratio)) {
				throw ModelException(MID_CH4_LPJGUESS,"Diffuse_Gas", "Non-finite convergence ratio in gas diffusion.");
			}

			if (ratio < 0.01 &&
				cncount > gasdiffix / 3) {

				stable = true;
			}

		} while (cncount < gasdiffix && !stable);
	}

	// Unit conversions - allocate concentrations to g layer-1 or mole layer-1
	/*float finalAmount = 0.0f;*/
	double finalAmount = 0.0;
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		//m_Cgas[i][j] = m_C[i][j] * atomic_mass * m_total_volume_water[i][j] / MMOL_PER_MOL;
		//finalAmount += m_Cgas[i][j];
		Cgas[j] = m_C[i][j] * atomic_mass * m_total_volume_water[i][j] / MMOL_PER_MOL;
		finalAmount += static_cast<double>(Cgas[j]);
	}

	// Return the amount of gas that has diffused INTO the soil
	// CO2 & CH4 - g
	// O2 - mol
	/*float gasIntoSoil = finalAmount - initialAmount;*/
	double gasIntoSoilDouble = finalAmount - initialAmount;

	if (!std::isfinite(gasIntoSoilDouble)) {
		throw ModelException(MID_CH4_LPJGUESS, "Diffuse_Gas", "Non-finite total gas diffusion amount.");
	}

	float gasIntoSoil = static_cast<float>(gasIntoSoilDouble);

	if (gas_type == CH4_gas)
		dailyDiff = gasIntoSoil;         // gC/m2/d

	return gasIntoSoil;
}		


bool CH4_LPJGUESS::IsWetlandGraminoidActive(const int i) {

	if (!IsMethaneWetland(i)) {
		return false;
	}

	const int landuse = static_cast<int>(m_landuse[i]);

	// 非森林湿地或本研究区暂定的湿地草地
	if (landuse != 9 &&
		landuse != 11 &&
		landuse != 15) {
		return false;
	}

	if (!std::isfinite(m_lai[i]) ||
		m_lai[i] <= 1.0e-6f) {
		return false;
	}

	if (!std::isfinite(m_rootDepth[i]) ||
		m_rootDepth[i] <= 0.0f) {
		return false;
	}

	return true;
}


float CH4_LPJGUESS::Calculate_tiller_areas(const int i) {

	// Clear previous-day tiller areas
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {
		m_tiller_area[i][j] = 0.0f;
	}

	if (!IsWetlandGraminoidActive(i)) {
		return 0.0f;
	}

	// Original LPJ-GUESS broadleaf SLA formulation
	// SLA unit: m2 leaf kgC-1
	float sla_c = 0.2f * pow(10.0f, 2.41f - 0.38f * log10(12.0f * LPJ_WETGRS_LEAFLONG));

	if (!std::isfinite(sla_c) || sla_c <= 0.0f) {
		return 0.0f;
	}

	// Daily LAI already contains the current phenological state.
	// LAI / SLA_C gives effective leaf carbon mass.
	float graminoid_leafcmass = m_lai[i] / sla_c;  // kgC m-2

	if (!std::isfinite(graminoid_leafcmass) || graminoid_leafcmass <= 0.0f) {
		return 0.0f;
	}

	graminoid_leafcmass *= G_PER_KG;  // kgC m-2 -> gC m-2

	float tiller_density = 0.0f;

	if (tiller_weight > 1.0e-12f) {
		tiller_density = graminoid_leafcmass / tiller_weight;
	}

	if (!std::isfinite(tiller_density) || tiller_density <= 1.0e-12f) {
		return 0.0f;
	}

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		if (m_rootFraction[i][j] <= 0.0f) {
			continue;
		}

		float tiller_frac = tiller_density * m_rootFraction[i][j];

		m_tiller_area[i][j] = PI * pow(tiller_radius, 2.0f) * tiller_frac * tiller_por;

		if (!std::isfinite(m_tiller_area[i][j]) || m_tiller_area[i][j] < 1.0e-9f) {
			m_tiller_area[i][j] = 0.0f;
		}
	}

	return tiller_density;
}

float CH4_LPJGUESS::Plant_Gas_Transport(const int i, Gastype gas_type, float& plantTransportToday) {


	// Plant transport of O2 or CH4
	// See Wania et al. (2010) - Sec 2.6

	// The limiting factor to plant transport are the number of tillers as
	// they represent the cross-sectional area available to gas transport.
	// The biomass is related to tiller density based on Schimel 1995. The
	// average biomass was 185g/m2 and the average number of tillers was 380/m2.
	// This gives us 2.05 tillers per g biomass and 0.48 g per tiller, which
	// corresponds to 0.22 g C per tiller

	float* Cgas = nullptr;     // gas pool for current cell, by layer
	float Ceq = 0.0f;          // equilibrium concentration [mmol m-3]
	float kgas = 0.0f;         // gas transfer velocity [m d-1]
	float atomic_mass = 1.0f;  // conversion factor

	// Select gas pool and corresponding parameters
	if (gas_type == CH4_gas) {
		Cgas = m_CH4[i];              // gC layer-1
		Ceq = m_Ceq_CH4[i];           // mmol m-3
		kgas = m_k_CH4[i];            // m d-1
		atomic_mass = atomiccmass;    // gC mol-1
	}
	else if (gas_type == CO2_gas) {
		Cgas = m_CO2_soil[i];         // gC layer-1
		Ceq = m_Ceq_CO2[i];           // mmol m-3
		kgas = m_k_CO2[i];            // m d-1
		atomic_mass = atomiccmass;    // gC mol-1
	}
	else { // O2_gas
		Cgas = m_O2[i];               // mol layer-1
		Ceq = m_Ceq_O2[i];            // mmol m-3
		kgas = m_k_O2[i];             // m d-1
		atomic_mass = 1.0f;           // already mol
	}

	// to return
	plantTransportToday = 0.0f;

	//// First calculate tiller area for this cell
	//float tiller_density = Calculate_tiller_areas(i);

	//// No active wetland graminoids -> no plant transport
	//if (tiller_density <= 1.0e-12f) {
	//	return 0.0f;
	//}

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		float Cnew = 0.0f;   // updated concentration [mmol m-3]

		if (!std::isfinite(m_total_volume_water[i][j]) || m_total_volume_water[i][j] <= 1.0e-12f) {
			continue;
		}

		// Convert pool to concentration
		// CH4 / CO2: gC layer-1 -> mmol m-3
		// O2: mol layer-1 -> mmol m-3
		Cgas[j] = Cgas[j] / atomic_mass / m_total_volume_water[i][j] * MMOL_PER_MOL;

		// No plant transport if:
		// 1) too little liquid water
		// 2) tiller area is negligible
		// 3) liquid water volume is too small
		if ((m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) < water_min ||
			m_tiller_area[i][j] <= 1.0e-12f ||
			m_volume_liquid_water[i][j] < 1.0e-4f) {

			Cnew = Cgas[j];  // no plant transport to/from this layer
		}
		else {
			// Exponential approach to equilibrium concentration
			float exponent = kgas * m_tiller_area[i][j] / m_volume_liquid_water[i][j];
			Cnew = Ceq + (Cgas[j] - Ceq) * exp(-exponent);

			// Layer plant-mediated transport.
            // O2: mol layer-1 d-1
            // CH4/CO2: gC m-2 d-1
			float layerPlantTransport = (Cgas[j] - Cnew) * atomic_mass * m_total_volume_water[i][j] / MMOL_PER_MOL;

			// Remove only tiny negative CH4 transport caused by
			// single-precision round-off. Revert the corresponding
			// layer pool update at the same time to preserve mass balance.
			if (gas_type == CH4_gas && layerPlantTransport < 0.0f && fabs(layerPlantTransport) < MAX_ERR) {
				layerPlantTransport = 0.0f;
				Cnew = Cgas[j];
			}

			plantTransportToday += layerPlantTransport;
		}

		// Convert concentration back to pool
		// CH4 / CO2: mmol m-3 -> gC layer-1
		// O2: mmol m-3 -> mol layer-1
		Cgas[j] = Cnew * atomic_mass * m_total_volume_water[i][j] / MMOL_PER_MOL;
	}

	return plantTransportToday;
}


float CH4_LPJGUESS::Plant_CH4_Transport(const int i, float& plantTransportToday, float& plantOxidToday) {

	plantTransportToday = 0.0f;
	plantOxidToday = 0.0f;

	if (!IsWetlandGraminoidActive(i)) {
		return 0.0f;
	}

	// m_nSoilLyrs is float*, so convert it to int before using it as an index
	int nSoilLyrs = static_cast<int>(m_nSoilLyrs[i]);

	if (nSoilLyrs <= 0) {
		return 0.0f;
	}

	// Soil temperature from STP_FP represents the temperature
    // at the midpoint of each soil layer.
    // Interpolate soil temperature to the Walter reference depth (500 mm).

	float Tsoil50 = m_Tsoil[i][nSoilLyrs - 1];

	float cumulative_depth = 0.0f;
	float previous_mid_depth = 0.0f;
	float previous_temperature = m_Tsoil[i][0];

	for (int j = 0; j < nSoilLyrs; j++) {

		float layer_thickness = m_layer_thickness[i][j];

		float layer_top_depth = cumulative_depth;
		float layer_bottom_depth = layer_top_depth + layer_thickness;

		float layer_mid_depth = layer_top_depth + 0.5f * layer_thickness;

		if (j == 0 && WALTER_T50_DEPTH_MM <= layer_mid_depth) {
			Tsoil50 = m_Tsoil[i][j];
			break;
		}

		if (j > 0 && WALTER_T50_DEPTH_MM <= layer_mid_depth) {

			float depth_interval = layer_mid_depth - previous_mid_depth;

			if (depth_interval > 0.0f) {

				float interpolation_fraction = (WALTER_T50_DEPTH_MM - previous_mid_depth) / depth_interval;

				Tsoil50 = previous_temperature + interpolation_fraction * (m_Tsoil[i][j] - previous_temperature);
			}
			else {
				Tsoil50 = m_Tsoil[i][j];
			}

			break;
		}

		previous_mid_depth = layer_mid_depth;
		previous_temperature = m_Tsoil[i][j];
		cumulative_depth = layer_bottom_depth;
	}

	// Calculate Walter plant growth factor
	const float walterTmat = m_walterTgr + 10.0f;

	float fgrow = 0.0f;

	if (Tsoil50 <= m_walterTgr) {
		fgrow = WALTER_AMIN;
	}
	else if (Tsoil50 < walterTmat) {

		float tempfactor = (walterTmat - Tsoil50) / (walterTmat - m_walterTgr);

		fgrow = WALTER_AMIN + (WALTER_AMAX - WALTER_AMIN) * (1.0f - tempfactor * tempfactor);
	}
	else {
		fgrow = WALTER_AMAX;
	}

	if (fgrow <= 0.0f) {
		return 0.0f;
	}

	// Calculate the total soil profile depth [mm]
	float profiledepth = 0.0f;

	for (int j = 0; j < nSoilLyrs; j++) {
		profiledepth += m_layer_thickness[i][j];
	}

	// Use the same effective root depth as InitRootFractions()
	float effective_rootdepth = m_rootDepth[i];

	if (effective_rootdepth > profiledepth) {
		effective_rootdepth = profiledepth;
	}

	if (effective_rootdepth <= 0.0f) {
		return 0.0f;
	}

	// Convert root depth from mm to cm
	float effective_rootdepth_cm = effective_rootdepth / 10.0f;

	// Calculate plant-mediated CH4 transport in each soil layer
	for (int j = 0; j < nSoilLyrs; j++) {

		float layer_thickness_cm = m_layer_thickness[i][j] / 10.0f;

		if (layer_thickness_cm <= 0.0f || m_rootFraction[i][j] <= 0.0f || m_CH4[i][j] <= 0.0f) {
			continue;
		}

		// No plant transport from an excessively dry layer
		if ((m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) < water_min) {
			continue;
		}

		//// Average Walter root density in this soil layer
		//float rootdensity = m_rootFraction[i][j] / layer_thickness_cm;

		//// Plant-mediated transport rate, h-1
		//float plant_rate = WALTER_KP * WALTER_TVEG * fgrow * rootdensity;

		// Convert the integrated root fraction into the
		// layer-mean Walter root distribution factor.
		// Walter froot is dimensionless and ranges from 0 to 2.
		float rootfactor = m_rootFraction[i][j] * effective_rootdepth_cm / layer_thickness_cm;

		if (rootfactor < 0.0f) {
			rootfactor = 0.0f;
		}
		else if (rootfactor > 2.0f) {
			rootfactor = 2.0f;
		}

		// Plant-mediated transport rate [h-1]
		float plant_rate = WALTER_KP * m_walterTveg * fgrow * rootfactor;

		// Convert hourly rate into daily removal fraction
		float daily_fraction = 1.0f - exp(-plant_rate * 24.0f);

		if (daily_fraction < 0.0f) {
			daily_fraction = 0.0f;
		}
		else if (daily_fraction > 1.0f) {
			daily_fraction = 1.0f;
		}

		float removed_ch4 = m_CH4[i][j] * daily_fraction;

		if (removed_ch4 > m_CH4[i][j]) {
			removed_ch4 = m_CH4[i][j];
		}

		float oxidized_ch4 = removed_ch4 * m_walterPox;

		float emitted_ch4 = removed_ch4 - oxidized_ch4;

		// Remove CH4 entering the plant transport pathway
		m_CH4[i][j] -= removed_ch4;

		// Rhizosphere-oxidized CH4 becomes CO2
		m_CO2_soil[i][j] += oxidized_ch4;

		// m_CH4_oxid is stored in mol C
		m_CH4_oxid[i][j] += oxidized_ch4 / atomiccmass;

		plantOxidToday += oxidized_ch4;
		plantTransportToday += emitted_ch4;
	}

	return plantTransportToday;
}


bool CH4_LPJGUESS::Calculate_Gas_Ebullition(int i, float& ebull_today) {

	// Gas ebullition
	// Returns the gas ebullition today
	// See Wania et al. (2010) - Sec 2.7

	float waterheight = 0.0;
	ebull_today = 0.0;

	const bool debugEbullCell = i == 1366 || i == 1367 || i == 1368;
	bool debugEbullSelectedYear = false;
	char debugEbullDate[16] = {};

	if (DEBUG_CH4_EBULL && debugEbullCell) {

		const std::time_t currentDate = static_cast<std::time_t>(m_date);

		std::tm dateInfo = {};

#ifdef _WIN32
		localtime_s(&dateInfo, &currentDate);
#else
		localtime_r(&currentDate, &dateInfo);
#endif

		const int debugYear = dateInfo.tm_year + 1900;

		debugEbullSelectedYear = debugYear >= 1980 && debugYear <= 2015;

		std::strftime(debugEbullDate, sizeof(debugEbullDate), "%Y-%m-%d", &dateInfo);
	}


	// Equivalent to IDX - 1 in the public LPJ-GUESS code.
    // It is only used as a comparison boundary and must never
    // be used as an array index.
	const int bubbleToLayer = -1;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// Reset today's layer ebullition diagnostics.
		m_CH4_ebull_ind[i][j] = 0.0f;
		m_CH4_ebull_vol[i][j] = 0.0f;

		float Tsoil = m_Tsoil[i][j];
		float TsoilK = Tsoil + K2degC;

		waterheight += m_total_volume_water[i][j];   // m

		// bubbleToLayer is -1, so all real soil layers
		// participate in ebullition.
		if (j > bubbleToLayer) {

			// Max CH4 that can be dissolved (Wania et al. (2010), Eqn 15)
			float CH4_diss_max = 0.05708 - 0.001545 * Max(0.0f, Tsoil) + 0.00002069 * Max(0.0f, Tsoil * Tsoil);
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
				m_CH4_diss[i][j] = Min(CH4_diss_max_g, henry_k_cc_CH4 *  m_CH4[i][j]);
				//std::cout << "henry_k_cc_CH4=" << henry_k_cc_CH4 << "\n";

				//// Override (a la Wania et al. 2010) with this new, simpler ebullition
				//m_CH4_diss[i][j] = min(CH4_diss_max_g, m_CH4[i][j]);

				m_CH4_gas[i][j] = m_CH4[i][j] - m_CH4_diss[i][j];              // g CH4-C layer-1 - gas
				m_CH4_gas_vol[i][j] = m_CH4_gas[i][j] / atomiccmass * R_gas * TsoilK / (hydro_press + atm_press);      // [m3]
				m_CH4_vgc[i][j] = m_CH4_gas_vol[i][j] / (m_layer_thickness[i][j] / MM_PER_M * SQ_M);            // m3/m3


				const float vgc_threshold = vgc_high * bubble_CH4_frac;

				if (DEBUG_CH4_EBULL && debugEbullCell && debugEbullSelectedYear) {

#pragma omp critical(CH4_EBULL_DIAG_WRITE)
						{
							static std::ofstream ebullFile;
							static bool ebullFileInitialized = false;

							if (!ebullFileInitialized) {

								ebullFile.open(
									"D:/SEIMS_C/data/CW_2/CW_2_longterm_model/"
									"OUTPUT0/CH4_EBULL_DIAG.csv",
									std::ios::out | std::ios::trunc
								);

								if (ebullFile.is_open()) {
									ebullFile
										<< "DATE,CELL,LAYER,TSOIL_C,LIQUID_WATER_DEPTH_M,"
										<< "CH4_TOTAL_GC,CH4_DISS_MAX_GC,"
										<< "CH4_DISS_GC,CH4_GAS_GC,"
										<< "CH4_VGC,VGC_THRESHOLD,TRIGGER,"
										<< "FRAC_WATER,FRAC_BELOW_PWP,FRAC_LIQUID,"
										<< "FAIR,ANOXIC,"
										<< "HAND_WATER_DEPTH_AFT_DEP_m\n";
								}

								ebullFileInitialized = true;
							}

							if (ebullFile.is_open()) {
								ebullFile
									<< std::fixed
									<< std::setprecision(8)
									<< debugEbullDate << ','
									<< i << ','
									<< j << ','
									<< Tsoil << ','
									<< m_volume_liquid_water[i][j] << ','
									<< m_CH4[i][j] << ','
									<< CH4_diss_max_g << ','
									<< m_CH4_diss[i][j] << ','
									<< m_CH4_gas[i][j] << ','
									<< m_CH4_vgc[i][j] << ','
									<< vgc_threshold << ','
									<< (m_CH4_vgc[i][j] > vgc_threshold ? 1 : 0) << ','
									<< m_Frac_water[i][j] << ','
									<< m_Frac_water_belowpwp[i][j] << ','
									<< m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j] << ','
									<< m_Fair[i][j] << ','
									<< m_anoxic[i][j] << ','
									<< m_handWtrDep[i]
									<< '\n';
							}
						}
				}

				// Ebullition/bubble formation if the volumetric gas content (VGC) exceeds vgc_high * bubble_CH4_frac, 
				// where bubble_CH4_frac id the CH4 fraction of gas bubbles
				if (m_CH4_vgc[i][j] > vgc_high * bubble_CH4_frac) {

					m_CH4_ebull_vol[i][j] = (m_CH4_vgc[i][j] - vgc_low * bubble_CH4_frac) * m_layer_thickness[i][j] / MM_PER_M * SQ_M;   // m3 CH4 per m2 ground area
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
	bool allow_planttransport = true;
	bool allow_Ebullition = true;
	bool allow_CH4diffusion = true;
	bool allow_O2diffusion = true;
	float snowDepth = CalculateSnowDepth(i);

	// Daily diffusion 
	float O2_diff_today;  
	float CO2_diff_today;
	float CH4_diff_today;

	// Daily plant transport
	float O2_plant_today;
	float CH4_plant_today;
	float CO2_plant_today;

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
	CH4_plant_today = 0.0;
	CO2_plant_today = 0.0;
	O2_plant_today = 0.0;


	// *** STEP 0 ***

	// C budget before today's methane calculations

	Calculate_Carbon_Store(i, daynum, false);
	m_CH4_C_store_init = m_CH4_store;
	m_CO2_C_store_init = m_CO2_store;


	// *** STEP 1 ***
	// Diffusivities and layer depths, and update of gas constants

	// Set layer gas diffusivities
	Calculate_Gas_Diffusivities(i);
	// Units: m2 d-1

	// Layer thickness in m.
    // Water-volume variables are volumes per unit ground area (m3 m-2),
    // numerically equivalent to water depth (m).

	float cumulative_depth = 0.0f;


	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		float layer_thickness = m_layer_thickness[i][j];
		float porosity = m_soilPor[i][j];
		float water_content = Max(0.0f, m_soil_water_storage[i][j]);
		float ice_content = Max(0.0f, m_soilIceSto[i][j]);

		float liquid_below_pwp = Max(0.0f, m_soilWP[i][j] - ice_content);

		m_Frac_water[i][j] = water_content / layer_thickness;

		m_Frac_ice[i][j] = ice_content / layer_thickness;

		m_Frac_water_belowpwp[i][j] = liquid_below_pwp / layer_thickness;

		m_Dz_metre[i][j] = layer_thickness / MM_PER_M;

		m_volume_liquid_water[i][j] = (m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) * m_Dz_metre[i][j];

		m_total_volume_water[i][j] = m_volume_liquid_water[i][j] + m_Frac_ice[i][j] * m_Dz_metre[i][j];

		cumulative_depth += layer_thickness;
	}


	// If we have standing water...
	if (m_handWtrDep[i] > 0.0) {
		// m_handWtrDep in m
		/*m_Dz_metre[i][0] += m_handWtrDep[i];
		m_volume_liquid_water[i][0] += (m_Frac_water[i][0] + m_Frac_water_belowpwp[i][0]) * m_handWtrDep[i];
		m_total_volume_water[i][0] += (m_Frac_water[i][0] + m_Frac_ice[i][0] + m_Frac_water_belowpwp[i][0]) * m_handWtrDep[i];*/
		// Surface standing water is 100% liquid water.
		m_Dz_metre[i][0] += m_handWtrDep[i];
		m_volume_liquid_water[i][0] += m_handWtrDep[i];
		m_total_volume_water[i][0] += m_handWtrDep[i];
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

	// Convert all layer RSPC values to profile-total Rh.
    // m_sol_RSPC: kg C ha-1 d-1
    // total_Rh:   g C m-2 d-1
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		float rspc_layer_Rh = m_sol_RSPC[i][j] * KG_HA_TO_G_M2;

		total_Rh += rspc_layer_Rh;
	}
	
	// Today's decomposition of litter and soil C pools
	float C_input = 0.0;               // Conservation check - C in methane produced  记录今天产生的所有 C（CH₄ + CO₂）
	float CH4_Init_Prod = 0.0;         // The total CH₄ originally generated on that day (before oxidation, diffusion, and bubbling)
	float Total_CH4 = 0.0;             // Calculate the total amount of CH₄ in all layers

	float layerTopDepthMm = 0.0f;

	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		const float layerBottomDepthMm = layerTopDepthMm + m_layer_thickness[i][j];

		//// Original layer RSPC from NUTR_TF.
		//float rspc_layer_Rh = m_sol_RSPC[i][j] * KG_HA_TO_G_M2;

		//// LPJ-GUESS redistribution of profile-total Rh.
		//float m_dRh = m_peatRootFraction[i][j] * total_Rh;

		// Use the original layer-specific heterotrophic respiration from WISE.
        // m_sol_RSPC: kg C ha-1 d-1
        // m_dRh:      g C m-2 d-1
		float rspc_layer_Rh = m_sol_RSPC[i][j] * KG_HA_TO_G_M2;

		float m_dRh = rspc_layer_Rh;

		const bool FULL_ANOXIC_TEST = false;

		float anoxic = m_anoxic[i][j];

		if (FULL_ANOXIC_TEST) {
			anoxic = 1.0f;
		}

		float C_input_old = C_input;

		// *** CH4 PRODUCTION IN THIS LAYER ***
		if ((m_Frac_water[i][j] + m_Frac_water_belowpwp[i][j]) < water_min) {
			m_CH4_prod[i][j] = 0.0f;
		}
		else {
			m_CH4_prod[i][j] = anoxic * m_ch4Co2Ratio * m_dRh;
		}

		// ---------------------------------------------------------------------
        // Temporary diagnostics for CH4 production.
        // This block only writes diagnostic values and does not modify the model.
        // ---------------------------------------------------------------------
		/*const bool DEBUG_CH4_PRODUCTION = false;*/
		const bool DEBUG_CH4_PRODUCTION = true;

		if (DEBUG_CH4_PRODUCTION) {

			const bool debugCell =
				i == 1366 ||
				i == 1367 ||
				i == 1368;

			if (debugCell) {

				const std::time_t currentDate =
					static_cast<std::time_t>(m_date);

				std::tm dateInfo = {};

#ifdef _WIN32
				localtime_s(&dateInfo, &currentDate);
#else
				localtime_r(&currentDate, &dateInfo);
#endif

				const int debugYear = dateInfo.tm_year + 1900;

				// Write layer-level CH4 production diagnostics for 1980-2015.
				const bool debugSelectedYear = debugYear >= 1980 && debugYear <= 2015;

				if (debugSelectedYear) {

					char dateText[16] = {};

					std::strftime(
						dateText,
						sizeof(dateText),
						"%Y-%m-%d",
						&dateInfo
					);

					const float fracLiquid =
						m_Frac_water[i][j] +
						m_Frac_water_belowpwp[i][j];

					const bool passWater =
						fracLiquid >= water_min;

					const float ch4ProdCheck =
						passWater
						? anoxic * m_ch4Co2Ratio * m_dRh
						: 0.0f;

#pragma omp critical(CH4_PRODUCTION_DIAG_WRITE)
					{
						static std::ofstream debugFile;
						static bool debugFileInitialized = false;

						if (!debugFileInitialized) {

							const char* outputPath =
								"D:/SEIMS_C/data/CW_2/CW_2_longterm_model/"
								"OUTPUT0/CH4_PRODUCTION_DIAG.csv";

							debugFile.open(
								outputPath,
								std::ios::out | std::ios::trunc
							);

							if (!debugFile.is_open()) {
								std::cerr
									<< "[CH4_DIAG_ERROR] Cannot open: "
									<< outputPath
									<< std::endl;
							}
							else {
								debugFile
									<< "DATE,"
									<< "CELL,"
									<< "LAYER,"
									<< "LAYER_TOP_DEPTH_mm,"
									<< "LAYER_BOTTOM_DEPTH_mm,"
									<< "RSPC_kgCha_d,"
									<< "RSPC_LAYER_RH_gCm2d,"
									<< "PROFILE_TOTAL_RH_gCm2d,"
									<< "LPJ_PEAT_FRACTION_DIAG_ONLY,"
									<< "DWYER_ROOT_FRACTION,"
									<< "ROOT_DEPTH_mm,"
									<< "LAYER_RH_USED_gCm2d,"
									<< "SOIL_WATER_mm,"
									<< "PWP_mm,"
									<< "ICE_mm,"
									<< "THICKNESS_mm,"
									<< "FRAC_WATER,"
									<< "FRAC_BELOW_PWP,"
									<< "FRAC_LIQUID,"
									<< "PASS_WATER,"
									<< "POROSITY,"
									<< "FAIR,"
									<< "ANOXIC,"
									<< "CH4_CO2_RATIO,"
									<< "CH4_PROD_CHECK_gCm2d,"
									<< "CH4_PROD_MODEL_gCm2d,"
									<< "PROD_DIFFERENCE,"
									<< "TSOIL_C,"
									<< "HAND_WATER_DEPTH_AFT_DEP_m"
									<< '\n';
							}

							debugFileInitialized = true;
						}

						if (debugFile.is_open()) {

							debugFile
								<< std::fixed
								<< std::setprecision(8)
								<< dateText << ','
								<< i << ','
								<< j << ','
								<< layerTopDepthMm << ','
								<< layerBottomDepthMm << ','
								<< m_sol_RSPC[i][j] << ','
								<< rspc_layer_Rh << ','
								<< total_Rh << ','
								<< m_peatRootFraction[i][j] << ','
								<< m_rootFraction[i][j] << ','
								<< m_rootDepth[i] << ','
								<< m_dRh << ','
								<< m_soil_water_storage[i][j] << ','
								<< m_soilWP[i][j] << ','
								<< m_soilIceSto[i][j] << ','
								<< m_layer_thickness[i][j] << ','
								<< m_Frac_water[i][j] << ','
								<< m_Frac_water_belowpwp[i][j] << ','
								<< fracLiquid << ','
								<< (passWater ? 1 : 0) << ','
								<< m_soilPor[i][j] << ','
								<< m_Fair[i][j] << ','
								<< anoxic << ','
								<< m_ch4Co2Ratio << ','
								<< ch4ProdCheck << ','
								<< m_CH4_prod[i][j] << ','
								<< m_CH4_prod[i][j] - ch4ProdCheck << ','
								<< m_Tsoil[i][j] << ','
								<< m_handWtrDep[i]
								<< '\n';
						}
					}
				}
			}
		}

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
		float Ought_to_have_been = m_dRh;                 // Theoretically, the total amount of carbon that this layer should have produced today

		// Error (used to detect whether the model has correctly allocated carbon)
		float Er = Ought_to_have_been - C_Added;          // Theoretical production quantity - The actual quantity added to the system
		// The bottom of the current layer is the top of the next layer.
		layerTopDepthMm = layerBottomDepthMm;
	}

	// C conservation test:
	Calculate_Carbon_Store(i, daynum, true);

	m_CH4_C_store_now = m_CH4_store;
	m_CO2_C_store_now = m_CO2_store;
	float check = m_CH4_C_store_now + m_CO2_C_store_now - m_CH4_C_store_init - m_CO2_C_store_init - total_Rh;


	// *** STEP 3 ***
	// Diffusion of O2
	float DailyO2diffusion = 0.0;     // 当日扩散总量
	float MolesO2IntoSoil = 0.0;      // 进入土壤顶层的 O₂ 摩尔数

	/*if (allow_O2diffusion)
		MolesO2IntoSoil = Diffuse_Gas(i, O2_gas, DailyO2diffusion);*/

	if (allow_O2diffusion && snowDepth < SNOW_DIFFUSION_DEPTH_LIMIT_MM) {
		MolesO2IntoSoil = Diffuse_Gas(i, O2_gas, DailyO2diffusion);
	}
	O2_diff_today = DailyO2diffusion;                   // mol O2 into the soil (and then diffused downwards) 
	// Should be negative, i.e. O2 diffuses INTO the soil


	// *** STEP 4 ***
	// Plant transport of oxygen
	// LPJ-GUESS plant-mediated O2 transport

	O2_plant_today = 0.0f;
	float tillers = 0.0f;

	if (allow_planttransport) {

		tillers = Calculate_tiller_areas(i);

		if (tillers > 1.0e-12f) {
			Plant_Gas_Transport(i, O2_gas, O2_plant_today);
		}
	}


	// *** STEP 5 ***
	// Diffusion of CH4
	float CH4_before_diffusion = m_CH4_store;   // The content of CH₄ before diffusion
	float DailyCH4diffusion = 0.0;
	float GramCH4IntoSoil = 0.0;

	if (allow_CH4diffusion && snowDepth < SNOW_DIFFUSION_DEPTH_LIMIT_MM) {
		GramCH4IntoSoil = Diffuse_Gas(i, CH4_gas, DailyCH4diffusion);
	}
	CH4_diff_today = -DailyCH4diffusion;   // SHOULD BE 0 - gC m-2 d-1
	// Should be positive, i.e. upward flux
	

	if (fabs(CH4_diff_today) < MAX_ERR)
		CH4_diff_today = 0.0;              // remove tiny values

	// C conserved?
	Calculate_Carbon_Store(i, daynum, true);
	check = m_CH4_store - CH4_before_diffusion + CH4_diff_today;


	// *** STEP 6 ***
	// CH4 oxidation

	// CH4 units: gC layer-1 
	// O2 units: mol layer-1

	// Assume that 1/2 of the O2 is utilized by other electron
	// acceptors and you need 2 moles of O2 to oxidise 1 mole of CH4.
	for (int j = 0; j < m_nSoilLyrs[i]; j++) {

		// O2 is in [mol layer-1]
		m_O2[i][j] *= m_oxidFrac;            // since ((1-oxid_frac)*100)% of O2 used by roots themselves... [mol layer-1] 
		m_CH4[i][j] /= atomiccmass;          // mol layer-1
		m_CH4_oxid[i][j] = Min(m_CH4[i][j], 0.5f * m_O2[i][j]);               // usually 75%
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
	// Plant-mediated CH4 transport based on Walter and Heimann (2000)

	//CH4_plant_today = 0.0f;
	//float CH4_plant_oxid_today = 0.0f;

	//if (allow_planttransport) {
	//	Plant_CH4_Transport(i, CH4_plant_today,CH4_plant_oxid_today);

	//	// Add rhizosphere oxidation associated with plant transport
	//	// to the daily total methane oxidation
	//	CH4_oxid_daily += CH4_plant_oxid_today;
	//}

	// *** STEP 7 ***
    // LPJ-GUESS plant-mediated CH4 transport

	CH4_plant_today = 0.0f;

	if (allow_planttransport && tillers > 1.0e-12f) {

		Plant_Gas_Transport(i, CH4_gas, CH4_plant_today);
	}

	//CH4_plant_today = 0.0f;

	//if (allow_planttransport) {
	//	float tillers = Calculate_tiller_areas(i);

	//	// Only allow plant transport if there are active wetland graminoid tillers present
	//	if (tillers > 1.0e-12f) {
	//		Plant_Gas_Transport(i, CH4_gas, CH4_plant_today);   // CH4 into soil through plants (gC m-2)
	//	}
	//}

	// Check C budget
	Calculate_Carbon_Store(i, daynum, true);
	check = m_CH4_store + m_CO2_store + CH4_diff_today + CH4_plant_today;
	float checkafter2 = check - checkbefore;

	/*if (fabs(checkafter2) > MAX_ERR) {
		std::cout << "[CH4] Carbon imbalance after plant transport of CH4: "
			<< checkafter2
			<< "  (Cell " << i << ")"
			<< std::endl;
	}*/

	checkbefore = check;


	// *** STEP 8 ***
	// Ebullition of CH4
	if (allow_Ebullition)
		Calculate_Gas_Ebullition(i, CH4_ebull_today);
	// reduces CH4 and updates CH4_ebull_today

	// Check C budget
	Calculate_Carbon_Store(i, daynum, true);
	//check = m_CH4_store + m_CO2_store + CH4_diff_today + CH4_plant_today + CH4_ebull_today;
	check = m_CH4_store + m_CO2_store + CH4_diff_today  + CH4_ebull_today + CH4_plant_today;

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

	CO2_plant_today = 0.0;


	// *** STEP 9 ***
	// Conservation steps

	// Total C flux out of the soil today [gC m-2 d-1]
	//float total_C_flux = CO2_diff_today + CH4_diff_today + CH4_ebull_today + CH4_plant_today + CO2_plant_today;
	float total_C_flux = CO2_diff_today + CH4_diff_today + CH4_ebull_today + CH4_plant_today + CO2_plant_today;      
	// CO2_flux_today = CO2_diff_today + CO2_plant_today;
	// Calculate daily CH4 flux [gCH4-C m-2 d-1]
	//float CH4_flux_today = CH4_diff_today + CH4_plant_today + CH4_ebull_today;     
	float CH4_flux_today = CH4_diff_today + CH4_plant_today + CH4_ebull_today;         
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

	//m_CH4_prod_hour[i] = CH4_daily_total / 24.0f;      // Daily methane production  gC/m2/hour
	//m_CH4_oxid_hour[i] = CH4_oxid_daily / 24.0f;       // Daily methane oxidation   gC/m2/hour
	//m_CH4_diff_hour[i] = CH4_diff_today / 24.0f;       // Diffusive flux            gC/m2/hour
	//m_CH4_ebull_hour[i] = CH4_ebull_today / 24.0f;     // Ebullition flux           gC/m2/hour
	//m_CH4_plant_hour[i] = CH4_plant_today / 24.0f;     // Plant transport flux      gC/m2/hour
	//m_CH4_flux_hour[i] = CH4_flux_today / 24.0f;       // Total CH4 flux            gC/m2/hour

	// Convert internal g CH4-C m-2 d-1
    // to output mg CH4 m-2 h-1.
	m_CH4_prod_hour[i] = CH4_daily_total / 24.0f * GC_TO_MG_CH4;
	m_CH4_oxid_hour[i] = CH4_oxid_daily / 24.0f * GC_TO_MG_CH4;
	m_CH4_diff_hour[i] = CH4_diff_today / 24.0f * GC_TO_MG_CH4;
	m_CH4_ebull_hour[i] = CH4_ebull_today / 24.0f * GC_TO_MG_CH4;
	m_CH4_plant_hour[i] = CH4_plant_today / 24.0f * GC_TO_MG_CH4;
	m_CH4_flux_hour[i] = CH4_flux_today / 24.0f * GC_TO_MG_CH4;

	return true;
}


//int CH4_LPJGUESS::Execute() {
//
//	CheckInputData();
//	InitialOutputs();
//
////#pragma omp parallel for
//	for (int i = 0; i < m_nCells; i++) {
//
//		if (m_nSoilLyrs[i] <= 0)  continue;
//
//		if (!std::isfinite(m_handWtrDep[i])) {
//			throw ModelException(MID_CH4_LPJGUESS, "Execute",
//				"VAR_OL_HAND_WTRDEP_AFT_INFIL contains a non-finite value at cell " +
//				std::to_string(i) + ".");
//		}
//
//		if (m_handWtrDep[i] < -1.0e-8f) {
//			throw ModelException(MID_CH4_LPJGUESS, "Execute",
//				"VAR_OL_HAND_WTRDEP_AFT_INFIL is negative at cell " +
//				std::to_string(i) +
//				", value=" + std::to_string(m_handWtrDep[i]) + ".");
//		}
//
//		// PG_EPIC root-depth-based Walter transport fraction
//		InitRootFractions(i);
//
//		// LPJ-GUESS peat root fraction for profile Rh redistribution
//		InitPeatRootFractions(i);
//
//		InitFracAir(i);
//
//		InitAnoxic(i);
//
//		Methane(i, m_day);
//
//	}
//	return 0;
//}


int CH4_LPJGUESS::Execute() {

	CheckInputData();
	InitialOutputs();

	//#pragma omp parallel for
	for (int i = 0; i < m_nCells; i++) {

		// 每个时间步先清零当前HRU的日输出。
		// InitialOutputs()只负责首次分配数组，不会每日清零。
		m_CH4_prod_hour[i] = 0.0f;
		m_CH4_oxid_hour[i] = 0.0f;
		m_CH4_diff_hour[i] = 0.0f;
		m_CH4_ebull_hour[i] = 0.0f;
		m_CH4_plant_hour[i] = 0.0f;
		m_CH4_flux_hour[i] = 0.0f;

		if (m_nSoilLyrs[i] <= 0) {
			continue;
		}

		if (!std::isfinite(m_landuse[i])) {
			throw ModelException(
	            MID_CH4_LPJGUESS,
				"Execute",
				"LANDUSE contains a non-finite value at cell " +
				std::to_string(i) + ".");
		}

		if (!std::isfinite(m_cellLat[i])) {
			throw ModelException(
				MID_CH4_LPJGUESS,
				"Execute",
				"CELL_LAT contains a non-finite value at cell " +
				std::to_string(i) + ".");
		}

		// 根据土地利用和纬度自动选择甲烷计算方法。
		const MethaneMethod methaneMethod =
			SelectMethaneMethod(i);

		// 非湿地HRU不进行甲烷计算。
		if (methaneMethod == CH4_METHOD_NONE) {
			continue;
		}

		// 对参与甲烷计算的HRU检查HAND水深。
		if (!std::isfinite(m_handWtrDep[i])) {
			throw ModelException(
				MID_CH4_LPJGUESS,
				"Execute",
				"VAR_OL_HAND_WTRDEP_AFT_DEP contains "
				"a non-finite value at cell " +
				std::to_string(i) + ".");
		}

		if (m_handWtrDep[i] < -1.0e-8f) {
			throw ModelException(
				MID_CH4_LPJGUESS,
				"Execute",
				"VAR_OL_HAND_WTRDEP_AFT_DEP is negative "
				"at cell " +
				std::to_string(i) +
				", value=" +
				std::to_string(m_handWtrDep[i]) + ".");
		}


		// 低纬度湿地：Spahni简化方法。
		if (methaneMethod == CH4_METHOD_SPAHNI) {

			MethaneSpahni(i);
			continue;
		}

		// 高纬度湿地/泥炭地：Wania分层方法。
		if (methaneMethod == CH4_METHOD_WANIA) {

			// PG_EPIC动态根深生成Walter植物传输根系比例。
			InitRootFractions(i);

			// LPJ-GUESS泥炭地根系比例，用于剖面RSPC重新分配。
			InitPeatRootFractions(i);

			// 计算各土层空气体积分数。
			InitFracAir(i);

			// 计算各土层厌氧比例。
			InitAnoxic(i);

			// 执行Wania分层甲烷产生、扩散、氧化、
			// 植物传输和冒泡过程。
			Methane(i, m_day);
		}
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
	else if (StringMatch(sk, VAR_CH4_PLANT)) {
		*data = m_CH4_plant_hour;
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
	else if (StringMatch(sk, VAR_TILLER_AREA)) {
		*data = m_tiller_area;
	}
	else {
		throw ModelException(MID_CH4_LPJGUESS, "Get2DData", "Result " + sk + " does not exist.");
	}
}
