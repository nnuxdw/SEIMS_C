#include "api.h"

#include "NutrientTransportSediment.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new NutrientTransportSediment();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    mdi.SetAuthor("Huiran Gao, Liangjun Zhu");
    mdi.SetClass(MCLS_NUTRSED, MCLSDESC_NUTRSED);
    mdi.SetDescription(MDESC_NUTRSED);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetID(MDESC_NUTRSED);
    mdi.SetName(MDESC_NUTRSED);
    mdi.SetVersion("1.2");
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    // set the parameters
    mdi.AddParameter(VAR_CSWAT, UNIT_NON_DIM, DESC_CSWAT, Source_ParameterDB, DT_Single);
    mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SUBBSNID_NUM, UNIT_NON_DIM, DESC_SUBBSNID_NUM, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_SOL_BD, UNIT_DENSITY, DESC_SOL_BD, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_ROCK, UNIT_PERCENT, DESC_ROCK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_UL, Source_ParameterDB, DT_Raster2D);
    // parameters for subbasin sum
    mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);

    // set input from other modules
    mdi.AddInput(VAR_OLFLOW, UNIT_DEPTH_MM, DESC_OLFLOW, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SEDYLD, UNIT_KG, DESC_SEDYLD, Source_Module, DT_Raster1D);

    mdi.AddInput(VAR_SOL_AORGN, UNIT_CONT_KGHA, DESC_SOL_AORGN, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_SORGN, UNIT_CONT_KGHA, DESC_SOL_SORGN, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_FORGN, UNIT_CONT_KGHA, DESC_SOL_FORGN, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_FORGP, UNIT_CONT_KGHA, DESC_SOL_FORGP, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_HORGP, UNIT_CONT_KGHA, DESC_SOL_HORGP, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_ACTP, UNIT_CONT_KGHA, DESC_SOL_ACTP, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_STAP, UNIT_CONT_KGHA, DESC_SOL_STAP, Source_Module, DT_Raster2D);
    /// for C-FARM one carbon model
    mdi.AddInput(VAR_SOL_MP, UNIT_CONT_KGHA, DESC_SOL_MP, Source_Module_Optional, DT_Raster2D);

    /// for CENTURY C/N cycling model, set as optional inputs
    mdi.AddInput(VAR_SOL_LSN, UNIT_CONT_KGHA, DESC_SOL_LSN, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LMN, UNIT_CONT_KGHA, DESC_SOL_LMN, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_HPN, UNIT_CONT_KGHA, DESC_SOL_HPN, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_HSN, UNIT_CONT_KGHA, DESC_SOL_HSN, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_HPC, UNIT_CONT_KGHA, DESC_SOL_HPC, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_HSC, UNIT_CONT_KGHA, DESC_SOL_HSC, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LM, UNIT_CONT_KGHA, DESC_SOL_LM, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LMC, UNIT_CONT_KGHA, DESC_SOL_LMC, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LSC, UNIT_CONT_KGHA, DESC_SOL_LSC, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LS, UNIT_CONT_KGHA, DESC_SOL_LS, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LSL, UNIT_CONT_KGHA, DESC_SOL_LSL, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LSLC, UNIT_CONT_KGHA, DESC_SOL_LSLC, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_LSLNC, UNIT_CONT_KGHA, DESC_SOL_LSLNC, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_BMC, UNIT_CONT_KGHA, DESC_SOL_BMC, Source_Module_Optional, DT_Raster2D);
    mdi.AddInput(VAR_SOL_WOC, UNIT_CONT_KGHA, DESC_SOL_WOC, Source_Module_Optional, DT_Raster2D);

    mdi.AddInput(VAR_SSRU, UNIT_DEPTH_MM, DESC_SSRU, Source_Module_Optional, DT_Raster2D);   //m_sol_laterq
    mdi.AddInput(VAR_PERCO, UNIT_DEPTH_MM, DESC_PERCO, Source_Module_Optional, DT_Raster2D); //m_sol_perco
    /// end CENTURY variables

    // set the output variables
    // output surface runoff related variables
    mdi.AddOutput(VAR_SEDORGN, UNIT_CONT_KGHA, DESC_SEDORGN, DT_Raster1D);
    mdi.AddOutput(VAR_SEDORGP, UNIT_CONT_KGHA, DESC_SEDORGP, DT_Raster1D);
    mdi.AddOutput(VAR_SEDMINPA, UNIT_CONT_KGHA, DESC_SEDMINPA, DT_Raster1D);
    mdi.AddOutput(VAR_SEDMINPS, UNIT_CONT_KGHA, DESC_SEDMINPS, DT_Raster1D);

    // to channel
    mdi.AddOutput(VAR_SEDORGN_TOCH, UNIT_KG, DESC_SEDORGN_CH, DT_Array1D);
    mdi.AddOutput(VAR_SEDORGP_TOCH, UNIT_KG, DESC_SEDORGP_CH, DT_Array1D);
    mdi.AddOutput(VAR_SEDMINPA_TOCH, UNIT_KG, DESC_SEDMINPA_CH, DT_Array1D);
    mdi.AddOutput(VAR_SEDMINPS_TOCH, UNIT_KG, DESC_SEDMINPS_CH, DT_Array1D);

    /// outputs of CENTURY C/N cycling model
    mdi.AddOutput(VAR_SOL_LATERAL_C, UNIT_CONT_KGHA, DESC_SOL_LATERAL_C, DT_Raster2D);
    mdi.AddOutput(VAR_SOL_PERCO_C, UNIT_CONT_KGHA, DESC_SOL_PERCO_C, DT_Raster2D);
    mdi.AddOutput(VAR_LATERAL_C, UNIT_CONT_KGHA, DESC_LATERAL_C, DT_Raster1D);
    mdi.AddOutput(VAR_PERCO_C, UNIT_CONT_KGHA, DESC_PERCO_C, DT_Raster1D);
    mdi.AddOutput(VAR_SEDLOSS_C, UNIT_CONT_KGHA, DESC_SEDLOSS_C, DT_Raster1D);

    //ljj++
    mdi.AddParameter(VAR_ENRPOC, UNIT_NON_DIM, DESC_ENRPOC, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_KDOC, UNIT_NON_DIM, DESC_KDOC, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_PERCO_DOC, UNIT_NON_DIM, DESC_PERCO_DOC, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_STREAM_LINK, UNIT_NON_DIM, DESC_STREAM_LINK, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILDEPTH, UNIT_DEPTH_MM, DESC_SOILDEPTH, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(Tag_ROUTING_LAYERS, UNIT_NON_DIM, DESC_ROUTING_LAYERS, Source_ParameterDB, DT_Array2D);
	mdi.AddParameter(Tag_FLOWOUT_INDEX_D8, UNIT_NON_DIM, DESC_FLOWOUT_INDEX_D8, Source_ParameterDB, DT_Array1D);
    mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);

    mdi.AddOutput(VAR_LPOCtoCH, UNIT_KG, DESC_LPOCtoCH, DT_Array1D);
	mdi.AddOutput(VAR_RPOCtoCH, UNIT_KG, DESC_RPOCtoCH, DT_Array1D);
    mdi.AddOutput(VAR_surfDICtoCH, UNIT_KG, DESC_surfDICtoCH, DT_Array1D);
	mdi.AddOutput(VAR_latDICtoCH, UNIT_KG, DESC_latDICtoCH, DT_Array1D);
    mdi.AddOutput(VAR_LDOCtoCH, UNIT_KG, DESC_LDOCtoCH, DT_Array1D);
    mdi.AddOutput(VAR_surfRDOCtoCH, UNIT_KG, DESC_surfRDOCtoCH, DT_Array1D);
	mdi.AddOutput(VAR_latRDOCtoCH, UNIT_KG, DESC_latRDOCtoCH, DT_Array1D);
    mdi.AddOutput(VAR_PERC_LOWEST_DOC, UNIT_KG, DESC_PERC_LOWEST_DOC, DT_Array1D);
    mdi.AddOutput(VAR_PERC_LOWEST_DIC, UNIT_KG, DESC_PERC_LOWEST_DIC, DT_Array1D);

    mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);
    mdi.AddInput(VAR_SOL_RSPC, UNIT_CONT_KGHA, DESC_SOL_RSPC, Source_Module_Optional, DT_Raster2D);
	mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Array2D);
    mdi.AddInput("BRT", "none", "fraction of surface runoff that takes one day or less to reach the subbasin", Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, Source_Module, DT_Raster1D);

    // mdi.AddInput("SURFDOC", UNIT_KG, DESC_SEDYLD, Source_Module, DT_Raster1D);
    // mdi.AddInput("SURFPOC", UNIT_KG, DESC_SEDYLD, Source_Module, DT_Raster1D);
    // mdi.AddInput("SUBSURFDOC", UNIT_KG, DESC_SEDYLD, Source_Module, DT_Raster2D);

    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
