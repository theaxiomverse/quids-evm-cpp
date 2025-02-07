#pragma once

#include <memory>
#include <vector>
#include <array>
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumCircuit.hpp"
#include "neural/QuantumPolicyNetwork.hpp"
#include "neural/QuantumValueNetwork.hpp"

namespace quids {
namespace rl {

struct QuantumRLConfig {
    size_t stateSize;
    size_t actionSize;
    size_t numQubits;
    double learningRate;
    double discountFactor;
    size_t batchSize;
    size_t replayBufferSize;
    double explorationRate;
    quantum::QuantumCircuitConfig circuitConfig;
};

class QuantumRLAgent {
public:
    explicit QuantumRLAgent(const QuantumRLConfig& config);
    ~QuantumRLAgent() = default;

    // Disable copy to prevent quantum state duplication
    QuantumRLAgent(const QuantumRLAgent&) = delete;
    QuantumRLAgent& operator=(const QuantumRLAgent&) = delete;

    // Allow move semantics
    QuantumRLAgent(QuantumRLAgent&&) noexcept = default;
    QuantumRLAgent& operator=(QuantumRLAgent&&) noexcept = default;

    // Core RL functions
    Action decideActionQuantum(const QuantumState& state);
    void train(const std::vector<Experience>& experiences);
    void updateQuantumPolicy();

    // Configuration and metrics
    QuantumRLConfig getChildConfig() const;
    QuantumMetrics getMetrics() const;
    void setExplorationRate(double rate);

private:
    // Neural networks
    std::unique_ptr<neural::QuantumPolicyNetwork> policyNet_;
    std::unique_ptr<neural::QuantumValueNetwork> valueNet_;

    // Quantum circuit for state preparation and measurement
    std::unique_ptr<quantum::QuantumCircuit> circuit_;

    // Experience replay buffer (cache-aligned for SIMD)
    alignas(64) std::vector<Experience> replayBuffer_;

    // Configuration
    QuantumRLConfig config_;

    // Metrics
    struct alignas(64) Metrics {
        double averageReward{0.0};
        double policyLoss{0.0};
        double valueLoss{0.0};
        double quantumFidelity{0.0};
        size_t trainingSteps{0};
    } metrics_;

    // Internal helper functions
    QuantumState prepareQuantumState(const State& classicalState);
    Action measureQuantumState(const QuantumState& state);
    double calculateQuantumAdvantage(const QuantumState& state, const Action& action);
    void updateReplayBuffer(const Experience& experience);
    void optimizePolicyQuantum();
    void optimizeValueQuantum();

    // SIMD-optimized helper functions
    void processExperiencesBatch(const std::vector<Experience>& batch);
    void updateNetworksSIMD();

    // Constants
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t SIMD_WIDTH = 8;
    static constexpr size_t MIN_EXPERIENCES_FOR_TRAINING = 1000;
};

} // namespace rl
} // namespace quids 