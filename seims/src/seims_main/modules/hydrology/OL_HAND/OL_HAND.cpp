#include "OL_HAND.h"
#include <map>
#include <set> 
#include "text.h"

OL_HAND::OL_HAND() :
	m_dt(-1), m_inputSubbsnID(-1), m_nCells(-1), m_nSubbsns(-1),
	m_chWth(nullptr), m_chDepth(nullptr), m_chLen(nullptr), m_islake(nullptr), m_handWtrDep(nullptr),
	curLev(1), levCounter(0){
}

OL_HAND::~OL_HAND() {
    //if (m_output1Draster != nullptr) Release1DArray(m_output1Draster);
    //if (m_output2Draster != nullptr) Release2DArray(m_nCells, m_output2Draster);
    // NOTE: m_scenario and m_reaches will be released in DataCenter!
}

void OL_HAND::SetValue(const char* key, const float value) {
	string sk(key);
	if (StringMatch(sk, Tag_TimeStep)) m_dt = CVT_INT(value);
	else if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);
	//else if (StringMatch(sk, Tag_CellWidth)) m_CellWth = value;
	else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
	else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
	else {
		throw ModelException(MID_IUH_OL, "SetValue", "Parameter " + sk + " does not exist.");
	}
}

void OL_HAND::Set1DData(const char* key, const int n, float* data) {
	
	string sk(key);

    //if (StringMatch(sk, VAR_CN2)) m_raster1D = data;
    //else if (StringMatch(sk, VAR_SOILLAYERS)) m_nSoilLyrs = data;
    //else {
    //    throw ModelException("IO_TEST", "Set1DData", "Parameter " + string(key) + " is not exist");
    //}
	if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_OL_HAND, key, n, m_nCells);
		m_handArea = data;
	}
	else if (StringMatch(sk, VAR_BKST)) m_bankSto = data;
	else if (StringMatch(sk, VAR_BKST_LAST_STEP)) m_bankStoLastStep = data;
	else if (StringMatch(sk, VAR_CHST)) m_chSto = data;
	else if (StringMatch(sk, VAR_CHST_LAST_STEP)) m_chStoLastStep = data;
	else if (StringMatch(sk, VAR_SUBBSN)) m_subbsnID = data;
	else if (StringMatch(sk, VAR_CHWTRDEPTH)) m_chWtrDepth = data;
	else if (StringMatch(sk, VAR_CHWTRWIDTH)) m_chWtrWth = data;

	
	else {
		throw ModelException(MID_OL_HAND, "Set1DData", "Parameter " + sk + " does not exist.");
	}
}

void OL_HAND::Set2DData(const char* key, const int n, const int col, float** data) {
    string sk(key);
    //if (!CheckInputSize2D("IO_TEST", key, n, col, m_nCells, m_maxSoilLyrs)) return;
    //if (StringMatch(sk, VAR_CONDUCT)) {
    //    m_raster2D = data;
    //}
}


