#include "DepressionLinsley.h"

#include "text.h"

DepressionFSDaily::DepressionFSDaily() :
    m_nCells(-1), m_impoundTriger(nullptr), m_outletID(-1), m_nreach(-1),
    m_potVol(nullptr),
    m_depCo(NODATA_VALUE), m_depCap(nullptr), m_pet(nullptr),
    m_ei(nullptr), m_pe(nullptr), m_sd(nullptr),
    m_ed(nullptr), m_sr(nullptr), m_handWtrDep(nullptr), m_subbsnID(nullptr), m_chSto(nullptr), m_handArea(nullptr), m_hand_eavp(nullptr){
}

DepressionFSDaily::~DepressionFSDaily() {
    if (m_sd != nullptr) Release1DArray(m_sd);
    if (m_ed != nullptr) Release1DArray(m_ed);
    if (m_sr != nullptr) Release1DArray(m_sr);
}

bool DepressionFSDaily::CheckInputData() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_date);
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
    CHECK_NODATA(MID_DEP_LINSLEY, m_depCo);
    CHECK_POINTER(MID_DEP_LINSLEY, m_depCap);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pet);
    CHECK_POINTER(MID_DEP_LINSLEY, m_ei);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pe);
    return true;
}

void DepressionFSDaily::InitialOutputs() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
    if (nullptr == m_sd) {
        Initialize1DArray(m_nCells, m_sd, 0.f);
        Initialize1DArray(m_nCells, m_ed, 0.f);
        Initialize1DArray(m_nCells, m_sr, 0.f);
		if (m_handWtrDep == nullptr)
		{
			Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
		}
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            m_sd[i] = m_depCo * m_depCap[i];
        }
    }
}

void DepressionFSDaily::SetReaches(clsReaches* reaches) {
	if (nullptr == reaches) {
		throw ModelException(MID_MUSK_CH, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_nreach = reaches->GetReachNumber();
}

int DepressionFSDaily::Execute() {
    CheckInputData();
    InitialOutputs();
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {

        //////////////////////////////////////////////////////////////////////////
        // runoff
		float depDeficit = m_depCap[i] - m_sd[i];
		if (m_depCap[i] < 0.001f) {
			m_sr[i] = m_pe[i];
			m_sd[i] = 0.f;
		}
		// xiaodw, if depDeficit is zero, there will be a false number, thus depDeficit should be larger that 0.001f
		else if (depDeficit < 0.001f) {
			m_sd[i] = m_depCap[i];
			m_sr[i] = m_pe[i];
		}
		else if (m_pe[i] > 0.f ) {
			float pc = m_pe[i] - m_depCap[i] * log(1.f - m_sd[i] / m_depCap[i]);
			float deltaSd = m_pe[i] * exp(-pc / m_depCap[i]);
			if (deltaSd > m_depCap[i] - m_sd[i]) {
				deltaSd = m_depCap[i] - m_sd[i];
			}
			m_sd[i] += deltaSd;
			m_sr[i] = m_pe[i] - deltaSd;
		}
		else {
			m_sd[i] += m_pe[i];
			m_sr[i] = 0.f;
		}

        //////////////////////////////////////////////////////////////////////////
        // evaporation
		if (m_sd[i] > 0) {
			/// TODO: Is this logically right? PET is just potential, which include
			///       not only ET from surface water, but also from plant and soil.
			///       Please Check the corresponding theory. By LJ.
			// evaporation from depression storage
			if (MAX(m_pet[i] - m_ei[i] - m_hand_eavp[i],0.f)< m_sd[i]) {
				m_ed[i] = m_pet[i] - m_ei[i] - m_hand_eavp[i];
			}
			else {
				m_ed[i] = m_sd[i];
			}
			m_sd[i] -= m_ed[i];
		}
		else {
			m_ed[i] = 0.f;
			m_sd[i] = 0.f;
		}
        if (m_impoundTriger != nullptr && FloatEqual(m_impoundTriger[i], 0.f)) {
            if (m_potVol != nullptr) {
                m_potVol[i] += m_sr[i];
                m_potVol[i] += m_sd[i];
                m_sr[i] = 0.f;
                m_sd[i] = 0.f;
            }
        }
    }
    return true;
}

void DepressionFSDaily::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, VAR_DEPREIN)) m_depCo = value;
	else if (StringMatch(sk, VAR_OUTLETID)) m_outletID = CVT_INT(value);
    else {
        throw ModelException(MID_DEP_LINSLEY, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void DepressionFSDaily::Set1DData(const char* key, const int n, float* data) {

    string sk(key);
    if (StringMatch(sk, VAR_DEPRESSION)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_depCap = data;
    } else if (StringMatch(sk, VAR_INET)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_ei = data;
    } else if (StringMatch(sk, VAR_PET)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_pet = data;
    } else if (StringMatch(sk, VAR_EXCP)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_pe = data;
    } else if (StringMatch(sk, VAR_IMPOUND_TRIG)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_impoundTriger = data;
    } else if (StringMatch(sk, VAR_POT_VOL)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
        m_potVol = data;
    }else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_handWtrDep = data;
	} else if (StringMatch(sk, VAR_SUBBSN)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_subbsnID = data;
	} else if (StringMatch(sk, VAR_CHST)) {
		CheckInputSize(MID_SUR_MR, key, n - 1, m_nreach);
		m_chSto = data;
	} else if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_handArea = data;
	}else if (StringMatch(sk, VAR_DPST)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_sd = data;
	}else if (StringMatch(sk, VAR_HAND_EVAP)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_hand_eavp = data;
	}
	else {
        throw ModelException(MID_DEP_LINSLEY, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void DepressionFSDaily::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
    *n = m_nCells;
    if (StringMatch(sk, VAR_DPST)) {
        *data = m_sd;
    } else if (StringMatch(sk, VAR_DEET)) {
        *data = m_ed;
    } else if (StringMatch(sk, VAR_SURU)) {
        *data = m_sr;
    } else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		*data = m_handWtrDep;
	}else if (StringMatch(sk, VAR_CHST)) {
		m_chSto[0] = m_chSto[m_outletID];
		*data = m_chSto;
	}
	else {
        throw ModelException(MID_DEP_LINSLEY, "Get1DData", "Output " + sk + " does not exist.");
    }
}
