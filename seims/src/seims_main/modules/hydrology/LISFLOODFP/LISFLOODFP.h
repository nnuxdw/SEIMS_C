/*!
 * \brief A IO test demo of developing module for SEIMS.
 *
 * \author Dawei Xiao
 * \date 2018-02-07
 */

#ifndef SEIMS_MODULE_LISFLOODFP_H
#define SEIMS_MODULE_LISFLOODFP_H

#include "SimulationModule.h"
#include "Scenario.h"
#include "lisflood.h"
//#include "./lisflood2/DataTypes.h"

using namespace bmps;
using namespace std;


class LISFLOODFP : public SimulationModule {
public:

	LISFLOODFP();

    virtual ~LISFLOODFP();

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

	void SetReaches(clsReaches* reaches) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

	void SetValue(const char* key, const float value) OVERRIDE;

	void InitialOutputs() OVERRIDE;

	void RunCalculation();

	void InitializeLisfloodFP();


private:


	int m_dt;            ///< time step (sec)
	int m_nCells;
	int m_nSubbsns;
	int m_inputSubbsnID; ///< current subbasin ID, 0 for the entire watershed

	// Instances of Structures
	Arrays Raster;
	Files Fps;
	Fnames ParFp;
	States SimStates;
	Pars Params;
	Solver ParSolver;
	Pois PoisHandler;
	BoundCs Bounds;
	Stage OutLocs;
	SGCprams SGCchanprams;
	DamData DamDataprams;
	vector<ChannelSegmentType> ChannelSegments;
	//LISFLOODFPContext LFPContext;
	//int verbosemode;
	

	Arrays *Arrptr;
	Files* FpsPtr;
	Fnames *Fnameptr;
	States *Statesptr;
	Pars *Parptr;
	Solver *Solverptr;
	Pois *Poisptr;
	BoundCs *BCptr;
	Stage *Stageptr;
	SGCprams *SGCptr;
	DamData *Damptr;
	Stage *Locptr;
	char* tmpFileNamePtr;
	char* tmpSysCmdPtr;
	vector<ChannelSegmentType> *ChannelSegmentsVecPtr;
	SuperGridLinksList *Super_linksptr;
	LISFLOODFPContext* LFPContextPtr;

	

	

};
#endif 
