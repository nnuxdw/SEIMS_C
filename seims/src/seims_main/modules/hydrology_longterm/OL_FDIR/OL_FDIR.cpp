#include "OL_FDIR.h"

#include "text.h"

OL_FDIR::OL_FDIR() :
	m_TimeStep(-1), m_nCells(-1), m_CellWth(NODATA_VALUE), m_cellArea(NODATA_VALUE),
	m_nSubbsns(-1), m_inputSubbsnID(-1), m_subbsnID(nullptr),
	m_iuhCell(nullptr), m_iuhCols(-1), m_surfRf(nullptr),
	m_cellFlow(nullptr), m_cellFlowCols(-1), m_Q_SBOF(nullptr), m_OL_Flow(nullptr),
	//ljj
	m_area(nullptr), total_area(nullptr), m_maxSoilLyrs(-1), m_landCover(nullptr), m_flowOutIdxD8(nullptr),
	m_nRteLyrs(-1), m_rteLyrs(nullptr), m_rchID(nullptr), m_flowInIdxD8(nullptr), m_chWidth(nullptr),
	m_slope(nullptr), m_ks(nullptr), m_flowout_length(nullptr), m_surfRftotal(nullptr),
	m_Qtrans(nullptr)
{
}

OL_FDIR::~OL_FDIR() {
	if (m_Q_SBOF != nullptr) Release1DArray(m_Q_SBOF);
	if (m_cellFlow != nullptr) Release2DArray(m_nCells, m_cellFlow);
	if (m_OL_Flow != nullptr) Release1DArray(m_OL_Flow);
	if (m_surfRftotal != nullptr) Release1DArray(m_surfRftotal);
	if (m_Qtrans != nullptr) Release2DArray(m_nSubbsns + 1, m_Qtrans);
}

bool OL_FDIR::CheckInputData() {
	CHECK_POSITIVE(MID_IUH_OL, m_date);
	CHECK_POSITIVE(MID_IUH_OL, m_nSubbsns);
	CHECK_NONNEGATIVE(MID_IUH_OL, m_inputSubbsnID);
	CHECK_POSITIVE(MID_IUH_OL, m_nCells);
	CHECK_POSITIVE(MID_IUH_OL, m_CellWth);
	CHECK_NONNEGATIVE(MID_IUH_OL, m_TimeStep);
	CHECK_POINTER(MID_IUH_OL, m_subbsnID);
	CHECK_POINTER(MID_IUH_OL, m_iuhCell);
	CHECK_POINTER(MID_IUH_OL, m_surfRf);
	//ljj
	CHECK_POINTER(MID_IUH_OL, m_flowOutIdxD8);
	CHECK_POINTER(MID_IUH_OL, m_area);
	CHECK_POSITIVE(MID_IUH_OL, m_nRteLyrs);
	CHECK_POINTER(MID_IUH_OL, m_rteLyrs);
	CHECK_POINTER(MID_IUH_OL, m_rchID);
	CHECK_POINTER(MID_IUH_OL, m_slope);
	CHECK_POINTER(MID_IUH_OL, m_ks);
	CHECK_POINTER(MID_IUH_OL, m_chWidth);
	return true;
}

void OL_FDIR::InitialOutputs() {
	CHECK_POSITIVE(MID_IUH_OL, m_nSubbsns);
	//if (m_cellArea <= 0.f) m_cellArea = m_CellWth * m_CellWth; //ljj
	if (nullptr == m_Q_SBOF) {
		Initialize1DArray(m_nSubbsns + 1, m_Q_SBOF, 0.f);
	}
	if (nullptr == m_cellFlow) {
		for (int i = 0; i < m_nCells; i++) {
			m_cellFlowCols = Max(CVT_INT(m_iuhCell[i][1]) + 1, m_cellFlowCols);
		}
		//get m_cellFlowCols, i.e. the maximum of second column of OL_IUH plus 1.
		Initialize2DArray(m_nCells, m_cellFlowCols, m_cellFlow, 0.f);
	}
	if (nullptr == m_OL_Flow) {
		Initialize1DArray(m_nCells, m_OL_Flow, 0.f);
	}
	if (nullptr == m_surfRftotal) {
		Initialize1DArray(m_nCells, m_surfRftotal, 0.f);
	}
	int nLen = m_nSubbsns + 1;
	if (nullptr == m_Qtrans)  Initialize2DArray(nLen, 6, m_Qtrans, 0.f);
}

