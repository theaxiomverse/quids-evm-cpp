#include "quantum/QuantumState.hpp"
#include <blake3.h>
#include "zkp/QZKPGenerator.hpp"
#include <random>
#include <chrono>
#include <cmath>
#include <complex>
#include <iostream>
#include <algorithm>

namespace quids {
namespace zkp {

QZKPGenerator::QZKPGenerator() {
    // Initialize with default parameters
    optimal_phase_angles_ = {0.0, M_PI_4, M_PI_2, 3 * M_PI_4};
    optimal_measurement_qubits_ = 8;
}

void QZKPGenerator::update_optimal_parameters(
    const std::vector<double>& phase_angles,
    size_t measurement_qubits
) {
    if (phase_angles.empty() || measurement_qubits == 0) {
        return;
    }
    
    optimal_phase_angles_ = phase_angles;
    optimal_measurement_qubits_ = measurement_qubits;
}

std::vector<double> QZKPGenerator::get_optimal_phase_angles() const {
    return optimal_phase_angles_;
}

size_t QZKPGenerator::get_optimal_measurement_qubits() const {
    return optimal_measurement_qubits_;
}

QZKPGenerator::Proof QZKPGenerator::generate_proof(const quantum::QuantumState& state) {
    // Generate random measurements
    auto measurements = generate_random_measurements(optimal_measurement_qubits_);
    
    // Apply random phase transformations
    auto phases = generate_random_phases();
    quantum::QuantumState transformed_state = state;
    apply_random_transformations(transformed_state, phases);
    
    // Perform measurements
    std::vector<bool> measurement_outcomes;
    for (size_t i = 0; i < measurements.size(); i++) {
        transformed_state.apply_measurement(measurements[i]);
    }
    measurement_outcomes = transformed_state.get_measurement_outcomes();
    
    // Create proof data
    Proof proof;
    proof.measurement_outcomes = measurement_outcomes;
    proof.measurement_qubits = measurements;
    proof.phase_angles = phases;
    proof.timestamp = std::chrono::system_clock::now();
    
    return proof;
}

bool QZKPGenerator::verify_proof(const Proof& proof, const quantum::QuantumState& state) const {
    if (proof.measurement_outcomes.size() != proof.measurement_qubits.size()) {
        return false;
    }
    
    // Recreate the transformed state
    quantum::QuantumState transformed_state = state;
    apply_random_transformations(transformed_state, proof.phase_angles);
    
    // Verify measurements
    for (size_t i = 0; i < proof.measurement_qubits.size(); i++) {
        transformed_state.apply_measurement(proof.measurement_qubits[i]);
    }
    auto expected_outcomes = transformed_state.get_measurement_outcomes();
    
    return verify_measurements(expected_outcomes, proof.measurement_outcomes);
}

std::vector<size_t> QZKPGenerator::generate_random_measurements(size_t n_qubits) {
    std::vector<size_t> measurements;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, n_qubits - 1);
    
    for (size_t i = 0; i < optimal_measurement_qubits_; i++) {
        measurements.push_back(dist(gen));
    }
    
    return measurements;
}

std::vector<double> QZKPGenerator::generate_random_phases() {
    std::vector<double> phases;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0, 2 * M_PI);
    
    for (size_t i = 0; i < optimal_phase_angles_.size(); i++) {
        phases.push_back(dist(gen));
    }
    
    return phases;
}

void QZKPGenerator::apply_random_transformations(
    quantum::QuantumState& state,
    const std::vector<double>& phases
) const {
    for (size_t i = 0; i < phases.size(); i++) {
        state.apply_phase(i, phases[i]);
    }
}

bool QZKPGenerator::verify_measurements(
    const std::vector<bool>& expected,
    const std::vector<bool>& actual,
    double tolerance
) const {
    if (expected.size() != actual.size()) {
        return false;
    }
    
    size_t matches = 0;
    for (size_t i = 0; i < expected.size(); i++) {
        if (expected[i] == actual[i]) {
            matches++;
        }
    }
    
    double match_ratio = static_cast<double>(matches) / expected.size();
    return match_ratio >= (1.0 - tolerance);
}

QZKPProof generate_commitment(const quantum::QuantumState& state) {
    QZKPProof proof;
    // TODO: Implement commitment scheme
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

} // namespace zkp
} // namespace quids 