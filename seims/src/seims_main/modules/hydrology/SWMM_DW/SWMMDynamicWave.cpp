/*
 * Dynamic-wave solver derived from the conduit-routing portions of:
 *   EPA SWMM 5.2.4, dynwave.c and dwflow.c (July 2023).
 * This adaptation keeps the numerical scheme but uses SI units and WISE
 * node/link records instead of SWMM's global project objects.
 */
#include "SWMMDynamicWave.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "text.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifdef PI
#undef PI
#endif

namespace {
const double GRAVITY = 9.80665;
const double EPSILON = 1.e-6;
const double OMEGA = 0.5;
const double MAX_VELOCITY = 50.0;
const double kPi = 3.14159265358979323846;

bool GetNumericField(const bson_t* document, const char* const* names,
                     const size_t count, double& value) {
    bson_iter_t iterator;
    float number = 0.f;
    for (size_t i = 0; i < count; ++i) {
        if (bson_iter_init_find(&iterator, document, names[i]) &&
            GetNumericFromBsonIterator(&iterator, number)) {
            value = number;
            return true;
        }
    }
    return false;
}

double ReadRequired(const bson_t* document, const char* const* names,
                    const size_t count, const string& collection, const string& field) {
    double value = 0.0;
    if (!GetNumericField(document, names, count, value)) {
        throw ModelException("SWMM_DW", "LoadNetwork",
                             "Required numeric field " + field + " is missing in " + collection + ".");
    }
    return value;
}

double ReadOptional(const bson_t* document, const char* const* names,
                    const size_t count, const double default_value) {
    double value = default_value;
    GetNumericField(document, names, count, value);
    return value;
}

string FindNetworkCollection(MongoClient* client, const string& db_name,
                             const std::vector<string>& candidates, const string& role) {
    std::unique_ptr<MongoDatabase> database(new MongoDatabase(client->GetDatabase(db_name)));
    std::vector<string> available;
    database->GetCollectionNames(available);
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (ValueInVector(candidates[i], available)) return candidates[i];
    }
    throw ModelException("SWMM_DW", "LoadNetwork", "No " + role +
                         " collection found. Expected one of: " + candidates[0] + ".");
}
}

SWMMDynamicWave::SWMMDynamicWave() : network_loaded_(false), max_subbasin_id_(0),
    subbasin_count_(0), max_trials_(8),             // Default: eight Picard iterations per substep.
    dt_channel_(-1.0),                               // Must be set by WISE in seconds before Execute().
    head_tolerance_(0.001),                          // Default convergence tolerance: 1 mm.
    min_route_step_(1.0),                            // Default smallest internal dynamic-wave step: 1 s.
    courant_factor_(0.75),                           // Default Courant safety factor; 0 disables substepping.
    min_surface_area_(1.167),                        // SWMM default node area (4-ft diameter) converted to m2.
    q_surf_(nullptr), q_interflow_(nullptr), q_groundwater_(nullptr) {
}

void SWMMDynamicWave::SetMongoDBContext(MongoClient* client, const string& db_name) {
    if (network_loaded_) return;
    if (nullptr == client || db_name.empty()) {
        throw ModelException("SWMM_DW", "SetMongoDBContext", "The active MongoDB context is invalid.");
    }
    LoadNetwork(client, db_name);
    network_loaded_ = true;
}

