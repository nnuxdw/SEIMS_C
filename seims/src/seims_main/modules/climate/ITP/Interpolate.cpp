#include "Interpolate.h"

#include "utils_time.h"
#include "text.h"

Interpolate::Interpolate() : m_dataType(0), m_nStatioins(-1),m_weight_nStations(-1),
                             m_stationData(nullptr), m_nCells(-1), m_itpWeights(nullptr), m_itpVertical(false),
                             m_hStations(nullptr), m_dem(nullptr), m_lapseRate(nullptr),
                             m_itpOutput(nullptr),m_itpWeights_id(nullptr),
                             //ljj+
                             m_datatypes(nullptr),m_lapse(NODATA_VALUE) {
}

void Interpolate::SetClimateDataType(const float value) {
    if (FloatEqual(value, 1.0f)) {
        m_dataType = 0; /// Precipitation
    } else if (FloatEqual(value, 2.0f) || FloatEqual(value, 3.0f) || FloatEqual(value, 4.0f)|| FloatEqual(value, 9.0f)|| FloatEqual(value, 10.0f)) {
        m_dataType = 1; /// Temperature
    } else if (FloatEqual(value, 5.0f)) {
        m_dataType = 2; /// PET
    } else if (FloatEqual(value, 6.0f) || FloatEqual(value, 7.0f) || FloatEqual(value, 8.0f)) {
        m_dataType = 3; /// Meteorology
    }
}

Interpolate::~Interpolate() {
    if (m_itpOutput != nullptr) Release1DArray(m_itpOutput);
}

int Interpolate::Execute() {
    CheckInputData();
    if (nullptr == m_itpOutput) { Initialize1DArray(m_nCells, m_itpOutput, 0.f); }
    size_t err_count = 0;
    if(m_dataType == 0) m_lapse = 0.00027; //
    if(m_dataType == 1) m_lapse = -0.005; //青藏高原多种分辨率月温度递减率网格数据集 zhangfan
    if(m_dataType == 2) m_lapse = 0;
    if(m_dataType == 3) m_lapse = 0;
//#pragma omp parallel for reduction(+: err_count)
    for (int i = 0; i < m_nCells; i++) {
        int index = 0;
        float value = 0.f;
        for (int j = 0; j < m_weight_nStations; j++) {            //by wanghaocheng
            //index = i * m_nStatioins + j;
            ////value += m_stationData[j] * m_itpWeights[index];
            //value += m_stationData[j] * m_itpWeights[i][j];
            //by wanghaocheng
            int k = (int)m_itpWeights_id[i][j];
            value += m_stationData[k] * m_itpWeights[i][j];
            if (value != value) {
                err_count++;
                //cout << "CELL:" << i << ", Site: " << j << ", Weight: " << m_itpWeights[index] <<
                //        ", siteData: " << m_stationData[j] << ", Value:" << value << ";" << endl;
                cout << "CELL:" << i << ", Site: " << k << ", Weight: " << m_itpWeights[i][j] <<
                    ", siteData: " << m_stationData[k] << ", Value:" << value << ";" << endl; //by wanghaocheng
            }
            //ljj++ did not use itp directly due to the m_lapseRate is not defined
            // float delta = m_dem[i] - m_hStations[k];//by wanghaocheng m_hStations[j] j->k
            // float adjust = m_itpWeights[i][j] * m_lapse * delta;
            // value += adjust;
            // if(m_dataType == 0) value = Max(value,0.f);
            // if (m_itpVertical) {
            //     float delta = m_dem[i] - m_hStations[k];//by wanghaocheng m_hStations[j] j->k
            //     float factor = m_lapseRate[m_month - 1][m_dataType];
            //     float adjust = m_itpWeights[i][j] * delta * factor * 0.01f;
            //     value += adjust;
            // }
            if(m_dataType == 0) value = Max(value,0.f);
        }
        m_itpOutput[i] = value;
    }
    if (err_count > 0) {
        throw ModelException(MID_ITP, "Execute", "Error occurred in weight data!");
    }
    return true;
}

