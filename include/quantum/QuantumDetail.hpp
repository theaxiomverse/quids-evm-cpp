#pragma once
#include "quantum/QuantumState.hpp"
#include "quantum/QuantumTypes.hpp"

namespace quids {
namespace quantum {
namespace detail {

double calculateFidelity(const QuantumState& state1, const QuantumState& state2);
double calculateEntanglement(const QuantumState& state);
ErrorSyndrome detectErrors(const QuantumState& state);
QuantumState correctErrors(const QuantumState& state, const ErrorSyndrome& syndrome);
double calculateQuantumSecurity(const QuantumState& state);

} // namespace detail
} // namespace quantum
} // namespace quids 