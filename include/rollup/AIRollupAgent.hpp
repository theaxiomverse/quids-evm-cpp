#pragma once
#include <memory>
#include "zkp/QZKPGenerator.hpp"
#include "quantum/QuantumState.hpp"
#include "rollup/RollupPerformanceMetrics.hpp"

namespace quids {
namespace rollup {

enum class RollupConsensusType {
    QUANTUM_ZKP,
    HYBRID_QUANTUM_POS,
    QUANTUM_PBFT,
    QUANTUM_DAG
};

class AIRollupAgent {
public:
    virtual ~AIRollupAgent() = default;
    
    // Analysis and optimization
    virtual void analyzeRollupMetrics(const quids::rollup::RollupPerformanceMetrics& metrics) = 0;
    virtual void optimizeQuantumParameters() = 0;
    virtual RollupConsensusType selectConsensusAlgorithm() = 0;
    
    // Quantum-specific optimizations
    virtual std::vector<double> optimizePhaseAngles() = 0;
    virtual size_t predictOptimalMeasurementQubits() = 0;
    virtual double calculateSecurityThreshold() = 0;
    
    // Chain management
    virtual bool shouldSpawnChildRollup() const = 0;
    virtual std::unique_ptr<AIRollupAgent> createChildAgent() = 0;
};

// Reinforcement Learning implementation
class RLRollupAgent : public AIRollupAgent {
private:
    struct State {
        quids::rollup::RollupPerformanceMetrics metrics;
        std::vector<double> phase_angles;
        size_t measurement_qubits;
        double security_threshold;
    };
    
    State current_state_;
    std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator_;
    
public:
    explicit RLRollupAgent(std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator);
    
    void analyzeRollupMetrics(const quids::rollup::RollupPerformanceMetrics& metrics) override;
    void optimizeQuantumParameters() override;
    RollupConsensusType selectConsensusAlgorithm() override;
    
    std::vector<double> optimizePhaseAngles() override;
    size_t predictOptimalMeasurementQubits() override;
    double calculateSecurityThreshold() override;
    
    bool shouldSpawnChildRollup() const override;
    std::unique_ptr<AIRollupAgent> createChildAgent() override;
    
private:
    double calculateReward(const State& prev_state, const State& new_state);
    void updatePolicy(double reward);
};

} // namespace rollup
} // namespace quids 