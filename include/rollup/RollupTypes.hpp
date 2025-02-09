#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "quantum/QuantumParameters.hpp"
#include "rollup/RollupPerformanceMetrics.hpp"
#include "rollup/CrossChainState.hpp"

namespace quids {
namespace rollup {

// Forward declarations
class RollupPerformanceMetrics;

struct OptimizationResult {
    QuantumParameters parameters;
    double objective_score;
    std::vector<std::pair<std::string, double>> objective_breakdown;
    bool success_flag;
    std::vector<std::string> tradeoff_explanations;
    std::vector<double> optimized_values;
    
};

struct ComplexQueryResult {
    bool success;
    std::vector<uint8_t> data;
    double confidence;
    std::string explanation;
    std::vector<std::string> suggested_actions;
};

struct EnhancedQueryResult {
    double confidence;
    std::string explanation;
    std::vector<std::string> suggested_actions;
};

} // namespace rollup
} // namespace quids 