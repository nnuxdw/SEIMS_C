#include "NodeReachNetwork.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace {
const double gravity = 9.81;

std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) result.push_back(field);
    return result;
}

int Integer(const std::string& value) {
    size_t used = 0;
    const int result = std::stoi(value, &used);
    if (value.find_first_not_of(" \t\r", used) != std::string::npos) throw std::invalid_argument("Invalid integer field");
    return result;
}

double Number(const std::string& value) {
    size_t used = 0;
    const double result = std::stod(value, &used);
    if (value.find_first_not_of(" \t\r", used) != std::string::npos || !std::isfinite(result))
        throw std::invalid_argument("Invalid numeric field");
    return result;
}
}

int NodeReachNetwork::AddNode(int id, double bed) {
    if (id <= 0 || !std::isfinite(bed)) throw std::invalid_argument("Node ID and bed elevation must be valid");
    const auto found = m_nodeIndex.find(id);
    if (found != m_nodeIndex.end()) {
        if (std::abs(m_nodes[found->second].bed - bed) > 1.e-5)
            throw std::invalid_argument("A node has inconsistent bed elevations");
        return found->second;
    }
    const int index = static_cast<int>(m_nodes.size());
    m_nodeIndex[id] = index;
    m_nodes.push_back({id, bed, bed, 0.0, false, {}});
    return index;
}