void SWMMDynamicWave::LoadNetwork(MongoClient* client, const string& db_name) {
    const string node_table_name = FindNetworkCollection(client, db_name,
        std::vector<string>{"SWMM_NODES", "SWMM_NODE", "RIVER_NODES"}, "node");
    const string reach_table_name = FindNetworkCollection(client, db_name,
        std::vector<string>{"SWMM_REACHES", "SWMM_LINKS", "RIVER_REACHES", "RIVER_LINKS"}, "reach");
    const char* node_id_names[] = {"NODE_ID", "ID"};
    const char* node_subbasin_names[] = {"SUBBASINID", "SUBBASIN_ID"};
    const char* node_invert_names[] = {"INVERT_ELEV", "INVERT", "ELEVATION"};
    const char* node_max_depth_names[] = {"MAX_DEPTH", "FULL_DEPTH"};
    const char* node_init_depth_names[] = {"INIT_DEPTH", "INITIAL_DEPTH"};
    const char* node_surcharge_names[] = {"SURCHARGE_DEPTH", "SUR_DEPTH"};
    const char* node_ponded_names[] = {"PONDED_AREA", "SURFACE_AREA", "AREA"};
    const char* node_stage_names[] = {"BOUNDARY_STAGE", "STAGE", "OUTFALL_STAGE"};
    const char* node_type_names[] = {"NODE_TYPE", "IS_OUTFALL", "OUTFALL"};

    std::map<int, int> node_index;
    std::unique_ptr<MongoCollection> node_table(
        new MongoCollection(client->GetCollection(db_name, node_table_name)));
    bson_t* query = bson_new();
    mongoc_cursor_t* cursor = node_table->ExecuteQuery(query);
    const bson_t* document = nullptr;
    while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &document)) {
        NodeDefinition node;
        node.id = static_cast<int>(ReadRequired(document, node_id_names, 2, node_table_name, "NODE_ID"));
        node.subbasin_id = static_cast<int>(ReadOptional(document, node_subbasin_names, 2, 0.0));
        // SUBBASINID=0 (the default) means the node receives no SBOF/SBIF/SBQG lateral inflow.
        node.invert_elev = ReadRequired(document, node_invert_names, 3, node_table_name, "INVERT_ELEV");
        // INVERT_ELEV is mandatory and is the node bed elevation in metres.
        node.has_max_depth = GetNumericField(document, node_max_depth_names, 2, node.max_depth);
        if (!node.has_max_depth) node.max_depth = 0.0; // Later derived from the highest connected channel top.
        node.init_depth = ReadOptional(document, node_init_depth_names, 2, 0.0); // Default: dry node.
        node.surcharge_depth = ReadOptional(document, node_surcharge_names, 2, 0.0); // Default: no pressure surcharge.
        node.ponded_area = ReadOptional(document, node_ponded_names, 3, 0.0); // Default: use SWMM_MIN_SURFACE_AREA.
        node.boundary_stage = ReadOptional(document, node_stage_names, 3, node.invert_elev);
        // BOUNDARY_STAGE defaults to the invert elevation and is used only by NODE_TYPE=1.
        const double node_type = ReadOptional(document, node_type_names, 3, 0.0);
        node.is_outfall = node_type == 1.0; // NODE_TYPE=0: junction; NODE_TYPE=1: fixed-stage outfall.
        if (node.max_depth < 0.0 || node.init_depth < 0.0 || node.ponded_area < 0.0 ||
            (node_type != 0.0 && node_type != 1.0) ||
            node_index.find(node.id) != node_index.end()) {
            bson_destroy(query);
            mongoc_cursor_destroy(cursor);
            throw ModelException("SWMM_DW", "LoadNetwork", "Invalid or duplicate NODE_ID " + std::to_string(node.id) + ".");
        }
        NodeState state;
        state.data = node;
        state.crown_depth = 0.0;
        state.old_depth = 0.0;
        state.new_depth = 0.0;
        state.old_net_inflow = 0.0;
        state.inflow = 0.0;
        state.outflow = 0.0;
        state.surface_area = min_surface_area_;
        state.sum_dqdh = 0.0;
        state.overflow = 0.0;
        state.d_ydt = 0.0;
        node_index[node.id] = static_cast<int>(nodes_.size());
        nodes_.push_back(state);
        max_subbasin_id_ = std::max(max_subbasin_id_, node.subbasin_id);
    }
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    if (nodes_.empty()) {
        throw ModelException("SWMM_DW", "LoadNetwork", node_table_name + " contains no valid records.");
    }
    std::sort(nodes_.begin(), nodes_.end(), [](const NodeState& left, const NodeState& right) {
        return left.data.id < right.data.id;
    });
    node_index.clear();
    for (size_t i = 0; i < nodes_.size(); ++i) node_index[nodes_[i].data.id] = static_cast<int>(i);

    const char* reach_id_names[] = {"REACH_ID", "LINK_ID", "ID"};
    const char* reach_subbasin_names[] = {"SUBBASINID", "SUBBASIN_ID"};
    const char* from_node_names[] = {"FROM_NODE", "FROM_NODE_ID", "NODE1"};
    const char* to_node_names[] = {"TO_NODE", "TO_NODE_ID", "NODE2"};
    const char* shape_names[] = {"SHAPE", "SHAPE_CODE", "XSECT_TYPE"};
    const char* barrels_names[] = {"BARRELS", "NUM_BARRELS"};
    const char* length_names[] = {"LENGTH", "CH_LEN"};
    const char* manning_names[] = {"MANNING_N", "CH_N", "ROUGHNESS"};
    const char* inlet_offset_names[] = {"INLET_OFFSET", "OFFSET1", "IN_OFFSET"};
    const char* outlet_offset_names[] = {"OUTLET_OFFSET", "OFFSET2", "OUT_OFFSET"};
    const char* depth_names[] = {"FULL_DEPTH", "DEPTH", "GEOM1", "DIAMETER"};
    const char* width_names[] = {"BOTTOM_WIDTH", "WIDTH", "GEOM2"};
    const char* side_slope_names[] = {"SIDE_SLOPE", "SIDE_SLOPES", "GEOM3"};
    const char* init_flow_names[] = {"INIT_FLOW", "INITIAL_FLOW"};
    const char* max_flow_names[] = {"MAX_FLOW", "QMAX", "Q_LIMIT"};
    const char* inlet_loss_names[] = {"INLET_LOSS", "LOSS_INLET"};
    const char* outlet_loss_names[] = {"OUTLET_LOSS", "LOSS_OUTLET"};
    const char* average_loss_names[] = {"AVERAGE_LOSS", "LOSS_AVERAGE"};

    std::unique_ptr<MongoCollection> reach_table(
        new MongoCollection(client->GetCollection(db_name, reach_table_name)));
    std::map<int, bool> reach_ids;
    query = bson_new();
    cursor = reach_table->ExecuteQuery(query);
    while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &document)) {
        ReachDefinition reach;
        reach.id = static_cast<int>(ReadRequired(document, reach_id_names, 3, reach_table_name, "REACH_ID"));
        reach.subbasin_id = static_cast<int>(ReadOptional(document, reach_subbasin_names, 2, 0.0));
        reach.from_node = static_cast<int>(ReadRequired(document, from_node_names, 3, reach_table_name, "FROM_NODE"));
        reach.to_node = static_cast<int>(ReadRequired(document, to_node_names, 3, reach_table_name, "TO_NODE"));
        // SHAPE=0: open rectangle; 1: open trapezoid (default); 2: circular conduit;
        // 3: closed rectangle. Use 0 or 1 for rivers and open channels.
        reach.shape = static_cast<int>(ReadOptional(document, shape_names, 3, 1.0));
        reach.barrels = static_cast<int>(ReadOptional(document, barrels_names, 2, 1.0)); // Default: one passage.
        reach.length = ReadRequired(document, length_names, 2, reach_table_name, "LENGTH"); // Metres; mandatory.
        reach.manning_n = ReadRequired(document, manning_names, 3, reach_table_name, "MANNING_N"); // Dimensionless; mandatory.
        reach.inlet_offset = ReadOptional(document, inlet_offset_names, 3, 0.0); // [m] above FROM_NODE invert; default 0.
        reach.outlet_offset = ReadOptional(document, outlet_offset_names, 3, 0.0); // [m] above TO_NODE invert; default 0.
        reach.full_depth = ReadRequired(document, depth_names, 4, reach_table_name, "FULL_DEPTH");
        // FULL_DEPTH [m] is bankfull depth for open channels, diameter for SHAPE=2, and height for SHAPE=3.
        if (reach.shape == CIRCULAR) {
            reach.bottom_width = reach.full_depth; // SHAPE=2: width is not used; FULL_DEPTH is the diameter.
        } else {
            reach.bottom_width = ReadRequired(document, width_names, 3, reach_table_name, "BOTTOM_WIDTH");
            // BOTTOM_WIDTH [m] is mandatory for SHAPE=0, 1, and 3; no synthetic river width is assumed.
        }
        reach.side_slope = ReadOptional(document, side_slope_names, 3,
                                        reach.shape == TRAPEZOID ? 2.0 : 0.0);
        // SIDE_SLOPE is one-bank H:V. For SHAPE=1 it defaults to 2.0 (2H:1V); it is ignored otherwise.
        reach.init_flow = ReadOptional(document, init_flow_names, 2, 0.0); // [m3/s], signed FROM_NODE -> TO_NODE; default 0.
        reach.max_flow = ReadOptional(document, max_flow_names, 3, 0.0); // [m3/s], 0 means no flow cap.
        reach.inlet_loss = ReadOptional(document, inlet_loss_names, 2, 0.0); // Dimensionless inlet K; default 0.
        reach.outlet_loss = ReadOptional(document, outlet_loss_names, 2, 0.0); // Dimensionless outlet K; default 0.
        reach.average_loss = ReadOptional(document, average_loss_names, 2, 0.0); // Dimensionless distributed K; default 0.
        if (node_index.find(reach.from_node) == node_index.end() ||
            node_index.find(reach.to_node) == node_index.end() || reach.length <= 0.0 ||
            reach.manning_n <= 0.0 || reach.full_depth <= 0.0 || reach.bottom_width <= 0.0 ||
            reach.barrels <= 0 || reach.side_slope < 0.0 || reach.shape < RECT_OPEN || reach.shape > RECT_CLOSED ||
            reach_ids.find(reach.id) != reach_ids.end()) {
            bson_destroy(query);
            mongoc_cursor_destroy(cursor);
            throw ModelException("SWMM_DW", "LoadNetwork", "Invalid " + reach_table_name +
                                 " record " + std::to_string(reach.id) + ".");
        }
        LinkState state;
        state.data = reach;
        state.node1 = node_index.at(reach.from_node);
        state.node2 = node_index.at(reach.to_node);
        state.old_flow = reach.init_flow;
        state.new_flow = reach.init_flow;
        state.old_area = 0.0;
        state.new_area = 0.0;
        state.dqdh = 0.0;
        state.froude = 0.0;
        state.volume = 0.0;
        state.surface_area1 = 0.0;
        state.surface_area2 = 0.0;
        links_.push_back(state);
        reach_ids[reach.id] = true;
        max_subbasin_id_ = std::max(max_subbasin_id_, reach.subbasin_id);
    }
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    if (links_.empty()) {
        throw ModelException("SWMM_DW", "LoadNetwork", reach_table_name + " contains no valid records.");
    }
    std::sort(links_.begin(), links_.end(), [](const LinkState& left, const LinkState& right) {
        return left.data.id < right.data.id;
    });
    InitializeStates();
}

