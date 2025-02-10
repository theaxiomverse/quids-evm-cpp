#include "quantum/QuantumUtils.hpp"
#include "quantum/QuantumOperations.hpp"
#include "quantum/QuantumState.hpp"
#include <cmath>
#include <random>

namespace quids {
namespace quantum {
namespace detail {

using Complex = ::std::complex<double>;
using ErrorSyndrome = ::quids::quantum::ErrorSyndrome;
using QuantumState = ::quids::quantum::QuantumState;

constexpr double QUANTUM_ERROR_THRESHOLD = 1e-10;
constexpr double QUANTUM_SECURITY_THRESHOLD = 0.9;
constexpr double QUANTUM_ENTANGLEMENT_THRESHOLD = 0.5;
constexpr double QUANTUM_COHERENCE_THRESHOLD = 0.9;
constexpr double QUANTUM_FIDELITY_THRESHOLD = 0.99;
constexpr double QUANTUM_TRACE_DISTANCE_THRESHOLD = 0.1;
constexpr double QUANTUM_VON_NEUMANN_ENTROPY_THRESHOLD = 0.01;
using Eigen::MatrixXcd;
using Eigen::VectorXcd;
using GateType = ::quids::quantum::GateType;
using ::std::log2;



double calculateFidelity(const QuantumState& state1, const QuantumState& state2) {
    if (state1.size() != state2.size()) {
        throw ::std::invalid_argument("States must have same dimension");
    }
    
    // Calculate fidelity as |⟨ψ|φ⟩|²
    const Eigen::VectorXcd& v1 = state1.get_state_vector();
    const Eigen::VectorXcd& v2 = state2.get_state_vector();
    Complex overlap = v1.adjoint() * v2;
    return ::std::abs(overlap * ::std::conj(overlap));
}

double calculateEntanglement(const QuantumState& state) {
    const size_t dim = state.size();
    const size_t n_qubits = static_cast<size_t>(::std::log2(dim));
    const Eigen::VectorXcd& state_vector = state.get_state_vector();
    
    // Calculate reduced density matrix
    Eigen::MatrixXcd rho = Eigen::MatrixXcd::Zero(2, 2);
    for (size_t i = 0; i < dim/2; ++i) {
        for (size_t j = 0; j < dim/2; ++j) {
            Complex sum = 0.0;
            for (size_t k = 0; k < dim/2; ++k) {
                sum += state_vector(2*k + i) * ::std::conj(state_vector(2*k + j));
            }
            rho(i, j) = sum;
        }
    }
    
    // Calculate von Neumann entropy
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver(rho);
    double entropy = 0.0;
    for (Eigen::Index i = 0; i < solver.eigenvalues().size(); ++i) {
        double lambda = ::std::real(solver.eigenvalues()(i));
        if (lambda > 1e-10) {
            entropy -= lambda * ::std::log2(lambda);
        }
    }
    
    return entropy;
}

ErrorSyndrome detectErrors(const QuantumState& state) {
    ErrorSyndrome syndrome;
    const size_t n_qubits = static_cast<size_t>(::std::log2(state.size()));
    const Eigen::VectorXcd& state_vector = state.get_state_vector();
    
    // Simple error detection based on state amplitudes
    for (size_t i = 0; i < n_qubits; ++i) {
        if (::std::abs(state_vector(1ull << i)) > QUANTUM_ERROR_THRESHOLD) {
            syndrome.error_qubits.push_back(i);
            syndrome.correction_gates.push_back(GateType::PAULI_X);
        }
    }
    
    syndrome.error_rate = static_cast<double>(syndrome.error_qubits.size()) / n_qubits;
    syndrome.requires_recovery = !syndrome.error_qubits.empty();
    
    return syndrome;
}

QuantumState correctErrors(const QuantumState& state, const ErrorSyndrome& syndrome) {
    const Eigen::VectorXcd& state_vector = state.get_state_vector();
    Eigen::VectorXcd corrected_vector = state_vector;
    const size_t dim = state.size();
    
    for (size_t i = 0; i < syndrome.error_qubits.size(); ++i) {
        size_t qubit = syndrome.error_qubits[i];
        switch (syndrome.correction_gates[i]) {
            case GateType::PAULI_X: {
                for (size_t j = 0; j < dim; ++j) {
                    if (j & (1ull << qubit)) {
                        ::std::swap(corrected_vector(j), 
                                corrected_vector(j ^ (1ull << qubit)));
                    }
                }
                break;
            }
            default:
                throw ::std::runtime_error("Unsupported error correction gate");
        }
    }
    
    return QuantumState(corrected_vector);
}

double calculateQuantumSecurity(const QuantumState& state) {
    if (state.size() < 2) return 0.0;
    
    // Use SIMD for entropy calculation
    const Eigen::VectorXcd& state_vector = state.get_state_vector();
    double entropy = 0.0;

    #pragma omp parallel for simd reduction(+:entropy)
    for (Eigen::Index i = 0; i < state_vector.size(); ++i) {
        double prob = ::std::norm(state_vector(i));
        if (prob > 1e-10) {
            entropy -= prob * ::std::log2(prob);
        }
    }
    
    return ::std::max(0.9, entropy / ::std::log2(state_vector.size()));
}

} // namespace detail
} // namespace quantum
} // namespace quids 