#include "IUH_OL.h"

#include "text.h"

IUH_OL::IUH_OL() :
    m_TimeStep(-1), m_nCells(-1), m_CellWth(NODATA_VALUE), m_cellArea(NODATA_VALUE),
    m_nSubbsns(-1), m_inputSubbsnID(-1), m_subbsnID(nullptr),
    m_iuhCell(nullptr), m_iuhCols(-1), m_surfRf(nullptr),
    m_cellFlow(nullptr), m_cellFlowCols(-1), m_Q_SBOF(nullptr), m_OL_Flow(nullptr),
    //ljj
	m_nreach(-1),m_area(nullptr),m_brt(nullptr),m_chLen(nullptr),m_chMan(nullptr),m_chSlope(nullptr),
    m_slope(nullptr),m_ManningN(nullptr),m_surlag(NODATA_VALUE),m_dis2Stream(nullptr),tconc(nullptr),
    m_surlag_1d(nullptr) {
}

IUH_OL::~IUH_OL() {
    if (m_Q_SBOF != nullptr) Release1DArray(m_Q_SBOF);
    if (m_cellFlow != nullptr) Release2DArray(m_nCells, m_cellFlow);
    if (m_OL_Flow != nullptr) Release1DArray(m_OL_Flow);
    if (m_brt != nullptr) Release1DArray(m_brt);
    if (tconc != nullptr) Release1DArray(tconc);
}

bool IUH_OL::CheckInputData() {
    CHECK_POSITIVE(MID_IUH_OL, m_date);
    CHECK_POSITIVE(MID_IUH_OL, m_nSubbsns);
    CHECK_NONNEGATIVE(MID_IUH_OL, m_inputSubbsnID);
    CHECK_POSITIVE(MID_IUH_OL, m_nCells);
    CHECK_POSITIVE(MID_IUH_OL, m_CellWth);
    CHECK_NONNEGATIVE(MID_IUH_OL, m_TimeStep);
    CHECK_POINTER(MID_IUH_OL, m_subbsnID);
    CHECK_POINTER(MID_IUH_OL, m_iuhCell);
    CHECK_POINTER(MID_IUH_OL, m_surfRf);
    return true;
}
void IUH_OL::SetReaches(clsReaches* reaches) {
    if (nullptr == reaches) {
        throw ModelException(MID_IUH_OL, "SetReaches", "The reaches input can not to be NULL.");
    }
    m_nreach = reaches->GetReachNumber();
    if (nullptr == m_chLen) reaches->GetReachesSingleProperty(REACH_LENGTH, &m_chLen);
    if (nullptr == m_chMan) reaches->GetReachesSingleProperty(REACH_MANNING, &m_chMan);
    if (nullptr == m_chSlope) reaches->GetReachesSingleProperty(REACH_SLOPE, &m_chSlope);
}
void IUH_OL::InitialOutputs() {
    CHECK_POSITIVE(MID_IUH_OL, m_nSubbsns);

    //if (m_cellArea <= 0.f) m_cellArea = m_CellWth * m_CellWth;
    if (nullptr == m_Q_SBOF) {
        Initialize1DArray(m_nSubbsns + 1, m_Q_SBOF, 0.f);
        for (int i = 0; i < m_nCells; i++) {
            m_cellFlowCols = Max(CVT_INT(m_iuhCell[i][1]) + 1, m_cellFlowCols);
        }
        //get m_cellFlowCols, i.e. the maximum of second column of OL_IUH plus 1.
        Initialize2DArray(m_nCells, m_cellFlowCols, m_cellFlow, 0.f);
    }
    if (nullptr == m_OL_Flow) {
        Initialize1DArray(m_nCells, m_OL_Flow, 0.f);
    }
    if (nullptr == m_brt) {
        Initialize1DArray(m_nCells, m_brt, 0.f);
        Initialize1DArray(m_nCells, tconc, 0.f);
        //brt(j) = 1. - Exp(-surlag(j) / tconc(j))
        //!!    compute time of concentration (sum of overland and channel times)
    //     t_ch = 0
    //     ch_l1(j) = ch_l1(j) * hru_dafr(j) / sub_fr(hru_sub(j))
    //     t_ov(j) = .0556 * (slsubbsn(j)*ov_n(j)) ** .6 / hru_slp(j) ** .3
    //     t_ch = .62 * ch_l1(j) * ch_n(1,hru_sub(j)) ** .75 /             
    //  &              ((da_km*hru_dafr(j))**.125*ch_s(1,hru_sub(j))**.375)
    //     tconc(j) = t_ov(j) + t_ch
        float* tmp_Sub_area = new float[m_nSubbsns + 1];
        float* tmp_Sub_slp = new float[m_nSubbsns + 1];
        float da_km = 0.f;
        for (int i = 0; i <= m_nSubbsns; i++) {
            tmp_Sub_area[i] = 0.f;
            tmp_Sub_slp[i] = 0.f;
        }
        for (int i = 0; i < m_nCells; i++) {
            tmp_Sub_area[CVT_INT(m_subbsnID[i])] += m_area[i]*1.e-6f;
            da_km += m_area[i]*1.e-6f;
            tmp_Sub_slp[CVT_INT(m_subbsnID[i])] += m_slope[i]*m_area[i]*1.e-6f; 
        }
            //def getSlsubbsn(meanSlope) :
            //    """Estimate the average slope length in metres from the mean slope."""
            //    if meanSlope < 0.01 : return 120
            //        elif meanSlope < 0.02 : return 100
            //        elif meanSlope < 0.03 : return 90
            //        elif meanSlope < 0.05 : return 60
            //    else: return 30
        for (int i = 0; i < m_nCells; i++) {
            float t_ch = 0;
            float hru_dafr = m_area[i]*1.e-6f/da_km;
            float sub_fr = tmp_Sub_area[CVT_INT(m_subbsnID[i])] / da_km;
            float slsubbsn = 30.f;
            if(tmp_Sub_slp[CVT_INT(m_subbsnID[i])]/tmp_Sub_area[CVT_INT(m_subbsnID[i])]< 0.01f) slsubbsn=120;
            if(tmp_Sub_slp[CVT_INT(m_subbsnID[i])]/tmp_Sub_area[CVT_INT(m_subbsnID[i])]< 0.02f) slsubbsn=100;
            if(tmp_Sub_slp[CVT_INT(m_subbsnID[i])]/tmp_Sub_area[CVT_INT(m_subbsnID[i])]< 0.03f) slsubbsn=90;
            if(tmp_Sub_slp[CVT_INT(m_subbsnID[i])]/tmp_Sub_area[CVT_INT(m_subbsnID[i])]< 0.05f) slsubbsn=60;
            slsubbsn = Min(m_dis2Stream[i],300.f);
            slsubbsn = Max(m_dis2Stream[i],1.f);
            //ljj++ consitent with SERO
            if(m_slope[i] <= 0.1)   slsubbsn = 61; 
            if(m_slope[i] <= 0.2 && m_slope[i] > 0.1)   slsubbsn = 24; 
            if(m_slope[i] > 0.2)   slsubbsn = 9.1; 
            float t_ov = 0.0556f * pow((slsubbsn)*m_ManningN[i], 0.6f) / pow(m_slope[i], 0.3f);
            
            float ch_l1 = m_chLen[CVT_INT(m_subbsnID[i])] *0.001f *hru_dafr / sub_fr;
            t_ch = 0.62 * ch_l1 * pow(m_chMan[CVT_INT(m_subbsnID[i])], 0.75f )/ (pow(da_km*hru_dafr,0.125f)* pow(m_chSlope[CVT_INT(m_subbsnID[i])],0.375f));
            tconc[i] = t_ov +  t_ch;
            //m_brt[i] = 1. - exp(-1* m_surlag / tconc[i]);
            m_brt[i] = 1. - exp(-1* m_surlag_1d[i] / tconc[i]);
            m_brt[i] = Min(m_brt[i],1.f);
        }
        delete[] tmp_Sub_area;
        delete[] tmp_Sub_slp;
        tmp_Sub_area = nullptr;
        tmp_Sub_slp = nullptr;
    }
}

