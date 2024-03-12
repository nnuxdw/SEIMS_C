#include "SSR_DA.h"

#include "text.h"

SSR_DA::SSR_DA() :
	m_inputSubbsnID(-1), m_nCells(-1), m_CellWth(-1.f), m_maxSoilLyrs(-1), m_nSoilLyrs(nullptr),
	m_soilThk(nullptr),
	m_dt(-1), m_ki(NODATA_VALUE),
	m_soilFrozenTemp(NODATA_VALUE), m_slope(nullptr), m_ks(nullptr), m_soilSat(nullptr),
	m_poreIdx(nullptr),
	m_soilFC(nullptr), m_soilWP(nullptr),
	m_soilWtrSto(nullptr), m_soilWtrStoPrfl(nullptr), m_soilTemp(nullptr), m_chWidth(nullptr),
	m_rchID(nullptr), m_flowInIdxD8(nullptr), m_rteLyrs(nullptr),
	m_nRteLyrs(-1), m_nSubbsns(-1), m_subbsnID(nullptr),
	/// outputs
	m_subSurfRf(nullptr), m_subSurfRfVol(nullptr), m_ifluQ2Rch(nullptr),
	//ljj++
	m_area(nullptr), m_flowout_length(nullptr), m_soilTempprofile(nullptr),
	m_soilIceSto(nullptr), m_clay(nullptr), m_soilPor(nullptr)
{
}

SSR_DA::~SSR_DA() {
	if (m_subSurfRf != nullptr) Release2DArray(m_nCells, m_subSurfRf);
	if (m_subSurfRfVol != nullptr) Release2DArray(m_nCells, m_subSurfRfVol);
	if (m_ifluQ2Rch != nullptr) Release1DArray(m_ifluQ2Rch);
}

