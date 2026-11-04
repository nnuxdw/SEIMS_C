#include "api.h"

#include "MetadataInfo.h"
#include "SoilTemperatureNOAA.h"
#include "text.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance() {
    return new SoilTemperatureNOAA();
}

extern "C" SEIMS_MODULE_API const char* MetadataInformation() {
    MetadataInfo mdi;
    mdi.SetAuthor("Junzhi Liu, Liangjun Zhu, Tang Kai");
    mdi.SetClass(MCLS_SOLT, MCLSDESC_SOLT);
    mdi.SetDescription(MDESC_STP_NOAA);
    mdi.SetID(MID_STP_NOAA);
    mdi.SetName(MID_STP_NOAA);
    mdi.SetVersion("1.1");
    mdi.SetEmail(SEIMS_EMAIL);
    mdi.SetWebsite(SEIMS_SITE);
    mdi.SetHelpfile("");

    mdi.AddParameter(Tag_TimeStep, UNIT_SECOND, DESC_TIMESTEP, File_Input, DT_Single);
    mdi.AddParameter(VAR_SOL_TA0, UNIT_NON_DIM, DESC_SOL_TA0, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TA1, UNIT_NON_DIM, DESC_SOL_TA1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TA2, UNIT_NON_DIM, DESC_SOL_TA2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TA3, UNIT_NON_DIM, DESC_SOL_TA3, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TB1, UNIT_NON_DIM, DESC_SOL_TB1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TB2, UNIT_NON_DIM, DESC_SOL_TB2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TD1, UNIT_NON_DIM, DESC_SOL_TD1, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_SOL_TD2, UNIT_NON_DIM, DESC_SOL_TD2, Source_ParameterDB, DT_Single);
    mdi.AddParameter(VAR_K_SOIL10, UNIT_NON_DIM, DESC_K_SOIL10, Source_ParameterDB, DT_Single);

    mdi.AddParameter(VAR_B, UNIT_NON_DIM, DESC_B, Source_ParameterDB_Optional, DT_Single);
    mdi.AddParameter(VAR_QUARTZ, UNIT_NON_DIM, DESC_QUARTZ, Source_ParameterDB_Optional, DT_Single);
    mdi.AddParameter(VAR_SATPSI, UNIT_LEN_M, DESC_SATPSI, Source_ParameterDB_Optional, DT_Single);
    mdi.AddParameter(VAR_LATENT_HEAT, UNIT_LATENTHEAT_JKG, DESC_LATENT_HEAT,
                     Source_ParameterDB_Optional, DT_Single);

    mdi.AddParameter(VAR_SOILLAYERS, UNIT_NON_DIM, DESC_SOILLAYERS, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOILDEPTH, UNIT_DEPTH_MM, DESC_SOILDEPTH, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOILTHICK, UNIT_DEPTH_MM, DESC_SOILTHICK, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_POROST, UNIT_VOL_FRA_M3M3, DESC_POROST, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_WPMM, UNIT_DEPTH_MM, DESC_SOL_WPMM, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_BD, UNIT_DENSITY, DESC_SOL_BD, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOLAVBD, UNIT_DENSITY, DESC_SOL_BD, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter("Clay", "%", "Percent of clay content", Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter("Sand", "%", "Percent of sand content", Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_UL, UNIT_DEPTH_MM, DESC_SOL_UL, Source_ParameterDB, DT_Raster2D);
    mdi.AddParameter(VAR_SOL_RSDIN, UNIT_CONT_KGHA, DESC_SOL_RSDIN, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_SOIL_T10, UNIT_NON_DIM, DESC_SOIL_T10, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_LANDUSE, UNIT_NON_DIM, DESC_LANDUSE, Source_ParameterDB, DT_Raster1D);
    mdi.AddParameter(VAR_TMEAN_ANN, UNIT_TEMP_DEG, DESC_TMEAN_ANN, Source_ParameterDB, DT_Raster1D);

    mdi.AddInput(VAR_TMEAN, UNIT_TEMP_DEG, DESC_TMEAN, Source_Module, DT_Raster1D);
    mdi.AddInput(VAR_SOL_ST, UNIT_DEPTH_MM, DESC_SOL_ST, Source_Module, DT_Raster2D);
    mdi.AddInput(VAR_SOL_SW, UNIT_DEPTH_MM, DESC_SOL_SW, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_SNAC, UNIT_DEPTH_MM, DESC_SNAC, Source_Module_Optional, DT_Raster1D);
    mdi.AddInput(VAR_SOL_RSD, UNIT_CONT_KGHA, DESC_SOL_RSD, Source_Module_Optional, DT_Raster2D);

    mdi.AddOutput(VAR_SOTE, UNIT_TEMP_DEG, DESC_SOTE, DT_Raster1D);
    mdi.AddOutput(VAR_TMEAN1, UNIT_TEMP_DEG, DESC_TMEAN1, DT_Raster1D);
    mdi.AddOutput(VAR_TMEAN2, UNIT_TEMP_DEG, DESC_TMEAN2, DT_Raster1D);
    mdi.AddOutput(VAR_SOILT, UNIT_TEMP_DEG, DESC_SOTE, DT_Array2D);
    mdi.AddOutput(VAR_SOTE1, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE5, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE15, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE30, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE60, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE100, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOTE200, UNIT_TEMP_DEG, DESC_SOTE, DT_Array1D);
    mdi.AddOutput(VAR_SOLICE, UNIT_DEPTH_MM, DESC_SOLICE, DT_Array2D);
    mdi.AddOutput(VAR_SOLWC, UNIT_DEPTH_MM, DESC_SOLWC, DT_Array2D);

    mdi.AddOutput(VAR_SOIL_TEMPERATURE, UNIT_TEMP_K, DESC_SOIL_TEMPERATURE, DT_Array2D);
    mdi.AddOutput(VAR_SOIL_ICE_CONTENT, UNIT_VOL_FRA_M3M3, DESC_SOIL_ICE_CONTENT, DT_Array2D);
    mdi.AddOutput(VAR_SOIL_LIQUID_CONTENT, UNIT_VOL_FRA_M3M3, DESC_SOIL_LIQUID_CONTENT, DT_Array2D);
    mdi.AddOutput(VAR_ICE_FRACTION_SCHAAKE, UNIT_LEN_M, DESC_ICE_FRACTION_SCHAAKE, DT_Raster1D);
    mdi.AddOutput(VAR_ICE_FRACTION_XINAN, UNIT_FRACTION_DIMLESS, DESC_ICE_FRACTION_XINAN, DT_Raster1D);
    mdi.AddOutput(VAR_GROUND_HEAT_FLUX, UNIT_HEATFLUX_WM2, DESC_GROUND_HEAT_FLUX, DT_Raster1D);

    string res = mdi.GetXMLDocument();
    char* tmp = new char[res.size() + 1];
    strprintf(tmp, res.size() + 1, "%s", res.c_str());
    return tmp;
}
