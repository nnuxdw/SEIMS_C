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

	mdi.AddParameter(VAR_CH4_R, UNIT_NON_DIM, DESC_CH4_R, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_CH4_TREF, UNIT_NON_DIM, DESC_CH4_TREF, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_CH4_TAU_PROD, UNIT_LEN_M, DESC_CH4_TAU_PROD, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_CH4_Z_OATZ, UNIT_LEN_M, DESC_CH4_Z_OATZ, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_CH4_TAU_OXID, UNIT_LEN_M, DESC_CH4_TAU_OXID, Source_ParameterDB, DT_Single);

	/// Set inputs from other modules (Source_Module or Source_Module_Optional)
	mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Array2D);
	mdi.AddInput(VAR_SOL_WOC, UNIT_CONT_KGHA, DESC_SOL_WOC, Source_Module, DT_Raster2D);

	mdi.AddInput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OLFLOW, Source_Module, DT_Raster1D);        // hand水深(m)
	mdi.AddInput(VAR_INFIL, UNIT_DEPTH_MM, DESC_INFIL, Source_Module, DT_Raster1D);              // 入渗(mm)
	mdi.AddInput(VAR_NEPR, UNIT_DEPTH_MM, DESC_NEPR, Source_Module, DT_Raster1D);                 // from interception module,净降水(mm)
	mdi.AddInput(VAR_PERCO, UNIT_DEPTH_MM, DESC_PERCO, Source_Module, DT_Array2D);              // 渗漏(mm)
	mdi.AddInput(VAR_SSRU, UNIT_DEPTH_MM, DESC_SSRU, Source_Module_Optional, DT_Raster2D);      // 壤中流(mm)
	mdi.AddInput(VAR_DPST, UNIT_DEPTH_MM, DESC_DPST, Source_Module, DT_Raster1D);               // depression storage(mm)
	mdi.AddInput(VAR_INET, UNIT_DEPTH_MM, DESC_INET, Source_Module, DT_Raster1D);               // Evaporation from intercepted storage
	mdi.AddInput(VAR_EXCP, UNIT_DEPTH_MM, DESC_EXCP, Source_Module, DT_Raster1D);                //Excess precipitation
	//mdi.AddInput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_SOET, UNIT_DEPTH_MM, DESC_SOET, Source_Module, DT_Raster1D);               // actual soil evaporation

    /// Set output variables of the current module
	mdi.AddOutput(VAR_CH4_PRODUCTION, UNIT_G_M2_HOUR, DESC_CH4_PRODUCTION, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_EMISSION_FLUX, UNIT_G_M2_HOUR, DESC_CH4_EMISSION_FLUX, DT_Raster1D);
	mdi.AddOutput(VAR_CH4_TOTAL, UNIT_G_M2_HOUR, DESC_CH4_TOTAL, DT_Single);

	mdi.AddOutput(VAR_SOILSAT_L1, UNIT_WAT_RATIO, DESC_SOILSAT_L1, DT_Raster1D);
	mdi.AddOutput(VAR_SOILSAT_L2, UNIT_WAT_RATIO, DESC_SOILSAT_L2, DT_Raster1D);
	mdi.AddOutput(VAR_SOILSAT_L3, UNIT_WAT_RATIO, DESC_SOILSAT_L3, DT_Raster1D);
	mdi.AddOutput(VAR_SOILSAT_L4, UNIT_WAT_RATIO, DESC_SOILSAT_L4, DT_Raster1D);
	mdi.AddOutput(VAR_SOILSAT_L5, UNIT_WAT_RATIO, DESC_SOILSAT_L5, DT_Raster1D);
	mdi.AddOutput(VAR_SOILSAT_L6, UNIT_WAT_RATIO, DESC_SOILSAT_L6, DT_Raster1D);
	mdi.AddOutput(VAR_SOILSAT_L7, UNIT_WAT_RATIO, DESC_SOILSAT_L7, DT_Raster1D);


    /// Write out the XML file.
    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
