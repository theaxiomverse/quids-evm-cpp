#include "quantum/QuantumCircuit.hpp"
#include "quantum/QuantumOperations.hpp"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <cmath>
#include <vector>

namespace quids {
namespace quantum {

using GateType = ::quids::quantum::GateType;
using GateMatrix = ::quids::quantum::GateMatrix;
using QuantumState = ::quids::quantum::QuantumState;
using QuantumMeasurement = ::quids::quantum::QuantumMeasurement;
using StateVector = ::quids::quantum::StateVector;
using QuantumCircuitConfig = ::quids::quantum::QuantumCircuitConfig;

using ErrorSyndrome = ::quids::quantum::ErrorSyndrome;
using detail::calculateFidelity;
using detail::detectErrors;


QuantumCircuit::QuantumCircuit(const ::quids::quantum::QuantumCircuitConfig& config)
    : stateVector_(1ull << config.num_qubits)
    , config_(config) {
    // Initialize state vector to |0...0⟩
    stateVector_.setZero();
    stateVector_(0) = 1.0;

    initializeGateCache();

    if (config.use_error_correction) {
        performErrorCorrection();
    }
}

    void QuantumCircuit::resetState() {
    stateVector_.setZero();
    stateVector_(0) = 1.0;
}

void QuantumCircuit::loadState(const QuantumState& state) {
    const Eigen::VectorXcd& state_vector = state.get_state_vector();
    if (state_vector.size() != stateVector_.size()) {
        throw std::invalid_argument("State dimensions do not match");
    }
    stateVector_ = state_vector;
}

QuantumState QuantumCircuit::getState() const {
    return QuantumState(stateVector_);
}

void QuantumCircuit::applyGate(GateType gate, const std::vector<size_t>& qubits) {
    for (size_t qubit : qubits) {
        if (qubit >= config_.num_qubits) {
            throw std::out_of_range("Qubit index out of range");
        }
    }

    GateMatrix gateMatrix = constructGateMatrix(gate);
    applyGateToState(gateMatrix, qubits);

    if (config_.use_error_correction) {
        performErrorCorrection();
    }
}

void QuantumCircuit::applyHadamard(size_t qubit) {
    const size_t dim = 1ull << config_.num_qubits;
    const double factor = 1.0 / std::sqrt(2.0);

    StateVector newState = StateVector::Zero(dim);
    for (size_t i = 0; i < dim; ++i) {
        const size_t flipped = i ^ (1ull << qubit);
        if ((i & (1ull << qubit)) == 0) {
            newState(i) = factor * (stateVector_(i) + stateVector_(flipped));
        } else {
            newState(i) = factor * (stateVector_(i) - stateVector_(flipped));
        }
    }
    stateVector_ = std::move(newState);
}

void QuantumCircuit::applyCNOT(size_t control, size_t target) {
    if (control >= config_.num_qubits || target >= config_.num_qubits) {
        throw std::out_of_range("Qubit indices out of range");
    }

    const size_t dim = 1ull << config_.num_qubits;
    StateVector newState = stateVector_;

    for (size_t i = 0; i < dim; ++i) {
        if (i & (1ull << control)) {
            const size_t flipped = i ^ (1ull << target);
            std::swap(newState(i), newState(flipped));
        }
    }
    stateVector_ = std::move(newState);
}

QuantumMeasurement QuantumCircuit::measure(const QuantumState& state) {
    const Eigen::VectorXcd& state_vector = state.get_state_vector();
    const size_t dim = state_vector.size();
    
    // Create measurement result
    QuantumMeasurement result{};
    result.probabilities.resize(dim);
    
    // Calculate measurement probabilities
    for (size_t i = 0; i < dim; ++i) {
        result.probabilities[i] = std::norm(state_vector(i));
    }
    
    // Find most likely outcome
    result.outcome = std::distance(
        result.probabilities.begin(),
        std::max_element(result.probabilities.begin(), result.probabilities.end())
    );
    
    // Create temporary QuantumState from stateVector_ for fidelity calculation
    QuantumState current_state(stateVector_);
    result.fidelity = detail::calculateFidelity(state, current_state);
    
    // Store amplitudes
    result.amplitudes.resize(dim);
    for (size_t i = 0; i < dim; ++i) {
        result.amplitudes[i] = state_vector(i);
    }
    
    // Record measured qubits
    result.measured_qubits = {0}; // Measured the first qubit
    
    // Collapse state based on measurement
    Eigen::VectorXcd collapsed = Eigen::VectorXcd::Zero(dim);
    collapsed(result.outcome) = state_vector(result.outcome) / 
        std::sqrt(result.probabilities[result.outcome]);
    stateVector_ = collapsed;
    
    return result;
}

std::vector<size_t> QuantumCircuit::measureAll() {
    std::vector<size_t> results;
    const size_t num_qubits = static_cast<size_t>(std::log2(stateVector_.size()));
    results.reserve(num_qubits);
    
    for (size_t i = 0; i < num_qubits; ++i) {
        QuantumState current_state(stateVector_);
        auto measurement = measure(current_state);
        results.push_back(measurement.outcome);
    }
    
    return results;
}

void QuantumCircuit::applyErrorCorrection() {
    // Create QuantumState from current state vector
    QuantumState current_state(stateVector_);
    
    // Get current error syndrome
    auto syndrome = detail::detectErrors(current_state);
    
    if (syndrome.requires_recovery) {
        // Apply error correction operations
        for (size_t i = 0; i < syndrome.error_qubits.size(); ++i) {
            auto correction = constructGateMatrix(syndrome.correction_gates[i]);
            std::vector<size_t> qubits = {syndrome.error_qubits[i]};
            applyGateToState(correction, qubits);
        }
        
        // Verify correction
        QuantumState corrected_state(stateVector_);
        auto measurement = measure(corrected_state);
        if (measurement.fidelity < ERROR_THRESHOLD) {
            throw std::runtime_error("Error correction failed");
        }
    }
}

size_t QuantumCircuit::getNumQubits() const {
    return config_.num_qubits;
}

void QuantumCircuit::initializeGateCache() {
    // Initialize common gates
    gateCache_.hadamard << 1.0/std::sqrt(2.0), 1.0/std::sqrt(2.0),
                          1.0/std::sqrt(2.0), -1.0/std::sqrt(2.0);

    gateCache_.pauliX << 0.0, 1.0,
                        1.0, 0.0;

    gateCache_.pauliY << 0.0, std::complex<double>(0.0, -1.0),
                        std::complex<double>(0.0, 1.0), 0.0;

    gateCache_.pauliZ << 1.0, 0.0,
                        0.0, -1.0;

    // Initialize CNOT gate
    gateCache_.cnot.setZero();
    gateCache_.cnot(0,0) = 1.0;
    gateCache_.cnot(1,1) = 1.0;
    gateCache_.cnot(2,3) = 1.0;
    gateCache_.cnot(3,2) = 1.0;
}

GateMatrix QuantumCircuit::constructGateMatrix(GateType gate) const {
    switch (gate) {
        case GateType::HADAMARD:
            return gateCache_.hadamard;
        case GateType::PAULI_X:
            return gateCache_.pauliX;
        case GateType::PAULI_Y:
            return gateCache_.pauliY;
        case GateType::PAULI_Z:
            return gateCache_.pauliZ;
        default:
            throw std::runtime_error("Unsupported gate type");
    }
}

void QuantumCircuit::performErrorCorrection() {
    // Initialize error correction scheme
    errorCorrection_.syndromeQubits = {config_.num_qubits - 2, config_.num_qubits - 1};
    
    // Add basic error correction operations with empty parameters and custom_matrix
    errorCorrection_.correctionOperations = {
        {GateType::CNOT, {0, config_.num_qubits - 2}, {}, GateMatrix()},
        {GateType::CNOT, {1, config_.num_qubits - 1}, {}, GateMatrix()}
    };
}

double QuantumCircuit::calculateStateOverlap(const StateVector& state1, const StateVector& state2) const {
    std::complex<double> overlap = (state1.adjoint() * state2)(0);
    return std::abs(overlap);
}

void QuantumCircuit::applyGateToState(const GateMatrix& gate, const std::vector<size_t>& qubits) {
    // Apply gate to state vector using tensor product structure
    const size_t n_qubits = static_cast<size_t>(std::log2(stateVector_.size()));
    
    // Validate qubit indices
    for (size_t qubit : qubits) {
        if (qubit >= n_qubits) {
            throw std::out_of_range("Qubit index out of range");
        }
    }
    
    // Apply gate using Eigen operations
    for (Eigen::Index i = 0; i < stateVector_.size(); i++) {
        size_t idx = 0;
        for (size_t qubit : qubits) {
            idx |= ((i >> qubit) & 1) << qubit;
        }
        stateVector_(idx) = (gate * stateVector_(i))(0);
    }
}

} // namespace quantum
} // namespace quids 