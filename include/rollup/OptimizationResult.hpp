#pragma once

#include <vector>
#include <string>

namespace quids {
namespace rollup {

// struct OptimizationResult {
//     bool success_flag{false};
    std::vector<std::string> tradeoff_explanations;
    std::vector<double> optimized_values;
//};

} // namespace rollup
} // namespace quids 