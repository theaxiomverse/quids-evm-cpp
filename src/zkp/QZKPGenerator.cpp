#include <cmath>
#include <vector>
#include <cstddef>

// Define math constants if not already defined.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 (M_PI/2.0)
#endif

#ifndef M_PI_4
#define M_PI_4 (M_PI/4.0)
#endif

// Define the default parameters with proper type and semicolon.
static const std::vector<double> DEFAULT_PHASE_ANGLES = { 0.0, M_PI_4, M_PI_2, 3 * M_PI_4 };
static constexpr size_t DEFAULT_MEASUREMENT_QUBITS = 8;

#include "quantum/QuantumState.hpp"
#include <blake3.h>
#include "zkp/QZKPGenerator.hpp"
#include <random>
#include <chrono>
#include <complex>
#include <iostream>
#include <algorithm>
#include <thread>
#include <array>

namespace quids {
namespace zkp {

QZKPGenerator::QZKPGenerator() {
    // Initialize with default parameters
    optimal_phase_angles_ = DEFAULT_PHASE_ANGLES;
    optimal_measurement_qubits_ = DEFAULT_MEASUREMENT_QUBITS;
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
    if (!proof.is_valid) {
        return false;
    }

    // Create a copy of the state for verification
    quantum::QuantumState verification_state = state;
    
    // Apply transformations using stored phase angles
    apply_random_transformations(verification_state, proof.phase_angles);
    
    // Perform measurements and compare results
    for (size_t i = 0; i < proof.measurement_qubits.size(); i++) {
        verification_state.apply_measurement(proof.measurement_qubits[i]);
        auto outcomes = verification_state.get_measurement_outcomes();
        if (!outcomes.empty() && outcomes.back() != proof.measurement_outcomes[i]) {
            return false;
        }
    }
    
    return true;
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

QZKPGenerator::Proof QZKPGenerator::generate_proof_parallel(
    const quantum::QuantumState& state
) {
    constexpr size_t NUM_THREADS = 4;
    std::vector<std::thread> threads;
    std::vector<std::vector<uint8_t>> partial_proofs(NUM_THREADS);
    
    // Generate partial proofs in parallel
    for (size_t i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&, i]() {
            auto start_idx = (state.size() * i) / NUM_THREADS;
            auto end_idx = (state.size() * (i + 1)) / NUM_THREADS;
            partial_proofs[i] = generate_partial_proof(state, start_idx, end_idx);
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Combine partial proofs
    return combine_partial_proofs(partial_proofs);
}

std::vector<uint8_t> QZKPGenerator::generate_partial_proof(
    const quantum::QuantumState& state,
    size_t start_idx,
    size_t end_idx
) {
    // Get subset of state vector
    const auto& full_state = state.get_state_vector();
    Eigen::VectorXcd partial_state = full_state.segment(start_idx, end_idx - start_idx);
    
    // Create quantum state from partial vector
    quantum::QuantumState partial_quantum_state(partial_state);
    
    // Generate proof for partial state
    auto proof = generate_proof(partial_quantum_state);
    return proof.proof_data;
}

QZKPGenerator::Proof QZKPGenerator::combine_partial_proofs(
    const std::vector<std::vector<uint8_t>>& partial_proofs
) {
    Proof combined_proof;
    
    // Combine proof data
    size_t total_size = 0;
    for (const auto& proof : partial_proofs) {
        total_size += proof.size();
    }
    
    combined_proof.proof_data.reserve(total_size);
    for (const auto& proof : partial_proofs) {
        combined_proof.proof_data.insert(
            combined_proof.proof_data.end(),
            proof.begin(),
            proof.end()
        );
    }
    
    // Generate random measurements and phases for combined proof
    combined_proof.measurement_qubits = generate_random_measurements(optimal_measurement_qubits_);
    combined_proof.phase_angles = generate_random_phases();
    combined_proof.measurement_outcomes.resize(combined_proof.measurement_qubits.size());
    combined_proof.timestamp = std::chrono::system_clock::now();
    
    return combined_proof;
}

bool QZKPGenerator::verify_share(const quantum::QuantumState &qs, const std::array<unsigned char, 32> &share) {
    // Minimal stub: implement your actual share verification here.
    // For now, return true to allow linking.
    return true;
}

} // namespace zkp
} // namespace quids 