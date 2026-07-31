/*!
 * \file SWMMDynamicWave.h
 * \brief SWMM 5.2 dynamic-wave river-network routing module for WISE.
 *
 * The numerical core is a SI-units migration of the conduit parts of EPA
 * SWMM 5.2.4's dynwave.c and dwflow.c. WISE provides lateral inflows, while
 * this DLL loads its own node/link network exactly once from MongoDB.
 */
#ifndef SEIMS_MODULE_SWMM_DYNAMIC_WAVE_H
#define SEIMS_MODULE_SWMM_DYNAMIC_WAVE_H

#include <map>
#include <vector>

#include "SimulationModule.h"
#include "db_mongoc.h"

using namespace db_mongoc;

class SWMMDynamicWave : public SimulationModule {
public:
    SWMMDynamicWave();
    ~SWMMDynamicWave() OVERRIDE {}

    int Execute() OVERRIDE;
    void SetValue(const char* key, float data) OVERRIDE;
    void Set1DData(const char* key, int n, float* data) OVERRIDE;
    void Get1DData(const char* key, int* n, float** data) OVERRIDE;
    void SetMongoDBContext(MongoClient* client, const string& db_name) OVERRIDE;
    TimeStepType GetTimeStepType() OVERRIDE { return TIMESTEP_CHANNEL; }
    bool CheckInputData() OVERRIDE;

private:
    enum CrossSectionShape {
        RECT_OPEN = 0,
        TRAPEZOID = 1,
        CIRCULAR = 2,
        RECT_CLOSED = 3
    };

    struct NodeDefinition {
        int id;
        int subbasin_id;
        double invert_elev;
        double max_depth;
        double init_depth;
        double surcharge_depth;
        double ponded_area;
        double boundary_stage;
        bool is_outfall;
    };

    struct ReachDefinition {
        int id;
        int subbasin_id;
        int from_node;
        int to_node;
        int shape;
        int barrels;
        double length;
        double manning_n;
        double inlet_offset;
        double outlet_offset;
        double full_depth;
        double bottom_width;
        double side_slope;
        double init_flow;
        double max_flow;
        double inlet_loss;
        double outlet_loss;
        double average_loss;
    };

    struct NodeState {
        NodeDefinition data;
        double crown_depth;
        double old_depth;
        double new_depth;
        double old_net_inflow;
        double inflow;
        double outflow;
        double surface_area;
        double sum_dqdh;
        double overflow;
        double d_ydt;
    };

    struct LinkState {
        ReachDefinition data;
        int node1;
        int node2;
        double old_flow;
        double new_flow;
        double old_area;
        double new_area;
        double dqdh;
        double froude;
        double volume;
        double surface_area1;
        double surface_area2;
    };

    void LoadNetwork(MongoClient* client, const string& db_name);
    void InitializeStates();
    void RouteTimeStep(double dt);
    void InitializeIteration();
    void SolveLink(LinkState& link, int iteration, double dt);
    bool SolveNodeDepths(int iteration, double dt);
    double StableTimeStep(double maximum_step) const;
    double LateralInflow(const NodeState& node) const;
    void UpdateOutputArrays();

    bool IsClosed(const LinkState& link) const;
    double FlowArea(const LinkState& link, double depth) const;
    double TopWidth(const LinkState& link, double depth) const;
    double HydraulicRadius(const LinkState& link, double depth) const;
    double SlotWidth(const LinkState& link, double depth) const;
    double FullArea(const LinkState& link) const;

    bool network_loaded_;
    int max_subbasin_id_;
    int subbasin_count_;
    int max_trials_;
    double dt_channel_;
    double head_tolerance_;
    double min_route_step_;
    double courant_factor_;
    double min_surface_area_;
    std::vector<NodeState> nodes_;
    std::vector<LinkState> links_;

    float* q_surf_;
    float* q_interflow_;
    float* q_groundwater_;
    std::vector<float> q_reach_;
    std::vector<float> node_depth_;
    std::vector<float> node_head_;
    std::vector<float> node_overflow_;
    std::vector<float> link_flow_;
    std::vector<float> link_velocity_;
};

#endif  // SEIMS_MODULE_SWMM_DYNAMIC_WAVE_H
