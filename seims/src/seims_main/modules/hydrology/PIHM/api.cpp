#include "api.h"

#include "pihm.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new class PIHM();
}

/// function to return the XML Metadata document string
extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;

    mdi.SetAuthor("Liangjun Zhu");
    mdi.SetClass("TEST", "Functionality test of the module template!");
    mdi.SetDescription("Template of SEIMS module");
    mdi.SetID("PIHM");
    mdi.SetName("PIHM");
    mdi.SetVersion("1.0");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    /// Set parameters from database (Source_ParameterDB or Source_ParameterDB_Optional)

    /// Parameters with basic data types
    //mdi.AddParameter("SingleValueParam", "UNIT", "DESC", Source_ParameterDB, DT_Single);
    //mdi.AddParameter("OptioanlParam", "UNIT", "DESC", Source_ParameterDB_Optional, DT_Single);
    //mdi.AddParameter("1DArrayParam", "UNIT", "DESC", Source_ParameterDB, DT_Array1D);
    //mdi.AddParameter("1DRasterParam", "UNIT", "DESC", Source_ParameterDB, DT_Raster1D);
    //mdi.AddParameter("2DArrayParam", "UNIT", "DESC", Source_ParameterDB, DT_Array2D);
    //mdi.AddParameter("2DRasterParam", "UNIT", "DESC", Source_ParameterDB, DT_Raster2D);

    /// Parameters with complex data types
    //mdi.AddParameter(VAR_REACH_PARAM, UNIT_NON_DIM, DESC_REACH_PARAM, Source_ParameterDB, DT_Reach);
    //mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);
    //mdi.AddParameter(VAR_SCENARIO, UNIT_NON_DIM, DESC_SCENARIO, Source_ParameterDB, DT_Scenario);
	mdi.AddParameter(Tag_TimeStep, UNIT_HOUR, DESC_TIMESTEP, File_Input, DT_Single);
	mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_SUBBSNID_NUM, UNIT_NON_DIM, DESC_SUBBSNID_NUM, Source_ParameterDB, DT_Single);
	mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
	//mdi.AddParameter(VAR_OL_IUH, UNIT_NON_DIM, DESC_OL_IUH, Source_ParameterDB, DT_Array2D);
	mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	//mdi.AddParameter(VAR_SLOPE, UNIT_PERCENT, DESC_SLOPE, Source_ParameterDB, DT_Raster1D);
	//mdi.AddParameter(VAR_STREAM_LINK, UNIT_NON_DIM, DESC_STREAM_LINK, Source_ParameterDB, DT_Raster1D);

    /// Set inputs from other modules (Source_Module or Source_Module_Optional)
	mdi.AddInput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_SURFRFTOTAL, UNIT_FLOW_CMS, DESC_SURFRFTOTAL, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_SSRUVOL, UNIT_VOL_M3, DESC_SSRUVOL, Source_Module,DT_Raster2D);
	mdi.AddInput(VAR_PCP, UNIT_DEPTH_MM, DESC_PCP, Source_Module, DT_Raster1D); /// ITP_P
	mdi.AddInput(VAR_TMEAN, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_TMAX, UNIT_TEMP_DEG, DESC_TMAX, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_TMIN, UNIT_TEMP_DEG, DESC_TMIN, Source_Module, DT_Raster1D);
	mdi.AddInput(DataType_RelativeAirMoisture, UNIT_PERCENT, DESC_RM, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_WS, UNIT_SPEED_MS, DESC_WS, Source_Module, DT_Raster1D);
	mdi.AddInput(DataType_SolarRadiation, UNIT_SR, DESC_SR, Source_Module, DT_Raster1D);
	//mdi.AddInput(VAR_SBGS, UNIT_DEPTH_MM, DESC_SBGS, Source_Module, DT_Array1D);VAR_SBQG
	mdi.AddInput(VAR_SBQG, UNIT_FLOW_CMS, DESC_SBQG, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_GW_SUBBASIN_AREA, UNIT_AREA_M2, DESC_GW_SUBBASIN_AREA, Source_Module, DT_Array1D);
	
	mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);

	mdi.AddParameter(VAR_AHRU, UNIT_DEPTH_MM, DESC_AHRU, Source_ParameterDB, DT_Raster1D);

	mdi.AddOutput(VAR_OLFLOW, UNIT_DEPTH_MM, DESC_OLFLOW, DT_Raster1D);
    //mdi.AddInput("SingleInput", "UNIT", "DESC", Source_ParameterDB, DT_Single);
    //mdi.AddInput("1DArrayInput", "UNIT", "DESC", Source_ParameterDB, DT_Array1D);
    //mdi.AddInput("1DRasterInput", "UNIT", "DESC", Source_ParameterDB, DT_Raster1D);
    //mdi.AddInput("2DArrayInput", "UNIT", "DESC", Source_ParameterDB, DT_Array2D);
    //mdi.AddInput("2DRasterInput", "UNIT", "DESC", Source_ParameterDB, DT_Raster2D);


    /// Set output variables of the current module
    //mdi.AddOutput("SingleOutput", "UNIT", "DESC", DT_Single);
    //mdi.AddOutput("1DArrayOutput", "UNIT", "DESC", DT_Array1D);
    //mdi.AddOutput("1DRasterOutput", "UNIT", "DESC", DT_Raster1D);
    //mdi.AddOutput("2DArrayOutput", "UNIT", "DESC", DT_Array2D);
    //mdi.AddOutput("2DRasterOutput", "UNIT", "DESC", DT_Raster2D);

    /// Set In/Output variables with transferred data type
    //mdi.AddInOutput("1DArrayOutputSingleInOutput", "UNIT", "DESC", DT_Array1D, TF_SingleValue);
    //mdi.AddInOutput("2DArrayOutput1DArrayInOutput", "UNIT", "DESC", DT_Array2D, TF_OneArray1D);
    /// Write out the XML file.
    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