void OL_HAND::SetReaches(clsReaches* reaches) {
 //   if (nullptr != reaches) m_reaches = reaches;
	//m_chNumber = reaches->GetReachNumber();

	//if (nullptr == m_reachDownStream) reaches->GetReachesSingleProperty(REACH_DOWNSTREAM, &m_reachDownStream);

	//m_reachUpStream = reaches->GetUpStreamIDs();
	if (nullptr == reaches) {
		throw ModelException(MID_OL_HAND, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_reachLayers = reaches->GetReachLayers();
	m_nreach = reaches->GetReachNumber();
	if (nullptr == m_chWth) reaches->GetReachesSingleProperty(REACH_WIDTH, &m_chWth);
	if (nullptr == m_chDepth) reaches->GetReachesSingleProperty(REACH_DEPTH, &m_chDepth);
	if (nullptr == m_chLen) reaches->GetReachesSingleProperty(REACH_LENGTH, &m_chLen);
	if (nullptr == m_islake) reaches->GetReachesSingleProperty(REACH_ISLAKE, &m_islake);
}

bool OL_HAND::CheckInputData() {
    /// m_date is protected variable member in base class SimulationModule.
    //CHECK_POSITIVE("IO_TEST", m_date);
    //CHECK_POSITIVE("IO_TEST", m_nCells);
    //CHECK_POINTER("IO_TEST", m_raster1D);
    //CHECK_POINTER("IO_TEST", m_raster2D);
    //CHECK_POINTER("IO_TEST", m_nSoilLyrs);
    return true;
}

void OL_HAND::InitialOutputs() {
	CHECK_POSITIVE(MID_OL_HAND, m_nreach);
	CHECK_POSITIVE(MID_OL_HAND, m_nCells);
	if (m_Hands.empty())
	{
		m_handWtrDep = new(nothrow) float[m_nCells];
		// load floodstep
		string filename = "G:\\program\\seims\\data\\Ammersee\\Ammerse_data\\rundata\\FloodStep.txt";
		LoadHandIdsToChHandLevels(filename);
		// initialize each level's sum area and vol
		for (int sbid = 1; sbid <= m_nreach; ++sbid) {
			
			for (int lev = 1; lev <= m_Hands[sbid].n_levels; lev++) {

				int m_levelHandNum = m_Hands[sbid].levels[lev].m_levelHandNum; 
				//  sum area of each level's hands should contain channel's area
				if (lev <= 1)
				{
					// for test
					//m_Hands[sbid].levels[lev].m_levelHandSumArea = 1.0;
					m_Hands[sbid].levels[lev].m_levelHandSumArea =  m_chWth[sbid] * m_chLen[sbid];
				}
				// sum area of each level's hands should contain all lower hand's area
				else
				{
					m_Hands[sbid].levels[lev].m_levelHandSumArea =  m_Hands[sbid].levels[lev-1].m_levelHandSumArea;
				}
				// sum area of each level's hands should contain this level's hands area
				for (int idx = 0; idx < m_levelHandNum; idx++)
				{
					int hand_id = m_Hands[sbid].levels[lev].handIds[idx];
					// for test
					//m_Hands[sbid].levels[lev].m_levelHandSumArea += 1.0;
					m_Hands[sbid].levels[lev].m_levelHandSumArea += m_handArea[hand_id];
				}
				m_Hands[sbid].levels[lev].m_levelHandSumVol = m_Hands[sbid].levels[lev].m_levelHandSumArea * m_Hands[sbid].levels[lev].m_levelHandDepth;
				// for test
				m_Hands[sbid].volToAdd = 0.0;
				// don't need to initialize water depth if use the new HandInundation function? and initialize it to zero is ok
				m_Hands[sbid].levels[lev].m_levelWtrDep = 0.0;
				
			}
		}
		// initialize water depth of each level
		// don't need to initialize water depth if use the new HandInundation function?
		for (int sbid = 1; sbid <= m_nreach; ++sbid) {
			//HandInundation(sbid, m_bankSto[sbid]);
			m_Hands[sbid].m_CurInundationLevel = 1;
			/*m_Hands[sbid].m_CurInundationLevel = 1;
			float residualWtrVol = m_bankSto[sbid];
			for (int lev = 1; lev <= m_Hands[sbid].n_levels; lev++) {
				if (residualWtrVol <= 0.0)
				{
					m_Hands[sbid].levels[lev].m_levelWtrDep = 0.0;
					continue;
				}

				if (residualWtrVol > m_Hands[sbid].levels[lev].m_levelHandSumVol)
				{
					m_Hands[sbid].m_CurInundationLevel++;
					m_Hands[sbid].levels[lev].m_levelWtrDep += m_Hands[sbid].levels[lev].m_levelHandDepth;
					residualWtrVol -= m_Hands[sbid].levels[lev].m_levelHandSumVol;
				}
				else {
					m_Hands[sbid].levels[lev].m_levelWtrDep += residualWtrVol / m_Hands[sbid].levels[lev].m_levelHandSumArea;
					residualWtrVol = 0.0;
				}

			}*/
		}
		// todo: m_handOvFlow 和m_Hands[sbid].levels[lev].m_levelWtrDep初始值是否相等？关键是VAR_OLFLOW在其它模块算出来的意义，是否涵盖了河道泛滥的淹水？


	}
	//if (nullptr == m_handWtrDep) {
	//	Initialize1DArray(m_nCells, m_handWtrDep, 0.f);
	//}


}

int OL_HAND::Execute() {
    /// Initialize output variables
    //if (nullptr == m_output1Draster) Initialize1DArray(m_nCells, m_output1Draster, 0.f);

    //if (nullptr == m_output2Draster) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_output2Draster, NODATA_VALUE);

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
			int reachIndex = it->second[i]; // index in the array, i.e., subbasinID

			if (m_inputSubbsnID == 0 || m_inputSubbsnID == reachIndex) {
				// 防止越界：确认层级存在
				int maxLev = m_Hands[reachIndex].n_levels;
				int curLev = m_Hands[reachIndex].m_CurInundationLevel;
				m_Hands[reachIndex].volToAdd += 0.5f * m_Hands[reachIndex].levels[curLev].m_levelHandSumVol;

				if (m_islake[reachIndex] == 1) {
					m_chSto[reachIndex] = m_Hands[reachIndex].volToAdd;
					HandInundation(reachIndex, m_chSto[reachIndex]);
				}
				else {
					m_bankSto[reachIndex] = m_Hands[reachIndex].volToAdd;
					HandInundation(reachIndex, m_bankSto[reachIndex]);
				}

				//levCounter++;
				//if (levCounter == 2) {
				//	curLev++;        // 每两次进入下一层
				//	levCounter = 0;  // 重置计数器
				//}
				
			}
		}
	}

    /// Execute function
//#pragma omp parallel for
//    for (int i = 0; i < m_nCells; i++) {
//        m_output1Draster[i] = m_raster1D[i] * 0.5f;
//        for (int j = 0; j < m_nSoilLyrs[i]; j++) {
//            m_output2Draster[i][j] = m_raster2D[i][j] + 2.f;
//        }
//    }
//
//    int nReaches = m_reaches->GetReachNumber();
    return 0;
}

