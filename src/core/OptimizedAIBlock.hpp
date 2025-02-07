#pragma once

#include <atomic>
#include <vector>
#include <memory>
#include <array>
#include "quantum/QuantumTypes.hpp"
#include "memory/MemoryPool.hpp"
#include "consensus/QuantumConsensus.hpp"
#include "state/LockFreeStateManager.hpp"
#include "rl/QuantumRLAgent.hpp"

namespace quids {
namespace core {

class OptimizedAIBlock {
public:
    OptimizedAIBlock(const BlockConfig& config);
    ~OptimizedAIBlock() = default;

    // Disable copy to prevent concurrent modifications
    OptimizedAIBlock(const OptimizedAIBlock&) = delete;
    OptimizedAIBlock& operator=(const OptimizedAIBlock&) = delete;

    // Allow move semantics
    OptimizedAIBlock(OptimizedAIBlock&&) noexcept = default;
    OptimizedAIBlock& operator=(OptimizedAIBlock&&) noexcept = default;

    // Main processing functions
    void processBlockParallel();
    bool validateBlock(const Block& block) const;
    void finalizeBlockZK();

    // Metrics and state
    EnhancedBlockMetrics getMetrics() const;
    QuantumState getQuantumState() const;
    
    // Chain management
    bool shouldSpawnChildChain() const;
    std::unique_ptr<OptimizedAIBlock> spawnChildChain();

private:
    // Core components
    memory::MemoryPool<Transaction> txPool_;
    state::LockFreeStateManager stateManager_;
    consensus::QuantumConsensusModule consensus_;
    rl::QuantumRLAgent agent_;

    // Metrics tracking
    struct EnhancedBlockMetrics {
        std::atomic<double> throughput{0.0};
        std::atomic<double> latency{0.0};
        std::atomic<double> energyUsage{0.0};
        std::atomic<int> validatorCount{0};
        std::atomic<double> securityScore{0.0};
        quantum::QuantumSecurityMetrics quantumMetrics;
    };

    // Cache-aligned metrics
    alignas(64) EnhancedBlockMetrics metrics_;
    
    // SIMD-optimized state
    alignas(64) std::array<double, 8> stateBuffer_;
    
    // Internal processing functions
    EnhancedBlockMetrics collectMetricsAsync() const;
    void applyOptimizationsParallel(const Action& action);
    void processTransactionsSIMD(const std::vector<Transaction>& batch);
    void updateQuantumState();
    
    // Constants
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t SIMD_WIDTH = 8;
    static constexpr size_t MAX_BATCH_SIZE = 1024;
};

} // namespace core
} // namespace quids 