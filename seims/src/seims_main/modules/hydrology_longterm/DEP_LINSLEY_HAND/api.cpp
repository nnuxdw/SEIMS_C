#include "api.h"

#include "DepressionLinsleyHand.h"
#include "MetadataInfo.h"
#include "text.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new DepressionLinsleyHand();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    // set the information properties
    mdi.SetAuthor("Junzhi Liu, Liangjun Zhu");
    mdi.SetClass(MCLS_DEP, MCLSDESC_DEP);
    mdi.SetDescription(MDESC_DEP_LINSLEY);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetHelpfile("DEP_LINSLEY.chm");
    mdi.SetID(MID_DEP_LINSLEY);
    mdi.SetName(MID_DEP_LINSLEY);
    mdi.SetVersion("1.2");
    mdi.SetWebsite(SEIMS_SITE);

    mdi.AddParameter(VAR_DEPREIN, UNIT_NON_DIM, DESC_DEPREIN, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_DEPRESSION, UNIT_DEPTH_MM, DESC_DEPRESSION, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_OUTLETID, UNIT_NON_DIM, DESC_OUTLETID, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
    mdi.AddInput(VAR_INET, UNIT_DEPTH_MM, DESC_INET, Source_Module, DT_Raster1D); //Evaporation from intercepted storage
    mdi.AddInput(VAR_PET, UNIT_DEPTH_MM, DESC_PET, Source_Module, DT_Raster1D);   //PET
    mdi.AddInput(VAR_EXCP, UNIT_DEPTH_MM, DESC_EXCP, Source_Module, DT_Raster1D); //Excess precipitation
    mdi.AddInput(VAR_IMPOUND_TRIG, UNIT_NON_DIM, DESC_IMPOUND_TRIG, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_POT_VOL, UNIT_DEPTH_MM, DESC_POT_VOL, Source_Module_Optional, DT_Raster1D);
	mdi.AddInput(VAR_OL_HAND_WTRDEP_AFT_INFIL, UNIT_LEN_M, DESC_OLFLOW, Source_Module, DT_Raster1D);   // xiaodw, infundation water depth,m 
	//mdi.AddInput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, Source_Module, DT_Array1D);
	//mdi.AddInput(VAR_DPST, UNIT_DEPTH_MM, DESC_DPST, Source_Module, DT_Raster1D);

	mdi.AddParameter(VAR_HAND_Subbasin, UNIT_NON_DIM, DESC_HAND_Subbasin, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_Flood_Level, UNIT_NON_DIM, DESC_HAND_Flood_Level, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_LevelDepth, UNIT_LEN_M, DESC_HAND_LevelDepth, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_SumArea, UNIT_AREA_M2, DESC_HAND_SumArea, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_SumVolume, UNIT_VOL_M3, DESC_HAND_SumVolume, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_AvgDepth, UNIT_LEN_M, DESC_HAND_AvgDepth, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_AccVolume, UNIT_VOL_M3, DESC_HAND_AccVolume, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_LowerAccDepthFlat, UNIT_LEN_M, DESC_HAND_LowerAccDepthFlat, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_HAND_LowerAccDepthLen, UNIT_NON_DIM, DESC_HAND_LowerAccDepthLen, Source_ParameterDB, DT_Raster1D);

    mdi.AddOutput(VAR_DPST, UNIT_DEPTH_MM, DESC_DPST, DT_Raster1D);
    mdi.AddOutput(VAR_DEET, UNIT_DEPTH_MM, DESC_DEET, DT_Raster1D);
    mdi.AddOutput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, DT_Raster1D);
	mdi.AddOutput(VAR_OL_HAND_WTRDEP_AFT_DEP, UNIT_LEN_M, DESC_OLFLOW, DT_Raster1D); // xiaodw, infundation water depth,m 
	mdi.AddOutput(VAR_HAND_EVAP, UNIT_DEPTH_MM, DESC_HAND_EVAP, DT_Raster1D);
	mdi.AddOutput(VAR_HAND_DEP, UNIT_DEPTH_MM, DESC_HAND_DEP, DT_Raster1D);

    // set the dependencies
    mdi.AddDependency(MCLS_CLIMATE, MCLSDESC_CLIMATE);
    mdi.AddDependency(MCLS_INTERC, MCLSDESC_INTERC);
    mdi.AddDependency(MCLS_SUR_RUNOFF, MCLSDESC_SUR_RUNOFF);

    string res = mdi.GetXMLDocument();

    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
