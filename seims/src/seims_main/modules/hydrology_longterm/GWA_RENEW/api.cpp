#include "api.h"

#include "ReservoirMethodNEW.h"
#include "MetadataInfo.h"
#include "text.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new ReservoirMethodNEW();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    string res = "";
    MetadataInfo mdi;

    // set the information properties
    mdi.SetAuthor("Wu Hui; Zhiqiang Yu; Liang-Jun Zhu");
    mdi.SetClass(MCLS_GW, MCLSDESC_GW);
    mdi.SetDescription(MDESC_GWA_RE);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetHelpfile("GWA_RE.chm");
    mdi.SetID(MID_GWA_RE);
    mdi.SetName(MID_GWA_RE);
    mdi.SetVersion("1.1");
    mdi.SetWebsite(SEIMS_SITE);

    mdi.AddParameter(Tag_TimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Config, DT_Single);
    mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
    mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SUBBSNID_NUM, UNIT_NON_DIM, DESC_SUBBSNID_NUM, Source_ParameterDB, DT_Single);
    /// add DT_Subbasin
    mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);

    mdi.AddParameter(VAR_GW0, UNIT_DEPTH_MM, DESC_GW0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_GWMAX, UNIT_DEPTH_MM, DESC_GWMAX, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_DF_COEF, UNIT_NON_DIM, DESC_DF_COEF, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KG, UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_Base_ex, UNIT_NON_DIM, DESC_BASE_EX, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_SOILDEPTH, UNIT_DEPTH_MM, DESC_SOILDEPTH, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SLOPE, UNIT_PERCENT, DESC_SLOPE, Source_ParameterDB, DT_Raster1D);
	// xiaodw++
	mdi.AddParameter(VAR_GWMAX_1D, UNIT_DEPTH_MM, DESC_GWMAX, Source_ParameterDB, DT_Array1D);

    mdi.AddInput(VAR_INET, UNIT_DEPTH_MM, DESC_INET, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_DEET, UNIT_DEPTH_MM, DESC_DEET, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SOET, UNIT_DEPTH_MM, DESC_SOET, Source_Module, DT_Raster1D);
    //mdi.AddInput(VAR_AET_PLT, UNIT_DEPTH_MM, DESC_AET_PLT, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_PET, UNIT_DEPTH_MM, DESC_PET, Source_Module, DT_Raster1D);
    // VAR_GWNEW is OPTIONALLY from IUH_CH or other channel routing module
    mdi.AddInput(VAR_GWNEW, UNIT_DEPTH_MM, DESC_GWNEW, Source_Module_Optional, DT_Array1D);
    // VAR_PERCO is from percolation modules
    mdi.AddInput(VAR_PERCO, UNIT_DEPTH_MM, DESC_PERCO, Source_Module, DT_Array2D);
    mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Array2D);
	mdi.AddInput(VAR_HAND_EVAP, UNIT_DEPTH_MM, DESC_HAND_EVAP, Source_Module, DT_Raster1D);   // xiaodw, hand water evap,mm 
	mdi.AddInput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OLFLOW, Source_Module, DT_Raster1D);   // xiaodw, infundation water depth,m 


    mdi.AddOutput(VAR_GWWB, UNIT_NON_DIM, DESC_NONE, DT_Array2D);
    mdi.AddOutput(VAR_REVAP, UNIT_DEPTH_MM, DESC_REVAP, DT_Raster1D);              //used by soil water balance module
    mdi.AddOutput(VAR_RG, UNIT_DEPTH_MM, DESC_RG, DT_Array1D);     //used by soil water balance module
    mdi.AddOutput(VAR_SBQG, UNIT_FLOW_CMS, DESC_SBQG, DT_Array1D); //used by channel flow routing module
    //mdi.AddOutput(VAR_SBQGSUBAREA, UNIT_FLOW_CMS, DESC_SBQG, DT_Array1D); //used to expression
    mdi.AddOutput(VAR_SBPET, UNIT_DEPTH_MM, DESC_SBPET, DT_Array1D);
    mdi.AddOutput(VAR_SBGS, UNIT_DEPTH_MM, DESC_SBGS, DT_Array1D);
	mdi.AddOutput(VAR_CHST_LAST_STEP, UNIT_VOL_M3, DESC_CHST_LAST_STEP, DT_Array1D);   // xiaodw add, output for OL_HAND module


    mdi.AddParameter(VAR_AHRU, UNIT_DEPTH_MM, DESC_AHRU, Source_ParameterDB, DT_Raster1D);

    //whc++ for tiledrain
    //mdi.AddParameter(VAR_GWT0, UNIT_DEPTH_MM, DESC_GW0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SCENARIO, UNIT_NON_DIM, DESC_SCENARIO, Source_ParameterDB_Optional, DT_Scenario);
    mdi.AddParameter(VAR_ROOTDEPTH, UNIT_LEN_M, DESC_ROOTDEPTH, Source_ParameterDB, DT_Raster1D);
    //mdi.AddOutput(VAR_BMP_TILEDRAIN, UNIT_NON_DIM, DESC_NONE, DT_Array2D);
    mdi.AddParameter(VAR_FIELDCAP, UNIT_VOL_FRA_M3M3, DESC_FIELDCAP, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddOutput(VAR_GWSUBAREA, UNIT_FLOW_CMS, DESC_SBQG, DT_Array1D); //used to expression
    //mdi.AddOutput(VAR_SBQGSUBAREA, UNIT_FLOW_CMS, DESC_SBQG, DT_Array1D); //used to expression
    //mdi.AddOutput(VAR_PERCOWB, UNIT_DEPTH_MM, DESC_PERCO, DT_Array2D);
    mdi.AddParameter(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_UL, Source_ParameterDB, DT_Raster2D); // m_sat
    mdi.AddInput(VAR_POT_VOL, UNIT_DEPTH_MM, DESC_POT_VOL, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_IMPOUND_TRIG, UNIT_NON_DIM, DESC_IMPOUND_TRIG, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_INFIL, UNIT_DEPTH_MM, DESC_INFIL, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, Source_Module, DT_Raster1D);
    mdi.AddParameter(VAR_SOL_AWC, UNIT_DEPTH_MM, DESC_SOL_AWC, Source_ParameterDB, DT_Raster2D);   // m_fc
    //mdi.AddParameter(VAR_WASCOB, UNIT_LEN_M, DESC_FLOWOUT_LEN, Source_ParameterDB_Optional, DT_Raster1D);
	mdi.AddParameter("gw_delay_1d", UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter("Kg_1d", UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter("Base_ex_1d", UNIT_NON_DIM, DESC_BASE_EX, Source_ParameterDB, DT_Raster1D);

    res = mdi.GetXMLDocument();

    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