bool SSR_DA::FlowInSoil(const int id) {
	int frez = 1;
	float s0 = Max(m_slope[id], 0.01f);
	// float flowWidth = m_CellWth;
	float flowWidth = m_flowout_length[id]; //ljj
	// there is no land in this cell
	//TODO ljj:23-10-13:对于河道的地块，流路宽度应该等于河道长度
	// if (m_rchID[id] > 0) {
	//     flowWidth -= m_chWidth[id];
	// }
	// initialize for current cell of current timestep
	//  j 代表层，xiaodw
	for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
		m_subSurfRf[id][j] = 0.f;
		m_subSurfRfVol[id][j] = 0.f;
	}
	/* Previous code. Update: In my view, if the flowWidth is less than 0, the subsurface flow
	 * from the upstream cells should be added to stream cell directly, which will be summarized
	 * for channel flow routing. By lj, 2018-4-12
	// return with initial values if flowWidth is less than 0
	if (flowWidth <= 0) return true;
	*/
	// number of flow-in cells
	int nUpstream = CVT_INT(m_flowInIdxD8[id][0]);
	m_soilWtrStoPrfl[id] = 0.f; // update soil storage on profile
	for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
		float smOld = m_soilWtrSto[id][j];
		//sum the upstream subsurface flow
		float qUp = 0.f;    // mm
		float qUpVol = 0.f; // m^3
		// If no in cells flowin (i.e., nUpstream = 0), the for-loop will be ignored.
		// 遍历地块的上游地块，xiaodw
		for (int upIndex = 1; upIndex <= nUpstream; upIndex++) {
			// 上游地块单元的ID，xiaodw
			int flowInID = CVT_INT(m_flowInIdxD8[id][upIndex]);
			// IMPORTANT!!! If the upstream cell is from another subbasin, CONTINUE to next upstream cell. By lj.
			if (CVT_INT(m_subbsnID[flowInID]) != CVT_INT(m_subbsnID[id])) { continue; }
			// 如果上游地块单元的第j层有地下水流，计算所有上游该层的地下水径流量，xiaodw
			if (m_subSurfRf[flowInID][j] > 0.f) {
				qUp += m_subSurfRf[flowInID][j]; // * m_flowInPercentage[id][upIndex]; // TODO: Consider MFD algorithms
				qUpVol += m_subSurfRfVol[flowInID][j];
			}
		}
		// add upstream water to the current cell
		if (qUp <= 0.f || qUpVol <= 0.f) {
			qUp = 0.f;
			qUpVol = 0.f;
		}
		// if the flowWidth is less than 0, the subsurface flow from the upstream cells
		// should be added to stream cell directly, which will be summarized
		// for channel flow routing. By lj, 2018-4-12
		if (flowWidth <= 0.f) {
			m_subSurfRf[id][j] = qUp;
			m_subSurfRfVol[id][j] = qUpVol;
			continue;
		}
		if (m_soilWtrSto[id][j] != m_soilWtrSto[id][j] || m_soilWtrSto[id][j] < 0.f) {
			cout << "cell id: " << id << ", layer: " << j << ", moisture is less than zero: "
				<< m_soilWtrSto[id][j] << ", previous: " << smOld << ", qUp: " << qUp << ", depth:"
				<< m_soilThk[id][j] << endl;
			return false;
		}
		// 上游地块的地下水流入当前地块, xiaodw
		m_soilWtrSto[id][j] += qUp; // mm

		// if soil moisture is below the field capacity, no interflow will be generated
		if (m_soilWtrSto[id][j] <= m_soilFC[id][j]) continue;
		// Otherwise, calculate interflow:
		// for the upper two layers, soil may be frozen
		// also check if there are upstream inflow
		// if (j == 0 && m_soilTemp[id] <= m_soilFrozenTemp && qUp <= 0.f) {
		//     continue;
		// }
		if (frez = 0 && m_soilTempprofile[id][j] <= m_soilFrozenTemp && qUp <= 0.f) {
			continue;
		}

		float k = 0.f, maxSoilWater = 0.f, soilWater = 0.f, fcSoilWater = 0.f;
		soilWater = m_soilWtrSto[id][j];
		maxSoilWater = m_soilSat[id][j];

		//ljj++
		//  土层冻结，导致其最大储水能力减小，xiaodw
		if (frez == 1) maxSoilWater -= m_soilIceSto[id][j];//ljj++ frozen
		maxSoilWater = Max(0.f, maxSoilWater);
		// (土壤水储量+土壤冰储量) / (土壤孔隙度*土壤厚度), xiaodw
		float FACTR = Max(0.01, (m_soilWtrSto[id][j] + m_soilIceSto[id][j]) / (m_soilPor[id][j] * m_soilThk[id][j]));
		FACTR = Min(FACTR, 1.f);
		float BEXP = 2.91 + 0.159*m_clay[id][j];
		float EXPON = 2.0*BEXP + 3.0;
		//WCND  = myDKSAT * FACTR ** EXPON
		float alpha = 3;
		float f_frozen = exp(-alpha * (1 - Min(m_soilIceSto[id][j] / (m_soilPor[id][j] * m_soilThk[id][j]), 1)));
		f_frozen = Max(0.f, f_frozen);
		f_frozen = f_frozen - exp(-alpha);
		float WCND = (1 - f_frozen)*m_ks[id][j] * Min(1.0, pow(FACTR, EXPON));
		//if(id==11) cout<<m_soilIceSto[id][j]/(m_soilPor[id][j]*m_soilThk[id][j])<<"      "<<FACTR<<"      "<<f_frozen<<"     "<<WCND<<"     "<<m_ks[id][j]<<endl;
		WCND = Min(WCND, m_ks[id][j]);
		//

		fcSoilWater = m_soilFC[id][j];
		//the moisture content can exceed the porosity in the way the algorithm is implemented
		if (m_soilWtrSto[id][j] > maxSoilWater) {
			k = m_ks[id][j];
			if (frez == 1) k = WCND;  //ljj
		}
		else {
			/// Using Clapp and Hornberger (1978) equation to calculate unsaturated hydraulic conductivity.
			float dcIndex = 2.f * m_poreIdx[id][j] + 3.f; // pore disconnectedness index
			k = m_ks[id][j] * pow(m_soilWtrSto[id][j] / maxSoilWater, dcIndex);
			if (frez == 1) k = WCND;  //ljj
			if (k <= UTIL_ZERO) k = 0.f;
			//cout << id << "\t" << j << "\t" << k << endl;
		}
		// 1. / 3600. = 0.0002777777777777778
		m_subSurfRf[id][j] = m_ki * s0 * k * m_dt * 0.0002777777777777778f * m_soilThk[id][j] * 0.001f / flowWidth;
		// the unit is mm
		// 如果地下水储量 - 地下水径流量后，依然超出土壤孔隙度（土壤最大储水量），则地下水径流量=土壤水储量-最大储水量，xiaodw
		if (soilWater - m_subSurfRf[id][j] > maxSoilWater) {
			m_subSurfRf[id][j] = soilWater - maxSoilWater;
		}
		else if (soilWater - m_subSurfRf[id][j] < fcSoilWater) {
			// 如果 减去后，都不够植物根系吸收需要的水量，则地下水径流量=土壤水储量-根系吸收量，xiaodw
			m_subSurfRf[id][j] = soilWater - fcSoilWater;
		}
		m_subSurfRf[id][j] = Max(0.f, m_subSurfRf[id][j]);

		// m_subSurfRfVol[id][j] = m_subSurfRf[id][j] * 0.001f * m_CellWth * flowWidth; //m3
		m_subSurfRfVol[id][j] = m_subSurfRf[id][j] * 0.001f * m_area[id]; //ljj change for field
		m_subSurfRfVol[id][j] = Max(UTIL_ZERO, m_subSurfRfVol[id][j]);
		//Adjust the moisture content in the current layer, and the layer immediately below it
		// 土壤水储量 - 地下水径流量
		m_soilWtrSto[id][j] -= m_subSurfRf[id][j];
		// 土壤层剖面水储量之和 + 当前土壤层的水储量
		m_soilWtrStoPrfl[id] += m_soilWtrSto[id][j];
		if (m_soilWtrSto[id][j] != m_soilWtrSto[id][j] || m_soilWtrSto[id][j] < 0.f) {
			cout << "cell id: " << id << ", layer: " << j << ", moisture is less than zero: "
				<< m_soilWtrSto[id][j] << ", subsurface runoff: " << m_subSurfRf[id][j] << ", depth:"
				<< m_soilThk[id][j] << endl;
			return false;
		}
	}
	return true;
}

