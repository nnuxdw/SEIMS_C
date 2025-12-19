#include "api.h"
#include "CH4_WETMETH.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new CH4_WETMETH();
}

/// function to return the XML Metadata document string
extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;

    mdi.SetAuthor("");
    mdi.SetClass("ECOLOGY", "WETMETH (Wetland CH4 Model) for methane production and oxidation");
    mdi.SetDescription("WETMETH model implementation for calculating methane emissions from wetlands");
    mdi.SetID("CH4_WETMETH");
    mdi.SetName("CH4_WETMETH");
    mdi.SetVersion("1.0");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    /// Set parameters from database (Source_ParameterDB or Source_ParameterDB_Optional)
    mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    // mdi.AddParameter(VAR_SOL_OM, UNIT_PERCENT, DESC_SOL_OM, Source_ParameterDB, DT_Raster2D); // commented out, using VAR_SOL_WOC from NUTR_TF instead
    mdi.AddParameter(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_UL, Source_ParameterDB, DT_Raster2D);
    mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);

	/// Set inputs from other modules (Source_Module or Source_Module_Optional)
	mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Array2D);
	mdi.AddInput(VAR_SOL_WOC, UNIT_CONT_KGHA, DESC_SOL_WOC, Source_Module, DT_Raster2D);


    /// Set output variables of the current module
	mdi.AddOutput(VAR_CH4_PRODUCTION, UNIT_G_M2_HOUR, DESC_CH4_PRODUCTION, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_EMISSION_FLUX, UNIT_G_M2_HOUR, DESC_CH4_EMISSION_FLUX, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_TOTAL, UNIT_G_M2_HOUR, DESC_CH4_TOTAL, DT_Single);


    /// Write out the XML file.
    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
