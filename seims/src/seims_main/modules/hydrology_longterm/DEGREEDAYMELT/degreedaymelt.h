/*!
 * \brief
 * \author Ruoyun Cao
 * \date 2021-8-3
 */

#ifndef SEIMS_DEGREEDAYMELT_H
#define SEIMS_DEGREEDAYMELT_H

#include "SimulationModule.h"

using namespace std;

/*!
 * \defgroup SNO_DD
 * \brief Calculate snow melt by Degree-Day method
 *
 */

/*!
 * \class DEGREEDAYMELT
 * \ingroup DEGREEDAYMELT
 * \brief Calculate snow melt by Degree-Day method
 *
 */

class DEGREEDAYMELT : public SimulationModule {
public:
    //! Constructor
    DEGREEDAYMELT(void);

    //! Destructor
    ~DEGREEDAYMELT(void);

    virtual int Execute(void);

    virtual void SetValue(const char *key, float data);

    void Set1DData(const char *key, int n, float *data);

    virtual void Get1DData(const char *key, int *n, float **data);

    //bool CheckInputSize(const char *key, int n);

    bool CheckInputData(void);

    void InitialOutputs(void);

private:
	bool glacier(int i);
	bool normal(int i);
	
	//////////////////////////////////////////////////////////////////////////// parameters
	/// cell size
	float m_CellWidth;

	//basal accumulation coefficient
	float m_beta0;

	///daily average air temperature
	float *m_tMean;

	///Threshold temperature to split snowfall and rainfall 
	float m_tGmit;

	///Degree-day factor of snow 
	float m_dd_Snow;

	///Degree-day factor for ice melt
	float m_dd_glacierMelt;
	
	//precipitation
	float *m_prec;

	///radiation ablation factor of shortwave radiation 
	float m_srf;

	///albedo
	float *m_albedo;

	///incident shortwave radiation
	float *m_sr;

	//the number of glaciers 
	int m_nGlc;
	int m_nCells;
	int m_nSubbsns;

	/// landuse type, for distinguish calculation, such as water body.
	float* m_landUse;
	//////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////// outputs


	//Glacier volume(m^3)
	float *m_glacQ;

	//Glacier Runoff(mm)
	float *m_dGlacRunoff;

	float *sublimation;

	float* sw;
	float* su;
	float* sf;
	float* ss;
	float* sg;
	float* sfg;

	float* m_snomelt;
	float* Qfg;
	float* Qf;
	float* Qs;

	float* m_area;
	float* m_packT;
	float* m_maxTemp;

	float Kfg;
	float Ca;
	float Cg;
	float m_snowTemp;
	float m_csnow6;
	float m_csnow12;
	float m_t0;
	float m_lagSnow;


};
#endif /* SEIMS_DEGREEDAYMELT_H */
