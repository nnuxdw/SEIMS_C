#include "api.h"

#include "LISFLOODFP_MUSK.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new LISFLOODFP_MUSK();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    string res;

    mdi.SetAuthor("Dawei Xiao");
    mdi.SetClass(MCLS_OL_ROUTING, MCLSDESC_OL_ROUTING);
    mdi.SetDescription(MDESC_LISFLOODFP_MUSK);
    mdi.SetID(MID_LISFLOODFP_MUSK);
    mdi.SetName(MID_LISFLOODFP_MUSK);
    mdi.SetVersion("1.0");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");
	
	mdi.AddParameter(Tag_TimeStep, UNIT_HOUR, DESC_TIMESTEP, File_Input, DT_Single);
	//mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
	//mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	//mdi.AddInput(VAR_BKST, UNIT_VOL_M3, DESC_BKST, Source_Module,DT_Array1D);
	//mdi.AddInput(VAR_BKST_LAST_STEP, UNIT_VOL_M3, DESC_BKST_LAST_STEP, Source_Module, DT_Array1D);
	//mdi.AddInput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, Source_Module, DT_Array1D);
	//mdi.AddInput(VAR_CHST_LAST_STEP, UNIT_VOL_M3, DESC_CHST_LAST_STEP, Source_Module, DT_Array1D);
	//mdi.AddInput(VAR_CHWTRDEPTH, UNIT_LEN_M, DESC_CHWTDEPTH, Source_Module, DT_Array1D);
	//mdi.AddInput(VAR_CHWTRWIDTH, UNIT_LEN_M, DESC_CHWTWIDTH, Source_Module, DT_Array1D); 

	//// add reach information
	//mdi.AddParameter(VAR_REACH_PARAM, UNIT_NON_DIM, DESC_REACH_PARAM, Source_ParameterDB, DT_Reach);
	//mdi.AddOutput(VAR_OL_HAND_WTRDEP, UNIT_DEPTH_MM, DESC_OLFLOW, DT_Raster1D);

	/************************************MUSK_CH**********************************/
	mdi.AddParameter(Tag_ChannelTimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Input, DT_Single);
	mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_OUTLETID, UNIT_NON_DIM, DESC_OUTLETID, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_EP_CH, UNIT_WTRDLT_MMH, DESC_EP_CH, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_BNK0, UNIT_STRG_M3M, DESC_BNK0, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_CHS0_PERC, UNIT_NON_DIM, DESC_CHS0_PERC, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_A_BNK, UNIT_NON_DIM, DESC_A_BNK, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_B_BNK, UNIT_NON_DIM, DESC_B_BNK, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_MSK_X, UNIT_NON_DIM, DESC_MSK_X, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_MSK_CO1, UNIT_NON_DIM, DESC_MSK_CO1, Source_ParameterDB, DT_Single);
	// add reach information
	mdi.AddParameter(VAR_REACH_PARAM, UNIT_NON_DIM, DESC_REACH_PARAM, Source_ParameterDB, DT_Reach);
	// add BMPs management operations, such as point source discharge
	mdi.AddParameter(VAR_SCENARIO, UNIT_NON_DIM, DESC_SCENARIO, Source_ParameterDB, DT_Scenario);

	// Inputs from other modules
	mdi.AddInput(VAR_SBPET, UNIT_DEPTH_MM, DESC_SBPET, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_SBGS, UNIT_DEPTH_MM, DESC_SBGS, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_SBOF, UNIT_FLOW_CMS, DESC_SBOF, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_SBQG, UNIT_FLOW_CMS, DESC_SBQG, Source_Module, DT_Array1D);

	// Outputs
	mdi.AddInOutput(VAR_QRECH, UNIT_FLOW_CMS, DESC_QRECH, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_QS, UNIT_NON_DIM, DESC_QS, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_QI, UNIT_NON_DIM, DESC_QI, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_QG, UNIT_NON_DIM, DESC_QG, DT_Array1D, TF_SingleValue);

	mdi.AddOutput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, DT_Array1D);
	mdi.AddOutput(VAR_CHST_LAST_STEP, UNIT_VOL_M3, DESC_CHST_LAST_STEP, DT_Array1D);   // xiaodw add, output for OL_HAND module
	mdi.AddOutput(VAR_RTE_WTRIN, UNIT_VOL_M3, DESC_RTE_WTRIN, DT_Array1D);
	mdi.AddOutput(VAR_RTE_WTROUT, UNIT_VOL_M3, DESC_RTE_WTROUT, DT_Array1D);
	mdi.AddOutput(VAR_BKST, UNIT_VOL_M3, DESC_BKST, DT_Array1D);
	mdi.AddOutput(VAR_BKST_LAST_STEP, UNIT_VOL_M3, DESC_BKST_LAST_STEP, DT_Array1D);   // xiaodw add, output for OL_HAND module

	mdi.AddOutput(VAR_CHWTRDEPTH, UNIT_LEN_M, DESC_CHWTDEPTH, DT_Array1D);
	mdi.AddOutput(VAR_CHWTRWIDTH, UNIT_LEN_M, DESC_CHWTWIDTH, DT_Array1D);
	mdi.AddOutput(VAR_CHBTMWIDTH, UNIT_LEN_M, DESC_CHBTMWIDTH, DT_Array1D);
	mdi.AddOutput(VAR_CHCROSSAREA, UNIT_AREA_M2, DESC_CHCROSSAREA, DT_Array1D);

	//ljj++
	mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);
	mdi.AddParameter(VAR_KG, UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_Base_ex, UNIT_NON_DIM, DESC_BASE_EX, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_GWMAX, UNIT_DEPTH_MM, DESC_GWMAX, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_K_PET, UNIT_NON_DIM, DESC_PET_K, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_LAKE_EVP, UNIT_NON_DIM, DESC_LAKE_EVP, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_LAKE_SEEP, UNIT_NON_DIM, DESC_LAKE_SEEP, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_LAKE_MNVOL, UNIT_NON_DIM, DESC_LAKE_MNVOL, Source_ParameterDB, DT_Single);
	mdi.AddParameter("LAKEB", UNIT_NON_DIM, DESC_LAKE_MNVOL, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_AHRU, UNIT_DEPTH_MM, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_DEM, UNIT_LEN_M, DESC_DEM, Source_ParameterDB_Optional, DT_Raster1D);
	mdi.AddParameter(VAR_RUNOFF_CO, UNIT_NON_DIM, DESC_RUNOFF_CO, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_SLOPE, UNIT_PERCENT, DESC_SLOPE, Source_ParameterDB, DT_Raster1D);

	mdi.AddInput(VAR_PCP, UNIT_DEPTH_MM, DESC_PCP, Source_Module, DT_Raster1D);
	mdi.AddInput(VAR_PET, UNIT_WTRDLT_MMD, DESC_PET, Source_Module, DT_Raster1D);
	//mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);   // xiaodw comment, don't need soil temperature now

	mdi.AddOutput(VAR_qout, UNIT_NON_DIM, DESC_QRECH, DT_Raster1D);
	mdi.AddOutput(VAR_qsurf, UNIT_NON_DIM, DESC_QS, DT_Raster1D);
	mdi.AddOutput("lake_wb", UNIT_NON_DIM, DESC_NONE, DT_Array2D);

	mdi.AddOutput("LAKE_P", UNIT_NON_DIM, DESC_QS, DT_Raster1D);
	mdi.AddOutput("LAKE_E", UNIT_NON_DIM, DESC_QS, DT_Raster1D);
	mdi.AddOutput("Qout", UNIT_NON_DIM, DESC_QS, DT_Raster1D);

	mdi.AddOutput("rrtime", UNIT_NON_DIM, "the water travel time ", DT_Array1D);
	/************************************End MUSK_CH**********************************/

    /// write out the XML file.
    res = mdi.GetXMLDocument();

    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
