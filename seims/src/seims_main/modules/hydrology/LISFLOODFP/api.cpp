#include "api.h"

#include "LISFLOODFP.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new LISFLOODFP();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    string res;

    mdi.SetAuthor("Dawei Xiao");
    mdi.SetClass(MCLS_OL_ROUTING, MCLSDESC_OL_ROUTING);
    mdi.SetDescription(MDESC_LISFLOODFP);
    mdi.SetID(MID_LISFLOODFP);
    mdi.SetName(MID_LISFLOODFP);
    mdi.SetVersion("1.0");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");
	
	mdi.AddParameter(Tag_TimeStep, UNIT_HOUR, DESC_TIMESTEP, File_Input, DT_Single);
	//mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
	//mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
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

    /// write out the XML file.
    res = mdi.GetXMLDocument();

    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
