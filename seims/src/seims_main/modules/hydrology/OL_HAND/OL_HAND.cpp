#include "OL_HAND.h"
#include <map>
#include <set>
#include "text.h"
#include <string>
#include <cctype>
#include <algorithm>


OL_HAND::OL_HAND() :
	m_dt(-1), m_inputSubbsnID(-1), m_nCells(-1), m_nSubbsns(-1),
	m_chWth(nullptr), m_chDepth(nullptr), m_chLen(nullptr), m_islake(nullptr), m_handWtrDep(nullptr), m_chBedMeanElev(nullptr), m_isres(nullptr),
	curLev(0), levCounter(0), m_isHandFlooded(nullptr), m_subbasinInundationArea(nullptr), m_subbasinWtrDep(nullptr),m_sumInundationArea(0),m_outletID(-1), m_subbasinArea(nullptr)
  {
}

OL_HAND::~OL_HAND() {
    //if (m_output1Draster != nullptr) Release1DArray(m_output1Draster);
    //if (m_output2Draster != nullptr) Release2DArray(m_nCells, m_output2Draster);
    // NOTE: m_scenario and m_reaches will be released in DataCenter!
}

static inline std::string trim(std::string s) {
	// ȥ��ǰ���ո�
	s.erase(s.begin(),
		std::find_if(s.begin(), s.end(),
			[](int ch) { return !std::isspace(ch); }));

	// ȥ��β��ո�
	s.erase(std::find_if(s.rbegin(), s.rend(),
		[](int ch) { return !std::isspace(ch); }).base(),
		s.end());

	return s;
}

void OL_HAND::SetValue(const char* key, const float value) {
	string sk(key);
	if (StringMatch(sk, Tag_TimeStep)) m_dt = CVT_INT(value);
	else if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);
	//else if (StringMatch(sk, Tag_CellWidth)) m_CellWth = value;
	else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
	else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
	else if (StringMatch(sk, VAR_OUTLETID)) m_outletID = CVT_INT(value);
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
	else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		m_handWtrDep = data;
	}
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
	if (nullptr == m_isres) reaches->GetReachesSingleProperty(REACH_ISRES, &m_isres);
	if (nullptr == m_chBedMeanElev) reaches->GetReachesSingleProperty(REACH_BED_MEAN_ELEV, &m_chBedMeanElev);

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
		if (nullptr == m_handWtrDep)
		{
			Initialize1DArray(m_nCells, m_handWtrDep, 0.f);
		}
		if (nullptr == m_isHandFlooded)
		{
			Initialize1DArray(m_nCells, m_isHandFlooded, 0.f);
		}
		if (nullptr == m_subbasinArea)
		{
			Initialize1DArray(m_nreach + 1, m_subbasinArea, 0.f);
		}
		if (nullptr == m_subbasinInundationArea)
		{
			Initialize1DArray(m_nreach + 1, m_subbasinInundationArea, 0.f);
		}
		if (nullptr == m_subbasinWtrDep)
		{
			Initialize1DArray(m_nreach + 1, m_subbasinWtrDep, 0.f);
		}

#ifdef _WIN32
		string txt_filename = "G:/program/seims/SEIMS_HAND/data/poyang_lake1/rundata/FloodStep.txt";
		string csv_filename = "G:/program/seims/SEIMS_HAND/data/poyang_lake1/rundata/InundationMap.csv";
#else
		string txt_filename = "/data/user/xiaodw/software/WISE/data/poyang_lake1/rundata/FloodStep.txt";
		string csv_filename = "/data/user/xiaodw/software/WISE/data/poyang_lake1/rundata/InundationMap.csv";
