#pragma once

#include <vector>
#include <string>
#include <complex>
#include "quantum/QuantumParameters.hpp"
#include "rollup/RollupPerformanceMetrics.hpp"
#include "rollup/CrossChainState.hpp"

namespace quids {
namespace rollup {

// Forward declarations
class RollupPerformanceMetrics;
struct CrossChainState;
struct QuantumParameters;

struct OptimizationResult {
    double objective_value;
    std::vector<double> parameters;
    bool converged;
    size_t iterations;
};

struct ComplexQueryResult {
    std::vector<std::complex<double>> quantum_state;
    double fidelity;
    bool success;
};

} // namespace rollup
} // namespace quids 