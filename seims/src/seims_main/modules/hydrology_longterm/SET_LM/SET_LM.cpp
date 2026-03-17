#include "SET_LM.h"

#include "text.h"

SET_LM::SET_LM() :
    m_nCells(-1), m_maxSoilLyrs(-1), m_nSoilLyrs(nullptr), m_outletID(-1), m_nreach(-1),
    m_soilThk(nullptr), m_soilWtrSto(nullptr), m_soilFC(nullptr),
    m_pet(nullptr), m_IntcpET(nullptr),
    m_deprStoET(nullptr), m_maxPltET(nullptr), m_soilTemp(nullptr),
    m_soilFrozenTemp(NODATA_VALUE),
    m_soilET(nullptr),m_soilAWC(nullptr), m_handWtrDep(nullptr), m_subbsnID(nullptr), m_handArea(nullptr), m_hand_eavp(nullptr) {
}

SET_LM::~SET_LM() {
    if (m_soilET != nullptr) Release1DArray(m_soilET);
}

int SET_LM::Execute() {
    CheckInputData();
    InitialOutputs();
    int errCount = 0;
#pragma omp parallel for reduction(+: errCount)
    for (int i = 0; i < m_nCells; i++) {
        m_soilET[i] = 0.0f;
		//if (m_soilTemp[i] <= m_soilFrozenTemp) { continue; }     // xiaodw comment, don't need soil temperature now
	   //float etDeficiency = m_pet[i] - m_IntcpET[i] - m_deprStoET[i] - m_maxPltET[i];   // xiaodw comment, don't need plant et now, remove it 
		// xiaodw++, If a HAND  is inundated, its water is evaporated with priority
		float etDeficiency = m_pet[i] - m_IntcpET[i] - m_deprStoET[i] - m_hand_eavp[i];//m_deprStoET
		for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
			if (etDeficiency <= 0.f) break;
			float et2d = 0.f;
			//if (m_soilWtrSto[i][j] >= m_soilFC[i][j]) {
			float smBefore = m_soilWtrSto[i][j];
			if (m_soilWtrSto[i][j] >= m_soilAWC[i][j]) {
				et2d = etDeficiency;
			}
			else if (m_soilWtrSto[i][j] >= 0.f) {
				//et2d = etDeficiency * m_soilWtrSto[i][j] / m_soilFC[i][j];
				et2d = etDeficiency * m_soilWtrSto[i][j] / m_soilAWC[i][j];
			}
			else {
				et2d = 0.0f;
			}
			if (et2d > m_soilWtrSto[i][j]) {
				et2d = m_soilWtrSto[i][j];
				m_soilWtrSto[i][j] = 0.f;
			}
			else {
				m_soilWtrSto[i][j] -= et2d;
			}
			if (m_soilWtrSto[i][j] < 0.f) {
				cout << "SET_LM: moisture is less than zero" << m_soilWtrSto[i][j] << "\t" << et2d << endl;
				errCount++;
			}
			etDeficiency -= et2d;
			m_soilET[i] += et2d;
#ifdef DEBUG_SET_LM
			int SPECIFIED_SBID = 2;
			int subbasinId = CVT_INT(m_subbsnID[i]);
			//if (subbasinId == SPECIFIED_SBID) {

			//	cout << "Sbid: " << subbasinId << "   "
			//		<< " HandId: " << id << "   " << endl;
			//	for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
			//		cout
			//			<< "Layer: " << j << "   "
			//			<< " MoisBfe=" << m_soilMoistBfe[id][j] << "   "
			//			<< " MoisAft=" << m_soilMoist[id][j] << "   "
			//			<< " WtrStoBfe=" << m_soilWtrStoBfe[id][j] << "   "
			//			<< " WtrStoAft=" << m_soilWtrSto[id][j] << "   "
			//			<< " Perco=" << m_soilPerco[id][j] << "   "
			//			<< " SubF= " << m_subSurfRf[id][j] << "   "
			//			<< " SAT=" << m_soilSat[id][j] << "   "
			//			<< " AWC=" << m_soilAWC[id][j] << "   "
			//			<< " WP=" << m_soilWP[id][j] << "   "
			//			<< " THICK=" << m_soilThk[id][j] << "   "
			//			<< endl;
			//	}
			//	
			//}

#endif // DEBUG_SET_LM
			
		}
    }
    if (errCount > 0) {
        throw ModelException(MID_SET_LM, "Execute", "Soil moisture can not less than zero!");
    }
    return 0;
}

