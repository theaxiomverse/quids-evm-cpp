#pragma once
#include "quantum/QuantumState.hpp"
#include "quantum/QuantumTypes.hpp"

namespace quids {
namespace quantum {
namespace detail {

// Convert classical bits to quantum state
QuantumState classicalToQuantum(const std::vector<uint8_t>& classical_data);

// Calculate quantum state properties
double calculateEntanglement(const QuantumState& state);
double calculateCoherence(const QuantumState& state);
double calculateFidelity(const QuantumState& state1, const QuantumState& state2);

// Error correction utilities
ErrorSyndrome detectErrors(const QuantumState& state);
QuantumState correctErrors(const QuantumState& state, const ErrorSyndrome& syndrome);

} // namespace detail
} // namespace quantum
} // namespace quids 