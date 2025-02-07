#include "rollup/AIRollupAgent.hpp"
#include <cmath>
#include <algorithm>

namespace quids {
namespace rollup {

RLRollupAgent::RLRollupAgent(std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator)
    : zkp_generator_(zkp_generator) {
    // Initialize default state
    current_state_.phase_angles = {0.1, 0.2, 0.3, 0.4, 0.5};  // Start with proven angles
    current_state_.measurement_qubits = 9;  // Default from tested implementation
    current_state_.security_threshold = 0.75;  // Minimum ratio required
}

void RLRollupAgent::analyzeRollupMetrics(const RollupPerformanceMetrics& metrics) {
    State prev_state = current_state_;
    current_state_.metrics = metrics;
    
    // Calculate reward and update policy
    double reward = calculateReward(prev_state, current_state_);
    updatePolicy(reward);
}

void RLRollupAgent::optimizeQuantumParameters() {
    // Optimize based on current metrics
    if (current_state_.metrics.proof_generation_time > 1.0) {  // If proofs taking too long
        current_state_.measurement_qubits = std::max<size_t>(
            5,  // Minimum required for security
            current_state_.measurement_qubits - 1
        );
    }
    
    if (current_state_.metrics.success_rate < 0.8) {  // If accuracy too low
        // Adjust phase angles for better measurement accuracy
        for (double& angle : current_state_.phase_angles) {
            angle *= 0.95;  // Reduce angle magnitude for more stable measurements
        }
    }
}

RollupConsensusType RLRollupAgent::selectConsensusAlgorithm() {
    if (current_state_.metrics.active_validators < 5) {
        return RollupConsensusType::QUANTUM_ZKP;  // Simplest for small networks
    }
    
    if (current_state_.metrics.quantum_energy_usage > 100.0) {
        return RollupConsensusType::HYBRID_QUANTUM_POS;  // More energy efficient
    }
    
    if (current_state_.metrics.tx_throughput > 1e6) {
        return RollupConsensusType::QUANTUM_DAG;  // Better for high throughput
    }
    
    return RollupConsensusType::QUANTUM_PBFT;  // Default for general case
}

std::vector<double> RLRollupAgent::optimizePhaseAngles() {
    // Start with current angles
    std::vector<double> optimized = current_state_.phase_angles;
    
    // Adjust based on verification time
    if (current_state_.metrics.verification_time > 0.5) {  // If verification too slow
        // Reduce number of angles
        while (optimized.size() > 3 && current_state_.metrics.verification_time > 0.5) {
            optimized.pop_back();
        }
    }
    
    return optimized;
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
    // Dynamic threshold based on network conditions
    double base_threshold = 0.75;  // Minimum from tested implementation
    
    if (current_state_.metrics.active_validators > 10) {
        base_threshold += 0.05;  // Increase requirements for larger networks
    }
    
    if (current_state_.metrics.tx_throughput > 1e6) {
        base_threshold += 0.05;  // Increase requirements for high throughput
    }
    
    return std::min(0.95, base_threshold);  // Cap at 95% for practicality
}

bool RLRollupAgent::shouldSpawnChildRollup() const {
    return current_state_.metrics.tx_throughput > 1e6 &&  // High load
           current_state_.metrics.verification_time > 1.0;  // Slow verification
}

std::unique_ptr<AIRollupAgent> RLRollupAgent::createChildAgent() {
    // Create new agent with same ZKP generator but fresh state
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