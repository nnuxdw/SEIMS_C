/*!
 * \brief
 * \author Jing Ma
 * \date 2024-10-27
 */

#ifndef SEIMS_GLA_DH_H
#define SEIMS_GLA_DH_H

#include "SimulationModule.h"

using namespace std;

/*!
 * \defgroup SNO_DD
 * \brief Calculate snow melt by Degree-Day method
 *
 */

/*!
 * \class GLA_DH
 * \ingroup GLA_DH
 * \brief Calculate snow melt by Degree-Day method
 *
 */

class GLA_DH : public SimulationModule {
public:
    //! Constructor
    GLA_DH(void);

    //! Destructor
    ~GLA_DH(void);

    virtual int Execute(void);

    virtual void SetValue(const char *key, float data);

    void Set1DData(const char *key, int n, float *data);

	void Set2DData(const char* key, int n, int col, float** data);

	virtual void GetValue(const char *key, float * value);

    virtual void Get1DData(const char *key, int *n, float **data);

	virtual void Get2DData(const char* key, int* nrows, int* ncols, float*** data);

    //bool CheckInputSize(const char *key, int n);

    bool CheckInputData(void);

    void InitialOutputs(void);

private:
	bool normal(int i);
	bool glacier_all(int i,int j);
	bool hparameter(int j);

	/// Handle lookup tables ///

	/// glacier lookup table
	//void InitializeGlacierLookup();

	
	//////////////////////////////////////////////////////////////////////////// parameters
	float *Qave;
	float *gmb_ave;
	float Qnogla;
	float Qall;
	float Q;

	/// cell size
	float m_CellWidth;

	//The splitter 
	float D;

	//Recession coefficient of fast response reservoir 
	float Kf;

	//Recession coefficient of slow response reservoir 
	float Ks;

	//Recession coefficient of glacier streamflow 
	float Kfg;

	float Ca;
	float Cg;

	//Evaporation threshold value
	float Ce;

	//Root zone storage capacity 
	float Sumax;

	//Shape parameter 
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

	float** hlu_glacier_area;
	float* hlu_nonglacier_area;
	float* hlu_area;
	///radiation ablation factor of shortwave radiation 
	float m_srf;

	///albedo
	float *m_albedo;

	///incident shortwave radiation
	float *m_sr;

	//the number of glaciers 
	int m_nGlc;
	//the number of glaciers +1
	int m_nGlc_sum;
	int m_nCells;
	int m_nSubbsns;
	int n1;//hparameter的高程带

	///hparameter
	float** E;
	float** h;
	float** area;
	float** hscaled;
	float** ascaled;
	float* E_norm;
	float* dth;
	float* Emax;
	float* Emin;
	float* dtv;
	float* dtm;
	float* ascaled_sum;
	float* vscaled_sum;
	float ** m_hluID;


	//surface area of glacier
	//float *m_gArea;

	//the day of the year
	float *m_t;

	//float *m_iceStorage_init;

	float * m_subbsnID;

	float *m_weight;

	/// landuse type, for distinguish calculation, such as water body.
	float* m_landUse;

	/// glacier lookup table
	///float** m_glacierLookup;
	/// glacier number
	///int m_glacierNum;
	/// map from m_glacierLookup
	///map<int, float *> m_glacierLookupMap;
	//////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////// outputs


	//Ice storage(mm)
	float *m_iceStorage;

	//Snow storage��mm��
	float *m_snowStorage;

	//Glacier volume(m^3)
	float *m_glacQ;

	//Glacier Runoff(mm)
	//float *m_dGlacRunoff;
	float** m_dGlacRunoff;

	//float* sublimation;
	float** sublimation;

	
	//ljj++
	//float* isglacier;

	//float* sw;
	float** sw;
	float* su;
	float* sf;
	float* ss;
	//float* sg;
	float** sg;
	//float* sfg;
	float** sfg;

	//float* m_snomelt;
	float** m_snomelt;
	float* Qfg; 
	float** Qfgglacier;
	float* Qf;
	float* Qs;
	float* Qm;



	float* m_sub_qfast;
	float* m_sub_qslow;
	float* m_sub_qg;
	float* m_sub_glacierq;
	float* m_gmb;

	float m_snowTemp;
	float m_csnow6;
	float m_csnow12;
	float* sno_pack;



};
#endif /* SEIMS_DEGREEDAYMELT_H */