int SSR_DA::Execute() {
	CheckInputData();
	InitialOutputs();

	for (int ilyr = 0; ilyr < m_nRteLyrs; ilyr++) {
		// There are not any flow relationship within each routing layer.
		// So parallelization can be done here.
		int ncells = CVT_INT(m_rteLyrs[ilyr][0]);
		// DO NOT THROW EXCEPTION IN OMP FOR LOOP, i.e., FlowInSoil(id) function.
		int errCount = 0;
		// 遍历每个并行层的所有地块，xiaodw
#pragma omp parallel for reduction(+: errCount)
		for (int icell = 1; icell <= ncells; icell++) {
			// 地块id，xiaodw
			int id = CVT_INT(m_rteLyrs[ilyr][icell]);
			if (!FlowInSoil(id)) errCount++;
		}
		if (errCount > 0) {
			throw ModelException(MID_SSR_DA, "Execute:FlowInSoil",
				"Please check the error message for more information");
		}
	}
	for (int i = 0; i <= m_nSubbsns; i++) {
		m_ifluQ2Rch[i] = 0.f;
	}
	/// using openmp for reduction an array should be paid much more attention.
	/// here is a solution. https://stackoverflow.com/questions/20413995/reducing-on-array-in-openmp
	/// #pragma omp parallel for reduction(+:myArray[:6]) is supported with OpenMP 4.5.
	/// However, MSVC 2010-2015 are using OpenMP 2.0.
	/// Added by lj, 2017-8-23
#pragma omp parallel
	{
		float* tmp_qiSubbsn = new(nothrow) float[m_nSubbsns + 1];
		for (int i = 0; i <= m_nSubbsns; i++) {
			tmp_qiSubbsn[i] = 0.f;
		}
#pragma omp for
		for (int i = 0; i < m_nCells; i++) {
			if (m_rchID[i] <= 0.f) continue;
			float qiAllLayers = 0.f;
			// 计算每个地块所有土壤层的土壤水，xiaodw
			for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
				if (m_subSurfRfVol[i][j] > UTIL_ZERO) {
					qiAllLayers += m_subSurfRfVol[i][j] / m_dt; /// m^3/s
				}
			}
			// 一个地块所有土层的土壤水，都会分配到同一个subbasin，xiaodw
			tmp_qiSubbsn[CVT_INT(m_rchID[i])] += qiAllLayers;
		}
#pragma omp critical
		{
			// subbasin中的水被分配到河道，xiaodw
			for (int i = 1; i <= m_nSubbsns; i++) {
				m_ifluQ2Rch[i] += tmp_qiSubbsn[i];
			}
		}
		delete[] tmp_qiSubbsn;
		tmp_qiSubbsn = nullptr;
	} /* END of #pragma omp parallel */

	for (int i = 1; i <= m_nSubbsns; i++) {
		m_ifluQ2Rch[0] += m_ifluQ2Rch[i];
	}
	return 0;
}

