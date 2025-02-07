#include "rollup/AIRollupAgent.hpp"
#include <cmath>
#include <algorithm>

namespace quids {
namespace rollup {

RLRollupAgent::RLRollupAgent(std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator)
    : zkp_generator_(std::move(zkp_generator)) {
    // Initialize default state
    current_state_.phase_angles = {0.1, 0.2, 0.3, 0.4, 0.5};  // Start with proven angles
    current_state_.measurement_qubits = 9;  // Default from tested implementation
    current_state_.security_threshold = 0.75;  // Minimum ratio required
}

void RLRollupAgent::analyzeRollupMetrics(const RollupPerformanceMetrics& metrics) {
    current_state_.metrics = metrics;
}

void RLRollupAgent::optimizeQuantumParameters() {
    // Implementation for quantum parameter optimization
}

RollupConsensusType RLRollupAgent::selectConsensusAlgorithm() {
    const auto& metrics = current_state_.metrics;
    
    // High throughput scenario - use DAG
    if (metrics.tx_throughput > 2000) {
        return RollupConsensusType::QUANTUM_DAG;
    }
    
    // High security requirement scenario - use ZKP
    if (metrics.success_rate > 0.98 || metrics.failed_proofs < 5) {
        return RollupConsensusType::QUANTUM_ZKP;
    }
    
    // High network usage scenario - use DAG
    if (metrics.network_bandwidth > 150) {
        return RollupConsensusType::QUANTUM_DAG;
    }
    
    // Default to ZKP for better security
    return RollupConsensusType::QUANTUM_ZKP;
}

std::vector<double> RLRollupAgent::optimizePhaseAngles() {
    std::vector<double> angles;
    angles.reserve(5);
    
    // Generate optimized angles based on current metrics
    const auto& metrics = current_state_.metrics;
    
    // Add angles based on performance metrics
    if (metrics.success_rate > 0.9) {
        angles.push_back(M_PI / 4);  // 45 degrees
    }
    if (metrics.tx_throughput > 1000) {
        angles.push_back(M_PI / 3);  // 60 degrees
    }
    if (metrics.verification_time < 0.5) {
        angles.push_back(M_PI / 6);  // 30 degrees
    }
    
    return angles;
}

size_t RLRollupAgent::predictOptimalMeasurementQubits() {
    // Base prediction on current metrics
    double base_qubits = std::log2(current_state_.metrics.tx_throughput);
    return std::max<size_t>(
        5,  // Minimum for security
        std::min<size_t>(
            static_cast<size_t>(base_qubits),
            15  // Maximum practical limit
        )
    );
}

double RLRollupAgent::calculateSecurityThreshold() {
    const auto& metrics = current_state_.metrics;
    
    // Base threshold
    double threshold = 0.75;
    
    // Adjust based on metrics
    if (metrics.tx_throughput > 1000000) {
        threshold += 0.1;
    }
    if (metrics.active_validators > 10) {
        threshold += 0.05;
    }
    
    return threshold;
}

bool RLRollupAgent::shouldSpawnChildRollup() const {
    const auto& metrics = current_state_.metrics;
    return metrics.tx_throughput > 1000000 || 
           metrics.verification_time > 1.0;
}

std::unique_ptr<AIRollupAgent> RLRollupAgent::createChildAgent() {
    return std::make_unique<RLRollupAgent>(zkp_generator_);
}

double RLRollupAgent::calculateReward(const State& prev_state, const State& new_state) {
    double throughput_reward = (new_state.metrics.tx_throughput - prev_state.metrics.tx_throughput) / 1e6;
    double time_reward = (prev_state.metrics.verification_time - new_state.metrics.verification_time);
    double energy_reward = (prev_state.metrics.quantum_energy_usage - new_state.metrics.quantum_energy_usage) / 100.0;
    
    return throughput_reward * 0.4 +  // Prioritize throughput
           time_reward * 0.4 +        // Equal priority to verification speed
           energy_reward * 0.2;       // Some consideration for energy efficiency
}

void RLRollupAgent::updatePolicy(double reward) {
    // Simple policy update: if reward is positive, maintain direction of changes
    if (reward > 0) {
        // Store successful parameters for future reference
        zkp_generator_->update_optimal_parameters(
            current_state_.phase_angles,
            current_state_.measurement_qubits
        );
    } else {
        // Revert to last known good parameters
        current_state_.phase_angles = zkp_generator_->get_optimal_phase_angles();
        current_state_.measurement_qubits = zkp_generator_->get_optimal_measurement_qubits();
    }
}

} // namespace rollup
} // namespace quids 