void SET_LM::Get1DData(const char* key, int* nRows, float** data) {
    InitialOutputs();
    string s(key);
    if (StringMatch(s, VAR_SOET)) *data = m_soilET;
	else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
		*data = m_handWtrDep;
	}
    else {
        throw ModelException(MID_SET_LM, "Get1DData", "Result " + s + " does not exist.");
    }
    *nRows = m_nCells;
}


void SET_LM::SetValue(const char* key, const float value) {
    string s(key);
    if (StringMatch(s, VAR_T_SOIL)) m_soilFrozenTemp = value;
	else if (StringMatch(s, VAR_OUTLETID)) m_outletID = CVT_INT(value);
    else {
        throw ModelException(MID_SET_LM, "SetValue", "Parameter " + s + " does not exist.");
    }
}

void SET_LM::Set1DData(const char* key, const int n, float* data) {
    string s(key);
	
	if (StringMatch(s, VAR_SOILLAYERS)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_nSoilLyrs = data;
	}
	else if (StringMatch(s, VAR_INET)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_IntcpET = data;
	}
	else if (StringMatch(s, VAR_PET)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_pet = data;
	}
	else if (StringMatch(s, VAR_DEET)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_deprStoET = data;
	}
	else if (StringMatch(s, VAR_PPT)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_maxPltET = data;
	}
	else if (StringMatch(s, VAR_SOTE)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_soilTemp = data;
	}
	else if (StringMatch(s, VAR_OL_HAND_WTRDEP)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_handWtrDep = data;
	}
	else if (StringMatch(s, VAR_SUBBSN)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_subbsnID = data;
	}
	else if (StringMatch(s, VAR_AHRU)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_handArea = data;
	}
	else if (StringMatch(s, VAR_HAND_EVAP)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_hand_eavp = data;
	}
	else {
		throw ModelException(MID_SET_LM, "Set1DData", "Parameter " + s + " does not exist.");
	}

}

void SET_LM::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    string sk(key);
    CheckInputSize2D(MID_SET_LM, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
    if (StringMatch(sk, VAR_SOL_AWC)) m_soilAWC = data; //m_soilFC = data;
    else if (StringMatch(sk, VAR_SOL_ST)) m_soilWtrSto = data;
    else if (StringMatch(sk, VAR_SOILTHICK)) m_soilThk = data;
    else {
        throw ModelException(MID_SET_LM, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

bool SET_LM::CheckInputData() {
    CHECK_POSITIVE(MID_SET_LM, m_nCells);
    //CHECK_POINTER(MID_SET_LM, m_soilFC);
    CHECK_POINTER(MID_SET_LM, m_IntcpET);
    CHECK_POINTER(MID_SET_LM, m_pet);
    CHECK_POINTER(MID_SET_LM, m_deprStoET);
	//CHECK_POINTER(MID_SET_LM, m_maxPltET);    // xiaodw comment, don't need plant et now, remove it 
	CHECK_POINTER(MID_SET_LM, m_soilWtrSto);
	//CHECK_POINTER(MID_SET_LM, m_soilTemp);     // xiaodw comment, don't need soil temperature now, remove it 
	CHECK_NODATA(MID_SET_LM, m_soilFrozenTemp);
    return true;
}

void SET_LM::InitialOutputs() {
    CHECK_POSITIVE(MID_SET_LM, m_nCells);
    if (nullptr == m_soilET) Initialize1DArray(m_nCells, m_soilET, 0.f);
	if (m_handWtrDep == nullptr)
	{
		Initialize1DArray(m_nCells, m_handWtrDep, 0.f);//xdw++
	}
	if (m_hand_eavp == nullptr)
	{
		Initialize1DArray(m_nCells, m_hand_eavp, 0.f);//xdw++
	}
	
}
