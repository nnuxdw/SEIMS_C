#include "api.h"

#include "NutrCH_QUAL2E.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new NutrCH_QUAL2E();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    mdi.SetAuthor("Huiran Gao, Liangjun Zhu");
    mdi.SetClass(MCLS_NutCHRout, MCLSDESC_NutCHRout);
    mdi.SetDescription(MDESC_NUTRCH_QUAL2E);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetID(MDESC_NUTRCH_QUAL2E);
    mdi.SetName(MDESC_NUTRCH_QUAL2E);
    mdi.SetVersion("1.2");
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    // set the parameters
    mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
    mdi.AddParameter(Tag_ChannelTimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Input, DT_Single);
    mdi.AddParameter(VAR_RNUM1, UNIT_NON_DIM, DESC_RNUM1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_IGROPT, UNIT_NON_DIM, DESC_IGROPT, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AI0, UNIT_NUTR_RATIO, DESC_AI0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AI1, UNIT_NUTR_RATIO, DESC_AI1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AI2, UNIT_NUTR_RATIO, DESC_AI2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AI3, UNIT_NUTR_RATIO, DESC_AI3, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AI4, UNIT_NUTR_RATIO, DESC_AI4, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AI5, UNIT_NUTR_RATIO, DESC_AI5, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AI6, UNIT_NUTR_RATIO, DESC_AI6, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_LAMBDA0, UNIT_NON_DIM, DESC_LAMBDA0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_LAMBDA1, UNIT_NON_DIM, DESC_LAMBDA1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_LAMBDA2, UNIT_NON_DIM, DESC_LAMBDA2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_K_L, UNIT_SR, DESC_K_L, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_K_N, UNIT_CONCENTRATION, DESC_K_N, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_K_P, UNIT_CONCENTRATION, DESC_K_P, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_P_N, UNIT_CONCENTRATION, DESC_P_N, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_TFACT, UNIT_NON_DIM, DESC_TFACT, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_MUMAX, UNIT_PER_DAY, DESC_MUMAX, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_RHOQ, UNIT_PER_DAY, DESC_RHOQ, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_COD_N, UNIT_NON_DIM, DESC_COD_N, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_COD_K, UNIT_NON_DIM, DESC_COD_K, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_STREAM_LINK, UNIT_NON_DIM, DESC_STREAM_LINK, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_CH_ONCO, UNIT_CONCENTRATION, DESC_CH_ONCO, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_CH_OPCO, UNIT_CONCENTRATION, DESC_CH_OPCO, Source_ParameterDB, DT_Single);
    // add reach information
    mdi.AddParameter(VAR_REACH_PARAM, UNIT_NON_DIM, DESC_REACH_PARAM, Source_ParameterDB, DT_Reach);
    // add BMPs management operations, such as point source discharge
    mdi.AddParameter(VAR_SCENARIO, UNIT_NON_DIM, DESC_SCENARIO, Source_ParameterDB, DT_Scenario);

    // set the input variables
    mdi.AddInput(DataType_SolarRadiation, UNIT_SR, DESC_SR, Source_Module, DT_Raster1D);
    mdi.AddInput(DataType_MeanTemperature, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
    mdi.AddInput(DataType_RelativeAirMoisture, UNIT_PERCENT, DESC_RM, Source_Module, DT_Raster1D);
    mdi.AddInput(DataType_WindSpeed, UNIT_SPEED_MS, DESC_WS, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_DAYLEN, UNIT_HOUR, DESC_DAYLEN, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SOTE, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Raster1D);

    mdi.AddInput(VAR_QRECH, UNIT_FLOW_CMS, DESC_QRECH, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_RTE_WTRIN, UNIT_VOL_M3, DESC_RTE_WTRIN, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_RTE_WTROUT, UNIT_VOL_M3, DESC_RTE_WTROUT, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_CHWTRDEPTH, UNIT_LEN_M, DESC_CHWTDEPTH, Source_Module, DT_Array1D);
    // /// input from hillslope
    // //nutrient from surface water
    // mdi.AddInput(VAR_SUR_NO3_TOCH, UNIT_KG, DESC_SUR_NO3_ToCH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_SUR_NH4_TOCH, UNIT_KG, DESC_SUR_NH4_ToCH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_SUR_SOLP_TOCH, UNIT_KG, DESC_SUR_SOLP_ToCH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_SUR_COD_TOCH, UNIT_KG, DESC_SUR_COD_ToCH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_SEDORGN_TOCH, UNIT_KG, DESC_SEDORGN_CH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_SEDORGP_TOCH, UNIT_KG, DESC_SEDORGP_CH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_SEDMINPA_TOCH, UNIT_KG, DESC_SEDMINPA_CH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_SEDMINPS_TOCH, UNIT_KG, DESC_SEDMINPS_CH, Source_Module, DT_Array1D);
    // //nutrient from interflow
    // mdi.AddInput(VAR_LATNO3_TOCH, UNIT_KG, DESC_LATNO3_CH, Source_Module, DT_Array1D);
    // //nutrient from ground water
    // mdi.AddInput(VAR_NO3GW_TOCH, UNIT_KG, DESC_NO3GW_CH, Source_Module, DT_Array1D);
    // mdi.AddInput(VAR_MINPGW_TOCH, UNIT_KG, DESC_MINPGW_CH, Source_Module, DT_Array1D);
    // // channel erosion
    // mdi.AddInput(VAR_RCH_DEG, UNIT_KG, DESC_RCH_DEG, Source_Module_Optional, DT_Array1D);
    // set the output variables
    /// 1. Amount (kg) outputs
    mdi.AddInOutput(VAR_CH_ALGAE, UNIT_KG, DESC_CH_ALGAE, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_ORGN, UNIT_KG, DESC_CH_ORGN, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_ORGP, UNIT_KG, DESC_CH_ORGP, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_NH4, UNIT_KG, DESC_CH_NH4, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_NO2, UNIT_KG, DESC_CH_NO2, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_NO3, UNIT_KG, DESC_CH_NO3, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_SOLP, UNIT_KG, DESC_CH_SOLP, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_COD, UNIT_KG, DESC_CH_COD, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_CHLORA, UNIT_KG, DESC_CH_CHLORA, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_DOX, UNIT_KG, DESC_CH_DOX, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_TN, UNIT_KG, DESC_CH_TN, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_TP, UNIT_KG, DESC_CH_TP, DT_Array1D, TF_SingleValue);
    /// 2. Concentration (mg/L) outputs
    mdi.AddInOutput(VAR_CH_ALGAEConc, UNIT_CONCENTRATION, DESC_CH_ALGAE, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_ORGNConc, UNIT_CONCENTRATION, DESC_CH_ORGN, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_ORGPConc, UNIT_CONCENTRATION, DESC_CH_ORGP, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_NH4Conc, UNIT_CONCENTRATION, DESC_CH_NH4, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_NO2Conc, UNIT_CONCENTRATION, DESC_CH_NO2, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_NO3Conc, UNIT_CONCENTRATION, DESC_CH_NO3, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_SOLPConc, UNIT_CONCENTRATION, DESC_CH_SOLP, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_CODConc, UNIT_CONCENTRATION, DESC_CH_COD, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_CHLORAConc, UNIT_CONCENTRATION, DESC_CH_CHLORA, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_DOXConc, UNIT_CONCENTRATION, DESC_CH_DOX, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_TNConc, UNIT_CONCENTRATION, DESC_CH_TNConc, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_TPConc, UNIT_CONCENTRATION, DESC_CH_TPConc, DT_Array1D, TF_SingleValue);
    // 3. point source loadings
    mdi.AddOutput(VAR_PTTN2CH, UNIT_KG, DESC_PTTN2CH, DT_Array1D);
    mdi.AddOutput(VAR_PTTP2CH, UNIT_KG, DESC_PTTP2CH, DT_Array1D);
    mdi.AddOutput(VAR_PTCOD2CH, UNIT_KG, DESC_PTCOD2CH, DT_Array1D);
    // 4. nutrient stored in reaches
    mdi.AddInOutput(VAR_CHSTR_NH4, UNIT_KG, DESC_CHSTR_NH4, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CHSTR_NO3, UNIT_KG, DESC_CHSTR_NO3, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CHSTR_TN, UNIT_KG, DESC_CHSTR_TN, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CHSTR_TP, UNIT_KG, DESC_CHSTR_TP, DT_Array1D, TF_SingleValue);

    //ljj++
    mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);
    mdi.AddParameter(VAR_KLRD, UNIT_NON_DIM, DESC_KLRD, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KLD, UNIT_NON_DIM, DESC_KLD, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KLP, UNIT_NON_DIM, DESC_KLP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KRD, UNIT_NON_DIM, DESC_KRD, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SVLP, UNIT_NON_DIM, DESC_SVLP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SVRP, UNIT_NON_DIM, DESC_SVRP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KDLP, UNIT_NON_DIM, DESC_KDLP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KLRP, UNIT_NON_DIM, DESC_KLRP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KRP, UNIT_NON_DIM, DESC_KRP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KDRP, UNIT_NON_DIM, DESC_KDRP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_NPOC, UNIT_CONCENTRATION, DESC_NPOC, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);  
    mdi.AddInput(VAR_SNME, UNIT_DEPTH_MM, DESC_SNME, Source_Module, DT_Raster1D);  
    mdi.AddInput("GL_RO", UNIT_DEPTH_MM, DESC_SNME, Source_Module, DT_Array1D);  

	// mdi.AddInput(VAR_surfDICtoCH, UNIT_KG, DESC_surfDICtoCH, Source_Module, DT_Array1D);
	// mdi.AddInput(VAR_latDICtoCH, UNIT_KG, DESC_latDICtoCH, Source_Module, DT_Array1D);
	// mdi.AddInput(VAR_LDOCtoCH, UNIT_KG, DESC_LDOCtoCH, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_surfRDOCtoCH, UNIT_KG, DESC_surfRDOCtoCH, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_latRDOCtoCH, UNIT_KG, DESC_latRDOCtoCH, Source_Module, DT_Array1D);
	mdi.AddInput("gw_RDOCtoCH", UNIT_KG, DESC_GWD_RDOCtoCH, Source_Module, DT_Array1D);
    // mdi.AddInput("gw_DICtoCH", UNIT_KG, DESC_GWD_RDOCtoCH, Source_Module, DT_Array1D);
	// mdi.AddInput(VAR_LPOCtoCH, UNIT_KG, DESC_LPOCtoCH, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_RPOCtoCH, UNIT_KG, DESC_RPOCtoCH, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SEDSTO_CH, UNIT_KG, DESC_SEDSTO_CH, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_QS, UNIT_NON_DIM, DESC_QI,Source_Module, DT_Array1D);
    mdi.AddInput(VAR_QI, UNIT_NON_DIM, DESC_QI,Source_Module, DT_Array1D);
    mdi.AddInput(VAR_QG, UNIT_NON_DIM, DESC_QI,Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBOF, UNIT_FLOW_CMS, DESC_SBOF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBQG, UNIT_FLOW_CMS, DESC_SBQG, Source_Module, DT_Array1D);
    mdi.AddInput("rrtime", UNIT_DAY, "TODO", Source_Module, DT_Array1D);
    mdi.AddInput("LAKE_P", UNIT_DAY, "TODO", Source_Module, DT_Raster1D);
    mdi.AddInput("LAKE_E", UNIT_DAY, "TODO", Source_Module, DT_Raster1D);
    mdi.AddInput("gw_DOCsto", UNIT_DAY, "TODO", Source_Module, DT_Array1D);
    //mdi.AddParameter("krp_1d", UNIT_NON_DIM, DESC_KRP, Source_ParameterDB, DT_Raster1D);
    //mdi.AddParameter("kd_rp_1d", UNIT_NON_DIM, DESC_KDRP, Source_ParameterDB, DT_Raster1D);
    //mdi.AddParameter("sv_rp_1d", UNIT_NON_DIM, DESC_SVRP, Source_ParameterDB, DT_Raster1D);
    //mdi.AddParameter("krd_1d", UNIT_NON_DIM, DESC_KRD, Source_ParameterDB, DT_Raster1D);

	mdi.AddInOutput(VAR_CH_DIC, UNIT_KG, DESC_CH_DIC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CH_LDOC, UNIT_KG, DESC_CH_LDOC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CH_RDOC, UNIT_KG, DESC_CH_RDOC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CH_LPOC, UNIT_KG, DESC_CH_LPOC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CH_RPOC, UNIT_KG, DESC_CH_RPOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_TOTDOC, UNIT_KG, DESC_CH_TOTDOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput("CH_TOTPOC", UNIT_KG, DESC_CH_TOTDOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_SURFRDOC, UNIT_KG, DESC_CH_SURFRDOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CH_LATRDOC, UNIT_KG, DESC_CH_LATRDOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput("CH_GWRDOC", UNIT_KG, DESC_CH_GWSRDOC, DT_Array1D, TF_SingleValue);

    mdi.AddOutput(VAR_chtotdoc, UNIT_KG, DESC_CHSTR_LDOC, DT_Raster1D);
    mdi.AddOutput(VAR_chsurfdoc, UNIT_KG, DESC_CHSTR_LDOC, DT_Raster1D);
    mdi.AddOutput("chlatdoc", UNIT_KG, DESC_CHSTR_LDOC, DT_Raster1D);
    mdi.AddOutput("chgwdoc", UNIT_KG, DESC_CHSTR_LDOC, DT_Raster1D);
    mdi.AddOutput("chdoc", UNIT_KG, DESC_CHSTR_LDOC, DT_Array1D);
    mdi.AddOutput("chtemp", UNIT_TEMP_DEG, "TODO", DT_Array1D);
    mdi.AddOutput("lake_ocwb", UNIT_NON_DIM, DESC_NONE, DT_Array2D);

	mdi.AddOutput(VAR_CH_DICConc, UNIT_CONCENTRATION, DESC_CH_DICConc, DT_Array1D, TF_SingleValue);
	mdi.AddOutput(VAR_CH_LPOCConc, UNIT_CONCENTRATION, DESC_CH_LPOCConc, DT_Array1D, TF_SingleValue);
	mdi.AddOutput(VAR_CH_RPOCConc, UNIT_CONCENTRATION, DESC_CH_RPOCConc, DT_Array1D, TF_SingleValue);
	mdi.AddOutput(VAR_CH_LDOCConc, UNIT_CONCENTRATION, DESC_CH_LDOCConc, DT_Array1D, TF_SingleValue);
	mdi.AddOutput(VAR_CH_RDOCConc, UNIT_CONCENTRATION, DESC_CH_RDOCConc, DT_Array1D, TF_SingleValue);
    mdi.AddOutput(VAR_CH_TOTDOCConc, UNIT_CONCENTRATION, DESC_CH_TOTDOCConc, DT_Array1D, TF_SingleValue);
    mdi.AddOutput("CH_TOTPOCConc", UNIT_CONCENTRATION, DESC_CH_TOTDOCConc, DT_Array1D, TF_SingleValue);

    mdi.AddInOutput(VAR_CHSTR_DIC, UNIT_KG, DESC_CHSTR_DIC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CHSTR_LDOC, UNIT_KG, DESC_CHSTR_LDOC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CHSTR_RDOC, UNIT_KG, DESC_CHSTR_RDOC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CHSTR_LPOC, UNIT_KG, DESC_CHSTR_LPOC, DT_Array1D, TF_SingleValue);
	mdi.AddInOutput(VAR_CHSTR_RPOC, UNIT_KG, DESC_CHSTR_RPOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CHSTR_SURFRDOC, UNIT_KG, DESC_CHSTR_SURFRDOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CHSTR_LATRDOC, UNIT_KG, DESC_CHSTR_LATRDOC, DT_Array1D, TF_SingleValue);
    mdi.AddInOutput(VAR_CHSTR_GWDRDOC, UNIT_KG, DESC_CHSTR_GWDRDOC, DT_Array1D, TF_SingleValue);


    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