void OL_HAND::Get1DData(const char* key, int* n, float** data) {
    string sk(key);
	if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
	*data = m_handWtrDep;
	*n = m_nCells;
	}
}

void OL_HAND::Get2DData(const char* key, int* n, int* col, float*** data) {
    string sk(key);
    //if (StringMatch(sk, "K_M")) {
    //    *data = this->m_output2Draster;
    //    *n = this->m_nCells;
    //    *col = this->m_maxSoilLyrs;
    //}
}

void OL_HAND::updateLowerHandsWtrDep(const int reachId, const int lev) {

	//m_Hands[reachId].levels[lev].m_levelWtrDep = 0.0;
	for (int ll = 1; ll <= m_Hands[reachId].m_CurInundationLevel; ll++)
	{
		for (int idx = 0; idx < m_Hands[reachId].levels[ll].m_levelHandNum; idx++)
		{
			int handId = m_Hands[reachId].levels[ll].handIds[idx];
			m_handWtrDep[handId] = m_Hands[reachId].levels[ll].m_levelWtrDep;
		}
	}
	return;
}

/// process water which excess subbasin's full volume 
void OL_HAND::updateSbExcessWater(const int reachId,  float* vol) {
	m_Hands[reachId].excessWtrVol = *vol;
	*vol = 0.0;

	return;
}

bool OL_HAND::HandInundation(const int reachId, float sto) {
	m_Hands[reachId].m_CurInundationLevel = 1;
	float residualWtrVol = sto;
	int lev = 1;
	while (lev <= m_Hands[reachId].n_levels) {
		// water depth is reset to zero, and calculate it by sto each time step
		m_Hands[reachId].levels[lev].m_levelWtrDep = 0.0;
		
		if (residualWtrVol <= 0.0)
		{
			//m_Hands[reachId].levels[lev].m_levelWtrDep = 0.0;
			updateLowerHandsWtrDep(reachId, lev);
			// set upper levels' hand water depth to zero
			if (m_Hands[reachId].m_CurInundationLevel < m_Hands[reachId].n_levels)
			{
				for (int ll = m_Hands[reachId].m_CurInundationLevel + 1; ll <= m_Hands[reachId].n_levels; ll++)
				{
					for (int idx = 0; idx < m_Hands[reachId].levels[ll].m_levelHandNum; idx++)
					{
						int handId = m_Hands[reachId].levels[ll].handIds[idx];
						m_handWtrDep[handId] = 0.0;
					}
				}
			}
			break;
		}
		if (residualWtrVol > m_Hands[reachId].levels[lev].m_levelHandSumVol)
		{
			m_Hands[reachId].levels[lev].m_levelWtrDep += m_Hands[reachId].levels[lev].m_levelHandDepth;
			residualWtrVol -= m_Hands[reachId].levels[lev].m_levelHandSumVol;
			m_Hands[reachId].m_CurInundationLevel++;
		}
		else {
			m_Hands[reachId].levels[lev].m_levelWtrDep += residualWtrVol / m_Hands[reachId].levels[lev].m_levelHandSumArea;
			residualWtrVol = 0.0;
		}

		if (m_Hands[reachId].m_CurInundationLevel > m_Hands[reachId].n_levels && residualWtrVol > 0.0)
		{
			m_Hands[reachId].m_CurInundationLevel--;
			updateLowerHandsWtrDep(reachId, lev);
			//
			updateSbExcessWater(reachId, &residualWtrVol);

			break;
		}
		lev++;
	}
	//for (int lev = 1; lev <= m_Hands[reachId].n_levels; lev++) {
	//	


	//}

	return true;
}

