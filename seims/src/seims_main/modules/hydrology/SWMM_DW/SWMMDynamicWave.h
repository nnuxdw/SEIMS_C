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
        RECT_OPEN = 0,  // Open rectangular channel; BOTTOM_WIDTH is the bed width.
        TRAPEZOID = 1,  // Open trapezoidal channel; SIDE_SLOPE is the H:V side slope.
        CIRCULAR = 2,   // Closed circular conduit; FULL_DEPTH is the diameter.
        RECT_CLOSED = 3 // Closed rectangular conduit; BOTTOM_WIDTH x FULL_DEPTH.
    };

    struct NodeDefinition {
        int id;                   // NODE_ID: unique integer node identifier.
        int subbasin_id;          // SUBBASINID: WISE lateral-inflow index; 0 means no direct inflow.
        double invert_elev;       // INVERT_ELEV [m]: node/channel bed elevation.
        double max_depth;         // MAX_DEPTH [m]: bank/crest depth above invert.
        bool has_max_depth;       // True only when MAX_DEPTH/FULL_DEPTH is explicitly supplied.
        double init_depth;        // INIT_DEPTH [m]: initial water depth; default 0.
        double surcharge_depth;   // SURCHARGE_DEPTH [m]: permitted depth above max depth; default 0.
        double ponded_area;       // PONDED_AREA [m2]: fixed flooding/storage area; 0 uses computed minimum.
        double boundary_stage;    // BOUNDARY_STAGE [m]: fixed water-surface elevation at an outfall.
        bool is_outfall;          // NODE_TYPE: false=ordinary junction, true=fixed-stage outfall.
    };

    struct ReachDefinition {
        int id;                   // REACH_ID: unique integer reach identifier.
        int subbasin_id;          // SUBBASINID: QRECH output index; 0 omits this reach from QRECH by subbasin.
        int from_node;            // FROM_NODE: positive flow direction starts at this node.
        int to_node;              // TO_NODE: positive flow direction ends at this node.
        int shape;                // SHAPE: 0=open rectangle, 1=trapezoid, 2=circular, 3=closed rectangle.
        int barrels;              // BARRELS: number of identical parallel passages; default 1.
        double length;            // LENGTH [m]: hydraulic reach length.
        double manning_n;         // MANNING_N [-]: Manning roughness coefficient.
        double inlet_offset;      // INLET_OFFSET [m]: reach-bed elevation above FROM_NODE invert; default 0.
        double outlet_offset;     // OUTLET_OFFSET [m]: reach-bed elevation above TO_NODE invert; default 0.
        double full_depth;        // FULL_DEPTH [m]: bankfull depth or conduit diameter/height.
        double bottom_width;      // BOTTOM_WIDTH [m]: bed/rectangular width; ignored for circular reaches.
        double side_slope;        // SIDE_SLOPE [-]: one-bank H:V slope; used only by trapezoids.
        double init_flow;         // INIT_FLOW [m3/s]: signed initial discharge; default 0.
        double max_flow;          // MAX_FLOW [m3/s]: symmetric absolute flow cap; 0 disables the cap.
        double inlet_loss;        // INLET_LOSS [-]: inlet local-loss coefficient; default 0.
        double outlet_loss;       // OUTLET_LOSS [-]: outlet local-loss coefficient; default 0.
        double average_loss;      // AVERAGE_LOSS [-]: distributed local-loss coefficient; default 0.
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
    int max_trials_;              // SWMM_MAX_TRIALS: Picard iterations per internal step; default 8.
    double dt_channel_;           // CHANNEL_DT [s]: mandatory WISE channel-routing interval.
    double head_tolerance_;       // SWMM_HEAD_TOL [m]: Picard convergence tolerance; default 0.001.
    double min_route_step_;       // SWMM_MIN_ROUTE_STEP [s]: lower bound for internal substeps; default 1.
    double courant_factor_;       // SWMM_COURANT_FACTOR [-]: 0 disables Courant substepping; default 0.75.
    double min_surface_area_;     // SWMM_MIN_SURFACE_AREA [m2]: minimum node area; default 1.167.
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
