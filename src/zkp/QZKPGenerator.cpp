#include "quantum/QuantumState.h"
#include <blake3.h>
#include "zkp/QZKPGenerator.h"
#include <random>
#include <chrono>
#include <cmath>
#include <complex>
#include <iostream>
#include <algorithm>

QZKPGenerator::QZKPGenerator() {
    // Initialize with proven optimal parameters from testing
    optimal_phase_angles_ = {0.1, 0.2, 0.3, 0.4, 0.5};
    optimal_measurement_qubits_ = 9;
    best_verification_time_ = 1.0;
    best_success_rate_ = 0.75;
}

void QZKPGenerator::updateOptimalParameters(
    const std::vector<double>& phase_angles,
    size_t measurement_qubits
) {
    // Only update if the parameters are valid
    if (phase_angles.empty() || measurement_qubits < 5) {
        return;
    }
    
    optimal_phase_angles_ = phase_angles;
    optimal_measurement_qubits_ = measurement_qubits;
}

std::vector<double> QZKPGenerator::getOptimalPhaseAngles() const {
    return optimal_phase_angles_;
}

size_t QZKPGenerator::getOptimalMeasurementQubits() const {
    return optimal_measurement_qubits_;
}

QZKPGenerator::Proof QZKPGenerator::generate_proof(const quantum::QuantumState& state) {
    Proof proof;
    
    // Generate random measurements
    auto n_qubits = state.get_num_qubits();
    auto measurement_qubits = generate_random_measurements(n_qubits);
    proof.measurement_qubits = measurement_qubits;
    
    // Create working copy of state
    quantum::QuantumState working_state = state;
    
    // Generate random phases
    auto phase_angles = generate_random_phases();
    proof.phase_angles = phase_angles;
    
    // Apply random transformations
    apply_random_transformations(working_state, phase_angles);
    
    // Create measurement state
    quantum::QuantumState measurement_state = working_state;
    
    // Perform measurements
    for (size_t qubit : measurement_qubits) {
        measurement_state.apply_measurement(qubit);
    }
    
    // Store measurement outcomes
    proof.measurement_outcomes = measurement_state.get_measurement_outcomes();
    
    // Generate proof data
    for (size_t i = 0; i < measurement_qubits.size(); i++) {
        quantum::QuantumState temp_state = measurement_state;
        
        // Apply additional transformations for proof
        for (size_t j = 0; j < phase_angles.size(); j++) {
            temp_state.apply_phase(j, phase_angles[j]);
        }
        
        // Store state vector data
        auto state_vector = temp_state.get_state_vector();
        for (Eigen::Index j = 0; j < state_vector.size(); j++) {
            proof.proof_data.push_back(static_cast<uint8_t>(
                std::abs(state_vector[j].real() * 255)));
        }
    }
    
    return proof;
}

bool QZKPGenerator::verify_proof(const Proof& proof, const quantum::QuantumState& state) {
    if (proof.measurement_qubits.empty() || proof.phase_angles.empty()) {
        return false;
    }
    
    quantum::QuantumState working_state = state;
    
    // Apply same transformations as in proof generation
    apply_random_transformations(working_state, proof.phase_angles);
    
    // Verify measurements
    for (size_t qubit : proof.measurement_qubits) {
        if (qubit >= working_state.get_num_qubits()) {
            return false;
        }
    }
    
    // Create measurement state
    quantum::QuantumState measurement_state = working_state;
    
    // Perform measurements
    for (size_t qubit : proof.measurement_qubits) {
        measurement_state.apply_measurement(qubit);
    }
    
    // Verify measurement outcomes
    auto outcomes = measurement_state.get_measurement_outcomes();
    if (!verify_measurements(outcomes, proof.measurement_outcomes)) {
        return false;
    }
    
    // Verify proof data
    for (size_t i = 0; i < proof.measurement_qubits.size(); i++) {
        quantum::QuantumState temp_state = measurement_state;
        
        // Apply same transformations as in proof generation
        for (size_t j = 0; j < proof.phase_angles.size(); j++) {
            temp_state.apply_phase(j, proof.phase_angles[j]);
        }
        
        // Verify state vector data
        auto state_vector = temp_state.get_state_vector();
        for (Eigen::Index j = 0; j < 4 && static_cast<size_t>(j) < proof.proof_data.size(); j++) {
            double expected = std::abs(state_vector[j].real());
            double actual = proof.proof_data[j] / 255.0;
            if (std::abs(expected - actual) > 0.1) {
                return false;
            }
        }
    }
    
    return true;
}

std::vector<size_t> QZKPGenerator::generate_random_measurements(size_t n_qubits) {
    std::vector<size_t> qubits;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, n_qubits - 1);
    
    size_t num_measurements = n_qubits / 2;  // Measure half the qubits
    for (size_t i = 0; i < num_measurements; i++) {
        qubits.push_back(dis(gen));
    }
    
    return qubits;
}

std::vector<double> QZKPGenerator::generate_random_phases() {
    std::vector<double> phases;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0, 2 * M_PI);
    
    for (size_t i = 0; i < 4; i++) {  // Generate 4 random phases
        phases.push_back(dis(gen));
    }
    
    return phases;
}

void QZKPGenerator::apply_random_transformations(
    quantum::QuantumState& state,
    const std::vector<double>& phases
) {
    for (size_t i = 0; i < phases.size(); i++) {
        state.apply_phase(i, phases[i]);
    }
}

bool QZKPGenerator::verify_measurements(
    const std::vector<bool>& expected,
    const std::vector<bool>& actual,
    double tolerance
) {
    if (expected.size() != actual.size()) {
        return false;
    }
    
    size_t mismatches = 0;
    for (size_t i = 0; i < expected.size(); i++) {
        if (expected[i] != actual[i]) {
            mismatches++;
        }
    }
    
    return (static_cast<double>(mismatches) / expected.size()) <= tolerance;
}

QZKPProof generate_commitment(const quantum::QuantumState& state) {
    QZKPProof proof;
    
    // Get state vector
    auto state_vector = state.get_state_vector();
    
    // Generate proof data from state vector
    for (Eigen::Index i = 0; i < state_vector.size(); i++) {
        proof.proof_data.push_back(static_cast<uint8_t>(
            std::abs(state_vector[i].real() * 255)));
    }
    
    return proof;
}

std::vector<uint8_t> sign_proof(const Eigen::MatrixXcd& matrix) {
    std::vector<uint8_t> signature(BLAKE3_OUT_LEN);
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, matrix.data(), matrix.size() * sizeof(std::complex<double>));
    blake3_hasher_finalize(&hasher, signature.data(), BLAKE3_OUT_LEN);
    return signature;
}

std::vector<uint8_t> sign_proof(const std::vector<uint8_t>& proof_data) {
    std::vector<uint8_t> signature(BLAKE3_OUT_LEN);
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, proof_data.data(), proof_data.size());
    blake3_hasher_finalize(&hasher, signature.data(), BLAKE3_OUT_LEN);
    return signature;
} 