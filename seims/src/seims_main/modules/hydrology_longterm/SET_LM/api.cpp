#include "api.h"

#include "SET_LM.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new SET_LM();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    string res;
    MetadataInfo mdi;

    // set the information properties
    mdi.SetAuthor("Chunping Ou, Liangjun Zhu");
    mdi.SetClass(MCLS_AET, MCLSDESC_AET);
    mdi.SetDescription(MDESC_SET_LM);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetID(MID_SET_LM);
    mdi.SetName(MID_SET_LM);
    mdi.SetVersion("1.1");
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    mdi.AddParameter(VAR_T_SOIL, UNIT_TEMP_DEG, DESC_T_SOIL, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_AWC, UNIT_DEPTH_MM, DESC_SOL_AWC, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_OUTLETID, UNIT_NON_DIM, DESC_OUTLETID, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
	//mdi.AddParameter(Tag_ROUTING_LAYERS, UNIT_NON_DIM, DESC_ROUTING_LAYERS, Source_ParameterDB, DT_Array2D);
	mdi.AddInput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OLFLOW, Source_Module, DT_Raster1D);   // xiaodw, infundation water depth,m 

    // set the parameters (non-time series)
    mdi.AddInput(VAR_PET, UNIT_DEPTH_MM, DESC_PET, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_INET, UNIT_DEPTH_MM, DESC_INET, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_DEET, UNIT_DEPTH_MM, DESC_DEET, Source_Module, DT_Raster1D);
	//mdi.AddInput(VAR_SOTE, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Raster1D);   // xiaodw comment, don't need soil temperature now
	 //mdi.AddInput(VAR_PPT, UNIT_DEPTH_MM, DESC_PPT, Source_Module, DT_Raster1D);      // xiaodw comment, don't need plant et now, remove it 
    mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Raster2D);
	mdi.AddInput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, Source_Module, DT_Array1D);
    // set the output variables
    mdi.AddOutput(VAR_SOET, UNIT_DEPTH_MM, DESC_SOET, DT_Raster1D);
	mdi.AddOutput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OLFLOW, DT_Raster1D); // xiaodw, infundation water depth,m 


    // write out the XML file.
    res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