void SWMMDynamicWave::InitializeStates() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        NodeState& node = nodes_[i];
        node.crown_depth = 0.0;
        node.old_net_inflow = 0.0;
    }
    for (size_t i = 0; i < links_.size(); ++i) {
        LinkState& link = links_[i];
        link.old_area = std::max(EPSILON, FlowArea(link, 0.5 * link.data.full_depth));
        link.new_area = link.old_area;
        link.volume = link.old_area * link.data.length * link.data.barrels;
        nodes_[link.node1].crown_depth = std::max(nodes_[link.node1].crown_depth,
                                                   link.data.inlet_offset + link.data.full_depth);
        nodes_[link.node2].crown_depth = std::max(nodes_[link.node2].crown_depth,
                                                   link.data.outlet_offset + link.data.full_depth);
    }
    for (size_t i = 0; i < nodes_.size(); ++i) {
        NodeState& node = nodes_[i];
        // For an open-channel junction without a supplied bank depth, use the
        // highest connected channel top (the same crown reference SWMM builds
        // from conduit offsets and full depths). An explicit MAX_DEPTH always
        // takes precedence.
        if (!node.data.has_max_depth) node.data.max_depth = std::max(EPSILON, node.crown_depth);
        node.old_depth = std::min(node.data.init_depth, node.data.max_depth + node.data.surcharge_depth);
        node.new_depth = node.old_depth;
        if (node.data.is_outfall) {
            node.old_depth = std::max(0.0, node.data.boundary_stage - node.data.invert_elev);
            node.new_depth = node.old_depth;
        }
    }
    q_reach_.assign(max_subbasin_id_ + 1, 0.f);
    node_depth_.assign(nodes_.size(), 0.f);
    node_head_.assign(nodes_.size(), 0.f);
    node_overflow_.assign(nodes_.size(), 0.f);
    link_flow_.assign(links_.size(), 0.f);
    link_velocity_.assign(links_.size(), 0.f);
}

