#include "ol_hand.h"

#include "text.h"

OL_HAND::OL_HAND() :
    m_nCells(-1), m_raster1D(nullptr), m_maxSoilLyrs(-1),
    m_nSoilLyrs(nullptr), m_raster2D(nullptr),
    m_output1Draster(nullptr), m_output2Draster(nullptr),
    m_scenario(nullptr), m_reaches(nullptr) {
}

OL_HAND::~OL_HAND() {
    if (m_output1Draster != nullptr) Release1DArray(m_output1Draster);
    if (m_output2Draster != nullptr) Release2DArray(m_nCells, m_output2Draster);
    // NOTE: m_scenario and m_reaches will be released in DataCenter!
}

void OL_HAND::Set1DData(const char* key, const int n, float* data) {
    if (!CheckInputSize("IO_TEST", key, n, m_nCells)) return;
    string sk(key);
    if (StringMatch(sk, VAR_CN2)) m_raster1D = data;
    else if (StringMatch(sk, VAR_SOILLAYERS)) m_nSoilLyrs = data;
    else {
        throw ModelException("IO_TEST", "Set1DData", "Parameter " + string(key) + " is not exist");
    }
}

void OL_HAND::Set2DData(const char* key, const int n, const int col, float** data) {
    string sk(key);
    if (!CheckInputSize2D("IO_TEST", key, n, col, m_nCells, m_maxSoilLyrs)) return;
    if (StringMatch(sk, VAR_CONDUCT)) {
        m_raster2D = data;
    }
}

void OL_HAND::SetScenario(bmps::Scenario* sce) {
    if (nullptr != sce) m_scenario = sce;
}

void OL_HAND::SetReaches(clsReaches* reaches) {
    if (nullptr != reaches) m_reaches = reaches;
	m_chNumber = reaches->GetReachNumber();

	if (nullptr == m_reachDownStream) reaches->GetReachesSingleProperty(REACH_DOWNSTREAM, &m_reachDownStream);
	if (nullptr == m_chWidth) reaches->GetReachesSingleProperty(REACH_WIDTH, &m_chWidth);
	if (nullptr == m_reachN) reaches->GetReachesSingleProperty(REACH_MANNING, &m_reachN);

	m_reachUpStream = reaches->GetUpStreamIDs();
	m_reachLayers = reaches->GetReachLayers();
}

bool OL_HAND::CheckInputData() {
    /// m_date is protected variable member in base class SimulationModule.
    CHECK_POSITIVE("IO_TEST", m_date);
    CHECK_POSITIVE("IO_TEST", m_nCells);
    CHECK_POINTER("IO_TEST", m_raster1D);
    CHECK_POINTER("IO_TEST", m_raster2D);
    CHECK_POINTER("IO_TEST", m_nSoilLyrs);
    return true;
}

int OL_HAND::Execute() {
    /// Initialize output variables
    if (nullptr == m_output1Draster) Initialize1DArray(m_nCells, m_output1Draster, 0.f);

    if (nullptr == m_output2Draster) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_output2Draster, NODATA_VALUE);


	//check the data
	CheckInputData();

	InitialOutputs();
	//Output1DArray(m_nCells, m_prec, "f:\\p2.txt");
	for (auto it = m_reachLayers.begin(); it != m_reachLayers.end(); it++) {
		// There are not any flow relationship within each routing layer.
		// So parallelization can be done here.
		int nReaches = it->second.size();
		// the size of m_reachLayers (map) is equal to the maximum stream order
#pragma omp parallel for
		for (int i = 0; i < nReaches; ++i) {
			//int reachIndex = it->second[i]; // index in the array
			//vector<int> &vecCells = m_reachs[reachIndex];
			//int n = vecCells.size();
			//for (int iCell = 0; iCell < n; ++iCell) {
			//	ChannelFlow(reachIndex, iCell, vecCells[iCell]);
			//}
			//m_qSubbasin[reachIndex] = m_qCh[reachIndex][n - 1];
		}
	}
	return 0;

    /// Execute function
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        m_output1Draster[i] = m_raster1D[i] * 0.5f;
        for (int j = 0; j < m_nSoilLyrs[i]; j++) {
            m_output2Draster[i][j] = m_raster2D[i][j] + 2.f;
        }
    }
    /// Write Scenario Information
    // m_scenario->Dump("e:\\test\\bmpScenario2.txt");
    int nReaches = m_reaches->GetReachNumber();
    return 0;
}

void OL_HAND::Get1DData(const char* key, int* n, float** data) {
    string sk(key);
    if (StringMatch(sk, "CN2_M")) {
        *data = this->m_output1Draster;
        *n = this->m_nCells;
    }
}

void OL_HAND::Get2DData(const char* key, int* n, int* col, float*** data) {
    string sk(key);
    if (StringMatch(sk, "K_M")) {
        *data = this->m_output2Draster;
        *n = this->m_nCells;
        *col = this->m_maxSoilLyrs;
    }
}
