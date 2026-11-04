#include "api.h"
#include "degreedaymelt.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule *GetInstance() {
    return new DEGREEDAYMELT();
}

// function to return the XML Metadata document string
extern "C" SEIMS_MODULE_API const char *MetadataInformation() {
    MetadataInfo mdi;

    // set the information properties
    mdi.SetAuthor("Ruoyun Cao");
    mdi.SetClass("GLACIER", "Glacier processes");
    mdi.SetDescription("");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetID("GLACIER");
    mdi.SetName("GLACIER");
    mdi.SetVersion("1.0");
    mdi.SetWebsite("");
    mdi.SetHelpfile("");

    //set the parameter
	mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);
	// set the input variables
	mdi.AddInput(VAR_TMEAN, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_PCP, UNIT_DEPTH_MM, DESC_PCP, Source_Module, DT_Raster1D);
	mdi.AddInput(DataType_SolarRadiation, "UNIT_SRA", DESC_SRA, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_TMAX, UNIT_TEMP_DEG, DESC_TMAX, Source_Module, DT_Raster1D);

    mdi.AddParameter(VAR_AHRU, UNIT_DEPTH_MM, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
    // set the output variables

    //ljj++
	mdi.AddOutput("GL_V", UNIT_VOL_M3, "DESC_GL_V", DT_Array1D);
	mdi.AddOutput("GL_RO", UNIT_DEPTH_MM, "DESC_GL_RO", DT_Array1D);
    mdi.AddOutput("Qfg", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
    mdi.AddParameter("Kfg", UNIT_NON_DIM, "Kfg", Source_ParameterDB, DT_Single);
	mdi.AddParameter("Ca", UNIT_NON_DIM, "Ca", Source_ParameterDB, DT_Single);
	mdi.AddParameter("Cg", UNIT_NON_DIM, "Cg", Source_ParameterDB, DT_Single);
    mdi.AddParameter("GL_SNDD", UNIT_NON_DIM, "DESC_GL_SNDD", Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_C_SNOW6, UNIT_MELT_FACTOR, DESC_C_SNOW6, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_C_SNOW12, UNIT_MELT_FACTOR, DESC_C_SNOW12, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_T_SNOW, UNIT_TEMP_DEG, DESC_T_SNOW, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_T0, UNIT_TEMP_DEG, DESC_T0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_LAG_SNOW, UNIT_NON_DIM, DESC_LAG_SNOW, Source_ParameterDB, DT_Single);
    // write out the XML file.
    string res = mdi.GetXMLDocument();

    char *tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