bool SWMMDynamicWave::CheckInputData() {
    if (!network_loaded_) throw ModelException("SWMM_DW", "CheckInputData", "MongoDB network context was not set.");
    if (dt_channel_ <= 0.0) throw ModelException("SWMM_DW", "CheckInputData", "Channel time step was not set.");
    if (nullptr == q_surf_ || nullptr == q_interflow_ || nullptr == q_groundwater_) {
        throw ModelException("SWMM_DW", "CheckInputData", "SBOF, SBIF and SBQG inputs must all be set.");
    }
    if (subbasin_count_ <= 0) throw ModelException("SWMM_DW", "CheckInputData", "Invalid lateral-inflow array length.");
    return true;
}

void SWMMDynamicWave::SetValue(const char* key, const float data) {
    const string name(key);
    if (StringMatch(name, Tag_ChannelTimeStep)) dt_channel_ = data; // Mandatory channel interval [s]; must be > 0.
    else if (StringMatch(name, "SWMM_MAX_TRIALS")) max_trials_ = std::max(1, static_cast<int>(data));
    // SWMM_MAX_TRIALS: Picard iterations per internal step; integer >= 1, default 8.
    else if (StringMatch(name, "SWMM_HEAD_TOL")) head_tolerance_ = std::max(EPSILON, static_cast<double>(data));
    // SWMM_HEAD_TOL: node-depth convergence tolerance [m], > 0, default 0.001 m.
    else if (StringMatch(name, "SWMM_MIN_ROUTE_STEP")) min_route_step_ = std::max(0.001, static_cast<double>(data));
    // SWMM_MIN_ROUTE_STEP: lower bound for internal step [s], >= 0.001, default 1 s.
    else if (StringMatch(name, "SWMM_COURANT_FACTOR")) courant_factor_ = std::max(0.0, static_cast<double>(data));
    // SWMM_COURANT_FACTOR: 0 disables Courant substepping; positive values scale it, default 0.75.
    else if (StringMatch(name, "SWMM_MIN_SURFACE_AREA")) min_surface_area_ = std::max(EPSILON, static_cast<double>(data));
    // SWMM_MIN_SURFACE_AREA: minimum node storage area [m2], > 0, default 1.167 m2.
    else throw ModelException("SWMM_DW", "SetValue", "Unsupported parameter: " + name);
}

