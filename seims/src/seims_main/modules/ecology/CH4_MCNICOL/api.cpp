#include "api.h"
#include "CH4_MCNICOL.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
	return new CH4_MCNICOL();
}

/// function to return the XML Metadata document string
extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
	MetadataInfo mdi;

	mdi.SetAuthor("Jing Li");
	mdi.SetClass(MCLS_CH4, MCLSDESC_CH4);
	mdi.SetDescription(MDESC_CH4_MCNICOL);
	mdi.SetID(MID_CH4_MCNICOL);
	mdi.SetName(MID_CH4_MCNICOL);
	mdi.SetVersion("1.0");
	mdi.SetEmail(SEIMS_EMAIL);
	mdi.SetWebsite(SEIMS_SITE);
	mdi.SetHelpfile("");

    // Set parameters from database
    // Soil information used to estimate WFPS and water table below surface
	mdi.AddParameter(Tag_TimeStep, UNIT_HOUR, DESC_TIMESTEP, File_Input, DT_Single);
	mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_BLAI, UNIT_AREA_RATIO, DESC_BLAI, Source_ParameterDB, DT_Raster1D);

    // McNicol CH4 model parameters
    mdi.AddParameter(VAR_CH4_KP, UNIT_NON_DIM, DESC_CH4_KP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_P1, UNIT_NON_DIM, DESC_CH4_P1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_KO, UNIT_NON_DIM, DESC_CH4_KO, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_P2, UNIT_NON_DIM, DESC_CH4_P2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_QP, UNIT_NON_DIM, DESC_CH4_QP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_P3, UNIT_NON_DIM, DESC_CH4_P3, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_QO, UNIT_NON_DIM, DESC_CH4_QO, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_KEP, UNIT_NON_DIM, DESC_CH4_KEP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH4_P5, UNIT_NON_DIM, DESC_CH4_P5, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_CH4_LAI_LAG, UNIT_NON_DIM, DESC_CH4_LAI_LAG, Source_ParameterDB, DT_Single);

    // Set inputs from other modules
    // Soil temperature, from STP_FP
    mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);

    // Soil water storage, from soil water module
    mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Array2D);

    // HAND water depth after infiltration/depression process
    mdi.AddInput(VAR_OL_HAND_WTRDEP_AFT_DEP, UNIT_LEN_M, DESC_OLFLOW, Source_Module, DT_Raster1D);

    // LAI, from Biomass_EPIC
    mdi.AddInput(VAR_LAIDAY, UNIT_AREA_RATIO, DESC_LAIDAY, Source_Module, DT_Raster1D);

	// Maximum LAI for current year, from Biomass_EPIC
	mdi.AddInput(VAR_LAIYRMAX, UNIT_AREA_RATIO, DESC_LAIYRMAX, Source_Module, DT_Raster1D);

    // Set output variables of the current module
    mdi.AddOutput(VAR_CH4_MCNICOL_FLUX, UNIT_NON_DIM, DESC_CH4_MCNICOL_FLUX, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_MS, UNIT_NON_DIM, DESC_CH4_MCNICOL_MS, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_PROD, UNIT_NON_DIM, DESC_CH4_MCNICOL_PROD, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_OXID, UNIT_NON_DIM, DESC_CH4_MCNICOL_OXID, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_EMIT, UNIT_NON_DIM, DESC_CH4_MCNICOL_EMIT, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_ZW, UNIT_NON_DIM, DESC_CH4_MCNICOL_ZW, DT_Array1D);

    // Debug factors
    mdi.AddOutput(VAR_CH4_MCNICOL_FTP, UNIT_NON_DIM, DESC_CH4_MCNICOL_FTP, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_FWP, UNIT_NON_DIM, DESC_CH4_MCNICOL_FWP, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_FBP, UNIT_NON_DIM, DESC_CH4_MCNICOL_FBP, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_FTO, UNIT_NON_DIM, DESC_CH4_MCNICOL_FTO, DT_Array1D);
    mdi.AddOutput(VAR_CH4_MCNICOL_FWO, UNIT_NON_DIM, DESC_CH4_MCNICOL_FWO, DT_Array1D);

    // Write out the XML file
    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