bool OL_HAND::HandInundation(const int reachId, float sto, float stoLastStep) {
	int curInundationLev = m_Hands[reachId].m_CurInundationLevel;
	
	float curLevHandDepth = 0.0;   
	float curLevHandSumVol = 0.0; // cooresponding to m_levelHandSumArea
	//float bankSto = m_bankSto[reachId];
	//float bankStoLastStep = m_bankStoLastStep[reachId];
	//float deltaBankSto = bankStoLastStep - bankSto;
	float deltaBankSto = sto - stoLastStep;
	float deltaH = 0.0;
	float deltaHAcc = 0.0;
	float curLevWtrDep = 0.0;  // 
	float curLevHandSumArea = 0.0;   // sum area of each level's hands should contain all lower hand's area
	float curLevWtrVol = 0.0;
	int nextLev = 0;
	// for test
	deltaBankSto = 100.0;
	// add volume

	float residualWtrVol = deltaBankSto;
	while (residualWtrVol > 0.0)
	{
		nextLev = curInundationLev + 1;
		curLevWtrDep = m_Hands[reachId].levels[curInundationLev].m_levelWtrDep;  // 
		curLevHandSumArea = m_Hands[reachId].levels[curInundationLev].m_levelHandSumArea;   // sum area of each level's hands should contain all lower hand's area
		curLevWtrVol = curLevWtrDep * curLevHandSumArea;
		curLevHandDepth = m_Hands[reachId].levels[curInundationLev].m_levelHandDepth;
		curLevHandSumVol = m_Hands[reachId].levels[curInundationLev].m_levelHandSumVol; // cooresponding to m_levelHandSumArea

		// if water excess current level hand sum volume, add extral water to next level
		if (curLevWtrVol + residualWtrVol > curLevHandSumVol)
		{
			deltaH = curLevHandDepth - m_Hands[reachId].levels[curInundationLev].m_levelWtrDep;
			curInundationLev++;
		}
		else {
			deltaH = residualWtrVol / curLevHandSumArea;
		}
		deltaHAcc += deltaH;
		//m_Hands[reachId].levels[curInundationLev].m_levelWtrDep += deltaH;
		
		residualWtrVol -= deltaH * curLevHandSumArea;

		if (nextLev > m_Hands[reachId].n_levels)
		{
			// todo: allocate water to other subbasin? or keep water in this subbasin?
			residualWtrVol = 0.0;
			break;
		}
	}

	while (residualWtrVol < 0.0)
	{
		nextLev = curInundationLev - 1;
		curLevWtrDep = m_Hands[reachId].levels[curInundationLev].m_levelWtrDep;  // 
		curLevHandSumArea = m_Hands[reachId].levels[curInundationLev].m_levelHandSumArea;   // sum area of each level's hands should contain all lower hand's area
		curLevWtrVol = curLevWtrDep * curLevHandSumArea;
		curLevHandDepth = m_Hands[reachId].levels[curInundationLev].m_levelHandDepth;
		curLevHandSumVol = m_Hands[reachId].levels[curInundationLev].m_levelHandSumVol; // cooresponding to m_levelHandSumArea

		// if water excess current level hand sum volume, add extral water to next level
		if (curLevWtrVol + deltaBankSto < 0.0)
		{
			deltaH = m_Hands[reachId].levels[curInundationLev].m_levelWtrDep;
			curInundationLev--;
		}
		else {
			deltaH = residualWtrVol / curLevHandSumArea;
		}
		deltaHAcc -= deltaH;

		residualWtrVol -= deltaH * curLevHandSumArea;

		if (nextLev < 1)
		{
			// todo: there will be no water on floodplain 
			residualWtrVol = 0.0;
			break;
		}
	}
	
	// update water depth of each level's hands lower than current level
	for (int lev = 1; lev <= curInundationLev; lev++)
	{
		m_Hands[reachId].levels[lev].m_levelWtrDep += deltaHAcc;

		for (int idx = 0; idx < m_Hands[reachId].levels[lev].m_levelHandNum; idx++)
		{
			int handId = m_Hands[reachId].levels[lev].handIds[idx];
			m_handWtrDep[handId] = m_Hands[reachId].levels[lev].m_levelWtrDep;
		}
	}
	m_Hands[reachId].m_CurInundationLevel = curInundationLev;
	return true;
}

