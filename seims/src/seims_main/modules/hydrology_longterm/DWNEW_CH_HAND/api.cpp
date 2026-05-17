#include "api.h"

#include "DWNEW_CH_HAND.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new DWNEW_CH_HAND();
}
/// function to return the XML Metadata document string
extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    string res;
    MetadataInfo mdi;

    // set the information properties
    mdi.SetAuthor("lp; Xiaodw");
    mdi.SetClass(MCLS_CH_ROUTING, MCLSDESC_CH_ROUTING);
    mdi.SetDescription("Pure diffusive-wave channel routing with HAND inundation and optional external outlet boundary");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetHelpfile("");
    mdi.SetID(MID_DWNEW_CH_HAND);
    mdi.SetName(MID_DWNEW_CH_HAND);
    mdi.SetVersion("1.0");
    mdi.SetWebsite(SEIMS_SITE);

    // ======================== Parameters (Single) ========================
    mdi.AddParameter(Tag_ChannelTimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Input, DT_Single);
    mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_OUTLETID, UNIT_NON_DIM, DESC_OUTLETID, Source_ParameterDB, DT_Single);

    // External outlet boundary, inspired by CaMa-Flood-style downstream ghost boundary.
    // DW_OUTLET_BC_TYPE: 0=closed internal basin; 1=free/normal-depth; 2=fixed downstream stage.
    mdi.AddParameter("DW_OUTLET_BC_TYPE", UNIT_NON_DIM, "DWNEW outlet boundary type: 0 closed, 1 normal-depth/free outflow, 2 fixed-stage ghost boundary", Source_ParameterDB_Optional, DT_Single);
    mdi.AddParameter("DW_OUTLET_BC_STAGE", UNIT_LEN_M, "Downstream ghost water level for fixed-stage outlet boundary", Source_ParameterDB_Optional, DT_Single);
    mdi.AddParameter("DW_OUTLET_BC_SLOPE", UNIT_NON_DIM, "External normal-depth outlet slope used when DW_OUTLET_BC_TYPE=1", Source_ParameterDB_Optional, DT_Single);
    mdi.AddParameter("DW_OUTLET_BC_ALLOW_BACKFLOW", UNIT_NON_DIM, "Allow negative flow from fixed-stage outlet boundary when downstream stage is higher", Source_ParameterDB_Optional, DT_Single);
    mdi.AddParameter(VAR_EP_CH, UNIT_WTRDLT_MMH, DESC_EP_CH, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_BNK0, UNIT_STRG_M3M, DESC_BNK0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CHS0_PERC, UNIT_NON_DIM, DESC_CHS0_PERC, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_A_BNK, UNIT_NON_DIM, DESC_A_BNK, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_B_BNK, UNIT_NON_DIM, DESC_B_BNK, Source_ParameterDB, DT_Single);

    // Lake/Reservoir parameters (same as MUSK_CH)
    mdi.AddParameter(VAR_KG, UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_Base_ex, UNIT_NON_DIM, DESC_BASE_EX, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_GWMAX, UNIT_DEPTH_MM, DESC_GWMAX, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_K_PET, UNIT_NON_DIM, DESC_PET_K, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_LAKE_EVP, UNIT_NON_DIM, DESC_LAKE_EVP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_LAKE_SEEP, UNIT_NON_DIM, DESC_LAKE_SEEP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_LAKE_MNVOL, UNIT_NON_DIM, DESC_LAKE_MNVOL, Source_ParameterDB, DT_Single);
    mdi.AddParameter("LAKEB", UNIT_NON_DIM, DESC_LAKE_MNVOL, Source_ParameterDB, DT_Single);

    // ======================== Parameters (Raster1D) ========================
    mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_AHRU, UNIT_DEPTH_MM, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_DEM, UNIT_LEN_M, DESC_DEM, Source_ParameterDB_Optional, DT_Raster1D);
    mdi.AddParameter(VAR_RUNOFF_CO, UNIT_NON_DIM, DESC_RUNOFF_CO, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SLOPE, UNIT_PERCENT, DESC_SLOPE, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter("ep_ch_1d", UNIT_WTRDLT_MMH, DESC_EP_CH, Source_ParameterDB, DT_Raster1D);

    // ======================== Parameters (Reach / Subbasin / Scenario) ========================
    mdi.AddParameter(VAR_REACH_PARAM, UNIT_NON_DIM, DESC_REACH_PARAM, Source_ParameterDB, DT_Reach);
    mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);
    mdi.AddParameter(VAR_SCENARIO, UNIT_NON_DIM, DESC_SCENARIO, Source_ParameterDB, DT_Scenario);

    // ======================== HAND static data ========================
    mdi.AddParameter(VAR_HAND_Subbasin, UNIT_NON_DIM, DESC_HAND_Subbasin, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_Flood_Level, UNIT_NON_DIM, DESC_HAND_Flood_Level, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_LevelDepth, UNIT_LEN_M, DESC_HAND_LevelDepth, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_SumArea, UNIT_AREA_M2, DESC_HAND_SumArea, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_SumVolume, UNIT_VOL_M3, DESC_HAND_SumVolume, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_AvgDepth, UNIT_LEN_M, DESC_HAND_AvgDepth, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_AccVolume, UNIT_VOL_M3, DESC_HAND_AccVolume, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_LowerAccDepthFlat, UNIT_NON_DIM, DESC_HAND_LowerAccDepthFlat, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_HAND_LowerAccDepthLen, UNIT_NON_DIM, DESC_HAND_LowerAccDepthLen, Source_ParameterDB, DT_Raster1D);

    // ======================== Inputs from other modules ========================
    mdi.AddInput(VAR_SBPET, UNIT_DEPTH_MM, DESC_SBPET, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBGS, UNIT_DEPTH_MM, DESC_SBGS, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBOF, UNIT_FLOW_CMS, DESC_SBOF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBQG, UNIT_FLOW_CMS, DESC_SBQG, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_PCP, UNIT_DEPTH_MM, DESC_PCP, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_PET, UNIT_WTRDLT_MMD, DESC_PET, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_OL_HAND_INFIL, UNIT_DEPTH_MM, DESC_OL_HAND_INFIL, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_HAND_EVAP, UNIT_DEPTH_MM, DESC_HAND_EVAP, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_HAND_DEP, UNIT_DEPTH_MM, DESC_HAND_DEP, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_OL_HAND_BACK_FROM_GW, UNIT_DEPTH_MM, VAR_OL_HAND_BACK_FROM_GW, Source_Module_Optional, DT_Raster1D);
    //mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);

    // ======================== InOutput (for MPI data transfer) ========================
    // These 4 are identical to MUSK_CH — required for downstream module coupling
    mdi.AddInOutput(VAR_QRECH, UNIT_FLOW_CMS, DESC_QRECH, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_QS, UNIT_NON_DIM, DESC_QS, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_QI, UNIT_NON_DIM, DESC_QI, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_QG, UNIT_NON_DIM, DESC_QG, DT_Array1D, TF_SingleValue);

    // ======================== Outputs (same as MUSK_CH) ========================
    mdi.AddOutput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, DT_Array1D);
    mdi.AddOutput(VAR_RTE_WTRIN, UNIT_VOL_M3, DESC_RTE_WTRIN, DT_Array1D);
    mdi.AddOutput(VAR_RTE_WTROUT, UNIT_VOL_M3, DESC_RTE_WTROUT, DT_Array1D);
    mdi.AddOutput(VAR_BKST, UNIT_VOL_M3, DESC_BKST, DT_Array1D);
    mdi.AddOutput(VAR_CHWTRDEPTH, UNIT_LEN_M, DESC_CHWTDEPTH, DT_Array1D);
    mdi.AddOutput(VAR_CHWTRWIDTH, UNIT_LEN_M, DESC_CHWTWIDTH, DT_Array1D);
    mdi.AddOutput(VAR_CHBTMWIDTH, UNIT_LEN_M, DESC_CHBTMWIDTH, DT_Array1D);
    mdi.AddOutput(VAR_CHCROSSAREA, UNIT_AREA_M2, DESC_CHCROSSAREA, DT_Array1D);
    mdi.AddOutput(VAR_CHST_LAST_STEP, UNIT_VOL_M3, DESC_CHST_LAST_STEP, DT_Array1D);   // xiaodw add, output for OL_HAND module
    mdi.AddOutput(VAR_BKST_LAST_STEP, UNIT_VOL_M3, DESC_BKST_LAST_STEP, DT_Array1D);   // xiaodw add, output for OL_HAND module
    //mdi.AddOutput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OL_HAND_WTRDEP, DT_Raster1D);
    //mdi.AddOutput(VAR_IS_HAND_FLOODED, UNIT_NON_DIM, DESC_IS_HAND_FLOODED, DT_Raster1D);
    //mdi.AddOutput(VAR_SUBBASIN_FLOODED_AREA, UNIT_AREA_M2, DESC_SUBBASIN_FLOODED_AREA, DT_Array1D);
    //mdi.AddOutput(VAR_SUBBASIN_WTR_DEPTH, UNIT_LEN_M, DESC_SUBBASIN_WTR_DEPTH, DT_Array1D);

    // ljj++ outputs (same as MUSK_CH)
    mdi.AddOutput(VAR_qout, UNIT_NON_DIM, DESC_QRECH, DT_Raster1D);
    mdi.AddOutput(VAR_qsurf, UNIT_NON_DIM, DESC_QS, DT_Raster1D);
    mdi.AddOutput("lake_wb", UNIT_NON_DIM, DESC_NONE, DT_Array2D);
    mdi.AddOutput("LAKE_P", UNIT_NON_DIM, DESC_QS, DT_Raster1D);
    mdi.AddOutput("LAKE_E", UNIT_NON_DIM, DESC_QS, DT_Raster1D);
    mdi.AddOutput("Qout", UNIT_NON_DIM, DESC_QS, DT_Raster1D);
    mdi.AddOutput("rrtime", UNIT_NON_DIM, "the water travel time", DT_Array1D);

    res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
