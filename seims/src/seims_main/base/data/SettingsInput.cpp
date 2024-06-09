#include "SettingsInput.h"

#include "utils_time.h"
#include "text.h"

using namespace utils_time;

SettingsInput::SettingsInput(vector<string>& stringvector)
    : m_startDate(0), m_endDate(0), m_dtHs(3600), m_dtCh(86400),
      m_isStormModel(false) {
    Settings::SetSettingTagStrings(stringvector);
    if (StringMatch(GetValue(Tag_Mode), Tag_Mode_Storm)) {
        m_isStormModel = true;
    }
    if (!readSimulationPeriodDate()) {
        throw ModelException("SettingInput", "Constructor",
                             "The start time and end time in file.in is invalid or missing."
                             "The format would be YYYY/MM/DD/HH. Please check it.");
    }
}

SettingsInput* SettingsInput::Init(vector<string>& stringvector) {
    if (stringvector.empty()) {
        return nullptr;
    }
    return new SettingsInput(stringvector);
}

bool SettingsInput::readSimulationPeriodDate() {
    //read start and end time
    m_startDate = ConvertToTime(GetValue(Tag_StartTime), "%d-%d-%d %d:%d:%d", true);
    m_endDate = ConvertToTime(GetValue(Tag_EndTime), "%d-%d-%d %d:%d:%d", true);

    if (m_startDate <= 0 || m_endDate <= 0) return false;

    // make sure the start and end times are in the proper order
    if (m_endDate < m_startDate) {
        time_t tmp = m_startDate;
        m_startDate = m_endDate;
        m_endDate = tmp;
    }

    m_mode = GetUpper(GetValue(Tag_Mode));

    //read interval
    vector<string> dtList = SplitString(GetValue(Tag_Interval), ',');
    char* strend = nullptr;
    errno = 0;
	// xiaodw comment, these are the origin code which just support daily mode
	/*
    m_dtHs = strtol(dtList[0].c_str(), &strend, 10);
    m_dtCh = m_dtHs;
    if (dtList.size() > 1) {
        m_dtCh = strtol(dtList[1].c_str(), &strend, 10);
    }
    // convert the time interval to seconds to conform to time_t struct
    if (StringMatch(m_mode, Tag_Mode_Daily)) {
        m_dtHs *= 86400; // 86400 secs is 1 day
        m_dtCh *= 86400;
    }
	*/

	// xiaodw add, support sub daily mode, such as 3-hr. Of course MODE should be set in the FILE_IN collection in Mongo before use this new  mode
	double m_tmp_dtHs = strtod(dtList[0].c_str(), &strend);
	double m_tmp_dtCh = 0.0;
	m_tmp_dtCh = m_tmp_dtHs;
	if (dtList.size() > 1) {
		m_tmp_dtCh = strtod(dtList[1].c_str(), &strend);
	}
	// convert the time interval to seconds to conform to time_t struct
	if (StringMatch(m_mode, Tag_Mode_Daily)) {
		m_dtHs = (time_t)(m_tmp_dtHs * 86400); // 86400 secs is 1 day
		m_dtCh = (time_t)(m_tmp_dtCh * 86400);
	}
	else {
		m_dtHs = (time_t)m_tmp_dtHs;
		m_dtCh = (time_t)m_tmp_dtCh;
	}
    return true;
}

void SettingsInput::Dump(const string& fileName) {
    std::ofstream fs;
    fs.open(fileName.c_str(), std::ios::out);
    if (fs.is_open()) {
        fs << "Start Date :" << ConvertToString2(m_startDate) << endl;
        fs << "End Date :" << ConvertToString2(m_endDate) << endl;
        fs << "Interval :" << m_dtHs << "\t" << m_dtCh << endl;
        fs.close();
    }
}