void Interpolate::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, VAR_TSD_DT)) {
        SetClimateDataType(value);
    } else if (StringMatch(sk, Tag_VerticalInterpolation)) {
        m_itpVertical = value > 0;
    } else {
        throw ModelException(MID_ITP, "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void Interpolate::Set2DData(const char* key, const int n_rows, const int n_cols, float** data) {
    string sk(key);
    if (StringMatch(sk, Tag_LapseRate)) {
        if (m_itpVertical) {
            int n_month = 12;
            CheckInputSize(MID_ITP, key, n_rows, n_month);
            m_lapseRate = data;
        }
    }
    else if (StringMatch(sk, Tag_Weight[0]))
    {
        CheckInputSize2D(MID_ITP, key, n_rows, n_cols, m_nCells, m_weight_nStations);
        m_itpWeights = data;
    }
    else if (StringMatch(sk, Tag_Weight_ID[0])) {// by wanghaocheng
        CheckInputSize2D(MID_ITP, key, n_rows, n_cols, m_nCells, m_weight_nStations);
        m_itpWeights_id = data;

    }
    else {
        throw ModelException(MID_ITP, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}
void Interpolate::Set1DData(const char* key, const int n, float* data) {
    string sk(key);
    if (StringMatch(sk, Tag_DEM)) {
        //if (m_itpVertical) {
            CheckInputSize(MID_ITP, key, n, m_nCells);
            m_dem = data;
        //}
    // } else if (StringMatch(sk, Tag_Weight)) {
    //     CheckInputSize(MID_ITP, key, n, m_nCells);
    //     m_itpWeights = data;
    } else if (StringMatch(sk, Tag_Elevation_Precipitation) || StringMatch(sk, Tag_Elevation_Meteorology)
        || StringMatch(sk, Tag_Elevation_Temperature) || StringMatch(sk, Tag_Elevation_PET)) {
        //if (m_itpVertical) {
            //CheckInputSize(MID_ITP, key, n, m_nStatioins); by wanghaocheng n=3 m_nStatioins=2 m_nStatioins是用于插值的站点数 n为总气象站点数
            m_hStations = data;
        //}
    } else if (StringMatch(sk, VAR_DATATYPES)) {
        m_datatypes = data;
        SetClimateDataType(m_datatypes[0]);
    }else {
        string prefix = sk.substr(0, 1);
        if (StringMatch(prefix, DataType_Prefix_TS)) {
            CheckInputSize(MID_ITP, key, n, m_nStatioins); //by wanghaocheng
            m_stationData = data;
        } else {
            throw ModelException(MID_ITP, "Set1DData", "Parameter " + sk + " does not exist.");
        }
    }
}

bool Interpolate::CheckInputData() {
    CHECK_NONNEGATIVE(MID_ITP, m_dataType);
    CHECK_NONNEGATIVE(MID_ITP, m_month);
    CHECK_POINTER(MID_ITP, m_itpWeights);
    if (m_itpVertical) {
        CHECK_POINTER(MID_ITP, m_lapseRate);
        CHECK_POINTER(MID_ITP, m_dem);
        CHECK_POINTER(MID_ITP, m_hStations);
    }
    CHECK_POINTER(MID_ITP, m_stationData);
    return true;
}

void Interpolate::Get1DData(const char* key, int* n, float** data) {
    string sk(key);
    if (StringMatch(sk, Tag_DEM)) {
        *n = m_nCells;
        *data = m_dem;
    } else if (StringMatch(sk, Tag_StationElevation)) {
        *n = m_nStatioins;
        *data = m_hStations;
    } else if (StringMatch(sk, DataType_Prefix_TS)) {
        *n = m_nStatioins;
        *data = m_stationData;
    } else {
        *n = m_nCells;
        *data = m_itpOutput;
    }
}
