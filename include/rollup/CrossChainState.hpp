#pragma once

#include <vector>
#include <cstddef>
#include <chrono>
#include <string>
#include <numeric>
#include <algorithm>

namespace quids {
namespace rollup {

struct CrossChainState {
    // Chain metrics
    size_t active_chains{0};
    std::vector<double> chain_loads;
    std::vector<double> cross_chain_latencies;
    double total_throughput{0.0};
    std::vector<double> energy_distribution;
    
    // Additional metrics
    std::vector<double> chain_capacities;
    std::vector<double> chain_utilization;
    std::vector<size_t> pending_transactions;
    std::vector<double> error_rates;
    
    // Timing information
    std::chrono::system_clock::time_point last_update;
    std::vector<std::chrono::milliseconds> sync_intervals;
    
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
        energy_distribution(num_chains, initial_energy),
        chain_capacities(num_chains, 1000.0),
        chain_utilization(num_chains, 0.0),
        pending_transactions(num_chains, 0),
        error_rates(num_chains, 0.0),
        last_update(std::chrono::system_clock::now()),
        sync_intervals(num_chains, std::chrono::milliseconds(100))
    {}
    
    // Utility methods
    [[nodiscard]] bool is_balanced() const {
        if (chain_loads.empty()) return true;
        
        double avg_load = std::accumulate(
            chain_loads.begin(),
            chain_loads.end(),
            0.0
        ) / chain_loads.size();
        
        static constexpr double BALANCE_THRESHOLD = 0.1;  // 10% deviation allowed
        
        return std::all_of(
            chain_loads.begin(),
            chain_loads.end(),
            [avg_load](double load) {
                return std::abs(load - avg_load) <= BALANCE_THRESHOLD * avg_load;
            }
        );
    }
    
    [[nodiscard]] double get_average_latency() const {
        if (cross_chain_latencies.empty()) return 0.0;
        
        return std::accumulate(
            cross_chain_latencies.begin(),
            cross_chain_latencies.end(),
            0.0
        ) / cross_chain_latencies.size();
    }
    
    [[nodiscard]] size_t get_total_pending_transactions() const {
        return std::accumulate(
            pending_transactions.begin(),
            pending_transactions.end(),
            size_t{0}
        );
    }
    
    [[nodiscard]] double get_average_utilization() const {
        if (chain_utilization.empty()) return 0.0;
        
        const auto sum = std::accumulate(
            chain_utilization.begin(),
            chain_utilization.end(),
            0.0
        );
        return sum / chain_utilization.size();
    }
    
    void update_timing() {
        last_update = std::chrono::system_clock::now();
    }
    
    [[nodiscard]] bool needs_rebalancing() const {
        return !is_balanced() || get_average_utilization() > 0.8;  // 80% threshold
    }
    
    // Constants
    static constexpr size_t MAX_CHAINS = 100;
    static constexpr double MAX_LATENCY = 1.0;  // 1 second
    static constexpr double MIN_THROUGHPUT = 100.0;  // 100 TPS
    static constexpr double MAX_ERROR_RATE = 0.01;  // 1%
};

} // namespace rollup
} // namespace quids 