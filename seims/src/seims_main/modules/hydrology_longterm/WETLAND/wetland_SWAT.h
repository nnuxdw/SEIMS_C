/*!
 * \file pothole_SWAT.h
 * \brief Simulates depressional areas that do not drain to the stream network (pothole)
 *          and impounded areas such as rice paddies
 *
 * Changlog:
 *   - 1. 2016-09-27 - lj -
 *        -# Source code of SWAT include: pothole.f
 *        -# Add the simulation of Ammonia n transported with surface runoff
 *        -# Add m_depEvapor and m_depStorage from DEP_LENSLEY module
 *        -# Using a simple model (first-order kinetics equation) to simulate N transformation in impounded area.
 *   - 2. 2016-10-10 - lj - Update all related variables after the simulation of pothole.
 *   - 3. 2017-08-23 - lj - Solve inconsistent results when using openmp to reducing raster data according to subbasin ID.
 *
 * \author Liang-Jun Zhu
 */
#ifndef SEIMS_MODULE_WETLAND_H
#define SEIMS_MODULE_WETLAND_H

#include "SimulationModule.h"

class WETLAND: public SimulationModule {
public:
    WETLAND();

    ~WETLAND();

    void SetValue(const char* key, float value) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

    
    void SetSubbasins(clsSubbasins*) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;


    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* nrows, int* ncols, float*** data) OVERRIDE;




private:

    void WetlandSimulate(int id);

private:
    /// valid cells number
    int m_nCells;
    /// cell area, ha
    float m_cellArea;
    /// timestep, sec
    float m_timestep;
    /// soil layers
    float* m_nSoilLyrs;
    /// max soil layers
    int m_maxSoilLyrs;
    /// subbasin ID
    float* m_subbasin;
    /// subbasin number
    int m_nSubbasins;
    /**
    *	@brief Routing layers according to the flow direction
    *
    *	There are not flow relationships within each layer.
    *	The first element in each layer is the number of cells in the layer
    */
    float** m_rteLyrs;
    /// number of routing layers
    int m_nRteLyrs;
    /// saturated conductivity
    float** m_ks;
    /// soil thickness
    float** m_soilThick;

    /// pet
    float* m_pet;
    /// surface runoff, mm
    float* m_surfaceRunoff;
    /// surface runoff to channel, m^3/s
    float* m_surfqToCh;

	//ljj
	/// subbasin related
	//! subbasin IDs
	vector<int> m_subbasinIDs;
	/// subbasin grid (subbasins ID)
	float* m_subbsnID;
	/// subbasins information
	clsSubbasins* m_subbasinsInfo;


    float* m_rchID;
    float* m_pcp;
    float* m_pNet;
    float** m_soilPerco;
    //input 
    float* wet_mxvol;
    float* wet_nvol;
    float* wet_nsa;
    float* wet_mxsa;
    float* bw1;
    float* bw2;
    float evwet;
    float wetmxvol;
    float wetnvol;
    float wetk;
    float wetlagtime;
    float m_soilFrozenTemp;

    float* m_wet_k;
    float** m_subSurfRf;
    float** m_subSurfRfVol;
    float** m_soildepth;
    
	float* m_area;
    float* m_landCover;
    float* m_WetVol;
    float* m_latqToCh;
    float* m_subarea;
    float* m_wetarea;
    float* m_cellfr;
    float** m_flowInIdxD8;
    float* m_flowOutIdxD8;

    float** m_wetland_wt;
    float** m_wetland_oc;
    float*  m_soilTemp;

    float* m_soilSurfCbn;
	float* m_soilSurfInOrgnCbn;
	float* m_soilsediLPOC;
	float* m_soilsediRPOC;
    float* m_IfluCbntoCH;
	float* m_IfluInOrgnCbntoCH;
    float* m_sublatDOC;
    float* m_sublatDIC;
    float* m_LPOCtoCH;
    float* m_RPOCtoCH;
    float* m_surfDOCtoCH;
    float* m_surfDICtoCH;

    float* m_WetDOC;
    float* m_WetDIC;
    float* m_WetLPOC;
    float* m_WetRPOC;

    float* m_mvsurfdoc; //+upstream, kg
	float* m_mvsurfdic;
	float* m_mvsurflpoc;
	float* m_mvsurfrpoc;

    float* m_SurfRfVol;
    float* m_upstreamIfluInOrgnCbntoCH;
    float* m_upstreamIfluCbntoCH;
    float* m_soilPercoCbnLowest;
    float* m_PercoCbn;

    float* m_wetland_doccon;
    float* m_wetland_production;
    float* m_wetdoccon;
    float* m_surf_leachdoc;

    float ksr0;
    float ksr1;
    float ksr2;
    float krem0;
    float krem1;
    float krem2;
    float Cdoc;
};
#endif /* SEIMS_MODULE_IMP_SWAT_H */
