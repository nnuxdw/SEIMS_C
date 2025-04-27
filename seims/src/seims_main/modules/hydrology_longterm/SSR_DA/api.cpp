#include "api.h"

#include "SSR_DA.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new SSR_DA();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;

    mdi.SetAuthor("Zhiqiang Yu; Junzhi Liu; Liangjun Zhu");
    mdi.SetClass(MCLS_SUBSURFACE, MCLSDESC_SUBSURFACE);
    mdi.SetDescription(MDESC_SSR_DA);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetID(MID_SSR_DA);
    mdi.SetName(MID_SSR_DA);
    mdi.SetVersion("1.1");
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    mdi.AddParameter(VAR_SUBBSNID_NUM, UNIT_NON_DIM, DESC_SUBBSNID_NUM, Source_ParameterDB, DT_Single);
    mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
    mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
    mdi.AddParameter(Tag_TimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Input, DT_Single);
    mdi.AddParameter(VAR_KI, UNIT_NON_DIM, DESC_KI, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_T_SOIL, UNIT_TEMP_DEG, DESC_T_SOIL, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_CONDUCT, UNIT_WTRDLT_MMH, DESC_CONDUCT, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_UL, Source_ParameterDB, DT_Raster2D); // m_sat
    mdi.AddParameter(VAR_POREIDX, UNIT_NON_DIM, DESC_POREIDX, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_AWC, UNIT_DEPTH_MM, DESC_SOL_AWC, Source_ParameterDB, DT_Raster2D);   // m_fc
    mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D); // m_wp
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SLOPE, UNIT_PERCENT, DESC_SLOPE, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_CHWIDTH, UNIT_LEN_M, DESC_CHWIDTH, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_STREAM_LINK, UNIT_NON_DIM, DESC_STREAM_LINK, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(Tag_FLOWIN_INDEX_D8, UNIT_NON_DIM, DESC_FLOWIN_INDEX_D8, Source_ParameterDB, DT_Array2D);
    mdi.AddParameter(Tag_ROUTING_LAYERS, UNIT_NON_DIM, DESC_ROUTING_LAYERS, Source_ParameterDB, DT_Array2D);
    mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_DISTSTREAM, UNIT_LEN_M, DESC_DISTSTREAM, Source_ParameterDB, DT_Raster1D);

    //mdi.AddInput(VAR_SOTE, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Raster1D);   // xiaodw comment, don't need soil temperature now
    mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_SW, UNIT_DEPTH_MM, DESC_SOL_SW, Source_Module, DT_Raster1D);
    // set the output variables
    mdi.AddOutput(VAR_SSRU, UNIT_DEPTH_MM, DESC_SSRU, DT_Raster2D);
    mdi.AddOutput(VAR_SSRUVOL, UNIT_VOL_M3, DESC_SSRUVOL, DT_Raster2D);

    mdi.AddOutput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, DT_Array1D);

    //ljj++
    mdi.AddParameter("Clay", "%", "Percent of clay content", Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_FLOWOUT_LEN, UNIT_LEN_M, DESC_FLOWOUT_LEN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);
    mdi.AddInput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_POT_VOL, UNIT_DEPTH_MM, DESC_POT_VOL, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_IMPOUND_TRIG, UNIT_NON_DIM, DESC_IMPOUND_TRIG, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_INFIL, UNIT_DEPTH_MM, DESC_INFIL, Source_Module, DT_Raster1D);
    //mdi.AddInput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Array2D);     // xiaodw comment, don't need soil temperature now
    mdi.AddInput(VAR_SOLICE, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module_Optional, DT_Array2D);
    mdi.AddInput(VAR_PERCO, UNIT_DEPTH_MM, DESC_PERCO, Source_Module, DT_Raster2D);


    string res = mdi.GetXMLDocument();

    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
