#include "api.h"

#include "DOCGroundwater.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new DOCGroundwater();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    mdi.SetAuthor("Jiaojiao Liu");
	mdi.SetClass(MCLS_CarbonGW, MCLSDESC_CarbonGW);
	mdi.SetDescription(MDESC_NUTRGW);
	mdi.SetID(MDESC_CarbonGW);
	mdi.SetName(MDESC_CarbonGW);
	mdi.SetVersion("1.0");
	mdi.SetEmail(SEIMS_EMAIL);
	mdi.SetWebsite(SEIMS_SITE);
	mdi.SetHelpfile("");

    // set the parameters
	mdi.AddParameter(VAR_SUBBSN, UNIT_NON_DIM, DESC_SUBBSN, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_SUBBSNID_NUM, UNIT_NON_DIM, DESC_SUBBSNID_NUM, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_SUBBASIN_PARAM, UNIT_NON_DIM, DESC_SUBBASIN_PARAM, Source_ParameterDB, DT_Subbasin);
	mdi.AddParameter(Tag_CellSize, UNIT_NON_DIM, DESC_CellSize, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_HLDOCGW, UNIT_DAY, DESC_HLDOCGW, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_AHRU, UNIT_AREA_M2, DESC_AHRU, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_DF_COEF, UNIT_NON_DIM, DESC_DF_COEF, Source_ParameterDB, DT_Single);
	mdi.AddParameter("gw_delay", UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Single);
	//mdi.AddParameter("gw_delay_1d", UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Raster1D);
	//mdi.AddParameter("hlife_docgw_1d", UNIT_NON_DIM, DESC_KG, Source_ParameterDB, DT_Raster1D);
	
	// from other module
	mdi.AddInput(VAR_PERC_LOWEST_DOC, UNIT_CONT_KGHA, DESC_PERC_LOWEST_DOC, Source_Module, DT_Array1D);
	//mdi.AddInput(VAR_PERC_LOWEST_DIC, UNIT_CONT_KGHA, DESC_PERC_LOWEST_DIC, Source_Module, DT_Array1D);

	// output
	mdi.AddOutput("gw_RDOCtoCH", UNIT_KG, DESC_GWD_RDOCtoCH, DT_Array1D);
	mdi.AddOutput("gw_DOCsto", UNIT_KG, "TODO", DT_Array1D);
	//mdi.AddOutput("gw_DICtoCH", UNIT_KG, "TODO", DT_Array1D);


    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
