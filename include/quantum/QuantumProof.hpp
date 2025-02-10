#pragma once
#include "quantum/QuantumState.hpp"
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumGates.hpp"
#include "quantum/QuantumUtils.hpp"
#include <vector>
#include <chrono>
#include <cstdint>
namespace quids::quantum {

// Quantum proof for consensus
struct QuantumProof {
    ::quids::quantum::QuantumState initial_state{1};  // Initialize with 1 qubit
    ::std::vector<::quids::quantum::GateOperation> circuit_operations{};
    ::std::vector<::quids::quantum::QuantumMeasurement> measurements{};
    
    // Error detection data
    ::quids::quantum::ErrorSyndrome error_data{};
    
    // Timing information
    ::std::chrono::system_clock::time_point timestamp{::std::chrono::system_clock::now()};
    
    // Verification status
    bool is_verified{false};
    
    // Proof metadata
    struct Metadata {
        size_t num_qubits{1};
        size_t circuit_depth{0};
        double fidelity{1.0};
        double error_rate{0.0};
    } metadata;
    
    // Validation methods
    bool verify() const;
    bool check_consistency() const;
    double calculate_confidence() const;
};

} // namespace quids::quantum