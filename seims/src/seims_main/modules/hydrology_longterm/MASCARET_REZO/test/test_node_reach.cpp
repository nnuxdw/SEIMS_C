#include "NodeReachNetwork.h"

#include <cmath>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 5) return 2;
    NodeReachNetwork network;
    std::vector<NodeReachNetwork::Geometry> geometry(7, {100.0, 5.0, 1.0, 0.03});
    network.Load(argv[1], argv[2], geometry);
    std::vector<double> lateral(7, 0.0);
    lateral[1] = 1.0;
    double outlet = 0.0;
    for (int step = 0; step < 200; ++step) {
        network.Advance(1.0, lateral);
        outlet += network.Flow(6);
        for (int reach = 1; reach <= 6; ++reach) {
            if (!std::isfinite(network.Flow(reach)) || !std::isfinite(network.Depth(reach))) return 3;
        }
    }
    const double error = std::abs(network.TotalInternalVolume() + outlet - 200.0);
    std::cout << "mass_error=" << error << " outlet=" << outlet << std::endl;
    NodeReachNetwork reverse;
    reverse.Load(argv[3], argv[4], std::vector<NodeReachNetwork::Geometry>(2, {100.0, 5.0, 1.0, 0.03}));
    reverse.Advance(1.0, std::vector<double>(2, 0.0));
    std::cout << "reverse_flow=" << reverse.Flow(1) << std::endl;
    return error < 1.e-7 && network.Flow(2) > 0.0 && network.Flow(3) > 0.0 &&
        reverse.Flow(1) < 0.0 && reverse.TotalInternalVolume() > 0.0 ? 0 : 1;
}
