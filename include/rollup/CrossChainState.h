#pragma once

#include <vector>
#include <cstddef>

struct CrossChainState {
    size_t active_chains{0};
    std::vector<double> chain_loads;
    std::vector<double> cross_chain_latencies;
    double total_throughput{0.0};
    std::vector<double> energy_distribution;

    // Default constructor
    CrossChainState() = default;

    // Constructor with initialization
    CrossChainState(
        size_t num_chains,
        double initial_throughput = 0.0,
        double initial_energy = 0.0
    ) : active_chains(num_chains),
        chain_loads(num_chains, 1.0 / num_chains),
        cross_chain_latencies(num_chains, 0.001),
        total_throughput(initial_throughput),
        energy_distribution(num_chains, initial_energy)
    {}
}; 