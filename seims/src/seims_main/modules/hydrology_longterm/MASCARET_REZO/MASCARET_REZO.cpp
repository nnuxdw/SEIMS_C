#include "MASCARET_REZO.h"
#include "NodeReachNetwork.h"
#include "clsReach.h"
#include "text.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <utility>

using std::string;

namespace {
const double gravity = 9.81;
const double minimumDepth = 1.e-6;
}

MASCARET_REZO::MASCARET_REZO() : m_dt(-1), m_inputSubbasinID(-1), m_outletID(-1),
    m_nreach(-1), m_reaches(nullptr), m_surface(nullptr), m_interflow(nullptr),
    m_groundwater(nullptr) {}

MASCARET_REZO::~MASCARET_REZO() {}

void MASCARET_REZO::SetValue(const char* key, float value) {
    if (StringMatch(key, Tag_ChannelTimeStep)) m_dt = CVT_INT(value);
    else if (StringMatch(key, Tag_SubbasinId)) m_inputSubbasinID = CVT_INT(value);
    else if (StringMatch(key, VAR_OUTLETID)) m_outletID = CVT_INT(value);
    else throw ModelException("MASCARET_REZO", "SetValue", string("Unknown parameter: ") + key);
}

void MASCARET_REZO::Set1DData(const char* key, int n, float* data) {
    if (StringMatch(key, VAR_SBOF) || StringMatch(key, VAR_SBIF) || StringMatch(key, VAR_SBQG)) {
        CheckInputSize("MASCARET_REZO", key, n - 1, m_nreach);
        if (StringMatch(key, VAR_SBOF)) m_surface = data;
        else if (StringMatch(key, VAR_SBIF)) m_interflow = data;
        else m_groundwater = data;
    } else throw ModelException("MASCARET_REZO", "Set1DData", string("Unknown input: ") + key);
}

void MASCARET_REZO::SetReaches(clsReaches* reaches) {
    if (!reaches) throw ModelException("MASCARET_REZO", "SetReaches", "Reaches are required.");
    m_reaches = reaches;
    m_nreach = reaches->GetReachNumber();
    m_network.clear();
    m_graph.reset();
    m_volume.clear();
    m_flow.clear();
    m_discharge.clear();
    m_depth.clear();
}

bool MASCARET_REZO::CheckInputData() {
    CHECK_POSITIVE("MASCARET_REZO", m_dt);
    CHECK_POSITIVE("MASCARET_REZO", m_nreach);
    CHECK_POSITIVE("MASCARET_REZO", m_outletID);
    CHECK_POINTER("MASCARET_REZO", m_reaches);
    CHECK_POINTER("MASCARET_REZO", m_surface);
    CHECK_POINTER("MASCARET_REZO", m_interflow);
    CHECK_POINTER("MASCARET_REZO", m_groundwater);
    if (m_inputSubbasinID != 0 || m_outletID > m_nreach)
        throw ModelException("MASCARET_REZO", "CheckInputData", "Use a complete single-process network and a valid outlet ID.");
    return true;
}

