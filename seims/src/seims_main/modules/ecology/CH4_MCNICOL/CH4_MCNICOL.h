/*!
 * \file CH4_MCNICOL.h
 * \brief Reduced wetland methane model based on McNicol et al. (2026)
 *        CH4 production, oxidation and emission are represented by one ODE:
 *        dMs/dt = fp - fo - fe.
 *
 * \author Lijing
 */

#ifndef SEIMS_MODULE_CH4_MCNICOL_H
#define SEIMS_MODULE_CH4_MCNICOL_H

#include "SimulationModule.h"

 /** \defgroup CH4_MCNICOL
 * \ingroup Ecology
 * \brief Reduced wetland methane emission model.
 */

 /*!
  * \class CH4_MCNICOL
  * \ingroup CH4_MCNICOL
  * \brief Simulates wetland methane emission using a reduced process-based model.
  */

using namespace std;

class CH4_MCNICOL : public SimulationModule {
public:
	CH4_MCNICOL();

	~CH4_MCNICOL();

	void SetValue(const char* key, float value) OVERRIDE;

	void Set1DData(const char* key, int n, float* data) OVERRIDE;

	void Set2DData(const char* key, int nrows, int ncols, float** data) OVERRIDE;

	bool CheckInputData() OVERRIDE;

	void InitialOutputs() OVERRIDE;

	int Execute() OVERRIDE;

	void Get1DData(const char* key, int* n, float** data) OVERRIDE;

	void Get2DData(const char* key, int* nrows, int* ncols, float*** data) OVERRIDE;

private:
	//////////////////////////////////////////////////////////////////////////
	// Helper functions
	//////////////////////////////////////////////////////////////////////////

	/// Limit value into [lo, hi]
	float Clamp(float x, float lo, float hi);

	/// Estimate water table position from layer soil saturation for cell i, unit: cm
	float CalcWaterTableFromLayers(int i);

	/// Calculate thickness-weighted mean soil temperature for cell i
	float CalcMeanSoilTemp(int i);

private:
	//////////////////////////////////////////////////////////////////////////
	// Basic information
	//////////////////////////////////////////////////////////////////////////

	/// valid cells number
	int m_nCells;

	/// maximum soil layers
	int m_maxSoilLyrs;

	/// simulation time step, hour
	float m_dt;

	//////////////////////////////////////////////////////////////////////////
	// Model parameters
	//////////////////////////////////////////////////////////////////////////

	/// methane production rate, mg CH4 cm-2 d-1
	float m_kp;

	/// production decline exponent when water table is above soil surface
	float m_p1;

	/// maximum methane oxidation rate, d-1
	float m_ko;

	/// water-table control parameter for oxidation, cm-1
	float m_p2;

	/// Q10 for methane production
	float m_Qp;

	/// vegetation substrate exponent
	float m_p3;

	/// Q10 for methane oxidation
	float m_Qo;

	/// combined plant-mediated transport and ebullition rate, d-1
	float m_kEP;

	/// production decline exponent when water table is below soil surface
	float m_p5;

	/// LAI lag time for substrate availability, d
	float m_laiLagDays;

	//////////////////////////////////////////////////////////////////////////
	// Input variables
	//////////////////////////////////////////////////////////////////////////

	/// soil layers for each cell
	float* m_nSoilLyrs;

	/// HAND inundation water depth, m
	float* m_handWtrDep;

	/// leaf area index
	float* m_lai;

	/// maximum(potential) leaf area index(BLAI in cropLookup db)
	float* m_maxLai;

	/// maximum leaf area index for current year
	float* m_maxLaiYr;

	/// soil temperature of all layers, deg C
	float** m_soilTemp;

	/// soil thickness of all layers, mm
	float** m_soilThk;

	/// amount of water stored in soil layers, mm
	float** m_soilWtrSto;

	/// wilting point water content of soil layers, mm
	float** m_soilWP;

	/// soil porosity of all layers, mm/mm
	float** m_soilPor;

	//////////////////////////////////////////////////////////////////////////
	// State and output variables
	//////////////////////////////////////////////////////////////////////////

	/// LAI history buffer for lagged substrate calculation
	float** m_laiHist;

	/// length of LAI history buffer
	int m_laiHistLen;

	/// current index of LAI history buffer
	int m_laiHistIdx;

	/// methane emission flux, mg CH4 m-2 h-1
	float* m_ch4Flux;

	/// methane pool in soil column, mg CH4 cm-2
	float* m_ch4Ms;

	/// methane production rate, mg CH4 cm-2 d-1
	float* m_ch4Prod;

	/// methane oxidation rate, mg CH4 cm-2 d-1
	float* m_ch4Oxid;

	/// methane emission rate before unit conversion, mg CH4 cm-2 d-1
	float* m_ch4Emit;

	/// estimated water table position, cm
	/// z = 0 means soil surface, z > 0 means inundation, z < 0 means below surface
	float* m_ch4Zw;

	/// temperature factor for methane production
	float* m_ch4FTProd;

	/// water-table factor for methane production
	float* m_ch4FWProd;

	/// substrate factor from LAI
	float* m_ch4FBProd;

	/// temperature factor for methane oxidation
	float* m_ch4FTOxid;

	/// water-table factor for methane oxidation
	float* m_ch4FWOxid;
};

#endif /* SEIMS_MODULE_CH4_MCNICOL_H */