void SWMMDynamicWave::Set1DData(const char* key, const int n, float* data) {
    if (n <= 0 || nullptr == data) throw ModelException("SWMM_DW", "Set1DData", "Invalid lateral-inflow array.");
    if (subbasin_count_ > 0 && subbasin_count_ != n) {
        throw ModelException("SWMM_DW", "Set1DData", "SBOF, SBIF and SBQG array lengths differ.");
    }
    subbasin_count_ = n;
    const string name(key);
    if (StringMatch(name, VAR_SBOF)) q_surf_ = data; // Surface runoff [m3/s] indexed by SUBBASINID.
    else if (StringMatch(name, VAR_SBIF)) q_interflow_ = data; // Interflow [m3/s] indexed by SUBBASINID.
    else if (StringMatch(name, VAR_SBQG)) q_groundwater_ = data; // Groundwater flow [m3/s] indexed by SUBBASINID.
    else throw ModelException("SWMM_DW", "Set1DData", "Unsupported input: " + name);
    if (static_cast<int>(q_reach_.size()) < n) q_reach_.resize(n, 0.f);
}

double SWMMDynamicWave::LateralInflow(const NodeState& node) const {
    const int id = node.data.subbasin_id;
    if (id <= 0 || id >= subbasin_count_) return 0.0;
    return static_cast<double>(q_surf_[id]) + static_cast<double>(q_interflow_[id]) +
           static_cast<double>(q_groundwater_[id]);
}

void SWMMDynamicWave::InitializeIteration() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        NodeState& node = nodes_[i];
        node.inflow = LateralInflow(node);
        node.outflow = 0.0;
        node.surface_area = std::max(min_surface_area_, node.data.ponded_area);
        node.sum_dqdh = 0.0;
        node.overflow = 0.0;
        if (node.data.is_outfall) node.new_depth = std::max(0.0, node.data.boundary_stage - node.data.invert_elev);
    }
}

bool SWMMDynamicWave::IsClosed(const LinkState& link) const {
    return link.data.shape == CIRCULAR || link.data.shape == RECT_CLOSED;
}

double SWMMDynamicWave::SlotWidth(const LinkState& link, const double depth) const {
    if (!IsClosed(link) || depth < link.data.full_depth) return 0.0;
    return std::max(0.01 * link.data.bottom_width, 1.e-4);
}

double SWMMDynamicWave::FullArea(const LinkState& link) const {
    const double depth = link.data.full_depth;
    if (link.data.shape == CIRCULAR) return kPi * depth * depth / 4.0;
    if (link.data.shape == TRAPEZOID) return depth * (link.data.bottom_width + link.data.side_slope * depth);
    return link.data.bottom_width * depth;
}

