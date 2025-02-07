#pragma once
#include "quantum/QuantumState.hpp"
#include "quantum/QuantumTypes.hpp"

namespace quids {
namespace quantum {

// Quantum proof for consensus
struct QuantumProof {
    QuantumState initial_state{1};  // Initialize with 1 qubit
    std::vector<GateOperation> circuit_operations{};
    std::vector<QuantumMeasurement> measurements{};
    double verification_score{0.0};
    ErrorSyndrome error_data{};
};

} // namespace quantum
} // namespace quids 