int OL_FDIR::Execute() {
	CheckInputData();
	InitialOutputs();

	for (int n = 0; n <= m_nSubbsns; n++) {
		m_Q_SBOF[n] = 0.f;
	}

	for (int ilyr = 0; ilyr < m_nRteLyrs; ilyr++) {
		// There are not any flow relationship within each routing layer.
		// So parallelization can be done here.
		int ncells = CVT_INT(m_rteLyrs[ilyr][0]);
		for (int icell = 1; icell <= ncells; icell++) {
			int id = CVT_INT(m_rteLyrs[ilyr][icell]);
			int nUpstream = CVT_INT(m_flowInIdxD8[id][0]);
			//if this field is wetland, intercept the upstream water
			// 将当前HRU和其所有上游HRU的地表水流量加起来，xiaodw
			m_surfRftotal[id] = m_surfRf[id] * 0.001f * m_area[id] / m_TimeStep;  //m3/s
			for (int upIndex = 1; upIndex <= nUpstream; upIndex++) {
				// IMPORTANT!!! If the upstream cell is from another subbasin, CONTINUE to next upstream cell. By lj.
				int flowInID = CVT_INT(m_flowInIdxD8[id][upIndex]);
				if (CVT_INT(m_subbsnID[flowInID]) != CVT_INT(m_subbsnID[id])) { continue; }
				float init_surfRf = m_surfRf[id];
				float qUp = 0.f;

				if (m_surfRf[flowInID] > 0.f) {
					qUp = m_surfRftotal[flowInID];
				}
				if (qUp <= 0.f) qUp = 0.f;

				m_surfRftotal[id] += qUp;
				m_surfRftotal[id] = Max(m_surfRftotal[id], 0.f);
			}
		}
	}
# ifdef USE_OPENMP
#pragma omp parallel
#endif // USE_OPENMP
	{
		float* tmp_qsSub = new float[m_nSubbsns + 1];
		for (int i = 0; i <= m_nSubbsns; i++) {
			tmp_qsSub[i] = 0.f;
		}
		// 子流域内所有HRU的地表水都汇入河道
# ifdef USE_OPENMP
#pragma omp for
#endif // USE_OPENMP
		for (int i = 0; i < m_nCells; i++) {
			m_OL_Flow[i] = m_surfRf[i]; //mm
			if (m_rchID[i] <= 0.f) continue;
			tmp_qsSub[CVT_INT(m_rchID[i])] += m_surfRftotal[i]; // m3/s
		}
		// 每个子流域的原本的河道流量加上新汇入的流量
# ifdef USE_OPENMP
#pragma omp critical
#endif // USE_OPENMP
		{
			for (int i = 1; i <= m_nSubbsns; i++) {
				m_Q_SBOF[i] += tmp_qsSub[i];
			}
		}
		delete[] tmp_qsSub;
		tmp_qsSub = nullptr;
	} /* END of #pragma omp parallel */

	// 所有子流域的地表流量之和
	for (int n = 1; n <= m_nSubbsns; n++) {
		//get overland flow routing for entire watershed.
		m_Q_SBOF[0] += m_Q_SBOF[n];
	}
	return 0;
}

void OL_FDIR::SetValue(const char* key, const float value) {
	string sk(key);
	if (StringMatch(sk, Tag_TimeStep)) m_TimeStep = CVT_INT(value);
	else if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);
	else if (StringMatch(sk, Tag_CellWidth)) m_CellWth = value;
	else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
	else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
	else {
		throw ModelException(MID_IUH_OL, "SetValue", "Parameter " + sk + " does not exist.");
	}
}

void OL_FDIR::Set1DData(const char* key, const int n, float* data) {
	CheckInputSize(MID_IUH_OL, key, n, m_nCells);
	string sk(key);
	if (StringMatch(sk, VAR_SUBBSN)) m_subbsnID = data;
	else if (StringMatch(sk, VAR_SURU)) m_surfRf = data;
	else if (StringMatch(sk, "CELLAREA")) m_area = data;
	else if (StringMatch(sk, VAR_STREAM_LINK)) m_rchID = data;
	else if (StringMatch(sk, VAR_SLOPE)) {
		m_slope = data;
	}
	else if (StringMatch(sk, VAR_CHWIDTH)) {
		m_chWidth = data;
	}
	else if (StringMatch(sk, Tag_FLOWOUT_INDEX_D8)) {
		m_flowOutIdxD8 = data;
	}
	else {
		throw ModelException(MID_IUH_OL, "Set1DData", "Parameter " + sk + " does not exist.");
	}
}

void OL_FDIR::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
	string sk(key);
	if (StringMatch(sk, VAR_OL_IUH)) {
		CheckInputSize2D(MID_IUH_OL, VAR_OL_IUH, nrows, ncols, m_nCells, m_iuhCols);
		m_iuhCell = data;
		m_iuhCols = ncols;
	}
	else if (StringMatch(sk, Tag_ROUTING_LAYERS)) {
		CheckInputSize(MID_IUH_OL, key, nrows, m_nRteLyrs);
		m_rteLyrs = data;
	}
	else if (StringMatch(sk, Tag_FLOWIN_INDEX_D8)) {
		CheckInputSize(MID_IUH_OL, key, nrows, m_nCells);
		m_flowInIdxD8 = data;
	}
	else if (StringMatch(sk, VAR_CONDUCT)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_ks = data;
	}
	else {
		throw ModelException(MID_IUH_OL, "Set2DData", "Parameter " + sk + " does not exist.");
	}
}

void OL_FDIR::GetValue(const char* key, float* value) {
	InitialOutputs();
	string sk(key);

	if (StringMatch(sk, VAR_SBOF) && m_inputSubbsnID > 0) {
		/// For MPI version to transfer data across subbasins
		*value = m_Q_SBOF[m_inputSubbsnID];
	}
	else {
		throw ModelException(MID_IUH_OL, "GetValue", "Result " + sk + " does not exist.");
	}
}

void OL_FDIR::Get1DData(const char* key, int* n, float** data) {
	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, "olflow")) {
		*data = m_surfRf;
		*n = m_nCells;
	}
	else if (StringMatch(sk, VAR_SBOF)) {
		*data = m_Q_SBOF;
		*n = m_nSubbsns + 1;
	}
	else if (StringMatch(sk, VAR_OLFLOW)) {
		*data = m_OL_Flow;
		*n = m_nCells;
	}
	else {
		throw ModelException(MID_IUH_OL, "Get1DData", "Result " + sk + " does not exist.");
	}
}

void OL_FDIR::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	InitialOutputs();
	string sk(key);
	*nrows = m_nCells;
	*ncols = m_maxSoilLyrs;
	if (StringMatch(sk, "Qtrans")) {
		*data = m_Qtrans;
		*nrows = m_nSubbsns + 1;
		*ncols = 6;
	}
	else {
		throw ModelException(MID_IUH_OL, "Get2DData", "Output " + sk + " does not exist.");
	}
}