double SWMMDynamicWave::FlowArea(const LinkState& link, const double supplied_depth) const {
    const double depth = std::max(0.0, supplied_depth);
    if (IsClosed(link) && depth > link.data.full_depth) {
        return FullArea(link) + (depth - link.data.full_depth) * SlotWidth(link, depth);
    }
    if (link.data.shape == CIRCULAR) {
        const double radius = link.data.full_depth / 2.0;
        if (depth >= link.data.full_depth) return kPi * radius * radius;
        const double theta = 2.0 * std::acos(std::max(-1.0, std::min(1.0, 1.0 - depth / radius)));
        return 0.5 * radius * radius * (theta - std::sin(theta));
    }
    if (link.data.shape == TRAPEZOID) return depth * (link.data.bottom_width + link.data.side_slope * depth);
    return link.data.bottom_width * depth;
}

double SWMMDynamicWave::TopWidth(const LinkState& link, const double supplied_depth) const {
    const double depth = std::max(0.0, supplied_depth);
    if (IsClosed(link) && depth >= link.data.full_depth) return SlotWidth(link, depth);
    if (link.data.shape == CIRCULAR) {
        const double radius = link.data.full_depth / 2.0;
        if (depth <= 0.0 || depth >= link.data.full_depth) return EPSILON;
        return 2.0 * std::sqrt(depth * (2.0 * radius - depth));
    }
    if (link.data.shape == TRAPEZOID) return link.data.bottom_width + 2.0 * link.data.side_slope * depth;
    return link.data.bottom_width;
}

double SWMMDynamicWave::HydraulicRadius(const LinkState& link, const double supplied_depth) const {
    const double depth = std::max(EPSILON, std::min(supplied_depth, link.data.full_depth));
    const double area = std::max(EPSILON, FlowArea(link, depth));
    if (link.data.shape == CIRCULAR) {
        const double radius = link.data.full_depth / 2.0;
        const double theta = depth >= link.data.full_depth ? 2.0 * kPi :
            2.0 * std::acos(std::max(-1.0, std::min(1.0, 1.0 - depth / radius)));
        return area / std::max(EPSILON, radius * theta);
    }
    if (link.data.shape == TRAPEZOID) {
        return area / std::max(EPSILON, link.data.bottom_width +
                               2.0 * depth * std::sqrt(1.0 + link.data.side_slope * link.data.side_slope));
    }
    if (link.data.shape == RECT_CLOSED && depth >= link.data.full_depth) {
        return area / std::max(EPSILON, 2.0 * (link.data.bottom_width + link.data.full_depth));
    }
    return area / std::max(EPSILON, link.data.bottom_width + 2.0 * depth);
}

