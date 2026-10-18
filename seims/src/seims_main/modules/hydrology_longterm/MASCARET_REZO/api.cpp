#include "api.h"
#include "MASCARET_REZO.h"
#include "MetadataInfo.h"
#include "text.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new MASCARET_REZO();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo metadata;
    metadata.SetAuthor("WISE C++ hydraulic routing");
    metadata.SetClass(MCLS_CH_ROUTING, MCLSDESC_CH_ROUTING);
    metadata.SetDescription("C++ one-dimensional routing with optional node-reach topology for bifurcating and cyclic networks.");
    metadata.SetEmail(SEIMS_EMAIL);
    metadata.SetHelpfile("");
    metadata.SetID("MASCARET_REZO");
    metadata.SetName("MASCARET_REZO");
    metadata.SetVersion("0.3");
    metadata.SetWebsite(SEIMS_SITE);
    metadata.AddParameter(Tag_ChannelTimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Input, DT_Single);
    metadata.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
    metadata.AddParameter(VAR_OUTLETID, UNIT_NON_DIM, DESC_OUTLETID, Source_ParameterDB, DT_Single);
    metadata.AddParameter(VAR_REACH_PARAM, UNIT_NON_DIM, DESC_REACH_PARAM, Source_ParameterDB, DT_Reach);
    metadata.AddInput(VAR_SBOF, UNIT_FLOW_CMS, DESC_SBOF, Source_Module, DT_Array1D);
    metadata.AddInput(VAR_SBIF, UNIT_FLOW_CMS, DESC_SBIF, Source_Module, DT_Array1D);
    metadata.AddInput(VAR_SBQG, UNIT_FLOW_CMS, DESC_SBQG, Source_Module, DT_Array1D);
    metadata.AddOutput(VAR_QRECH, UNIT_FLOW_CMS, DESC_QRECH, DT_Array1D);
    metadata.AddOutput(VAR_CHWTRDEPTH, UNIT_LEN_M, DESC_CHWTDEPTH, DT_Array1D);
    const string xml = metadata.GetXMLDocument();
    char* result = new char[xml.size() + 1];
    strprintf(result, xml.size() + 1, "%s", xml.c_str());
    return result;
}
