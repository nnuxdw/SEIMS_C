#include "DepressionLinsleyHand.h"

#include "text.h"

DepressionLinsleyHand::DepressionLinsleyHand() :
    m_nCells(-1), m_impoundTriger(nullptr), m_outletID(-1), m_nreach(-1),
    m_potVol(nullptr),
    m_depCo(NODATA_VALUE), m_depCap(nullptr), m_pet(nullptr),
    m_ei(nullptr), m_pe(nullptr), m_sd(nullptr),
    m_ed(nullptr), m_sr(nullptr), m_handWtrDep(nullptr), m_subbsnID(nullptr), m_chSto(nullptr), m_handArea(nullptr), m_hand_eavp(nullptr){
}

DepressionLinsleyHand::~DepressionLinsleyHand() {
    if (m_sd != nullptr) Release1DArray(m_sd);
    if (m_ed != nullptr) Release1DArray(m_ed);
    if (m_sr != nullptr) Release1DArray(m_sr);
}

bool DepressionLinsleyHand::CheckInputData() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_date);
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
    CHECK_NODATA(MID_DEP_LINSLEY, m_depCo);
    CHECK_POINTER(MID_DEP_LINSLEY, m_depCap);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pet);
    CHECK_POINTER(MID_DEP_LINSLEY, m_ei);
    CHECK_POINTER(MID_DEP_LINSLEY, m_pe);
    return true;
}

void DepressionLinsleyHand::InitialOutputs() {
    CHECK_POSITIVE(MID_DEP_LINSLEY, m_nCells);
	if (nullptr == m_hand_eavp) {
		Initialize1DArray(m_nCells, m_hand_eavp, 0.f);
	}
	if (nullptr == m_ed) {
		Initialize1DArray(m_nCells, m_ed, 0.f);
	}
	if (nullptr == m_sr) {
		Initialize1DArray(m_nCells, m_sr, 0.f);
	}
	if (m_handWtrDep == nullptr)
	{
		Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
	}
    if (nullptr == m_sd) {
        Initialize1DArray(m_nCells, m_sd, 0.f);
#pragma omp parallel for
        for (int i = 0; i < m_nCells; i++) {
            m_sd[i] = m_depCo * m_depCap[i];
        }
    }
}

void DepressionLinsleyHand::SetReaches(clsReaches* reaches) {
	if (nullptr == reaches) {
		throw ModelException(MID_MUSK_CH, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_nreach = reaches->GetReachNumber();
}

int DepressionLinsleyHand::Execute() {
    CheckInputData();
    InitialOutputs();
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {

        //////////////////////////////////////////////////////////////////////////
		float handWtrDepMM = MAX(m_handWtrDep[i] * 1000.0, 0.0);
		int subbasinId = CVT_INT(m_subbsnID[i]);
		float depStoDeficit =MAX(m_depCap[i] - m_sd[i], 0.0);
        // runoff
		// don't need depression
        if (m_depCap[i] < 0.001f) {
            m_sr[i] = m_pe[i];
            m_sd[i] = 0.f;
		}
		//  inundation depth > depression Deficit
		else if (handWtrDepMM >= depStoDeficit)
		{
			m_chSto[subbasinId] -= m_handArea[i] * depStoDeficit  * 0.001;
			m_sd[i] = m_depCap[i];
			//m_sr[i] = m_pe[i];
			handWtrDepMM -= depStoDeficit;
		}
		else{
			m_sd[i] += handWtrDepMM;
			m_chSto[subbasinId] -= m_handArea[i] * handWtrDepMM  * 0.001;
			handWtrDepMM = 0.0;
		}

        //////////////////////////////////////////////////////////////////////////
        // evaporation
        if (handWtrDepMM > 0) {
			// xiaodw, handWtrDepMM has pirority to evap
			if (m_pet[i] - m_ei[i] < handWtrDepMM)
			{
				m_hand_eavp[i] = m_pet[i] - m_ei[i];
			}  else {
				m_hand_eavp[i] = handWtrDepMM;
            }
			m_chSto[subbasinId] -= m_handArea[i] * m_hand_eavp[i] * 0.001;
        } else {
			m_hand_eavp[i] = 0.f;
        }

    }
    return true;
}

void DepressionLinsleyHand::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, VAR_DEPREIN)) m_depCo = value;
	else if (StringMatch(sk, VAR_OUTLETID)) m_outletID = CVT_INT(value);
    else {
        throw ModelException(MID_DEP_LINSLEY, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void DepressionLinsleyHand::Set1DData(const char* key, const int n, float* data) {
    
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
	} else if (StringMatch(sk, VAR_DPST)) {
		CheckInputSize(MID_DEP_LINSLEY, key, n, m_nCells);
		m_sd = data;
	}
	else {
        throw ModelException(MID_DEP_LINSLEY, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void DepressionLinsleyHand::Get1DData(const char* key, int* n, float** data) {
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
	}else if (StringMatch(sk, VAR_HAND_EVAP)) {
		*data = m_hand_eavp;
	}

	
	else {
        throw ModelException(MID_DEP_LINSLEY, "Get1DData", "Output " + sk + " does not exist.");
    }
}