void NodeReachNetwork::Load(const std::string& networkFile, const std::string& boundaryFile,
                            const std::vector<Geometry>& geometry) {
    m_nodeIndex.clear();
    m_nodes.clear();
    m_edges.assign(geometry.size(), {});
    if (geometry.size() < 2) throw std::invalid_argument("At least one reach is required");
    std::ifstream network(networkFile);
    if (!network) throw std::runtime_error("Cannot open node-reach network file: " + networkFile);
    std::vector<bool> seen(geometry.size(), false);
    std::string line;
    int lineNumber = 0;
    while (std::getline(network, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;
        const auto fields = Fields(line);
        if (lineNumber == 1 && fields[0] == "reach_id") continue;
        if (fields.size() != 5) throw std::invalid_argument("Network CSV requires five columns");
        const int reach = Integer(fields[0]);
        const int fromID = Integer(fields[1]);
        const int toID = Integer(fields[2]);
        if (reach <= 0 || static_cast<size_t>(reach) >= geometry.size() || seen[reach] || fromID == toID)
            throw std::invalid_argument("Network contains invalid or duplicate reach ID or a self-loop");
        const Geometry& section = geometry[reach];
        if (!std::isfinite(section.length) || !std::isfinite(section.bottomWidth) ||
            !std::isfinite(section.sideSlope) || !std::isfinite(section.manningsN) ||
            section.length <= 0.0 || section.bottomWidth <= 0.0 ||
            section.sideSlope < 0.0 || section.manningsN <= 0.0)
            throw std::invalid_argument("Invalid reach hydraulic geometry");
        const int from = AddNode(fromID, Number(fields[3]));
        const int to = AddNode(toID, Number(fields[4]));
        m_edges[reach] = {from, to, section, 0.0};
        m_nodes[from].incident.push_back(reach);
        m_nodes[to].incident.push_back(reach);
        seen[reach] = true;
    }
    for (size_t reach = 1; reach < seen.size(); ++reach) {
        if (!seen[reach]) throw std::invalid_argument("Network must list every WISE reach exactly once");
    }
    std::ifstream boundary(boundaryFile);
    if (!boundary) throw std::runtime_error("Cannot open node boundary file: " + boundaryFile);
    lineNumber = 0;
    while (std::getline(boundary, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;
        const auto fields = Fields(line);
        if (lineNumber == 1 && fields[0] == "node_id") continue;
        if (fields.size() != 2) throw std::invalid_argument("Boundary CSV requires two columns");
        const int id = Integer(fields[0]);
        const auto found = m_nodeIndex.find(id);
        if (found == m_nodeIndex.end()) throw std::invalid_argument("Boundary node is absent from the network");
        Node& node = m_nodes[found->second];
        const double stage = Number(fields[1]);
        if (node.fixedStage || stage < node.bed) throw std::invalid_argument("Invalid or duplicate fixed-stage boundary");
        node.fixedStage = true;
        node.stage = stage;
    }
    std::vector<bool> visited(m_nodes.size(), false);
    for (size_t start = 0; start < m_nodes.size(); ++start) {
        if (visited[start]) continue;
        bool hasBoundary = false;
        std::queue<int> pending;
        pending.push(static_cast<int>(start));
        visited[start] = true;
        while (!pending.empty()) {
            const int nodeIndex = pending.front();
            pending.pop();
            hasBoundary = hasBoundary || m_nodes[nodeIndex].fixedStage;
            for (int reach : m_nodes[nodeIndex].incident) {
                const Edge& edge = m_edges[reach];
                const int neighbor = edge.from == nodeIndex ? edge.to : edge.from;
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    pending.push(neighbor);
                }
            }
        }
        if (!hasBoundary) throw std::invalid_argument("Every connected network needs a fixed-stage boundary");
    }
    for (size_t node = 0; node < m_nodes.size(); ++node)
        m_nodes[node].volume = Storage(static_cast<int>(node), m_nodes[node].stage);
}

double NodeReachNetwork::Area(const Geometry& geometry, double depth) const {
    return depth * (geometry.bottomWidth + geometry.sideSlope * depth);
}

double NodeReachNetwork::Storage(int node, double stage) const {
    const double depth = (std::max)(0.0, stage - m_nodes[node].bed);
    double volume = 0.0;
    for (int reach : m_nodes[node].incident) {
        const Geometry& geometry = m_edges[reach].geometry;
        volume += 0.5 * geometry.length * Area(geometry, depth);
    }
    return volume;
}

double NodeReachNetwork::StageForVolume(int node, double volume) const {
    const double bed = m_nodes[node].bed;
    if (volume <= 0.0) return bed;
    double lower = bed;
    double upper = bed + 1.0;
    while (Storage(node, upper) < volume) {
        upper = bed + 2.0 * (upper - bed);
        if (!std::isfinite(upper)) throw std::overflow_error("Node storage overflow");
    }
    for (int iteration = 0; iteration < 55; ++iteration) {
        const double middle = 0.5 * (lower + upper);
        if (Storage(node, middle) < volume) lower = middle;
        else upper = middle;
    }
    return 0.5 * (lower + upper);
}

void NodeReachNetwork::Step(double timestep, const std::vector<double>& nodeLateral) {
    std::vector<double> proposed(m_edges.size(), 0.0);
    std::vector<double> demand(m_nodes.size(), 0.0);
    std::vector<double> available(m_nodes.size(), 0.0);
    for (size_t node = 0; node < m_nodes.size(); ++node)
        available[node] = m_nodes[node].volume + timestep * nodeLateral[node];
    for (size_t reach = 1; reach < m_edges.size(); ++reach) {
        const Edge& edge = m_edges[reach];
        const double fromDepth = (std::max)(0.0, m_nodes[edge.from].stage - m_nodes[edge.from].bed);
        const double toDepth = (std::max)(0.0, m_nodes[edge.to].stage - m_nodes[edge.to].bed);
        const double depth = 0.5 * (fromDepth + toDepth);
        if (depth <= 1.e-8) continue;
        const double area = Area(edge.geometry, depth);
        const double perimeter = edge.geometry.bottomWidth +
            2.0 * depth * std::sqrt(1.0 + edge.geometry.sideSlope * edge.geometry.sideSlope);
        const double radius = area / perimeter;
        const double acceleration = timestep * gravity * area;
        const double momentum = edge.flow + acceleration *
            (m_nodes[edge.from].stage - m_nodes[edge.to].stage) / edge.geometry.length;
        const double damping = acceleration * edge.geometry.manningsN * edge.geometry.manningsN /
            (area * area * std::pow(radius, 4.0 / 3.0));
        proposed[reach] = 2.0 * momentum /
            (1.0 + std::sqrt(1.0 + 4.0 * damping * std::abs(momentum)));
        const int source = proposed[reach] >= 0.0 ? edge.from : edge.to;
        if (!m_nodes[source].fixedStage) demand[source] += timestep * std::abs(proposed[reach]);
    }
    std::vector<double> scale(m_nodes.size(), 1.0);
    for (size_t node = 0; node < m_nodes.size(); ++node) {
        if (!m_nodes[node].fixedStage && demand[node] > available[node])
            scale[node] = available[node] / demand[node];
    }
    for (size_t reach = 1; reach < m_edges.size(); ++reach) {
        Edge& edge = m_edges[reach];
        const int source = proposed[reach] >= 0.0 ? edge.from : edge.to;
        edge.flow = proposed[reach] * scale[source];
        const double transfer = timestep * edge.flow;
        available[edge.from] -= transfer;
        available[edge.to] += transfer;
    }
    for (size_t node = 0; node < m_nodes.size(); ++node) {
        if (m_nodes[node].fixedStage) continue;
        m_nodes[node].volume = (std::max)(0.0, available[node]);
        m_nodes[node].stage = StageForVolume(static_cast<int>(node), m_nodes[node].volume);
    }
}

void NodeReachNetwork::Advance(double duration, const std::vector<double>& lateral) {
    if (lateral.size() != m_edges.size() || !std::isfinite(duration) || duration <= 0.0)
        throw std::invalid_argument("Invalid time step or lateral inflow count");
    std::vector<double> nodeLateral(m_nodes.size(), 0.0);
    for (size_t reach = 1; reach < m_edges.size(); ++reach) {
        if (!std::isfinite(lateral[reach]) || lateral[reach] < 0.0)
            throw std::invalid_argument("Lateral inflow must be finite and nonnegative");
        nodeLateral[m_edges[reach].from] += 0.5 * lateral[reach];
        nodeLateral[m_edges[reach].to] += 0.5 * lateral[reach];
    }
    double remaining = duration;
    int substeps = 0;
    while (remaining > 0.0) {
        double timestep = remaining;
        for (size_t reach = 1; reach < m_edges.size(); ++reach) {
            const Edge& edge = m_edges[reach];
            const Node& from = m_nodes[edge.from];
            const Node& to = m_nodes[edge.to];
            const double fromStage = from.fixedStage ? from.stage :
                StageForVolume(edge.from, from.volume + remaining * nodeLateral[edge.from]);
            const double toStage = to.fixedStage ? to.stage :
                StageForVolume(edge.to, to.volume + remaining * nodeLateral[edge.to]);
            const double depth = 0.5 * ((std::max)(0.0, fromStage - from.bed) +
                (std::max)(0.0, toStage - to.bed));
            const double area = Area(edge.geometry, (std::max)(depth, 1.e-6));
            const double speed = std::abs(edge.flow) / area + std::sqrt(gravity * depth);
            if (speed > 0.0) timestep = (std::min)(timestep, 0.4 * edge.geometry.length / speed);
        }
        if (++substeps > 100000 || !std::isfinite(timestep) || timestep <= 0.0)
            throw std::runtime_error("Node-reach hydraulic substepping failed");
        Step(timestep, nodeLateral);
        remaining -= timestep;
        if (remaining < duration * 1.e-12) remaining = 0.0;
    }
}

double NodeReachNetwork::Flow(int reach) const {
    if (reach <= 0 || static_cast<size_t>(reach) >= m_edges.size()) throw std::out_of_range("Reach ID");
    return m_edges[reach].flow;
}

double NodeReachNetwork::Depth(int reach) const {
    if (reach <= 0 || static_cast<size_t>(reach) >= m_edges.size()) throw std::out_of_range("Reach ID");
    const Edge& edge = m_edges[reach];
    return 0.5 * ((std::max)(0.0, m_nodes[edge.from].stage - m_nodes[edge.from].bed) +
        (std::max)(0.0, m_nodes[edge.to].stage - m_nodes[edge.to].bed));
}

double NodeReachNetwork::TotalInternalVolume() const {
    double volume = 0.0;
    for (const Node& node : m_nodes) {
        if (!node.fixedStage) volume += node.volume;
    }
    return volume;
}
