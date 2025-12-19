#include "api.h"

#include "SoilTemperatureFINPL.h"
#include "text.h"
#include "MetadataInfo.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new SoilTemperatureFINPL();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;

    // set the information properties
    mdi.SetAuthor("Junzhi Liu, Liangjun Zhu");
    mdi.SetClass(MCLS_SOLT, MCLSDESC_SOLT);
    mdi.SetDescription(MDESC_STP_FP);
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetID(MID_STP_FP);
    mdi.SetName(MID_STP_FP);
    mdi.SetVersion("1.1");
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    /// from parameter database
    mdi.AddParameter(VAR_SOL_TA0, UNIT_NON_DIM, DESC_SOL_TA0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TA1, UNIT_NON_DIM, DESC_SOL_TA1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TA2, UNIT_NON_DIM, DESC_SOL_TA2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TA3, UNIT_NON_DIM, DESC_SOL_TA3, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TB1, UNIT_NON_DIM, DESC_SOL_TB1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TB2, UNIT_NON_DIM, DESC_SOL_TB2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TD1, UNIT_NON_DIM, DESC_SOL_TD1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TD2, UNIT_NON_DIM, DESC_SOL_TD2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_K_SOIL10, UNIT_NON_DIM, DESC_K_SOIL10, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_SOIL_T10, UNIT_NON_DIM, DESC_SOIL_T10, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);
    /// mean air temperature
    mdi.AddInput(VAR_TMEAN, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);

    /// output soil temperature
    mdi.AddOutput(VAR_SOTE, UNIT_TEMP_DEG, DESC_SOTE, DT_Raster1D);
    mdi.AddOutput(VAR_TMEAN1, UNIT_TEMP_DEG, DESC_TMEAN1, DT_Raster1D); /// mean air temperature of the day(d-1)
    mdi.AddOutput(VAR_TMEAN2, UNIT_TEMP_DEG, DESC_TMEAN2, DT_Raster1D); /// mean air temperature of the day(d-2)

    //ljj++
    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_DEM, UNIT_LEN_M, DESC_DEM, Source_ParameterDB_Optional, DT_Raster1D);
    mdi.AddParameter(VAR_TMEAN_ANN, UNIT_TEMP_DEG, DESC_TMEAN_ANN, Source_ParameterDB, DT_Raster1D);
    mdi.AddInput(DataType_MaximumMonthlyTemperature, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
    mdi.AddInput(DataType_MinimumMonthlyTemperature, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
    mdi.AddInput(DataType_SolarRadiation, UNIT_SR, DESC_SR, Source_Module, DT_Raster1D);
    mdi.AddInput(DataType_RelativeAirMoisture, UNIT_PERCENT, DESC_RM, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_TMAX, UNIT_TEMP_DEG, DESC_TMAX, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_TMIN, UNIT_TEMP_DEG, DESC_TMIN, Source_Module, DT_Raster1D);

    mdi.AddParameter(VAR_SOILDEPTH, UNIT_DEPTH_MM, DESC_SOILDEPTH, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_BD, UNIT_DENSITY, DESC_SOL_BD, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOLAVBD, UNIT_DENSITY, DESC_SOL_BD, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter("Clay", "%", "Percent of clay content", Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter("Sand", "%", "Percent of sand content", Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_UL, Source_ParameterDB, DT_Raster2D);   // m_sat

    mdi.AddParameter(VAR_TSOIL1, UNIT_NON_DIM, DESC_TSOIL1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_TSOIL2, UNIT_NON_DIM, DESC_TSOIL2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_TSOIL3, UNIT_NON_DIM, DESC_TSOIL3, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_TSOIL4, UNIT_NON_DIM, DESC_TSOIL4, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_TSOIL5, UNIT_NON_DIM, DESC_TSOIL5, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_DDEPTH1, UNIT_NON_DIM, DESC_TSOIL1, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_SNOCOVMX, UNIT_DEPTH_MM, DESC_SNOCOVMX, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SNO50COV, UNIT_NON_DIM, DESC_SNO50COV, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_T_SOIL, UNIT_TEMP_DEG, DESC_T_SOIL, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_SOL_RSDIN, UNIT_CONT_KGHA, DESC_SOL_RSDIN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_IGRO, UNIT_NON_DIM, DESC_IGRO, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_BLAI, UNIT_AREA_RATIO, DESC_BLAI, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOL_ALB, UNIT_NON_DIM, DESC_SOL_ALB, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_IDC, UNIT_NON_DIM, DESC_IDC, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_EXT_COEF, UNIT_NON_DIM, DESC_EXT_COEF, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_CELL_LAT, UNIT_LONLAT_DEG, DESC_CELL_LAT, Source_ParameterDB, DT_Raster1D);

    mdi.AddInput(VAR_DAYLEN, UNIT_HOUR, DESC_DAYLEN,Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Array2D);
    mdi.AddInput(VAR_SOL_COV, UNIT_CONT_KGHA, DESC_SOL_COV, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_SNAC, UNIT_DEPTH_MM, DESC_SNAC, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SOL_SW, UNIT_DEPTH_MM, DESC_SOL_SW, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_LAIDAY, UNIT_AREA_RATIO, DESC_LAIDAY, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SOL_RSD, UNIT_CONT_KGHA, DESC_SOL_RSD, Source_Module_Optional, DT_Raster2D);

    mdi.AddOutput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, DT_Array2D);
    mdi.AddOutput(VAR_SOTE1, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE5, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE15, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE30, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE60, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE100, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE200, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOLICE, UNIT_TEMP_DEG, DESC_SOLICE, DT_Array2D);
    mdi.AddOutput(VAR_SOLWC, UNIT_TEMP_DEG, DESC_SOLWC, DT_Array2D);

    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
