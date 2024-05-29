#include "pihm.h"
using namespace std;
PIHM_TOOLS_DEV::PIHM_TOOLS_DEV() {
}

PIHM_TOOLS_DEV::~PIHM_TOOLS_DEV() {
}

// 从args.txt文件中读取PIHM参数并返回char*数组
void PIHM_TOOLS_DEV::ReadArgumentsFromFile_dev(const string& filename, int& argc, char** &argv) {
	ifstream file(filename);
	if (!file.is_open()) {
		cerr << "无法打开文件: " << filename << endl;
		return;
	}

	// 使用stringstream来存储文件内容
	stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	// 将内容转存到string中，方便处理
	string content = buffer.str();

	// 使用vector来动态存储参数
	vector<string> args;
	istringstream iss(content);
	string arg;

	// 分割字符串为多个参数
	while (iss >> arg) {
		args.push_back(arg);
	}

	// 计算参数数量
	argc = args.size();

	// 创建一个新的char*数组来存储参数，需要额外一个位置用于null指针
	argv = new char*[argc + 1];

	// 复制参数到新的char*数组
	for (int i = 0; i < argc; ++i) {
		argv[i] = new char[args[i].length() + 1];
		strcpy(argv[i], args[i].c_str());
	}

	// 最后一个元素设置为null，符合main函数参数的标准
	argv[argc] = nullptr;

	return;
}

// 从select_hand_ids.txt文件中读取三角形hru的id list: [0,1,10,12,13,25,26,28,29]
void PIHM_TOOLS_DEV::read_ids_from_file_dev(const string& filename, vector<int> *ids) {
	ifstream file(filename);

	if (!file.is_open()) {
		cerr << "无法打开文件: " << filename << endl;
		return ;
	}

	string line;
	if (getline(file, line)) {
		stringstream ss(line);
		string id_str;

		while (getline(ss, id_str, ',')) {
			try {
				int id = stoi(id_str);
				ids->push_back(id);
			}
			catch (const invalid_argument& e) {
				cerr << "无效的ID: " << id_str << endl;
			}
			catch (const out_of_range& e) {
				cerr << "ID超出范围: " << id_str << endl;
			}
		}
	}

	file.close();
	return;
}

// 从final_downstream_file.txt文件中读取上下游关系
void PIHM_TOOLS_DEV::ReadUpDownStreamFile_dev(const string& filename, vector<hru_struct> *hrus) {
	// 创建新的 vector 对象
	//hrus = new vector<hru_struct>();
	ifstream file(filename);
	if (!file.is_open()) {
		cerr << "无法打开文件: " << filename << endl;
		delete hrus;
		hrus = nullptr;
		return;
	}
	string line;

	// 跳过第一行（标题行）
	getline(file, line);

	while (getline(file, line)) {
		istringstream iss(line);
		hru_struct hru;
		string down_id_str;

		iss >> hru.key >> hru.down_type >> down_id_str;

		if (hru.down_type == 0) {
			// down_type 为 0 时，down_id 是一个整数
			hru.down_id = stoi(down_id_str);
		}
		else if (hru.down_type == 1) {
			// down_type 为 1 时，down_id 是一个映射
			stringstream ss(down_id_str);
			string item;
			while (getline(ss, item, ',')) {
				size_t colon_pos = item.find(':');
				int id = stoi(item.substr(0, colon_pos));
				float proportion = stod(item.substr(colon_pos + 1));
				hru.down_ids[id] = proportion;
			}
		}

		hrus->push_back(hru);
	}

	file.close();
}

bool PIHM_TOOLS_DEV::CheckIdInHruIds_dev(int id, vector<int> *hru_ids) {

	auto it = std::find(hru_ids->begin(), hru_ids->end(), id);
	// 检查结果
	if (it != hru_ids->end()) {
		// id 存在于 hru_ids 中
		//std::cout << "ID " << id << " 存在于 hru_ids 中" << std::endl;
		return true;
	}
	//else {
	//	// id 不存在于 hru_ids 中
	//	std::cout << "ID " << id << " 不存在于 hru_ids 中" << std::endl;
	//}
	return false;
}

void PIHM_TOOLS_DEV::read_adj_tri_ids_from_file(const std::string& filename, int* adj_tri_ids, int * len_adj_tri_ids) {
	ifstream file(filename);

	if (!file.is_open()) {
		cerr << "Error opening file: " << filename << endl;
		return;
	}

	string line;
	int index = 0;  // Start storing IDs from index 1
	if (getline(file, line)) {
		stringstream ss(line);
		string id;
	
		while (getline(ss, id, ',')) {
			adj_tri_ids[stoi(id)] = index++;
		}
		*len_adj_tri_ids = index - 1;  // Store the length of the array at index 0
		 
	}
	else {
		cerr << "Error reading line from file: " << filename << endl;
	}

	file.close();
}

void PIHM_TOOLS_DEV::read_map_from_file(const std::string& filename, map<int, int*> *hru_tri_id_map) {
	ifstream file(filename);
	if (!file.is_open()) {
		cerr << "无法打开文件: " << filename << endl;
		return;
	}

	string line;
	while (getline(file, line)) {
		istringstream iss(line);
		int key;
		string value_str;
		if (!(iss >> key)) {
			cerr << "读取键失败: " << line << endl;
			continue;
		}
		if (!(iss >> value_str)) {
			cerr << "读取值失败: " << line << endl;
			continue;
		}

		vector<int> values;
		stringstream ss(value_str);
		string item;
		while (getline(ss, item, ',')) {
			try {
				values.push_back(stoi(item));
			}
			catch (const invalid_argument& e) {
				cerr << "无效的数字: " << item << endl;
			}
		}

		// 将 vector<int> 转换为 int* 并存储在 map 中
		int* value_array = new int[values.size()+1];
		// [0]存储数组长度
		value_array[0] = values.size();
		for (size_t i = 1; i < values.size(); ++i) {
			value_array[i] = values[i];
		}

		(*hru_tri_id_map)[key] = value_array;
	}
	file.close();
	return;
}


