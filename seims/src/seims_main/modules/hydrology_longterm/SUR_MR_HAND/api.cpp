#include "api.h"

#include "SUR_MR_HAND.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new SUR_MR_HAND();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;

    // set the information properties
    mdi.SetAuthor("Junzhi Liu, Zhiqiang Yu, Liangjun Zhu");
    mdi.SetClass(MCLS_SUR_RUNOFF, MCLSDESC_SUR_RUNOFF);
    mdi.SetDescription(MDESC_SUR_MR_HAND);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetHelpfile("");
    mdi.SetID(MID_SUR_MR_HAND);
    mdi.SetName(MID_SUR_MR_HAND);
    mdi.SetVersion("1.5");
    mdi.SetWebsite(SEIMS_SITE);

    mdi.AddParameter(Tag_HillSlopeTimeStep, UNIT_SECOND, DESC_DT_HS, File_Input, DT_Single);
    mdi.AddParameter(VAR_T_SOIL, UNIT_TEMP_DEG, DESC_T_SOIL, Source_ParameterDB, DT_Single);
    mdi.AddParameter("t_soil_1d", UNIT_TEMP_DEG, DESC_T_SOIL, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_K_RUN, UNIT_NON_DIM, DESC_K_RUN, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_P_MAX, UNIT_DEPTH_MM, DESC_P_MAX, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_S_FROZEN, UNIT_WAT_RATIO, DESC_S_FROZEN, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_RUNOFF_CO, UNIT_NON_DIM, DESC_RUNOFF_CO, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_MOIST_IN, UNIT_PERCENT, DESC_MOIST_IN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOL_AWC, UNIT_DEPTH_MM, DESC_SOL_AWC, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_UL, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_SUMSAT, UNIT_DEPTH_MM, DESC_SOL_SUMSAT, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_FIELDCAP, UNIT_VOL_FRA_M3M3, DESC_FIELDCAP, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_OUTLETID, UNIT_NON_DIM, DESC_OUTLETID, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);

    mdi.AddInput(VAR_NEPR, UNIT_DEPTH_MM, DESC_NEPR, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_TMEAN, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_DPST, UNIT_DEPTH_MM, DESC_DPST, Source_Module, DT_Raster1D);
	//mdi.AddInput(VAR_SOTE, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Raster1D);  // xiaodw comment, don't need soil temperature now
	mdi.AddInput(VAR_IMPOUND_TRIG, UNIT_NON_DIM, DESC_IMPOUND_TRIG, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_POT_VOL, UNIT_DEPTH_MM, DESC_POT_VOL, Source_Module_Optional, DT_Raster1D);

	mdi.AddInput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OLFLOW, Source_Module,DT_Raster1D);   // xiaodw, infundation water depth,m 
	//mdi.AddInput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, Source_Module, DT_Array1D);


    mdi.AddOutput(VAR_EXCP, UNIT_DEPTH_MM, DESC_EXCP, DT_Raster1D);
    mdi.AddOutput(VAR_INFIL, UNIT_DEPTH_MM, DESC_INFIL, DT_Raster1D);
    mdi.AddOutput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, DT_Raster2D);
    mdi.AddOutput(VAR_SOL_SW, UNIT_DEPTH_MM, DESC_SOL_SW, DT_Raster1D);
	mdi.AddOutput(VAR_OL_HAND_WTRDEP_AFT_INFIL, UNIT_LEN_M, DESC_OLFLOW, DT_Raster1D);
	mdi.AddOutput(VAR_OL_HAND_INFIL, UNIT_DEPTH_MM, DESC_OL_HAND_INFIL, DT_Raster1D);
	
	

    //ljj++
    mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_LEN_M, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_DEM, UNIT_LEN_M, DESC_DEM, Source_ParameterDB_Optional, DT_Raster1D);
    mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_STREAM_LINK, UNIT_NON_DIM, DESC_STREAM_LINK, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_SUBBSNID_NUM, UNIT_NON_DIM, DESC_SUBBSNID_NUM, Source_ParameterDB, DT_Single);
	//xdw++
	mdi.AddParameter(VAR_HAND_Subbasin, UNIT_NON_DIM, DESC_HAND_Subbasin, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_Flood_Level, UNIT_NON_DIM, DESC_HAND_Flood_Level, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_LevelDepth, UNIT_LEN_M, DESC_HAND_LevelDepth, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_SumArea, UNIT_AREA_M2, DESC_HAND_SumArea, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_SumVolume, UNIT_VOL_M3, DESC_HAND_SumVolume, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_AvgDepth, UNIT_LEN_M, DESC_HAND_AvgDepth, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_AccVolume, UNIT_VOL_M3, DESC_HAND_AccVolume, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_LowerAccDepthFlat, UNIT_LEN_M, DESC_HAND_LowerAccDepthFlat, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_LowerAccDepthLen, UNIT_NON_DIM, DESC_HAND_LowerAccDepthLen, Source_ParameterDB, DT_Raster1D);

	//mdi.AddInput(VAR_SOLICE, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module_Optional, DT_Array2D);   // xiaodw, don't need soil ice now
	mdi.AddInput(VAR_PCP, UNIT_DEPTH_MM, DESC_PCP, Source_Module, DT_Raster1D); /// ITP_P
    mdi.AddInput(VAR_PET, UNIT_DEPTH_MM, DESC_PET, Source_Module, DT_Raster1D);
	//xdw++
	mdi.AddOutput(VAR_FIELDCAPDEP, UNIT_DEPTH_MM, DESC_FIELDCAPDEP, DT_Array2D);
	mdi.AddOutput(VAR_POROSTDEP, UNIT_DEPTH_MM, DESC_POROSTDEP, DT_Array2D);
	mdi.AddOutput(VAR_SOL_AWC, UNIT_DEPTH_MM, DESC_SOL_AWC, DT_Array2D);
	mdi.AddOutput(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_AWC, DT_Array2D);
	mdi.AddOutput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OL_HAND_WTRDEP, DT_Raster1D); // xiaodw, infundation water depth,m 
	mdi.AddOutput(VAR_RUNOFF_PERCENTAGE, UNIT_NON_DIM, DESC_RUNOFF_PERCENTAGE, DT_Raster1D); //
	mdi.AddOutput(VAR_RUNOFF_CO, UNIT_NON_DIM, DESC_RUNOFF_CO, DT_Raster1D); //

    // write out the XML file.
    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
