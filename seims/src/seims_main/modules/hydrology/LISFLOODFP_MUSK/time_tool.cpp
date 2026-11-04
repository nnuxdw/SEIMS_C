#include "time_tool.h"
// xiaodw add, transform time from 2019031201 to timestamp
time_t getTimestampFromDateTime(const std::string& dateTimeStr) {
	// 确保输入的字符串是正确的长度
	if (dateTimeStr.length() != 10) {
		cout << "Invalid datetime string format:" << dateTimeStr << endl;
		return NULL;
		//throw std::invalid_argument("Invalid datetime string format");
	}

	// 解析字符串中的年、月、日和小时部分
	std::tm timeStruct = {};
	timeStruct.tm_year = std::stoi(dateTimeStr.substr(0, 4)) - 1900; // 年份，从 1900 开始计数
	timeStruct.tm_mon = std::stoi(dateTimeStr.substr(4, 2)) - 1;     // 月份，从 0 开始计数
	timeStruct.tm_mday = std::stoi(dateTimeStr.substr(6, 2));        // 日期
	timeStruct.tm_hour = std::stoi(dateTimeStr.substr(8, 2));        // 小时
	timeStruct.tm_min = 0;                                           // 分钟，设置为 0
	timeStruct.tm_sec = 0;                                           // 秒，设置为 0

	// 将 std::tm 结构体转换为 UTC 时间戳
#ifdef _WIN32
	time_t timestamp = _mkgmtime(&timeStruct); // Windows 上的 timegm 等价函数
#else
	time_t timestamp = timegm(&timeStruct); // POSIX 系统上的 timegm 函数
#endif

// 如果转换失败，返回 -1
	if (timestamp == -1) {
		cout << "Failed to convert datetime to timestamp:" << dateTimeStr << endl;
		return NULL;
		//throw std::runtime_error("Failed to convert datetime to timestamp");
	}

	return timestamp;
}