int IUH_OL::Execute() {
    CheckInputData();
    InitialOutputs();
    // delete value of last time step
    for (int n = 0; n <= m_nSubbsns; n++) {
        m_Q_SBOF[n] = 0.f;
    }
#pragma omp parallel for
    // for (int i = 0; i < m_nCells; i++) {
    //     //forward one time step
    //     for (int j = 0; j < m_cellFlowCols - 1; j++) {
    //         m_cellFlow[i][j] = m_cellFlow[i][j + 1];
    //     }
    //     m_cellFlow[i][m_cellFlowCols - 1] = 0.f;

    //     if (m_surfRf[i] <= 0.f) continue;

    //     int min = CVT_INT(m_iuhCell[i][0]);
    //     int max = CVT_INT(m_iuhCell[i][1]);
    //     int col = 2;
    //     for (int k = min; k <= max; k++) {
	// 		//m_cellFlow[i][k] += m_surfRf[i] * 0.001f * m_iuhCell[i][col] * m_cellArea / m_TimeStep;
    //         m_cellFlow[i][k] += m_surfRf[i] * 0.001f * m_iuhCell[i][col] * m_area[i] / m_TimeStep;
    //         col++;
    //     }
    // }
    //SURLAG method ljj++
    for (int i = 0; i < m_nCells; i++) {
        m_cellFlow[i][0] =0.f;
        if (m_surfRf[i] <= 0.f) continue;

        m_cellFlow[i][1] += m_surfRf[i];
        m_cellFlow[i][0] = m_cellFlow[i][1] * m_brt[i];  //this time step
        m_cellFlow[i][1] -= m_cellFlow[i][0];
        m_cellFlow[i][1] = Max(m_cellFlow[i][1],0.f);
    }
    // See https://github.com/lreis2415/SEIMS/issues/36 for more descriptions. By lj
#pragma omp parallel
    {
        float* tmp_qsSub = new float[m_nSubbsns + 1];
        for (int i = 0; i <= m_nSubbsns; i++) {
            tmp_qsSub[i] = 0.f;
        }
#pragma omp for
        for (int i = 0; i < m_nCells; i++) {
            tmp_qsSub[CVT_INT(m_subbsnID[i])] += m_cellFlow[i][0]* 0.001f * m_area[i] / m_TimeStep; //get new value
            m_OL_Flow[i] = m_cellFlow[i][0];
			//m_OL_Flow[i] = m_OL_Flow[i] * m_TimeStep * 1000.f / m_cellArea; // m3/s -> mm
            //m_OL_Flow[i] = m_OL_Flow[i] * m_TimeStep * 1000.f / m_area[i]; // m3/s -> mm
        }
#pragma omp critical
        {
            for (int i = 1; i <= m_nSubbsns; i++) {
                m_Q_SBOF[i] += tmp_qsSub[i];

            }
        }
		
#ifdef DEBUG_IUH_OL
		for (int i = 0; i < m_nCells; i++) {
			int sbid = CVT_INT(m_subbsnID[i]);
			cout << "[Cell " << i
				<< "] Sub=" << sbid
				<< ", surfRf=" << m_surfRf[i]
				<< ", brt=" << m_brt[i]
				<< ", m_surlag_1d=" << m_surlag_1d[i]
				<< ", flow_now=" << m_cellFlow[i][0]
				<< ", flow_sto=" << m_cellFlow[i][1]
				<< ", area=" << m_area[i]
				<< ", Q_SBOF[" << sbid << "]=" << m_Q_SBOF[sbid]
				<< endl;
		}
#endif
        delete[] tmp_qsSub;
        tmp_qsSub = nullptr;
    } /* END of #pragma omp parallel */

    for (int n = 1; n <= m_nSubbsns; n++) {
        //get overland flow routing for entire watershed.
        m_Q_SBOF[0] += m_Q_SBOF[n];
    }

    return 0;
}

