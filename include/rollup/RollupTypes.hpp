#pragma once
#include <vector>

namespace quids {
namespace rollup {

struct OptimizationResult {
    bool success{false};
    std::vector<double> parameters;
    double expected_improvement{0.0};
    
    explicit operator bool() const { return success; }
};

} // namespace rollup
} // namespace quids 