void OL_HAND::LoadHandIdsToChHandLevels(const string& filename) {
	ifstream infile(filename);
	if (!infile.is_open()) {
		cerr << "Failed to open file: " << filename << endl;
		return;
	}

	int max_sbid = 0, max_level = 0;
	string line;
	getline(infile, line); // Skip header

	map<pair<int, int>, int> handCounts;
	map<int, set<int> > subbasinLevels; // <sbid, set of levels>

	// 第一次遍历：统计 hand 数和 level 数
	while (getline(infile, line)) {
		istringstream iss(line);
		string hru_id_str, subbasin_str, level_str, interval_str, depth_str;

		if (!(iss >> hru_id_str >> subbasin_str >> level_str >> interval_str >> depth_str)) continue;

		int sbid = (int)(stof(subbasin_str));
		int level = (int)(stof(level_str));

		if (sbid > max_sbid) max_sbid = sbid;
		if (level > max_level) max_level = level;

		handCounts[make_pair(sbid, level)]++;
		subbasinLevels[sbid].insert(level); // 统计 level 数
	}

	if ((int)m_Hands.size() <= max_sbid) {
		m_Hands.resize(max_sbid + 1);
	}

	for (int sbid = 0; sbid <= max_sbid; ++sbid) {
		if (subbasinLevels.count(sbid)) {
			m_Hands[sbid].n_levels = (int)subbasinLevels[sbid].size();
		}
	}

	for (const auto& entry : handCounts) {
		int sbid = entry.first.first;
		int level = entry.first.second;
		int count = entry.second;

		if ((int)m_Hands[sbid].levels.size() <= level) {
			m_Hands[sbid].levels.resize(level + 1);
		}

		m_Hands[sbid].levels[level].m_levelHandNum = count;
		m_Hands[sbid].levels[level].handIds = new(nothrow) int[count];
		m_Hands[sbid].levels[level].m_chOverHeadVol = 0.0f;
		m_Hands[sbid].levels[level].m_levelHandDepth = 0.0f;
	}

	infile.clear();
	infile.seekg(0, ios::beg);
	getline(infile, line); // Skip header again

	map<pair<int, int>, int> handIndex;

	while (getline(infile, line)) {
		istringstream iss(line);
		string hru_id_str, subbasin_str, level_str, interval_str, depth_str;

		if (!(iss >> hru_id_str >> subbasin_str >> level_str >> interval_str >> depth_str)) continue;

		float hru_id_f = stof(hru_id_str);
		float subbasin_f = stof(subbasin_str);
		float level_f = stof(level_str);
		float depth_f = stof(depth_str);

		int sbid = (int)(subbasin_f);
		int level = (int)(level_f);
		int idx = handIndex[make_pair(sbid, level)]++;

		m_Hands[sbid].levels[level].handIds[idx] = hru_id_f;

		if (idx == 0) {
			m_Hands[sbid].levels[level].m_levelHandDepth = depth_f;
		}

	}

	cout << "Finished loading HAND data from file: " << filename << endl;
}




