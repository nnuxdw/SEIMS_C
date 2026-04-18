#include "api.h"

#include "CH4_LPJGUESS.h"
#include "MetadataInfo.h"
#include "text.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new CH4_LPJGUESS();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;

    mdi.SetAuthor("Jing Li");
    mdi.SetClass(MCLS_CH4, MCLSDESC_CH4);
    mdi.SetDescription(MDESC_CH4_LPJGUESS);
    mdi.SetID(MID_CH4_LPJGUESS);
    mdi.SetName(MID_CH4_LPJGUESS);
    mdi.SetVersion("4.1");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

	/// Set parameters from database (Source_ParameterDB or Source_ParameterDB_Optional)
	mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);                 //  HRU
	mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);     // 土壤层
	mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);      // 土壤厚度
	mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D);        // 萎蔫点含水量
	mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_CO2, UNIT_GAS_PPMV, DESC_CO2, Source_ParameterDB, DT_Single);                    // 大气CO2浓度
	mdi.AddParameter(VAR_BLAI, UNIT_AREA_RATIO, DESC_BLAI, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);

	/// lookup table as 2D array, such as crop, management, landuse, tillage, etc.
	mdi.AddParameter(VAR_CROP_LOOKUP, UNIT_NON_DIM, DESC_CROP_LOOKUP, Source_ParameterDB, DT_Array2D);

	/// Set input from other modules
	mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);         // 温度
	mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Array2D);      // 当前水位
	mdi.AddInput(VAR_SOLICE, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module_Optional, DT_Array2D);
	mdi.AddInput(VAR_SOL_RSPC, UNIT_CONT_KGHA, DESC_SOL_RSPC, Source_Module_Optional, DT_Raster2D);   // 异养呼吸
	mdi.AddInput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OLFLOW, Source_Module, DT_Raster1D);            // hand水深(m)
	mdi.AddInput(VAR_LAIDAY, UNIT_AREA_RATIO, DESC_LAIDAY, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_BIOMASS, UNIT_CONT_KGHA, DESC_BIOMASS, Source_Module, DT_Raster1D);


    /// Set the output variables
	mdi.AddOutput(VAR_CH4_PROD, UNIT_G_M2_HOUR, DESC_CH4_PROD, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_OXID, UNIT_G_M2_HOUR, DESC_CH4_OXID, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_DIFF, UNIT_G_M2_HOUR, DESC_CH4_DIFF, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_EBUL, UNIT_G_M2_HOUR, DESC_CH4_EBUL, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_FLUX, UNIT_G_M2_HOUR, DESC_CH4_FLUX, DT_Raster1D);




	/// Write out the XML file.
	string res = mdi.GetXMLDocument();
	char* tmp = new char[res.size() + 1];
	strprintf(tmp, res.size() + 1, "%s", res.c_str());
	return tmp;
}