void SWMMDynamicWave::SolveLink(LinkState& link, const int iteration, const double dt) {
    NodeState& node1 = nodes_[link.node1];
    NodeState& node2 = nodes_[link.node2];
    const double barrels = static_cast<double>(link.data.barrels);
    const double z1 = node1.data.invert_elev + link.data.inlet_offset;
    const double z2 = node2.data.invert_elev + link.data.outlet_offset;
    const double h1 = std::max(node1.data.invert_elev + node1.new_depth, z1);
    const double h2 = std::max(node2.data.invert_elev + node2.new_depth, z2);
    double y1 = std::max(EPSILON, h1 - z1);
    double y2 = std::max(EPSILON, h2 - z2);
    if (IsClosed(link)) {
        y1 = std::min(y1, link.data.full_depth + std::max(0.0, node1.data.surcharge_depth));
        y2 = std::min(y2, link.data.full_depth + std::max(0.0, node2.data.surcharge_depth));
    }
    const double a1 = std::max(EPSILON, FlowArea(link, y1));
    const double a2 = std::max(EPSILON, FlowArea(link, y2));
    const double y_mid = 0.5 * (y1 + y2);
    const double a_mid = std::max(EPSILON, FlowArea(link, y_mid));
    const double r1 = std::max(EPSILON, HydraulicRadius(link, y1));
    const double r_mid = std::max(EPSILON, HydraulicRadius(link, y_mid));
    const double q_old = link.old_flow / barrels;
    const double q_last = link.new_flow / barrels;
    const double velocity = std::max(-MAX_VELOCITY, std::min(MAX_VELOCITY, q_last / a_mid));
    const double celerity = std::sqrt(GRAVITY * a_mid / std::max(EPSILON, TopWidth(link, y_mid)));
    link.froude = std::fabs(velocity) / std::max(EPSILON, celerity);
    double sigma = 1.0;
    if (link.froude >= 1.0) sigma = 0.0;
    else if (link.froude > 0.5) sigma = 2.0 * (1.0 - link.froude);
    const bool full = y1 >= link.data.full_depth && y2 >= link.data.full_depth;
    const double rho = (!full && q_last > 0.0 && h1 >= h2) ? sigma : 1.0;
    const double a_weighted = a1 + (a_mid - a1) * rho;
    const double r_weighted = r1 + (r_mid - r1) * rho;
    const double dq1 = dt * GRAVITY * link.data.manning_n * link.data.manning_n * std::fabs(velocity) /
                       std::pow(std::max(EPSILON, r_weighted), 4.0 / 3.0);
    const double dq2 = dt * GRAVITY * a_weighted * (h2 - h1) / link.data.length;
    const double dq3 = sigma > 0.0 ? 2.0 * velocity * (a_mid - link.old_area) * sigma : 0.0;
    const double dq4 = sigma > 0.0 ? dt * velocity * velocity * (a2 - a1) / link.data.length * sigma : 0.0;
    const double local_loss = std::fabs(q_last) * (link.data.inlet_loss / a1 +
                              link.data.outlet_loss / a2 + link.data.average_loss / a_mid);
    const double dq5 = dt * local_loss / (2.0 * link.data.length);
    const double denominator = std::max(EPSILON, 1.0 + dq1 + dq5);
    double q = (q_old - dq2 + dq3 + dq4) / denominator;
    if (iteration > 0) {
        q = (1.0 - OMEGA) * q_last + OMEGA * q;
        if (q * q_last < 0.0) q = EPSILON * (q >= 0.0 ? 1.0 : -1.0);
    }
    if (link.data.max_flow > 0.0) {
        q = std::max(-link.data.max_flow / barrels, std::min(link.data.max_flow / barrels, q));
    }
    if (q > EPSILON && node1.new_depth <= EPSILON) q = EPSILON;
    if (q < -EPSILON && node2.new_depth <= EPSILON) q = -EPSILON;
    link.dqdh = GRAVITY * dt * a_weighted / link.data.length / denominator * barrels;
    link.new_flow = q * barrels;
    link.new_area = a_mid;
    link.volume = 0.5 * (a1 + a2) * link.data.length * barrels;
    const double w1 = TopWidth(link, y1);
    const double w2 = TopWidth(link, y2);
    const double w_mid = TopWidth(link, y_mid);
    link.surface_area1 = (w1 + w_mid) * link.data.length / 4.0;
    link.surface_area2 = (w_mid + w2) * link.data.length / 4.0;
}

bool SWMMDynamicWave::SolveNodeDepths(const int iteration, const double dt) {
    bool converged = true;
    for (size_t i = 0; i < nodes_.size(); ++i) {
        NodeState& node = nodes_[i];
        if (node.data.is_outfall) continue;
        const double previous = node.new_depth;
        const double d_q = node.inflow - node.outflow;
        const double d_v = 0.5 * (node.old_net_inflow + d_q) * dt;
        const bool surcharged = node.crown_depth > EPSILON && previous > node.crown_depth &&
                                node.data.surcharge_depth > 0.0;
        double depth = 0.0;
        if (surcharged && node.sum_dqdh > EPSILON) depth = previous + d_q / node.sum_dqdh;
        else {
            depth = node.old_depth + d_v / std::max(min_surface_area_, node.surface_area);
            if (iteration > 0) depth = (1.0 - OMEGA) * previous + OMEGA * depth;
        }
        const double max_depth = node.data.max_depth + node.data.surcharge_depth;
        node.overflow = 0.0;
        if (depth > max_depth) {
            node.overflow = std::max(0.0, (depth - max_depth) *
                                     std::max(min_surface_area_, node.surface_area) / dt);
            depth = max_depth;
        }
        node.new_depth = std::max(0.0, depth);
        node.d_ydt = std::fabs(node.new_depth - node.old_depth) / dt;
        if (std::fabs(previous - node.new_depth) > head_tolerance_) converged = false;
    }
    return converged;
}