void SSR_DA::SetValue(const char* key, const float value) {
	string s(key);
	if (StringMatch(s, VAR_T_SOIL)) {
		m_soilFrozenTemp = value;
	}
	else if (StringMatch(s, VAR_KI)) {
		m_ki = value;
	}
	else if (StringMatch(s, VAR_SUBBSNID_NUM)) {
		m_nSubbsns = CVT_INT(value);
	}
	else if (StringMatch(s, Tag_SubbasinId)) {
		m_inputSubbsnID = CVT_INT(value);
	}
	else if (StringMatch(s, Tag_CellWidth)) {
		m_CellWth = value;
	}
	else if (StringMatch(s, Tag_TimeStep)) {
		m_dt = CVT_INT(value);
	}
	else {
		throw ModelException(MID_SSR_DA, "SetValue", "Parameter " + s + " does not exist.");
	}
}

void SSR_DA::Set1DData(const char* key, const int nrows, float* data) {
	string s(key);
	CheckInputSize(MID_SSR_DA, key, nrows, m_nCells);
	if (StringMatch(s, VAR_SLOPE)) {
		m_slope = data;
	}
	else if (StringMatch(s, VAR_CHWIDTH)) {
		m_chWidth = data;
	}
	else if (StringMatch(s, VAR_STREAM_LINK)) {
		m_rchID = data;
	}
	else if (StringMatch(s, VAR_SOTE)) {
		m_soilTemp = data;
	}
	else if (StringMatch(s, VAR_SUBBSN)) {
		m_subbsnID = data;
	}
	else if (StringMatch(s, VAR_SOILLAYERS)) {
		m_nSoilLyrs = data;
	}
	else if (StringMatch(s, VAR_SOL_SW)) {
		m_soilWtrStoPrfl = data;
	}
	//ljj++
	else if (StringMatch(s, VAR_AHRU)) m_area = data;
	else if (StringMatch(s, VAR_FLOWOUT_LEN)) m_flowout_length = data;

	else {
		throw ModelException(MID_SSR_DA, "Set1DData", "Parameter " + s + " does not exist.");
	}
}

