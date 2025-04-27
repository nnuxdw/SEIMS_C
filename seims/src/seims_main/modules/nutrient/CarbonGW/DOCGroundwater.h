/*!
 * \file NutrientinGroundwater.h
 * \brief Calculates the nitrate and soluble phosphorus loading contributed by groundwater flow.
 *
 * Changelog:
 *
 * \author Huiran Gao, Liangjun Zhu
 */
#ifndef SEIMS_MODULE_CarbonGW_H
#define SEIMS_MODULE_CarbonGW_H

#include "SimulationModule.h"



/*!

 * \brief Calculates the nitrate and soluble phosphorus loading contributed by groundwater flow.
 *
 */

class DOCGroundwater : public SimulationModule {
public:
	DOCGroundwater();

    ~DOCGroundwater();

	void SetSubbasins(clsSubbasins* subbasins) OVERRIDE;

    void SetValue(const char* key, float value) OVERRIDE;

	void Set2DData(const char* key, int nrows, int ncols, float** data) OVERRIDE;

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    void InitialOutputs() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

	void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;



    TimeStepType GetTimeStepType() OVERRIDE{ return TIMESTEP_CHANNEL; }

private:
	/// validate cells number
	int m_nCells;
	/// cell width of the grid (m)
	float m_CellWidth;
	//parameters
	int m_nSubbsns;
	//! subbasin IDs
	vector<int> m_subbasinIDs;
	/// subbasins information
	clsSubbasins* m_subbasinsInfo;
	/// subbasin grid (subbasins ID)
	float* m_subbsnID;
	float* m_area;
	
	float m_hlife_docgw;
	float m_kdoc;
	float m_dp_co;
	float gw_delay;

	//input
	float* m_soilPerco;
	float* m_soilPercoDIC;
	float* m_recharge1;
	float* m_recharge2;
	float* curBasinArea;
	float* m_gw_shallow;

	//outputs
	float* m_gw_DOCSto; 
	float* m_gw_DOCconc;   
	float* m_Deepgrndwtr_DOC;
	float* m_gwDOCtoCH;
	float* m_gw_DICSto;    
	float* m_Deepgrndwtr_DIC;
	float* m_gwDICtoCH;

	int m_maxSoilLyrs;


};
#endif /* SEIMS_MODULE_CarbonGW_H */