#endif
		// load floodstep
		LoadHandIdsToChHandLevels(txt_filename, m_Hands);
		// load
		loadHandFromCSVIntoVector(csv_filename,m_Hands);

		// initialize water depth of each level
		// don't need to initialize water depth if use the new HandInundation function?
		for (int sbid = 1; sbid <= m_nreach; ++sbid) {
			//HandInundation(sbid, m_bankSto[sbid]);
			m_Hands[sbid].m_CurInundationLevel = 1;

			for (int i = 1; i <= m_Hands[sbid].n_levels; ++i) {
				m_Hands[sbid].levels[i].m_levelWtrDep = 0.0;
				//cout << "sbid: " << sbid << " level: " << i << endl;
			}
		}
		for (int sbid = 1; sbid <= m_nreach; ++sbid) {
			updateAllHandsWtrDep(sbid);
		}
		for (int sbid = 1; sbid <= m_nreach; ++sbid) {
			float subbasinArea = 0.0;
			//cout << endl;
			//cout << "sbid: " << sbid << endl;
			for (int ll = 1; ll <= m_Hands[sbid].n_levels; ll++)
			{
				for (int idx = 0; idx < m_Hands[sbid].levels[ll].m_levelHandNum; idx++)
				{
					int handId = m_Hands[sbid].levels[ll].handIds[idx];
					subbasinArea += m_handArea[handId];
					//cout << "handId: " << handId << "  area: " << m_handArea[handId] * 0.000001 << endl;

				}
			}
			//cout << "sb area: " << subbasinArea * 0.000001 << endl;

		}
		// todo: m_handOvFlow ��m_Hands[sbid].levels[lev].m_levelWtrDep��ʼֵ�Ƿ���ȣ��ؼ���VAR_OLFLOW������ģ������������壬�Ƿ񺭸��˺ӵ����ĵ���ˮ��


	}
}

int OL_HAND::Execute() {
	//check the data
	CheckInputData();

	InitialOutputs();

	for (auto it = m_reachLayers.begin(); it != m_reachLayers.end(); it++) {
		// There are not any flow relationship within each routing layer.
		// So parallelization can be done here.
		int nReaches = it->second.size();
		// the size of m_reachLayers (map) is equal to the maximum stream order
#pragma omp parallel for
		for (int i = 0; i < nReaches; ++i) {
			int reachIndex = it->second[i]; // index in the array, i.e., subbasinID

			if (m_inputSubbsnID == 0 || m_inputSubbsnID == reachIndex) {

				if (m_islake[reachIndex] == 1 || m_isres[reachIndex] == 1) {
					//m_chSto[reachIndex] = m_Hands[reachIndex].volToAdd;
					HandInundation_BinarySearch(reachIndex, m_chSto[reachIndex]);
				}
				else {
					//m_bankSto[reachIndex] = m_Hands[reachIndex].volToAdd;
					HandInundation_BinarySearch(reachIndex, m_chSto[reachIndex]);
				}

			}
		}
	}

    return 0;
}			

void OL_HAND::Get1DData(const char* key, int* n, float** data) {
    string sk(key);
	if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		*data = m_handWtrDep;
		*n = m_nCells;
	}else if (StringMatch(sk, VAR_IS_HAND_FLOODED)) {
		*data = m_isHandFlooded;
		*n = m_nCells;
	}else if (StringMatch(sk, VAR_SUBBASIN_FLOODED_AREA)) {
		if (nullptr != m_subbasinInundationArea)
		{
			m_subbasinInundationArea[0] = m_subbasinInundationArea[m_outletID];
		}
		*data = m_subbasinInundationArea;
	}
	else if (StringMatch(sk, VAR_SUBBASIN_WTR_DEPTH)) {
		*data = m_subbasinWtrDep;
	}



	//else if (StringMatch(sk, VAR_CHWTRDEPTH)) {
	//	m_chWtrDepth[0] = m_chWtrDepth[m_outletID];
	//	*data = m_chWtrDepth;

	//}



}

void OL_HAND::Get2DData(const char* key, int* n, int* col, float*** data) {
    string sk(key);
    //if (StringMatch(sk, "K_M")) {
    //    *data = this->m_output2Draster;
    //    *n = this->m_nCells;
    //    *col = this->m_maxSoilLyrs;
    //}
}


