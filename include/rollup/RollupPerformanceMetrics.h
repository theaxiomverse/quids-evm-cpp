#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

// Performance metrics for rollup operations
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
    
    // Resource usage metrics
    double quantum_energy_usage{0.0};  // Energy usage in arbitrary units
    double memory_usage{0.0};         // Memory usage in MB
    double network_bandwidth{0.0};     // Network bandwidth usage in MB/s
    
    // Chain metrics
    uint32_t block_height{0};
    double block_interval{0.0};  // Average time between blocks in seconds
    uint32_t chain_id{0};
    
    // Performance targets
    static constexpr double TARGET_TPS = 3000000.0;  // 3M TPS
    static constexpr double TARGET_LATENCY = 0.001;  // 1ms
    static constexpr double TARGET_ENERGY = 1000.0;  // Arbitrary units
    
    // Constructor
    RollupPerformanceMetrics() = default;
    
    // Utility methods
    bool isPerformanceOptimal() const {
        return tx_throughput >= TARGET_TPS &&
               avg_tx_latency <= TARGET_LATENCY &&
               quantum_energy_usage <= TARGET_ENERGY;
    }
    
    double calculateEfficiencyScore() const {
        double tps_score = tx_throughput / TARGET_TPS;
        double latency_score = TARGET_LATENCY / avg_tx_latency;
        double energy_score = TARGET_ENERGY / quantum_energy_usage;
        
        return (tps_score + latency_score + energy_score) / 3.0;
    }
    
    std::string getPerformanceSummary() const {
        return "TPS: " + std::to_string(tx_throughput) +
               ", Latency: " + std::to_string(avg_tx_latency * 1000.0) + "ms" +
               ", Energy: " + std::to_string(quantum_energy_usage);
    }
    
    // Comparison operators
    bool operator<(const RollupPerformanceMetrics& other) const {
        return calculateEfficiencyScore() < other.calculateEfficiencyScore();
    }
    
    bool operator>(const RollupPerformanceMetrics& other) const {
        return calculateEfficiencyScore() > other.calculateEfficiencyScore();
    }
    
    // Static factory methods
    static RollupPerformanceMetrics createOptimalMetrics() {
        RollupPerformanceMetrics metrics;
        metrics.tx_throughput = TARGET_TPS;
        metrics.avg_tx_latency = TARGET_LATENCY;
        metrics.quantum_energy_usage = TARGET_ENERGY;
        metrics.success_rate = 1.0;
        return metrics;
    }
    
    static RollupPerformanceMetrics createWorstCaseMetrics() {
        RollupPerformanceMetrics metrics;
        metrics.tx_throughput = TARGET_TPS * 0.1;
        metrics.avg_tx_latency = TARGET_LATENCY * 10.0;
        metrics.quantum_energy_usage = TARGET_ENERGY * 2.0;
        metrics.success_rate = 0.8;
        return metrics;
    }
}; 