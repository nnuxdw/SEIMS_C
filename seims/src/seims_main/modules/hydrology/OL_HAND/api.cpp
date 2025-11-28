#include "api.h"

#include "OL_HAND.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new OL_HAND();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    string res;

    mdi.SetAuthor("Dawei Xiao");
    mdi.SetClass(MCLS_OL_ROUTING, MCLSDESC_OL_ROUTING);
    mdi.SetDescription(MDESC_OL_HAND);
    mdi.SetID(MID_OL_HAND);
    mdi.SetName(MID_OL_HAND);
    mdi.SetVersion("1.0");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");
	
	mdi.AddParameter(Tag_TimeStep, UNIT_HOUR, DESC_TIMESTEP, File_Input, DT_Single);
	//mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(Tag_SubbasinId, UNIT_NON_DIM, Tag_SubbasinId, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_OUTLETID, UNIT_NON_DIM, DESC_OUTLETID, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	mdi.AddInput(VAR_BKST, UNIT_VOL_M3, DESC_BKST, Source_Module,DT_Array1D);
	mdi.AddInput(VAR_BKST_LAST_STEP, UNIT_VOL_M3, DESC_BKST_LAST_STEP, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_CHST, UNIT_VOL_M3, DESC_CHST, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_CHST_LAST_STEP, UNIT_VOL_M3, DESC_CHST_LAST_STEP, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_CHWTRDEPTH, UNIT_LEN_M, DESC_CHWTDEPTH, Source_Module, DT_Array1D);
	mdi.AddInput(VAR_CHWTRWIDTH, UNIT_LEN_M, DESC_CHWTWIDTH, Source_Module, DT_Array1D);
	
	// add reach information
	mdi.AddParameter(VAR_REACH_PARAM, UNIT_NON_DIM, DESC_REACH_PARAM, Source_ParameterDB, DT_Reach);


	mdi.AddOutput(VAR_OL_HAND_WTRDEP, UNIT_LEN_M, DESC_OLFLOW, DT_Raster1D);
	mdi.AddOutput(VAR_IS_HAND_FLOODED, UNIT_NON_DIM, DESC_IS_HAND_FLOODED, DT_Raster1D);
	mdi.AddOutput(VAR_SUBBASIN_FLOODED_AREA, UNIT_AREA_M2, DESC_SUBBASIN_FLOODED_AREA, DT_Array1D);
	mdi.AddOutput(VAR_SUBBASIN_WTR_DEPTH, UNIT_AREA_M2, DESC_SUBBASIN_WTR_DEPTH, DT_Array1D);
	

    /// write out the XML file.
    res = mdi.GetXMLDocument();

    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