void MASCARET_REZO::BuildNetwork() {
    if (m_graph || !m_network.empty()) return;
    float *width = nullptr, *bankDepth = nullptr, *length = nullptr;
    float *sideSlope = nullptr, *slope = nullptr, *manning = nullptr;
    m_reaches->GetReachesSingleProperty(REACH_WIDTH, &width);
    m_reaches->GetReachesSingleProperty(REACH_DEPTH, &bankDepth);
    m_reaches->GetReachesSingleProperty(REACH_LENGTH, &length);
    m_reaches->GetReachesSingleProperty(REACH_SIDESLP, &sideSlope);
    m_reaches->GetReachesSingleProperty(REACH_SLOPE, &slope);
    m_reaches->GetReachesSingleProperty(REACH_MANNING, &manning);
    const char* networkFile = std::getenv("WISE_REZO_NETWORK_FILE");
    const char* boundaryFile = std::getenv("WISE_REZO_BOUNDARY_FILE");
    if ((networkFile && *networkFile) != (boundaryFile && *boundaryFile))
        throw ModelException("MASCARET_REZO", "BuildNetwork", "Both node-reach network and boundary files are required.");
    if (networkFile && *networkFile) {
        std::vector<NodeReachNetwork::Geometry> geometry(m_nreach + 1);
        for (int reach = 1; reach <= m_nreach; ++reach) {
            if (!std::isfinite(width[reach]) || !std::isfinite(bankDepth[reach]) ||
                !std::isfinite(length[reach]) || !std::isfinite(sideSlope[reach]) ||
                !std::isfinite(manning[reach]) || width[reach] <= 0.0 ||
                bankDepth[reach] <= 0.0 || length[reach] <= 0.0 ||
                sideSlope[reach] < 0.0 || manning[reach] <= 0.0)
                throw ModelException("MASCARET_REZO", "BuildNetwork", "Invalid reach hydraulic geometry.");
            geometry[reach] = {length[reach],
                (std::max)(width[reach] - 2.0 * sideSlope[reach] * bankDepth[reach], 0.1),
                sideSlope[reach], manning[reach]};
        }
        std::unique_ptr<NodeReachNetwork> graph(new NodeReachNetwork());
        try {
            graph->Load(networkFile, boundaryFile, geometry);
        } catch (const std::exception& error) {
            throw ModelException("MASCARET_REZO", "BuildNetwork", error.what());
        }
        m_graph = std::move(graph);
        return;
    }
    const std::map<int, int>& downstream = m_reaches->GetDownStreamID();
    m_network.resize(m_nreach + 1);
    for (int reach = 1; reach <= m_nreach; ++reach) {
        const auto found = downstream.find(reach);
        if (found == downstream.end() || (found->second != -1 &&
            (found->second < 1 || found->second > m_nreach || found->second == reach)) ||
            !std::isfinite(width[reach]) || !std::isfinite(bankDepth[reach]) ||
            !std::isfinite(length[reach]) || !std::isfinite(sideSlope[reach]) ||
            !std::isfinite(slope[reach]) || !std::isfinite(manning[reach]) ||
            width[reach] <= 0 || bankDepth[reach] <= 0 || length[reach] <= 0 ||
            sideSlope[reach] < 0 || slope[reach] < 0 || manning[reach] <= 0)
            throw ModelException("MASCARET_REZO", "BuildNetwork", "Invalid reach geometry, roughness, or topology.");
        Reach& current = m_network[reach];
        current.length = length[reach];
        current.sideSlope = sideSlope[reach];
        current.bottomWidth = (std::max)(width[reach] - 2.0 * sideSlope[reach] * bankDepth[reach], 0.1);
        current.manningsN = manning[reach];
        current.bed = slope[reach] * length[reach];
        current.bedDown = 0.0;
        current.downstream = found->second;
    }
    std::vector<int> visit(m_nreach + 1, 0);
    std::function<double(int)> bedAt = [&](int reach) -> double {
        if (visit[reach] == 1) throw ModelException("MASCARET_REZO", "BuildNetwork", "River network contains a cycle.");
        if (visit[reach] == 2) return m_network[reach].bed;
        visit[reach] = 1;
        Reach& current = m_network[reach];
        current.bedDown = current.downstream == -1 ? 0.0 : bedAt(current.downstream);
        current.bed += current.bedDown;
        visit[reach] = 2;
        return current.bed;
    };
    for (int reach = 1; reach <= m_nreach; ++reach) bedAt(reach);
    m_volume.assign(m_nreach + 1, 0.0);
    m_flow.assign(m_nreach + 1, 0.0);
}

double MASCARET_REZO::Area(int reach, double depth) const {
    const Reach& current = m_network[reach];
    return depth * (current.bottomWidth + current.sideSlope * depth);
}

double MASCARET_REZO::Depth(int reach, double volume) const {
    const Reach& current = m_network[reach];
    const double area = (std::max)(0.0, volume) / current.length;
    if (current.sideSlope == 0.0) return area / current.bottomWidth;
    return 2.0 * area / (current.bottomWidth +
        std::sqrt(current.bottomWidth * current.bottomWidth + 4.0 * current.sideSlope * area));
}

double MASCARET_REZO::Discharge(int reach, double timestep) const {
    const Reach& current = m_network[reach];
    const double depth = Depth(reach, m_volume[reach]);
    if (depth <= minimumDepth) return 0.0;
    const double area = Area(reach, depth);
    const double perimeter = current.bottomWidth + 2.0 * depth * std::sqrt(1.0 + current.sideSlope * current.sideSlope);
    const double radius = area / perimeter;
    const double downstreamDepth = current.downstream == -1 ? depth : Depth(current.downstream, m_volume[current.downstream]);
    const double headGradient = (current.bed + depth - current.bedDown - downstreamDepth) / current.length;
    const double acceleration = timestep * gravity * area;
    const double friction = current.manningsN * current.manningsN /
        (area * area * std::pow(radius, 4.0 / 3.0));
    const double momentum = m_flow[reach] + acceleration * headGradient;
    const double damping = acceleration * friction;
    return 2.0 * momentum / (1.0 + std::sqrt(1.0 + 4.0 * damping * std::abs(momentum)));
}

