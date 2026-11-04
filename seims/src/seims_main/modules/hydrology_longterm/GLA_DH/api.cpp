#include "api.h"
#include "glacier_deltah.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule *GetInstance() {
    return new GLA_DH();
}

// function to return the XML Metadata document string
extern "C" SEIMS_MODULE_API const char *MetadataInformation() {
    MetadataInfo mdi;

    // set the information properties
    mdi.SetAuthor("Jing Ma");
    mdi.SetClass("GLACIER", "Glacier processes");
    mdi.SetDescription("");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetID("GLACIER");
    mdi.SetName("GLACIER");
    mdi.SetVersion("1.0");
    mdi.SetWebsite("");
    mdi.SetHelpfile("");

    //set the parameter
    //mdi.AddParameter(VAR_SUBBSNID_NUM, UNIT_NON_DIM, DESC_SUBBSNID_NUM, Source_ParameterDB, DT_Single);
	mdi.AddParameter("HLU_ID", UNIT_NON_DIM, "DESC_HLU_ID", Source_ParameterDB, DT_Raster2D);
    
	mdi.AddParameter("GL_FACTOR", UNIT_NON_DIM, "DESC_GL_FACTOR", Source_ParameterDB, DT_Single);
	
	//mdi.AddParameter(VAR_TYEAR, UNIT_DAY, DESC_TYEAR, Source_Module, DT_Raster1D);

	//mdi.AddParameter(VAR_GL_SNDD, UNIT_NON_DIM, "DESC_GL_SNDD", Source_ParameterDB, DT_Single);
	mdi.AddParameter("GL_SRF", UNIT_NON_DIM, "DESC_GL_SRF", Source_ParameterDB, DT_Single);
	mdi.AddParameter("Kfg", UNIT_NON_DIM, "Kfg", Source_ParameterDB, DT_Single);
	mdi.AddParameter("Ca", UNIT_NON_DIM, "Ca", Source_ParameterDB, DT_Single);
	mdi.AddParameter("Cg", UNIT_NON_DIM, "Cg", Source_ParameterDB, DT_Single);
	mdi.AddParameter("GL_SNDD", UNIT_NON_DIM, "DESC_GL_SNDD", Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_C_SNOW6, UNIT_MELT_FACTOR, DESC_C_SNOW6, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_C_SNOW12, UNIT_MELT_FACTOR, DESC_C_SNOW12, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_T_SNOW, UNIT_TEMP_DEG, DESC_T_SNOW, Source_ParameterDB, DT_Single);

	//mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Array1D);

	//mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
	//mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Array1D);
	// set the input variables
	mdi.AddInput(VAR_TMEAN, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_PCP, UNIT_DEPTH_MM, DESC_PCP, Source_Module, DT_Raster1D);
	mdi.AddInput(DataType_SolarRadiation, "UNIT_SRA", DESC_SRA, Source_Module, DT_Raster1D);
	//mdi.AddInput(DataType_Albedo, UNIT_NON_DIM, "DESC_GL_ALBEDO", Source_Module, DT_Raster1D);
	
	/// lookup table as 2D array, such as crop, management, landuse, tillage, etc.
	//mdi.AddParameter(VAR_GLACIER_LOOKUP, UNIT_NON_DIM, DESC_GLACIER_LOOKUP, Source_ParameterDB, DT_Array2D);
	//mdi.AddParameter(VAR_CELLAREA, UNIT_AREA_M2, "DESC_CELLAREA", Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter("GLACIER_AREA", UNIT_AREA_M2, "DESC_GL_AREA", Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter("HLU_AREA", UNIT_AREA_M2, "DESC_HLU_AREA", Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter("Emax", UNIT_LEN_M, "DESC_Emax", Source_ParameterDB, DT_Array1D);
	mdi.AddParameter("Emin", UNIT_LEN_M, "DESC_Emin", Source_ParameterDB, DT_Array1D);
    //mdi.AddParameter("ISGLACIER", UNIT_NON_DIM, "ISGLACIER", Source_ParameterDB, DT_Array1D);
	///hparameter
	mdi.AddParameter("AREA_H", UNIT_AREA_M2, "DESC_GL_AREA", Source_ParameterDB, DT_Array2D);
	mdi.AddParameter("THICKNESS", UNIT_LEN_M, "DESC_GL_AREA", Source_ParameterDB, DT_Array2D);
	mdi.AddParameter("ELEVATION_AVE", UNIT_LEN_M, "DESC_GL_AREA", Source_ParameterDB, DT_Array2D);

    // set the output variables

    //ljj++
    mdi.AddOutput("Sw", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
    mdi.AddOutput("Su", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
    mdi.AddOutput("Sf", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
    mdi.AddOutput("Ss", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
    mdi.AddOutput("Sg", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
    mdi.AddOutput("Sfg", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);

	

    // mdi.AddOutput("T_raster", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Raster1D);
    // mdi.AddOutput("T_times", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
    // mdi.AddOutput("P_times", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);
	
	mdi.AddOutput("Qfg", UNIT_DEPTH_MM, "DESC_GL_SNST", DT_Array1D);

    //mdi.AddOutput("GMB", UNIT_DEPTH_MM, "GMB", DT_Array1D);
    //mdi.AddOutput("Qm", UNIT_DEPTH_MM, "total flow", DT_Array1D);
	//mdi.AddOutput("m_snomelt", UNIT_DEPTH_MM, "snow melt", DT_Raster1D);
	//mdi.AddOutput("m_dGlacRunoff", UNIT_DEPTH_MM, "Glacier runoff", DT_Raster1D);
	//mdi.AddOutput("sublimation", UNIT_DEPTH_MM, "sublimation", DT_Raster1D);
	mdi.AddOutput("GMB", UNIT_DEPTH_MM, "GMB", DT_Array1D);
	mdi.AddOutput("Qm", UNIT_DEPTH_MM, "glacier total flow", DT_Array1D);
	mdi.AddOutput("Qnogla",UNIT_DEPTH_MM, "nogla", DT_Single);
	mdi.AddOutput("Qall", UNIT_DEPTH_MM, "Qall", DT_Single);
	mdi.AddOutput("Q", UNIT_VOL_M3, "Q", DT_Single);


	mdi.AddOutput("GL_AREA", UNIT_AREA_M2, "DESC_GL_AREA", DT_Array2D);
	mdi.AddOutput("GL_THICKNESS", UNIT_LEN_M, "DESC_GL_THICKNESS", DT_Array2D);
	mdi.AddOutput("GL_AREA_SUM", UNIT_AREA_M2, "DESC_GL_AREA_SUM", DT_Array1D);
	mdi.AddOutput("GL_VOLUMN_SUM", UNIT_VOL_M3, "DESC_GL_VOLUMN_SUM", DT_Array1D);
    // write out the XML file.
    string res = mdi.GetXMLDocument();

    char *tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