void OL_HAND::updateAllHandsWtrDep(const int reachId) {
	float inundationArea = 0.0;
	float subbasinArea = 0.0;


	//m_Hands[reachId].levels[lev].m_levelWtrDep = 0.0;
	for (int ll = 1; ll <= m_Hands[reachId].n_levels; ll++)
	{
		for (int idx = 0; idx < m_Hands[reachId].levels[ll].m_levelHandNum; idx++)
		{
			int handId = m_Hands[reachId].levels[ll].handIds[idx];
			m_handWtrDep[handId] = m_Hands[reachId].levels[ll].m_levelWtrDep;
			if (m_handWtrDep[handId] > FLOOD_DEPTH_THRESH)
			{
				m_isHandFlooded[handId] = 1.0;
				inundationArea += m_handArea[handId];
			}
			else {
				m_isHandFlooded[handId] = 0.0;
			}
			subbasinArea += m_handArea[handId];

		}
	}
	m_subbasinInundationArea[reachId] = inundationArea * 0.000001;
	m_subbasinArea[reachId] = subbasinArea * 0.000001;
	m_subbasinWtrDep[reachId] = m_Hands[reachId].levels[1].m_levelWtrDep;
	//m_chWtrDepth[reachId] = m_Hands[reachId].levels[1].m_levelWtrDep;
	return;
}


/// process water which excess subbasin's full volume
void OL_HAND::updateSbExcessWater(const int reachId,  float* vol) {
	m_Hands[reachId].excessWtrVol = *vol;
	*vol = 0.0;

	return;
}


bool OL_HAND::HandInundation_BinarySearch(const int reachId, float sto) {
	if (sto <= 0.0f) return false;

	Hand& hand = m_Hands[reachId];
	const int n = hand.n_levels;
	vector<Level>& levels = hand.levels;

	// ���ֲ��ң��ҵ� sto ������һ�㣨��һ�� AccVol >= sto��
	int left = 1, right = n, target_level = n;

	while (left <= right) {
		int mid = (left + right) / 2;
		if (sto <= levels[mid].m_levelAccVol) {
			target_level = mid;
			right = mid - 1;
		}
		else {
			left = mid + 1;
		}
	}

	hand.m_CurInundationLevel = target_level;  // ���Ŵ� 1 ��ʼ
	float remaining = 0.0;
	if (target_level == 1)
	{
		remaining = sto;
	}
	else {
		remaining = sto - levels[target_level-1].m_levelAccVol;
	}
	if (remaining < 0.0)
	{
		cout <<  "reachId: " << reachId << " target_level-1: " << target_level - 1 <<  " sto: " << sto << " AccVol: " << levels[target_level - 1].m_levelAccVol << " remaining: " << remaining << endl;
		exit(0);
	}

	//  target_level ��֮�µ�ˮ��  ���� target_level ���е�ǰδ��������һС��ˮ��
	float partial_depth = (levels[target_level].m_levelSumArea > 0.0f) ? remaining / levels[target_level].m_levelSumArea : 0.0f;
	for (int i = 1; i <= target_level; ++i) {
		levels[i].m_levelWtrDep = levels[i].m_levelLowerAccDepth[target_level] + partial_depth;
	}

	// �������в�ˮ��Ϊ 0
	for (int i = target_level + 1; i <= n; ++i) {
		levels[i].m_levelWtrDep = 0.0f;
	}

	// ����������������ʣ�ಿ����Ϊ����ˮ
	float maxVolume = levels[n].m_levelAccVol;
	if (sto > maxVolume) {
		hand.excessWtrVol = sto - maxVolume;
	}

	updateAllHandsWtrDep(reachId);

#ifdef DEBUG_OL_HAND
	if (reachId == 1171)
	{
		cout << "===== HAND Debug: reachId = " << reachId << " =====\n";
		cout << "Input water volume (sto) = " << sto << " m3\n";
		cout << "Located in level = " << target_level << " / " << n << "\n";

		if (target_level > 1) {
			cout << "Volume below this level = " << levels[target_level - 1].m_levelAccVol << " m3\n";
		}
		cout << "Remaining volume in this level = " << remaining << " m3\n";
		cout << "Partial depth added = " << partial_depth << " m\n\n";

		cout << "---- Level Water Depths ----\n";
		for (int i = 1; i <= n; ++i) {
			cout << "Level " << i
				<< ": WaterDepth = " << levels[i].m_levelWtrDep
				<< " m"
				<< (i == target_level ? "  <-- active level" : "")
				<< "\n";
		}

		if (sto > maxVolume) {
			cout << "Excess Water = " << hand.excessWtrVol << " m3 (exceeds HAND max storage)\n";
		}

		cout << "====================================\n\n";
		cout.flush();
	}

#endif
	return true;
}




