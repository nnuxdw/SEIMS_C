#include "api.h"

#include "wetland_SWAT.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new WETLAND();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    string res;

    mdi.SetAuthor("jiaojiao Liu");
    mdi.SetClass(MCLS_PADDY, MCLSDESC_PADDY);
    mdi.SetDescription(MDESC_IMP_SWAT);
    mdi.SetID(MID_IMP_SWAT);
    mdi.SetName(MID_IMP_SWAT);
    mdi.SetVersion("1.2");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");
    /// set parameters from database
    mdi.AddParameter(Tag_TimeStep, UNIT_DAY, DESC_TIMESTEP, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_CONDUCT, UNIT_WTRDLT_MMH, DESC_CONDUCT, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);
    mdi.AddParameter(Tag_FLOWIN_INDEX_D8, UNIT_NON_DIM, DESC_FLOWIN_INDEX_D8, Source_ParameterDB, DT_Array2D);
    mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(Tag_FLOWOUT_INDEX_D8, UNIT_NON_DIM, DESC_FLOWOUT_INDEX_D8, Source_ParameterDB, DT_Array1D);
	mdi.AddParameter("CELLAREA", UNIT_AREA_M2, "area", Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_STREAM_LINK, UNIT_NON_DIM, DESC_STREAM_LINK, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILDEPTH, UNIT_DEPTH_MM, DESC_SOILDEPTH, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter("Wetmxvol",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("Wetnvol",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("evwet",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("wetk",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("wetlag",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("ksr0",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("ksr1",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("ksr2",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("krem0",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("krem1",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("krem2",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter("Cdoc",  "todo",  "todo", Source_ParameterDB, DT_Single);
    mdi.AddParameter(Tag_ROUTING_LAYERS, UNIT_NON_DIM, DESC_ROUTING_LAYERS, Source_ParameterDB, DT_Array2D);
    mdi.AddParameter(VAR_T_SOIL, UNIT_TEMP_DEG, DESC_T_SOIL, Source_ParameterDB, DT_Single);

    /// set input from other modules
    mdi.AddInput(VAR_LATERAL_C, UNIT_KG, "TODO", Source_Module, DT_Raster1D);  //from NUTRSED
    //mdi.AddInput(VAR_LATERAL_IC, UNIT_KG, DESC_LAT_DOCtoCH, Source_Module, DT_Raster1D);  //from NUTRSED
	mdi.AddInput(VAR_SURF_DOC, UNIT_CONT_KGHA, "TODO", Source_Module, DT_Raster1D);  //from NUTRSED
	mdi.AddInput(VAR_SURF_DIC, UNIT_CONT_KGHA, "TODO", Source_Module, DT_Raster1D);  //from NUTRSED
	mdi.AddInput(VAR_ENR_LPOC, UNIT_CONT_KGHA, "TODO", Source_Module, DT_Raster1D);  //from NUTRSED
    mdi.AddInput(VAR_LPOCtoCH, UNIT_KG, DESC_LDOCtoCH, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_RPOCtoCH, UNIT_KG, "TODO", Source_Module, DT_Array1D);
    mdi.AddInput(VAR_surfRDOCtoCH, UNIT_KG, "TODO", Source_Module, DT_Array1D);  //from NUTRSED
	mdi.AddInput(VAR_surfDICtoCH, UNIT_KG, "TODO", Source_Module, DT_Array1D);  //from NUTRSED
	mdi.AddInput(VAR_ENR_RPOC, UNIT_CONT_KGHA, "TODO", Source_Module, DT_Raster1D);  //from NUTRSED
    mdi.AddInput(VAR_latRDOCtoCH, UNIT_KG, "TODO", Source_Module, DT_Array1D);  //from NUTRSED
    mdi.AddInput(VAR_latDICtoCH, UNIT_KG, "TODO", Source_Module, DT_Array1D);  //from NUTRSED
    mdi.AddInput(VAR_PET, UNIT_DEPTH_MM, DESC_PET, Source_Module, DT_Raster1D); ///PET
    // mdi.AddInput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, Source_Module, DT_Raster1D); /// should be VAR_FLOW_OL
    mdi.AddInput(VAR_OLFLOW, UNIT_DEPTH_MM, DESC_OLFLOW, Source_Module, DT_Raster1D);      /// m_surfaceRunoff
    mdi.AddInput(VAR_SBOF, UNIT_FLOW_CMS, DESC_SBOF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_PCP, UNIT_DEPTH_MM, DESC_PCP, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_NEPR, UNIT_DEPTH_MM, DESC_NEPR, Source_Module, DT_Raster1D);// m_pNet
    mdi.AddInput(VAR_PERCO, UNIT_DEPTH_MM, DESC_PERCO, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SSRU, UNIT_DEPTH_MM, DESC_SSRU, Source_Module_Optional, DT_Raster2D);   //m_sol_laterq
    mdi.AddInput(VAR_SSRUVOL, UNIT_VOL_M3, DESC_SSRUVOL, Source_Module_Optional, DT_Raster2D);   //m_sol_laterq
    mdi.AddInput(VAR_SOTE, UNIT_TEMP_DEG, DESC_SOTE, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_PERC_LOWEST_DOC, UNIT_KG, DESC_PERC_LOWEST_DOC, Source_Module, DT_Array1D);
    mdi.AddParameter("hs_lakedepth", UNIT_LEN_M, "TODO", "TODO", DT_Raster1D);
    mdi.AddParameter("res_time", UNIT_LEN_M, "TODO", "TODO", DT_Raster1D);
    // //output
    // mdi.AddOutput(VAR_PERC_LOWEST_DOC, UNIT_CONT_KGHA, "TODO", DT_Array1D);
    // mdi.AddOutput("wetdoccon", UNIT_DEPTH_MM, "todo", DT_Raster1D);
    // mdi.AddOutput("wet_vol", UNIT_DEPTH_MM, "todo", DT_Raster1D);
    // mdi.AddOutput("wetland_oc", UNIT_DEPTH_MM, "todo", DT_Array2D);
    // mdi.AddOutput("wetland_wt", UNIT_DEPTH_MM, "todo", DT_Array2D);
    // mdi.AddOutput("surf_wetdoc", UNIT_DEPTH_MM, "todo", DT_Array1D);

    /// write out the XML file.
    res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
