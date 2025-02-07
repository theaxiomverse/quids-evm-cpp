#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <array>
#include <sstream>
#include <iomanip>

namespace quids {
namespace rollup {

class RollupPerformanceMetrics {
public:
    // Transaction metrics
    double tx_throughput{0.0};  // Transactions per second
    uint64_t total_transactions{0};
    uint32_t pending_transactions{0};
    double avg_tx_latency{0.0};  // Average transaction latency in seconds
    double success_rate{0.0};    // Transaction success rate (0.0-1.0)
    
    // Proof generation metrics
    double proof_generation_time{0.0};  // Time in seconds
    double verification_time{0.0};      // Time in seconds
    uint32_t active_validators{0};
    uint32_t total_proofs_generated{0};
    uint32_t failed_proofs{0};
    
    // Resource usage metrics
    double quantum_energy_usage{0.0};  // Energy usage in arbitrary units
    double memory_usage{0.0};         // Memory usage in MB
    double network_bandwidth{0.0};     // Network bandwidth usage in MB/s
    double cpu_usage{0.0};            // CPU usage percentage
    double gpu_usage{0.0};            // GPU usage percentage
    
    // Chain metrics
    uint32_t block_height{0};
    double block_interval{0.0};  // Average time between blocks in seconds
    uint32_t chain_id{0};
    uint32_t total_blocks{0};
    uint32_t orphaned_blocks{0};
    
    // Time-based metrics
    std::chrono::system_clock::time_point last_update;
    std::chrono::seconds uptime{0};
    
    // Performance targets
    static constexpr double TARGET_TPS = 3000000.0;  // 3M TPS
    static constexpr double TARGET_LATENCY = 0.001;  // 1ms
    static constexpr double TARGET_ENERGY = 1000.0;  // Arbitrary units
    static constexpr double MIN_SUCCESS_RATE = 0.99; // 99% success rate
    
    // Constructors
    RollupPerformanceMetrics() = default;
    ~RollupPerformanceMetrics() = default;
    
    // Copy and move
    RollupPerformanceMetrics(const RollupPerformanceMetrics&) = default;
    RollupPerformanceMetrics& operator=(const RollupPerformanceMetrics&) = default;
    RollupPerformanceMetrics(RollupPerformanceMetrics&&) noexcept = default;
    RollupPerformanceMetrics& operator=(RollupPerformanceMetrics&&) noexcept = default;
    
    // Performance assessment
    [[nodiscard]] bool is_performance_optimal() const {
        return tx_throughput >= TARGET_TPS &&
               avg_tx_latency <= TARGET_LATENCY &&
               quantum_energy_usage <= TARGET_ENERGY &&
               success_rate >= MIN_SUCCESS_RATE;
    }
    
    [[nodiscard]] double calculate_efficiency_score() const {
        double tps_score = tx_throughput / TARGET_TPS;
        double latency_score = TARGET_LATENCY / avg_tx_latency;
        double energy_score = TARGET_ENERGY / quantum_energy_usage;
        double reliability_score = success_rate / MIN_SUCCESS_RATE;
        
        return (tps_score + latency_score + energy_score + reliability_score) / 4.0;
    }
    
    [[nodiscard]] std::string get_performance_summary() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2)
            << "TPS: " << tx_throughput
            << ", Latency: " << (avg_tx_latency * 1000.0) << "ms"
            << ", Energy: " << quantum_energy_usage
            << ", Success Rate: " << (success_rate * 100.0) << "%"
            << ", Efficiency Score: " << calculate_efficiency_score();
        return oss.str();
    }
    
    // Comparison operators
    bool operator<(const RollupPerformanceMetrics& other) const {
        return calculate_efficiency_score() < other.calculate_efficiency_score();
    }
    
    bool operator>(const RollupPerformanceMetrics& other) const {
        return calculate_efficiency_score() > other.calculate_efficiency_score();
    }
    
    bool operator==(const RollupPerformanceMetrics& other) const {
        return calculate_efficiency_score() == other.calculate_efficiency_score();
    }
    
    bool operator!=(const RollupPerformanceMetrics& other) const {
        return !(*this == other);
    }
    
    // Static factory methods
    [[nodiscard]] static RollupPerformanceMetrics create_optimal_metrics() {
        RollupPerformanceMetrics metrics;
        metrics.tx_throughput = TARGET_TPS;
        metrics.avg_tx_latency = TARGET_LATENCY;
        metrics.quantum_energy_usage = TARGET_ENERGY;
        metrics.success_rate = 1.0;
        metrics.last_update = std::chrono::system_clock::now();
        return metrics;
    }
    
    [[nodiscard]] static RollupPerformanceMetrics create_worst_case_metrics() {
        RollupPerformanceMetrics metrics;
        metrics.tx_throughput = TARGET_TPS * 0.1;
        metrics.avg_tx_latency = TARGET_LATENCY * 10.0;
        metrics.quantum_energy_usage = TARGET_ENERGY * 2.0;
        metrics.success_rate = 0.8;
        metrics.last_update = std::chrono::system_clock::now();
        return metrics;
    }
    
    // Utility methods
    void reset() {
        *this = RollupPerformanceMetrics();
        last_update = std::chrono::system_clock::now();
    }
    
    void update_uptime() {
        uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - last_update
        );
    }
};

} // namespace rollup
} // namespace quids 