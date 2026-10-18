#ifndef SEIMS_MODULE_MASCARET_REZO_H
#define SEIMS_MODULE_MASCARET_REZO_H

#include "SimulationModule.h"
#include <memory>
#include <vector>

class NodeReachNetwork;

class MASCARET_REZO : public SimulationModule {
public:
    MASCARET_REZO();
    ~MASCARET_REZO() OVERRIDE;
    void SetValue(const char* key, float value) OVERRIDE;
    void Set1DData(const char* key, int n, float* data) OVERRIDE;
    void SetReaches(clsReaches* reaches) OVERRIDE;
    bool CheckInputData() OVERRIDE;
    void InitialOutputs() OVERRIDE;
    int Execute() OVERRIDE;
    void GetValue(const char* key, float* value) OVERRIDE;
    void Get1DData(const char* key, int* n, float** data) OVERRIDE;
    TimeStepType GetTimeStepType() OVERRIDE { return TIMESTEP_CHANNEL; }

private:
    struct Reach {
        double length;
        double bottomWidth;
        double sideSlope;
        double manningsN;
        double bed;
        double bedDown;
        int downstream;
    };
    void BuildNetwork();
    double Area(int reach, double depth) const;
    double Depth(int reach, double volume) const;
    double Discharge(int reach, double timestep) const;
    void Step(double timestep, const std::vector<double>& lateral);
    int m_dt;
    int m_inputSubbasinID;
    int m_outletID;
    int m_nreach;
    clsReaches* m_reaches;
    float* m_surface;
    float* m_interflow;
    float* m_groundwater;
    std::vector<Reach> m_network;
    std::unique_ptr<NodeReachNetwork> m_graph;
    std::vector<double> m_volume;
    std::vector<double> m_flow;
    std::vector<float> m_discharge;
    std::vector<float> m_depth;
};

#endif