vector<float> OL_HAND::parseAccDepthArray(const std::string& str) {
	std::vector<float> values;
	std::string s = str;

	// ȥ��ǰ��������
	if (!s.empty() && s.front() == '[') s.erase(0, 1);              // ɾ����һ���ַ�
	if (!s.empty() && s.back() == ']') s.erase(s.size() - 1);       // ɾ�����һ���ַ�


	std::stringstream ss(s);
	std::string token;

	while (getline(ss, token, ',')) {
		try {
			values.push_back(std::stof(token));
		}
		catch (...) {
			values.push_back(0.0f); // ��������
		}
	}
	return values;
}

void OL_HAND::loadHandFromCSVIntoVector(const string& csvPath, vector<Hand>& m_Hands) {
	ifstream file(csvPath);
	if (!file.is_open()) {
		cerr << "Failed to open file: " << csvPath << endl;
		return;
	}

	string line;
	getline(file, line);  // ������ͷ

	while (getline(file, line)) {
		stringstream ss(line);
		string token;
		vector<string> tokens;

		// ȡǰ 7 ���ֶ�:
		// 0 Subbasin
		// 1 Flood_Level
		// 2 LevelDepth
		// 3 SumArea
		// 4 SumVolume
		// 5 AvgDepth
		// 6 AccVolume
		for (int i = 0; i < 7 && std::getline(ss, token, ','); ++i) {
			tokens.push_back(token);
		}

		// �� 8 �У�LowerAccDepth������ "[0.0, ...]"����˫���Ű���
		std::string accDepthStr;
		std::getline(ss, accDepthStr, '"');  // ����ǰһ�����ţ������ǿմ���
		std::getline(ss, accDepthStr, '"');  // ȡ�м������ַ���

		// ---- ���������ֶ� ----
		int subbasin = std::stoi(tokens[0]);
		int lev = std::stoi(tokens[1]);

		// �ṹУ�� & ��ȡ Hand
		Hand& hand = m_Hands[subbasin];
		if (lev >= static_cast<int>(hand.levels.size())) {
			hand.levels.resize(lev + 1);
		}

		Level& level = hand.levels[lev];
		level.m_levelDepth = std::stof(tokens[2]);
		level.m_levelSumArea = std::stof(tokens[3]);
		level.m_levelSumVol = std::stod(tokens[4]);
		level.m_levelAvgDepth = std::stof(tokens[5]);
		level.m_levelAccVol = std::stod(tokens[6]);

		// ---- ���� LowerAccDepth ���� ----
		std::vector<float> accDepthVec = parseAccDepthArray(accDepthStr);
		level.m_levelLowerAccDepth = new float[accDepthVec.size()];
		for (size_t i = 0; i < accDepthVec.size(); ++i) {
			level.m_levelLowerAccDepth[i] = accDepthVec[i];
		}

		hand.n_levels = max(hand.n_levels, lev);
	}

	file.close();
	std::cout << "Finished loading Inundation data from file: " << csvPath << std::endl;
}



void OL_HAND::LoadHandIdsToChHandLevels(const string& filename, vector<Hand>& m_Hands) {
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

	// ��һ�α�����ͳ�� hand ���� level ��
	while (getline(infile, line)) {
		istringstream iss(line);
		string hru_id_str, subbasin_str, level_str, interval_str, depth_str;

		if (!(iss >> hru_id_str >> subbasin_str >> level_str >> interval_str >> depth_str)) continue;

		int sbid = (int)(stof(subbasin_str));
		int level = (int)(stof(level_str));

		if (sbid > max_sbid) max_sbid = sbid;
		if (level > max_level) max_level = level;

		handCounts[make_pair(sbid, level)]++;
		subbasinLevels[sbid].insert(level); // ͳ�� level ��
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
		m_Hands[sbid].levels[level].handIds =  vector<int>(count);
		//m_Hands[sbid].levels[level].m_chOverHeadVol = 0.0f;
		m_Hands[sbid].levels[level].m_levelAvgDepth = 0.0f;
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

		//if (idx == 0) {
		//	m_Hands[sbid].levels[level].m_levelAvgDepth = depth_f;
		//}

	}

	cout << "Finished loading HAND data from file: " << filename << endl;
}




