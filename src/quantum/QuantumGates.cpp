#include "quantum/QuantumGates.hpp"
#include "quantum/QuantumState.hpp"
#include <Eigen/Dense>
#include <cmath>

namespace quids::quantum::detail {

Eigen::Matrix2cd hadamard() {
    Eigen::Matrix2cd H;
    const double sqrt2 = 1.0 / std::sqrt(2.0);
    H << sqrt2,  sqrt2,
         sqrt2, -sqrt2;
    return H;
}

Eigen::Matrix2cd phase(double angle) {
    Eigen::Matrix2cd P;
    P << 1.0, 0.0,
         0.0, std::exp(std::complex<double>(0, angle));
    return P;
}

Eigen::Matrix4cd cnot() {
    Eigen::Matrix4cd CNOT;
    CNOT << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 0, 1,
            0, 0, 1, 0;
    return CNOT;
}

bool detectErrors(const ::quids::quantum::QuantumState& state) {
    // Simple error detection using parity checks
    const auto& vec = state.get_state_vector();
    bool has_error = false;
    
    for (Eigen::Index i = 0; i < vec.size(); i++) {
        if (std::abs(vec(i)) > 1.0 + 1e-10) {
            has_error = true;
            break;
        }
    }
    
    return has_error;
}

double calculateFidelity(const ::quids::quantum::QuantumState& state1, const ::quids::quantum::QuantumState& state2) {
    const auto& vec1 = state1.get_state_vector();
    const auto& vec2 = state2.get_state_vector();
    
    if (vec1.size() != vec2.size()) {
        throw ::std::invalid_argument("State vectors must have same dimension");
    }
    
    ::std::complex<double> overlap = vec1.dot(vec2);
    return ::std::norm(overlap);
}

} // namespace quids::quantum::detail