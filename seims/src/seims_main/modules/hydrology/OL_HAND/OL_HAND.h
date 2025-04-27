/*!
 * \brief A IO test demo of developing module for SEIMS.
 *
 * \author Liangjun Zhu
 * \date 2018-02-07
 */

#ifndef SEIMS_MODULE_IO_TEST_H
#define SEIMS_MODULE_IO_TEST_H

#include "SimulationModule.h"
#include "Scenario.h"
#include "clsReach.h"

using namespace bmps;

class OL_HAND : public SimulationModule {
public:
	OL_HAND();

    virtual ~OL_HAND();

    void Set1DData(const char* key, int n, float* data) OVERRIDE;

    void Set2DData(const char* key, int n, int col, float** data) OVERRIDE;

    void SetScenario(Scenario* sce) OVERRIDE;

    void SetReaches(clsReaches* reaches) OVERRIDE;

    bool CheckInputData() OVERRIDE;

    int Execute() OVERRIDE;

    void Get1DData(const char* key, int* n, float** data) OVERRIDE;

    void Get2DData(const char* key, int* n, int* col, float*** data) OVERRIDE;

private:
    /// valid cells number
    int m_nCells;
    /// input 1D raster data
    float* m_raster1D;
    /// maximum number of soil layers
    int m_maxSoilLyrs;
    /// soil layers
    float* m_nSoilLyrs;
    /// input 2D raster data
    float** m_raster2D;
    /// output 1D raster data
    float* m_output1Draster;
    /// output 2D raster data
    float** m_output2Draster;
    /// BMPs Scenario data
    Scenario* m_scenario;

    /// Reach information
    clsReaches* m_reaches;
	int m_chNumber;
	/// downstream id (The value is 0 if there if no downstream reach)
	float *m_reachDownStream;
	/// upstream id (The value is -1 if there if no upstream reach)
	vector <vector<int>> m_reachUpStream;
	/// reach manning's n
	float *m_reachN;
	/// map from subbasin id to index of the array
	map<int, int> m_idToIndex;
	/// channel width (raster type to keep consistent with the one in IKW_CH, zero for overland cells)
	float *m_chWidth;
	/// channels
	map<int, vector<int> > m_reachLayers;
};
#endif /* SEIMS_MODULE_IO_TEST_H */
