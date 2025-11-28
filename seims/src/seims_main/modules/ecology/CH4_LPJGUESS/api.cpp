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
	mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);                 //HRU
	mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);     //土壤层
	mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);      //土壤厚度
	mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_CO2, UNIT_GAS_PPMV, DESC_CO2, Source_ParameterDB, DT_Single);

	/// Set input from other modules
	mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);         //温度
	mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Raster2D);     //当前水位
	mdi.AddInput(VAR_SOLICE, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module_Optional, DT_Array2D);

    /// Set the output variables
	mdi.AddOutput(VAR_CH4_CONC, UNIT_KG, DESC_CH4_CONC, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_DIFF, UNIT_KG, DESC_CH4_DIFF, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_PLANT, UNIT_KG, DESC_CH4_PLANT, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_EBUL, UNIT_KG, DESC_CH4_EBUL, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_OXID, UNIT_KG, DESC_CH4_OXID, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_PROD, UNIT_KG, DESC_CH4_PROD, DT_Raster1D);
	mdi.AddOutput(VAR_SOL_ANOXIC, UNIT_WAT_RATIO, DESC_SOL_ANOXIC, DT_Raster2D);

	/// Write out the XML file.
	string res = mdi.GetXMLDocument();
	char* tmp = new char[res.size() + 1];
	strprintf(tmp, res.size() + 1, "%s", res.c_str());
	return tmp;
}