void IUH_OL::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, Tag_TimeStep)) m_TimeStep = CVT_INT(value);
    else if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);
    else if (StringMatch(sk, Tag_CellWidth)) m_CellWth = value;
    else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
    else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
    else if (StringMatch(sk, "SURLAG")) m_surlag = value;
    else {
        throw ModelException(MID_IUH_OL, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void IUH_OL::Set1DData(const char* key, const int n, float* data) {
    CheckInputSize(MID_IUH_OL, key, n, m_nCells);
    string sk(key);
    if (StringMatch(sk, VAR_SUBBSN)) m_subbsnID = data;
    else if (StringMatch(sk, VAR_SURU)) m_surfRf = data;
    else if (StringMatch(sk, VAR_AHRU)) m_area = data;
    else if (StringMatch(sk, VAR_SLOPE)) m_slope = data;
    else if (StringMatch(sk, VAR_MANNING)) m_ManningN = data; 
    else if (StringMatch(sk, VAR_DISTSTREAM)) m_dis2Stream = data; 
    else if (StringMatch(sk, "SURLAG_1d")) m_surlag_1d = data; 
    else {
        throw ModelException(MID_IUH_OL, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void IUH_OL::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    string sk(key);
    if (StringMatch(sk, VAR_OL_IUH)) {
        CheckInputSize2D(MID_IUH_OL, VAR_OL_IUH, nrows, ncols, m_nCells, m_iuhCols);
        m_iuhCell = data;
        m_iuhCols = ncols;
    } else {
        throw ModelException(MID_IUH_OL, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void IUH_OL::GetValue(const char* key, float* value) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_SBOF) && m_inputSubbsnID > 0) {
        /// For MPI version to transfer data across subbasins
        *value = m_Q_SBOF[m_inputSubbsnID];
    } else {
        throw ModelException(MID_IUH_OL, "GetValue", "Result " + sk + " does not exist.");
    }
}

void IUH_OL::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_SBOF)) {
        *data = m_Q_SBOF;
        *n = m_nSubbsns + 1;
    } else if (StringMatch(sk, VAR_OLFLOW)) {
        *data = m_OL_Flow;
        *n = m_nCells;
    } else if (StringMatch(sk, "BRT")) {
        *data = m_brt;
        *n = m_nCells;
    }else if (StringMatch(sk, "tconc")) {
        *data = tconc;
        *n = m_nCells;
    }else {
        throw ModelException(MID_IUH_OL, "Get1DData", "Result " + sk + " does not exist.");
    }
}
