#include "Interception_MCS.h"

#include "text.h"
#include "utils_time.h"

clsPI_MCS::clsPI_MCS() :
    m_embnkFr(0.15f), m_pcp2CanalFr(0.5f), m_landUse(nullptr), m_outletID(-1), m_nreach(-1),
    m_intcpStoCapExp(-1.f), m_initIntcpSto(0.f), m_maxIntcpStoCap(nullptr),
    m_minIntcpStoCap(nullptr),
    m_pcp(nullptr), m_pet(nullptr), m_canSto(nullptr),
    m_intcpLoss(nullptr), m_netPcp(nullptr), m_nCells(-1),  m_handWtrDep(nullptr), m_subbsnID(nullptr), m_chSto(nullptr), m_handArea(nullptr) {
#ifndef STORM_MODE
    m_IntcpET = nullptr;
#else
    m_hilldt = -1.f;
    m_slope = nullptr;
#endif
}

clsPI_MCS::~clsPI_MCS() {
    if (m_intcpLoss != nullptr) Release1DArray(m_intcpLoss);
    if (m_canSto != nullptr) Release1DArray(m_canSto);
    if (m_netPcp != nullptr) Release1DArray(m_netPcp);
#ifndef STORM_MODE
    if (m_IntcpET != nullptr) Release1DArray(m_IntcpET);
#endif
}

void clsPI_MCS::Set1DData(const char* key, int n, float* data) {
    string s(key);
	if (StringMatch(s, VAR_PCP)) {
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_pcp = data;
	}
    else if (StringMatch(s, VAR_PET)) {
#ifndef STORM_MODE
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_pet = data;
#endif
	}
	else if (StringMatch(s, VAR_INTERC_MAX)) {
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_maxIntcpStoCap = data;
	}
	else if (StringMatch(s, VAR_INTERC_MIN)) {
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_minIntcpStoCap = data;
	}
	else if (StringMatch(s, VAR_LANDUSE)) {
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_landUse = data;
	}
	else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_handWtrDep = data;
	}
	else if (StringMatch(s, VAR_SUBBSN)) {
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_subbsnID = data;
	}
	else if (StringMatch(s, VAR_CHST)) {
		CheckInputSize(MID_PI_MCS, key, n - 1, m_nreach);
		m_chSto = data;
	}
	else if (StringMatch(s, VAR_AHRU)) {
		CheckInputSize(MID_PI_MCS, key, n, m_nCells);
		m_handArea = data;
	}
    else {
        throw ModelException(MID_PI_MCS, "Set1DData", "Parameter " + s + " does not exist.");
    }
}

void clsPI_MCS::SetValue(const char* key, const float value) {
    string s(key);
    if (StringMatch(s, VAR_PI_B)) m_intcpStoCapExp = value;
    else if (StringMatch(s, VAR_INIT_IS)) m_initIntcpSto = value;
    else if (StringMatch(s, VAR_PCP2CANFR_PR)) m_pcp2CanalFr = value;
    else if (StringMatch(s, VAR_EMBNKFR_PR)) m_embnkFr = value;
	else if (StringMatch(s, VAR_OUTLETID)) m_outletID = CVT_INT(value);
#ifdef STORM_MODE
    else if (StringMatch(s, Tag_HillSlopeTimeStep)) m_hilldt = data;
#endif // STORM_MODE
    else {
        throw ModelException(MID_PI_MCS, "SetValue", "Parameter " + s + " does not exist.");
    }
}

void clsPI_MCS::Get1DData(const char* key, int* nRows, float** data) {
    InitialOutputs();
    string s = key;
    if (StringMatch(s, VAR_INLO)) {
        *data = m_intcpLoss;
    } else if (StringMatch(s, VAR_INET)) {
#ifndef STORM_MODE
        *data = m_IntcpET;
#endif
    } else if (StringMatch(s, VAR_CANSTOR)) {
        *data = m_canSto;
    } else if (StringMatch(s, VAR_NEPR)) {
        *data = m_netPcp;
    } else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
		*data = m_handWtrDep;
	}
	else if (StringMatch(s, VAR_CHST)) {
		m_chSto[0] = m_chSto[m_outletID];
		*data = m_chSto;
	}
	else {
        throw ModelException(MID_PI_MCS, "Get1DData", "Result " + s + " does not exist.");
    }
    *nRows = m_nCells;
}

void clsPI_MCS::InitialOutputs() {
    if (m_canSto == nullptr) {
        Initialize1DArray(m_nCells, m_canSto, m_initIntcpSto);
    }
#ifndef STORM_MODE
    if (m_IntcpET == nullptr) {
        Initialize1DArray(m_nCells, m_IntcpET, 0.f);
    }
#endif
    if (m_netPcp == nullptr) {
        Initialize1DArray(m_nCells, m_netPcp, 0.f);
    }
    if (m_intcpLoss == nullptr) {
        Initialize1DArray(m_nCells, m_intcpLoss, 0.f);
    }
	if (m_handWtrDep == nullptr)
	{
		Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
	}
}