void SSR_DA::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
	string sk(key);
	if (StringMatch(sk, VAR_SOILTHICK)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilThk = data;
	}
	else if (StringMatch(sk, VAR_CONDUCT)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_ks = data;
	}
	else if (StringMatch(sk, VAR_SOL_UL)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilSat = data;
	}
	else if (StringMatch(sk, VAR_SOL_AWC)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilFC = data;
	}
	else if (StringMatch(sk, VAR_SOL_WPMM)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilWP = data;
	}
	else if (StringMatch(sk, VAR_POREIDX)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_poreIdx = data;
	}
	else if (StringMatch(sk, VAR_SOL_ST)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilWtrSto = data;
	}
	else if (StringMatch(sk, Tag_ROUTING_LAYERS)) {
		CheckInputSize(MID_SSR_DA, key, nrows, m_nRteLyrs);
		m_rteLyrs = data;
	}
	else if (StringMatch(sk, Tag_FLOWIN_INDEX_D8)) {
		CheckInputSize(MID_SSR_DA, key, nrows, m_nCells);
		m_flowInIdxD8 = data;
	}
	//ljj++
	else if (StringMatch(sk, VAR_SOILT)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilTempprofile = data;
	}
	else if (StringMatch(sk, VAR_SOLICE)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilIceSto = data;
	}
	else if (StringMatch(sk, "clay")) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_clay = data;
	}
	else if (StringMatch(sk, VAR_POROST)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilPor = data;
	}
	else {
		throw ModelException(MID_SSR_DA, "Set2DData", "Parameter " + sk + " does not exist.");
	}
}

void SSR_DA::Get1DData(const char* key, int* n, float** data) {
	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, VAR_SBIF)) *data = m_ifluQ2Rch;
	else {
		throw ModelException(MID_SSR_DA, "Get1DData", "Result " + sk + " does not exist.");
	}
	*n = m_nSubbsns + 1;
}

void SSR_DA::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	InitialOutputs();
	string sk(key);
	*nrows = m_nCells;
	*ncols = m_maxSoilLyrs;

	if (StringMatch(sk, VAR_SSRU)) {
		*data = m_subSurfRf;
	}
	else if (StringMatch(sk, VAR_SSRUVOL)) {
		*data = m_subSurfRfVol;
	}
	else {
		throw ModelException(MID_SSR_DA, "Get2DData", "Output " + sk + " does not exist.");
	}
}

bool SSR_DA::CheckInputData() {
	CHECK_NONNEGATIVE(MID_SSR_DA, m_inputSubbsnID);
	CHECK_POSITIVE(MID_SSR_DA, m_nCells);
	CHECK_POSITIVE(MID_SSR_DA, m_ki);
	CHECK_NODATA(MID_SSR_DA, m_soilFrozenTemp);
	CHECK_POSITIVE(MID_SSR_DA, m_dt);
	CHECK_POSITIVE(MID_SSR_DA, m_CellWth);
	CHECK_POSITIVE(MID_SSR_DA, m_nSubbsns);
	CHECK_POSITIVE(MID_SSR_DA, m_nRteLyrs);
	CHECK_POINTER(MID_SSR_DA, m_subbsnID);
	CHECK_POINTER(MID_SSR_DA, m_nSoilLyrs);
	CHECK_POINTER(MID_SSR_DA, m_soilThk);
	CHECK_POINTER(MID_SSR_DA, m_slope);
	CHECK_POINTER(MID_SSR_DA, m_poreIdx);
	CHECK_POINTER(MID_SSR_DA, m_ks);
	CHECK_POINTER(MID_SSR_DA, m_soilSat);
	CHECK_POINTER(MID_SSR_DA, m_soilFC);
	CHECK_POINTER(MID_SSR_DA, m_soilWP);
	CHECK_POINTER(MID_SSR_DA, m_soilWtrSto);
	CHECK_POINTER(MID_SSR_DA, m_soilWtrStoPrfl);
	CHECK_POINTER(MID_SSR_DA, m_soilTemp);
	CHECK_POINTER(MID_SSR_DA, m_chWidth);
	CHECK_POINTER(MID_SSR_DA, m_rchID);
	CHECK_POINTER(MID_SSR_DA, m_flowInIdxD8);
	CHECK_POINTER(MID_SSR_DA, m_rteLyrs);
	return true;
}

void SSR_DA::InitialOutputs() {
	CHECK_POSITIVE(MID_SSR_DA, m_nCells);
	CHECK_POSITIVE(MID_SSR_DA, m_nSubbsns);
	if (nullptr == m_ifluQ2Rch) Initialize1DArray(m_nSubbsns + 1, m_ifluQ2Rch, 0.f);
	if (nullptr == m_subSurfRf) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_subSurfRf, 0.f);
	if (nullptr == m_subSurfRfVol) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_subSurfRfVol, 0.f);
}
