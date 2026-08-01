#include "api.h"

#include "MetadataInfo.h"
#include "SWMMDynamicWave.h"
#include "text.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new SWMMDynamicWave();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    mdi.SetAuthor("WISE / SWMM coupling");
    mdi.SetClass(MCLS_CH_ROUTING, MCLSDESC_CH_ROUTING);
    mdi.SetDescription("EPA SWMM 5.2 dynamic-wave routing for an arbitrary river node/link network.");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetHelpfile("SWMM_DW.md");
    mdi.SetID("SWMM_DW");
    mdi.SetName("SWMM_DW");
    mdi.SetVersion("0.1");
    mdi.SetWebsite(SEIMS_SITE);

    mdi.AddParameter(Tag_ChannelTimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Input, DT_Single); // Required, > 0 s.
    mdi.AddParameter("SWMM_MAX_TRIALS", UNIT_NON_DIM, "Maximum Picard iterations per internal routing step",
                     Source_ParameterDB_Optional, DT_Single); // Integer >= 1; default 8.
    mdi.AddParameter("SWMM_HEAD_TOL", UNIT_LEN_M, "Node head convergence tolerance", Source_ParameterDB_Optional, DT_Single); // > 0; default 0.001 m.
    mdi.AddParameter("SWMM_MIN_ROUTE_STEP", UNIT_SECOND, "Minimum internal dynamic-wave step", Source_ParameterDB_Optional, DT_Single); // >= 0.001; default 1 s.
    mdi.AddParameter("SWMM_COURANT_FACTOR", UNIT_NON_DIM, "Courant factor; zero uses the channel time step",
                     Source_ParameterDB_Optional, DT_Single); // 0 disables substepping; default 0.75.
    mdi.AddParameter("SWMM_MIN_SURFACE_AREA", "m2", "Minimum node surface area", Source_ParameterDB_Optional, DT_Single); // > 0; default 1.167 m2.

    mdi.AddInput(VAR_SBOF, UNIT_FLOW_CMS, DESC_SBOF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, Source_Module, DT_Array1D);
    mdi.AddInput(VAR_SBQG, UNIT_FLOW_CMS, DESC_SBQG, Source_Module, DT_Array1D);

    mdi.AddOutput(VAR_QRECH, UNIT_FLOW_CMS, DESC_QRECH, DT_Array1D);
    mdi.AddOutput("SWMM_NODE_DEPTH", UNIT_LEN_M, "Water depth at SWMM_NODES records", DT_Array1D);
    mdi.AddOutput("SWMM_NODE_HEAD", UNIT_LEN_M, "Hydraulic grade elevation at SWMM_NODES records", DT_Array1D);
    mdi.AddOutput("SWMM_NODE_OVERFLOW", UNIT_FLOW_CMS, "Flooded-node overflow at SWMM_NODES records", DT_Array1D);
    mdi.AddOutput("SWMM_LINK_FLOW", UNIT_FLOW_CMS, "Signed river discharge at SWMM_REACHES records", DT_Array1D);
    mdi.AddOutput("SWMM_LINK_VELOCITY", "m/s", "Signed river velocity at SWMM_REACHES records", DT_Array1D);

    const string result = mdi.GetXMLDocument();
    char* metadata = new char[result.size() + 1];
    strprintf(metadata, result.size() + 1, "%s", result.c_str());
    return metadata;
}