void SWMMDynamicWave::RouteTimeStep(const double dt) {
    for (int iteration = 0; iteration < max_trials_; ++iteration) {
        InitializeIteration();
        for (size_t i = 0; i < links_.size(); ++i) SolveLink(links_[i], iteration, dt);
        for (size_t i = 0; i < links_.size(); ++i) {
            LinkState& link = links_[i];
            NodeState& node1 = nodes_[link.node1];
            NodeState& node2 = nodes_[link.node2];
            if (link.new_flow >= 0.0) {
                node1.outflow += link.new_flow;
                node2.inflow += link.new_flow;
            } else {
                node1.inflow -= link.new_flow;
                node2.outflow -= link.new_flow;
            }
            node1.surface_area += link.surface_area1 * link.data.barrels;
            node2.surface_area += link.surface_area2 * link.data.barrels;
            node1.sum_dqdh += link.dqdh;
            node2.sum_dqdh += link.dqdh;
        }
        if (SolveNodeDepths(iteration, dt)) break;
    }
    for (size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i].old_depth = nodes_[i].new_depth;
        nodes_[i].old_net_inflow = nodes_[i].inflow - nodes_[i].outflow;
    }
    for (size_t i = 0; i < links_.size(); ++i) {
        links_[i].old_flow = links_[i].new_flow;
        links_[i].old_area = links_[i].new_area;
    }
}

double SWMMDynamicWave::StableTimeStep(const double maximum_step) const {
    if (courant_factor_ <= 0.0) return maximum_step;
    double step = maximum_step;
    for (size_t i = 0; i < links_.size(); ++i) {
        const LinkState& link = links_[i];
        const double area = std::max(EPSILON, link.new_area);
        const double velocity = std::fabs(link.new_flow / (area * link.data.barrels));
        const double celerity = std::sqrt(GRAVITY * area /
            std::max(EPSILON, TopWidth(link, 0.5 * link.data.full_depth)));
        step = std::min(step, courant_factor_ * link.data.length / std::max(EPSILON, velocity + celerity));
    }
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].d_ydt > EPSILON) step = std::min(step, 0.5 / nodes_[i].d_ydt);
    }
    return std::max(std::min(maximum_step, step), std::min(min_route_step_, maximum_step));
}

void SWMMDynamicWave::UpdateOutputArrays() {
    std::fill(q_reach_.begin(), q_reach_.end(), 0.f);
    for (size_t i = 0; i < nodes_.size(); ++i) {
        node_depth_[i] = static_cast<float>(nodes_[i].new_depth);
        node_head_[i] = static_cast<float>(nodes_[i].data.invert_elev + nodes_[i].new_depth);
        node_overflow_[i] = static_cast<float>(nodes_[i].overflow);
    }
    for (size_t i = 0; i < links_.size(); ++i) {
        const LinkState& link = links_[i];
        link_flow_[i] = static_cast<float>(link.new_flow);
        link_velocity_[i] = static_cast<float>(link.new_flow /
            std::max(EPSILON, link.new_area * link.data.barrels));
        if (link.data.subbasin_id > 0 && link.data.subbasin_id < static_cast<int>(q_reach_.size())) {
            q_reach_[link.data.subbasin_id] += link_flow_[i];
        }
        if (nodes_[link.node2].data.is_outfall && link.new_flow > 0.0 && !q_reach_.empty()) q_reach_[0] += link_flow_[i];
        if (nodes_[link.node1].data.is_outfall && link.new_flow < 0.0 && !q_reach_.empty()) q_reach_[0] -= link_flow_[i];
    }
}

int SWMMDynamicWave::Execute() {
    CheckInputData();
    double elapsed = 0.0;
    while (elapsed < dt_channel_ - EPSILON) {
        const double dt = StableTimeStep(dt_channel_ - elapsed);
        RouteTimeStep(dt);
        elapsed += dt;
    }
    UpdateOutputArrays();
    return 0;
}

void SWMMDynamicWave::Get1DData(const char* key, int* n, float** data) {
    const string name(key);
    if (StringMatch(name, VAR_QRECH)) {
        *n = static_cast<int>(q_reach_.size());
        *data = q_reach_.data();
    } else if (StringMatch(name, "SWMM_NODE_DEPTH")) {
        *n = static_cast<int>(node_depth_.size());
        *data = node_depth_.data();
    } else if (StringMatch(name, "SWMM_NODE_HEAD")) {
        *n = static_cast<int>(node_head_.size());
        *data = node_head_.data();
    } else if (StringMatch(name, "SWMM_NODE_OVERFLOW")) {
        *n = static_cast<int>(node_overflow_.size());
        *data = node_overflow_.data();
    } else if (StringMatch(name, "SWMM_LINK_FLOW")) {
        *n = static_cast<int>(link_flow_.size());
        *data = link_flow_.data();
    } else if (StringMatch(name, "SWMM_LINK_VELOCITY")) {
        *n = static_cast<int>(link_velocity_.size());
        *data = link_velocity_.data();
    } else {
        throw ModelException("SWMM_DW", "Get1DData", "Unsupported output: " + name);
    }
}
