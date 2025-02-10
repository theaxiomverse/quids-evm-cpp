#pragma once
#include "quantum/QuantumState.hpp"
#include "quantum/QuantumTypes.hpp"

namespace quids {
namespace quantum {
namespace detail {

double calculateFidelity(const QuantumState& state1, const QuantumState& state2);
double calculateQuantumEntanglement(const QuantumState& state);
ErrorSyndrome detectQuantumErrors(const QuantumState& state);
QuantumState correctQuantumErrors(const QuantumState& state, const ErrorSyndrome& syndrome);
double calculateQuantumSecurityLevel(const QuantumState& state);

} // namespace detail
} // namespace quantum
} // namespace quids 