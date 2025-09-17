#include "ParamInfo.h"

#include <algorithm>

#include "utils_math.h"
#include "utils_string.h"
#include "text.h"

using namespace utils_math;
using namespace utils_string;

ParamInfo::ParamInfo() : Name(""), Units(""), Description(""), ModuleID(""), Dimension(DT_Unknown),
                         Transfer(TF_None), Source(""), Value(0.f), Impact(0.f), Change(""),
                         Maximum(0.f), Minimun(0.f), DependPara(nullptr), ClimateType(""),
                         IsConstant(false), IsOutput(false), OutputToOthers(false),
                         BasicName(""), initialized(false), Impact1(0.f) {
}

ParamInfo::ParamInfo(const ParamInfo& another) {
    Name = another.Name;
    Units = another.Units;
    Description = another.Description;
    Dimension = another.Dimension;
    Transfer = another.Transfer;
    Value = another.Value;
    Change = another.Change;
    Impact = another.Impact;
    Impact1 = another.Impact1;
    Maximum = another.Maximum;
    Minimun = another.Minimun;
    ModuleID = another.ModuleID;
    Source = another.Source;
    DependPara = another.DependPara;
    ClimateType = another.ClimateType;
    IsConstant = another.IsConstant;
    IsOutput = another.IsOutput;
    OutputToOthers = another.OutputToOthers;
    BasicName = another.BasicName;
    initialized = another.initialized;
}

ParamInfo::~ParamInfo() {
    if (DependPara != nullptr) {
        DependPara = nullptr;
    }
}
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

void ParamInfo::LoadCaliCSV(const std::string& para_name,const std::string& filename) {
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        use_cali_csv = false;
        return;
    }

    std::string line;
    std::getline(infile, line);
    std::istringstream header_ss(line);
    std::string col;
    std::vector<std::string> headers;
    while (std::getline(header_ss, col, ',')) {
        headers.push_back(col);
    }

    // 查找 "fid" 和 para_name 的列索引
    int fid_idx = -1, para_idx = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i] == "subbasin" || headers[i] =="FID") fid_idx = i;
        if (headers[i] == para_name) para_idx = i;
    }

    if (fid_idx == -1 || para_idx == -1) {
        use_cali_csv = false;
        return;
    }

    while (std::getline(infile, line)) {
        std::istringstream ss(line);
        std::vector<std::string> tokens;
        std::string token;

        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        int required_index = (fid_idx > para_idx) ? fid_idx : para_idx;
        if (tokens.size() > static_cast<size_t>(required_index)) {
            int fid = std::stoi(tokens[fid_idx]);
            float cali = std::stof(tokens[para_idx]);
            fid_cali_map[fid] = cali;

        }
    }
    use_cali_csv = true;
}

float ParamInfo::GetAdjustedValue(const float pre_value /* = NODATA_VALUE */) {
    float res = pre_value;
    if (FloatEqual(pre_value, NODATA_VALUE)) {
        res = Value;
    }
    if (FloatEqual(res, NODATA_VALUE)) {
        /// Do not change NoData value
        return res;
    }
    if (StringMatch(Change, PARAM_CHANGE_RC) && !FloatEqual(Impact, 1.f)) {
        res *= Impact;
    } else if (StringMatch(Change, PARAM_CHANGE_AC) && !FloatEqual(Impact, 0.f)) {
        res += Impact;
    } else if (StringMatch(Change, PARAM_CHANGE_VC) && !FloatEqual(Impact, NODATA_VALUE)) {
        res = Impact;
    } else if (StringMatch(Change, PARAM_CHANGE_NC)) {
        //don't change
        return res;
    }
    if (!FloatEqual(Maximum, NODATA_VALUE) && res > Maximum) res = Maximum;
    if (!FloatEqual(Minimun, NODATA_VALUE) && res < Minimun) res = Minimun;
    return res;
}

