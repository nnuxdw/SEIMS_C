#pragma once
#ifndef PIHM_TOOLS_DEV_HEADER
#define PIHM_TOOLS_DEV_HEADER

#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;
// 定义HRU结构体
typedef struct hru_struct {
	int key;
	int down_type;
	int down_id;
	map<int, float>  down_ids; // 用于存储复杂的Down_ID数据
} hru_struct;

typedef struct arg_struct {
	char** argv;
	int argc;
} arg_struct;

typedef struct PIHMToolDataStruct {
	map<int, map<int, float>> upstream_hru_down_tris_map;
	unordered_set<int> upstream_hru_id_keys;
	vector<hru_struct> *hrus;
	arg_struct *args;
	vector<int> *hru_ids;
	map<int, int*> *hru_tri_id_map;
	map<int, int> *tri_hru_id_map;
	int * all_adj_tris_ids;
	int len_all_adj_tris_ids;

	//unordered_set<int> upstream_hru_id_keys;
}PIHMToolDataStruct;

class PIHM_TOOLS_DEV {
public:
	PIHM_TOOLS_DEV();
	~PIHM_TOOLS_DEV(); //! Destructor

	//char final_downstream_file[MAXSTRING];
	//char *final_downstream_file;


	void ReadArgumentsFromFile_dev(const string& filename, int& argc, char** &argv);
	void ReadUpDownStreamFile_dev(const string& filename, vector<hru_struct>* hrus);
	void read_ids_from_file_dev(const string& filename, vector<int>* ids);
	bool CheckIdInHruIds_dev(int id, vector<int> *hru_ids);
	void read_map_from_file(const std::string& filename, map<int, int*>* hru_tri_id_map);
	void read_adj_tri_ids_from_file(const std::string& filename, int* adj_tri_ids, int * len_adj_tri_ids);

};






#endif
