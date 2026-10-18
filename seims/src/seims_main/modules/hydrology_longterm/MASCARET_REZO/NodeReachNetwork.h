#ifndef SEIMS_MODULE_REZO_NODE_REACH_NETWORK_H
#define SEIMS_MODULE_REZO_NODE_REACH_NETWORK_H

#include <map>
#include <string>
#include <vector>

class NodeReachNetwork {
public:
    struct Geometry {
        double length;
        double bottomWidth;
        double sideSlope;
        double manningsN;
    };

    void Load(const std::string& networkFile, const std::string& boundaryFile,
              const std::vector<Geometry>& geometry);
    void Advance(double duration, const std::vector<double>& lateral);
    double Flow(int reach) const;
    double Depth(int reach) const;
    double TotalInternalVolume() const;

private:
    struct Node {
        int id;
        double bed;
        double stage;
        double volume;
        bool fixedStage;
        std::vector<int> incident;
    };
    struct Edge {
        int from;
        int to;
        Geometry geometry;
        double flow;
    };

    int AddNode(int id, double bed);
    double Area(const Geometry& geometry, double depth) const;
    double Storage(int node, double stage) const;
    double StageForVolume(int node, double volume) const;
    void Step(double timestep, const std::vector<double>& nodeLateral);

    std::map<int, int> m_nodeIndex;
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;
};

#endif