void MASCARET_REZO::Step(double timestep, const std::vector<double>& lateral) {
    std::vector<double> proposed(m_nreach + 1, 0.0);
    std::vector<double> demand(m_nreach + 1, 0.0);
    std::vector<double> available(m_nreach + 1, 0.0);
    for (int reach = 1; reach <= m_nreach; ++reach) {
        proposed[reach] = Discharge(reach, timestep);
        const int source = proposed[reach] >= 0.0 ? reach : m_network[reach].downstream;
        if (source > 0) demand[source] += timestep * std::abs(proposed[reach]);
        available[reach] = m_volume[reach] + timestep * lateral[reach];
    }
    std::vector<double> sourceScale(m_nreach + 1, 1.0);
    for (int reach = 1; reach <= m_nreach; ++reach) {
        if (demand[reach] > available[reach]) sourceScale[reach] = available[reach] / demand[reach];
    }
    for (int reach = 1; reach <= m_nreach; ++reach) {
        const int downstream = m_network[reach].downstream;
        const int source = proposed[reach] >= 0.0 ? reach : downstream;
        if (source > 0) proposed[reach] *= sourceScale[source];
        const double transfer = timestep * proposed[reach];
        available[reach] -= transfer;
        if (downstream > 0) available[downstream] += transfer;
    }
    for (int reach = 1; reach <= m_nreach; ++reach) {
        m_volume[reach] = (std::max)(0.0, available[reach]);
        m_flow[reach] = proposed[reach];
    }
}

void MASCARET_REZO::InitialOutputs() {
    if (!m_discharge.empty()) return;
    m_discharge.assign(m_nreach + 1, 0.f);
    m_depth.assign(m_nreach + 1, 0.f);
}

int MASCARET_REZO::Execute() {
    CheckInputData();
    InitialOutputs();
    BuildNetwork();
    std::vector<double> lateral(m_nreach + 1, 0.0);
    for (int reach = 1; reach <= m_nreach; ++reach) {
        const double surface = m_surface[reach], interflow = m_interflow[reach], groundwater = m_groundwater[reach];
        if (!std::isfinite(surface) || !std::isfinite(interflow) || !std::isfinite(groundwater) ||
            surface < 0.0 || interflow < 0.0 || groundwater < 0.0)
            throw ModelException("MASCARET_REZO", "Execute", "Lateral inflow must be finite and nonnegative.");
        lateral[reach] = surface + interflow + groundwater;
    }
    if (m_graph) {
        try {
            m_graph->Advance(m_dt, lateral);
        } catch (const std::exception& error) {
            throw ModelException("MASCARET_REZO", "Execute", error.what());
        }
        for (int reach = 1; reach <= m_nreach; ++reach) {
            m_discharge[reach] = static_cast<float>(m_graph->Flow(reach));
            m_depth[reach] = static_cast<float>(m_graph->Depth(reach));
        }
        return 0;
    }
    double remaining = m_dt;
    int substeps = 0;
    while (remaining > 0.0) {
        double timestep = remaining;
        for (int reach = 1; reach <= m_nreach; ++reach) {
            const double depth = Depth(reach, m_volume[reach] + remaining * lateral[reach]);
            const double area = Area(reach, (std::max)(depth, minimumDepth));
            const double speed = std::abs(m_flow[reach]) / area + std::sqrt(gravity * depth);
            if (speed > 0.0) timestep = (std::min)(timestep, 0.4 * m_network[reach].length / speed);
        }
        if (++substeps > 100000 || !std::isfinite(timestep) || timestep <= 0.0)
            throw ModelException("MASCARET_REZO", "Execute", "Hydraulic substepping failed; check reach geometry and time step.");
        Step(timestep, lateral);
        remaining -= timestep;
        if (remaining < m_dt * 1.e-12) remaining = 0.0;
    }
    for (int reach = 1; reach <= m_nreach; ++reach) {
        m_discharge[reach] = static_cast<float>(m_flow[reach]);
        m_depth[reach] = static_cast<float>(Depth(reach, m_volume[reach]));
    }
    return 0;
}

void MASCARET_REZO::GetValue(const char* key, float* value) {
    if (!StringMatch(key, VAR_QRECH)) throw ModelException("MASCARET_REZO", "GetValue", string("Unknown output: ") + key);
    InitialOutputs();
    *value = m_discharge[m_outletID];
}

void MASCARET_REZO::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    *n = m_nreach + 1;
    if (StringMatch(key, VAR_QRECH)) *data = m_discharge.data();
    else if (StringMatch(key, VAR_CHWTRDEPTH)) *data = m_depth.data();
    else throw ModelException("MASCARET_REZO", "Get1DData", string("Unknown output: ") + key);
}