void clsPI_MCS::SetReaches(clsReaches* reaches) {
	if (nullptr == reaches) {
		throw ModelException(MID_MUSK_CH, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_nreach = reaches->GetReachNumber();
}

int clsPI_MCS::Execute() {
    //check input data
    CheckInputData();
    /// initialize outputs
    InitialOutputs();

#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
		int subbasinId = CVT_INT(m_subbsnID[i]);
        if (m_pcp[i] > 0.f) {
#ifdef STORM_MODE
            /// correction for slope gradient, water spreads out over larger area
            /// 1. / 3600. = 0.0002777777777777778
            m_P[i] = m_P[i] * m_hilldt * 0.0002777777777777778f * cos(atan(m_slope[i]));
#endif // STORM_MODE
			// xiaodw++, when inundation occours at a HAND, interception is now allowed, thus interception alse not allowed
			//float handWtrDepMM = m_handWtrDep[i] * 1000.0;
			//if (handWtrDepMM > 0.0)
			//{
			//	m_netPcp[i] = m_pcp[i];
			//	if (m_canSto[i] > 0.0)
			//	{
			//		m_chSto[subbasinId] += m_handArea[i] * m_canSto[i] * 0.001;
			//		m_canSto[i] = 0.0;
			//	}
			//	m_intcpLoss[i] = 0.f;
			//	m_IntcpET[i] = 0.f;
			//	continue;
			//}
            //interception storage capacity, 1. / 365. = 0.0027397260273972603
            float degree = 2.f * PI * (m_dayOfYear - 87.f) * 0.0027397260273972603f;
            /// For water, min and max are both 0, then no need for specific handling.
            float min = m_minIntcpStoCap[i];
            float max = m_maxIntcpStoCap[i];
            float capacity = min + (max - min) * pow(0.5f + 0.5f * sin(degree), m_intcpStoCapExp);

            //interception, currently, m_st[i] is storage of (t-1) time step
            float availableSpace = capacity - m_canSto[i];
            if (availableSpace < 0) {
                availableSpace = 0.f;
            }

            if (availableSpace < m_pcp[i]) {
                m_intcpLoss[i] = availableSpace;
                //if the cell is paddy, by default 15% part of pcp will be allocated to embankment area
                if (CVT_INT(m_landUse[i]) == LANDUSE_ID_PADDY) {
                    //water added into ditches from low embankment, should be added to somewhere else.
                    float pcp2canal = m_pcp[i] * m_pcp2CanalFr * m_embnkFr;
                    m_netPcp[i] = m_pcp[i] - m_intcpLoss[i] - pcp2canal;
                } else {
                    //net precipitation
                    m_netPcp[i] = m_pcp[i] - m_intcpLoss[i];
                }
            } else {
                m_intcpLoss[i] = m_pcp[i];
                m_netPcp[i] = 0.f;
            }

			// *** MODIFIED ***
			// Multiply net precipitation by 10
			//m_netPcp[i] *= 10.f;

            m_canSto[i] += m_intcpLoss[i];
        } else {
            m_intcpLoss[i] = 0.f;
            m_netPcp[i] = 0.f;
        }
#ifndef STORM_MODE
        //evaporation
        if (m_canSto[i] > m_pet[i]) {
            m_IntcpET[i] = m_pet[i];
        } else {
            m_IntcpET[i] = m_canSto[i];
        }
        m_canSto[i] -= m_IntcpET[i];
#endif
    }
    return 0;
}

//int clsPI_MCS::Execute() {
//	//check input data
//	CheckInputData();
//	/// initialize outputs
//	InitialOutputs();
//
//	const int DBG_I0 = 629;
//	const int DBG_I1 = 644;
//
//#pragma omp parallel for
//	for (int i = 0; i < m_nCells; i++) {
//		int subbasinId = CVT_INT(m_subbsnID[i]);
//
//		// ===== DEBUG BEFORE: print cell 629-644 pcp / interception / netPcp =====
//		if (i >= DBG_I0 && i <= DBG_I1) {
//			std::cout
//				<< "[PI_MCS][BEFORE] day=" << m_dayOfYear
//				<< " date=" << m_date
//				<< " i=" << i
//				<< " sb=" << (m_subbsnID ? (int)m_subbsnID[i] : -9999)
//				<< " pcp(mm)=" << (m_pcp ? m_pcp[i] : -9999.f)
//				<< " intcpLoss(mm)=" << (m_intcpLoss ? m_intcpLoss[i] : -9999.f)
//				<< " netPcp(mm)=" << (m_netPcp ? m_netPcp[i] : -9999.f)
//				<< " canSto(mm)=" << (m_canSto ? m_canSto[i] : -9999.f)
//				<< " pet(mm)=" << (m_pet ? m_pet[i] : -9999.f)
//#ifndef STORM_MODE
//				<< " intcpET(mm)=" << (m_IntcpET ? m_IntcpET[i] : -9999.f)
//#endif
//				<< std::endl;
//		}
//
//		if (m_pcp[i] > 0.f) {
//#ifdef STORM_MODE
//			/// correction for slope gradient, water spreads out over larger area
//			/// 1. / 3600. = 0.0002777777777777778
//			m_P[i] = m_P[i] * m_hilldt * 0.0002777777777777778f * cos(atan(m_slope[i]));
//#endif // STORM_MODE
//
//			//interception storage capacity, 1. / 365. = 0.0027397260273972603
//			float degree = 2.f * PI * (m_dayOfYear - 87.f) * 0.0027397260273972603f;
//			/// For water, min and max are both 0, then no need for specific handling.
//			float min = m_minIntcpStoCap[i];
//			float max = m_maxIntcpStoCap[i];
//			float capacity = min + (max - min) * pow(0.5f + 0.5f * sin(degree), m_intcpStoCapExp);
//
//			//interception, currently, m_canSto[i] is storage of (t-1) time step
//			float availableSpace = capacity - m_canSto[i];
//			if (availableSpace < 0) {
//				availableSpace = 0.f;
//			}
//
//			if (availableSpace < m_pcp[i]) {
//				m_intcpLoss[i] = availableSpace;
//				//if the cell is paddy, by default 15% part of pcp will be allocated to embankment area
//				if (CVT_INT(m_landUse[i]) == LANDUSE_ID_PADDY) {
//					//water added into ditches from low embankment, should be added to somewhere else.
//					float pcp2canal = m_pcp[i] * m_pcp2CanalFr * m_embnkFr;
//					m_netPcp[i] = m_pcp[i] - m_intcpLoss[i] - pcp2canal;
//				}
//				else {
//					//net precipitation
//					m_netPcp[i] = m_pcp[i] - m_intcpLoss[i];
//				}
//			}
//			else {
//				m_intcpLoss[i] = m_pcp[i];
//				m_netPcp[i] = 0.f;
//			}
//
//			m_canSto[i] += m_intcpLoss[i];
//		}
//		else {
//			m_intcpLoss[i] = 0.f;
//			m_netPcp[i] = 0.f;
//		}
//
//#ifndef STORM_MODE
//		//evaporation
//		if (m_canSto[i] > m_pet[i]) {
//			m_IntcpET[i] = m_pet[i];
//		}
//		else {
//			m_IntcpET[i] = m_canSto[i];
//		}
//		m_canSto[i] -= m_IntcpET[i];
//#endif
//
//		// ===== DEBUG AFTER: print cell 629-644 pcp / interception / netPcp =====
//		if (i >= DBG_I0 && i <= DBG_I1) {
//			std::cout
//				<< "[PI_MCS][AFTER ] day=" << m_dayOfYear
//				<< " date=" << m_date
//				<< " i=" << i
//				<< " sb=" << (m_subbsnID ? (int)m_subbsnID[i] : -9999)
//				<< " pcp(mm)=" << (m_pcp ? m_pcp[i] : -9999.f)
//				<< " intcpLoss(mm)=" << (m_intcpLoss ? m_intcpLoss[i] : -9999.f)
//				<< " netPcp(mm)=" << (m_netPcp ? m_netPcp[i] : -9999.f)
//				<< " canSto(mm)=" << (m_canSto ? m_canSto[i] : -9999.f)
//				<< " pet(mm)=" << (m_pet ? m_pet[i] : -9999.f)
//#ifndef STORM_MODE
//				<< " intcpET(mm)=" << (m_IntcpET ? m_IntcpET[i] : -9999.f)
//#endif
//				<< std::endl;
//		}
//	}
//	return 0;
//}


bool clsPI_MCS::CheckInputData() {
    CHECK_POSITIVE(MID_PI_MCS, m_date);
    CHECK_POSITIVE(MID_PI_MCS, m_nCells);
    CHECK_POINTER(MID_PI_MCS, m_pcp);
#ifndef STORM_MODE
    CHECK_POINTER(MID_PI_MCS, m_pet);
#else
    CHECK_POINTER(MID_PI_MCS, m_slope);
    CHECK_POINTER(MID_PI_MCS, m_hilldt);
#endif
    CHECK_POINTER(MID_PI_MCS, m_maxIntcpStoCap);
    CHECK_POINTER(MID_PI_MCS, m_minIntcpStoCap);
    CHECK_DATA(MID_PI_MCS, m_intcpStoCapExp > 1.5f || m_intcpStoCapExp < 0.5f,
        "The interception storage capacity exponent "
        "can not be " + ValueToString(m_intcpStoCapExp) + ". It should between 0.5 and 1.5.");
    CHECK_DATA(MID_PI_MCS, m_initIntcpSto > 1.f || m_initIntcpSto < 0.f, "The Initial interception storage cannot "
        "be " + ValueToString(m_initIntcpSto) + ". It should between 0 and 1.");
    return true;
}
