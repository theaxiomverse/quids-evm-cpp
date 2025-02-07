#include "rl/QuantumRLAgent.hpp"
#include <omp.h>
#include <immintrin.h>
#include <random>
#include "quantum/QuantumUtils.hpp"
#include "neural/QuantumOptimizer.hpp"

namespace quids {
namespace rl {

QuantumRLAgent::QuantumRLAgent(const QuantumRLConfig& config)
    : config_(config)
    , policyNet_(std::make_unique<neural::QuantumPolicyNetwork>(config.stateSize, config.actionSize, config.numQubits))
    , valueNet_(std::make_unique<neural::QuantumValueNetwork>(config.stateSize, config.numQubits))
    , circuit_(std::make_unique<quantum::QuantumCircuit>(config.circuitConfig)) {
    
    // Initialize replay buffer with SIMD-aligned capacity
    replayBuffer_.reserve(config.replayBufferSize);
}

Action QuantumRLAgent::decideActionQuantum(const QuantumState& state) {
    // Check if we should explore
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    if (dis(gen) < config_.explorationRate) {
        return generateRandomAction();
    }
    
    // Prepare quantum state for policy evaluation
    auto quantumState = prepareQuantumState(state);
    
    // Evaluate policy using quantum circuit
    circuit_->resetState();
    circuit_->loadState(quantumState);
    
    // Apply quantum policy network
    policyNet_->applyQuantumLayer(circuit_.get());
    
    // Measure quantum state to get action probabilities
    auto action = measureQuantumState(circuit_->getState());
    
    // Calculate quantum advantage
    action.confidence = calculateQuantumAdvantage(quantumState, action);
    
    return action;
}

void QuantumRLAgent::train(const std::vector<Experience>& experiences) {
    if (experiences.empty() || experiences.size() < MIN_EXPERIENCES_FOR_TRAINING) {
        return;
    }
    
    // Update replay buffer
    for (const auto& exp : experiences) {
        updateReplayBuffer(exp);
    }
    
    // Process experiences in SIMD batches
    const size_t numBatches = replayBuffer_.size() / config_.batchSize;
    
    #pragma omp parallel for
    for (size_t i = 0; i < numBatches; ++i) {
        std::vector<Experience> batch(
            replayBuffer_.begin() + i * config_.batchSize,
            replayBuffer_.begin() + (i + 1) * config_.batchSize
        );
        processExperiencesBatch(batch);
    }
    
    // Update networks using SIMD
    updateNetworksSIMD();
    
    // Update quantum policy
    updateQuantumPolicy();
}

void QuantumRLAgent::processExperiencesBatch(const std::vector<Experience>& batch) {
    const size_t batchSize = batch.size();
    const size_t simdWidth = SIMD_WIDTH;
    const size_t numIterations = batchSize / simdWidth;
    
    // Prepare SIMD-aligned arrays for batch processing
    alignas(64) std::array<double, SIMD_WIDTH> rewards;
    alignas(64) std::array<double, SIMD_WIDTH> values;
    alignas(64) std::array<double, SIMD_WIDTH> advantages;
    
    // Process batch using AVX-512
    #pragma omp parallel for simd aligned(rewards, values, advantages : 64)
    for (size_t i = 0; i < numIterations; ++i) {
        // Load rewards and values into SIMD registers
        __m512d rewardVec = _mm512_load_pd(&batch[i * simdWidth].reward);
        __m512d valueVec = _mm512_load_pd(&batch[i * simdWidth].value);
        
        // Calculate advantages using SIMD
        __m512d advantageVec = _mm512_sub_pd(rewardVec, valueVec);
        
        // Store results
        _mm512_store_pd(&advantages[0], advantageVec);
        
        // Update metrics
        updateMetricsSIMD(advantages);
    }
}

void QuantumRLAgent::updateNetworksSIMD() {
    // Update policy network
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            optimizePolicyQuantum();
        }
        #pragma omp section
        {
            optimizeValueQuantum();
        }
    }
}

void QuantumRLAgent::optimizePolicyQuantum() {
    neural::QuantumOptimizer optimizer(config_.learningRate);
    
    // Apply quantum gradient descent
    auto gradients = policyNet_->calculateQuantumGradients();
    optimizer.applyQuantumGradients(policyNet_.get(), gradients);
    
    // Update quantum circuit parameters
    circuit_->updateParameters(policyNet_->getQuantumParameters());
}

void QuantumRLAgent::optimizeValueQuantum() {
    neural::QuantumOptimizer optimizer(config_.learningRate);
    
    // Apply quantum gradient descent to value network
    auto gradients = valueNet_->calculateQuantumGradients();
    optimizer.applyQuantumGradients(valueNet_.get(), gradients);
}

QuantumState QuantumRLAgent::prepareQuantumState(const State& classicalState) {
    // Convert classical state to quantum state
    auto quantumState = quantum::classicalToQuantum(classicalState);
    
    // Apply quantum error correction
    quantum::correctQuantumErrors(quantumState);
    
    // Apply quantum encoding circuit
    circuit_->applyEncoding(quantumState);
    
    return quantumState;
}

Action QuantumRLAgent::measureQuantumState(const QuantumState& state) {
    // Perform quantum measurement
    auto measurement = circuit_->measure(state);
    
    // Convert measurement to action
    Action action;
    action.type = static_cast<ActionType>(measurement.mostProbableOutcome);
    action.parameters = measurement.amplitudes;
    action.confidence = measurement.fidelity;
    
    return action;
}

double QuantumRLAgent::calculateQuantumAdvantage(const QuantumState& state, const Action& action) {
    // Calculate quantum entanglement
    double entanglement = quantum::calculateEntanglement(state);
    
    // Calculate quantum coherence
    double coherence = quantum::calculateCoherence(state);
    
    // Combine metrics for advantage calculation
    return (entanglement + coherence) * action.confidence;
}

void QuantumRLAgent::updateReplayBuffer(const Experience& experience) {
    if (replayBuffer_.size() >= config_.replayBufferSize) {
        replayBuffer_.erase(replayBuffer_.begin());
    }
    
    replayBuffer_.push_back(experience);
}

QuantumRLConfig QuantumRLAgent::getChildConfig() const {
    QuantumRLConfig childConfig = config_;
    
    // Adjust parameters for child chain
    childConfig.numQubits = config_.numQubits / 2;
    childConfig.learningRate *= 0.9;
    childConfig.explorationRate *= 0.8;
    childConfig.batchSize /= 2;
    
    return childConfig;
}

void QuantumRLAgent::setExplorationRate(double rate) {
    config_.explorationRate = std::clamp(rate, 0.0, 1.0);
}

} // namespace rl
} // namespace quids 