float ParamInfo::GetAdjustedValueWithFID(int fid, const float pre_value /* = NODATA_VALUE */) {
    float res = pre_value;
    if (FloatEqual(pre_value, NODATA_VALUE)) {
        res = Value;
    }
    if (FloatEqual(res, NODATA_VALUE)) {
        /// Do not change NoData value
        return res;
    }
    // 选择使用 Impact1 还是 Impact
    float selectedImpact = Impact;
    if (fid >= 0) {
        if (use_cali_csv && fid_cali_map.find(fid) != fid_cali_map.end() && fid_cali_map[fid]!=-9999) {
            selectedImpact = fid_cali_map[fid];
        }
    }
    if (StringMatch(Change, PARAM_CHANGE_RC) && !FloatEqual(selectedImpact, 1.f)) {
        res *= selectedImpact;
    } else if (StringMatch(Change, PARAM_CHANGE_AC) && !FloatEqual(selectedImpact, 0.f)) {
        res += selectedImpact;
    } else if (StringMatch(Change, PARAM_CHANGE_VC) && !FloatEqual(selectedImpact, NODATA_VALUE)) {
        res = selectedImpact;

    } else if (StringMatch(Change, PARAM_CHANGE_NC)) {
        //don't change
        return res;
    }
    if (!FloatEqual(Maximum, NODATA_VALUE) && res > Maximum) res = Maximum;
    if (!FloatEqual(Minimun, NODATA_VALUE) && res < Minimun) res = Minimun;
    return res;
}

void ParamInfo::Adjust1DArray(const int n, float* data) {
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        if (!FloatEqual(data[i], NODATA_VALUE)) {
            /// Do not change NoData value
            //data[i] = GetAdjustedValue(data[i]);
            data[i] = GetAdjustedValueWithFID(i,data[i]);
        }
    }
}

void ParamInfo::Adjust1DRaster(const int n, float* data) {
    Adjust1DArray(n, data);
}

int ParamInfo::Adjust1DRaster(const int n, float* data, const float* units,
                              const vector<int>& selunits,
                              const float* lu, const vector<int>& sellu) {
    int count = 0;
#pragma omp parallel for reduction(+:count)
    for (int i = 0; i < n; i++) {
        if (FloatEqual(data[i], NODATA_VALUE)) {
            /// Do not change NoData value
            continue;
        }
        int curunit = CVT_INT(units[i]);
        int curlu = CVT_INT(lu[i]);
        if (find(selunits.begin(), selunits.end(), curunit) == selunits.end()) {
            continue;
        }
        if (find(sellu.begin(), sellu.end(), curlu) == sellu.end()) {
            continue;
        }
        //data[i] = GetAdjustedValue(data[i]);
        data[i] = GetAdjustedValueWithFID(i,data[i]);
        count += 1;
    }
    return count;
}

void ParamInfo::Adjust2DArray(const int n, float** data) {
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        int cur_cols = CVT_INT(data[i][0]);
        Adjust1DArray(cur_cols, data[i] + 1);
    }
}

void ParamInfo::Adjust2DRaster(const int n, const int lyrs, float** data) {
#pragma omp parallel for
    // for (int i = 0; i < n; i++) {
    //     Adjust1DArray(lyrs, data[i]);
    // }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < lyrs; j++) {
        //data[i][j] = GetAdjustedValue(data[i][j]);
        data[i][j] = GetAdjustedValueWithFID(i,data[i][j]);
        }
    }
}

int ParamInfo::Adjust2DRaster(const int n, const int lyrs, float** data, float* units,
                               const vector<int>& selunits, float* lu, const vector<int>& sellu) {
    int count = 0;
#pragma omp parallel for reduction(+:count)
    for (int i = 0; i < n; i++) {
        int curunit = CVT_INT(units[i]);
        int curlu = CVT_INT(lu[i]);
        if (find(selunits.begin(), selunits.end(), curunit) == selunits.end()) {
            continue;
        }
        if (find(sellu.begin(), sellu.end(), curlu) == sellu.end()) {
            continue;
        }
        for (int j = 0; j < lyrs; j++) {
            //data[i][j] = GetAdjustedValue(data[i][j]);
            data[i][j] = GetAdjustedValueWithFID(i,data[i][j]);
        }
        count += 1;
    }
    return count;
}
