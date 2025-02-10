#pragma once

#include <vector>
#include <complex>
#include <memory>
#include <Eigen/Dense>
#include "quantum/QuantumForward.hpp"  // Use forward declarations

namespace quids {
namespace quantum {

// Forward declarations
class QuantumCircuit;

// Type aliases for quantum state representations
using Complex = ::std::complex<double>;
using StateVector = Eigen::VectorXcd;
using GateMatrix = Eigen::Matrix<Complex, Eigen::Dynamic, Eigen::Dynamic>;
using DensityMatrix = Eigen::MatrixXcd;

// Gate types for quantum operations
enum class GateType {
    HADAMARD,
    PAULI_X,
    PAULI_Y,
    PAULI_Z,
    CNOT,
    SWAP,
    TOFFOLI,
    PHASE,
    ROTATION,
    CUSTOM
};

// Structure for quantum gate operations
struct GateOperation {
    GateType type{GateType::HADAMARD};
    ::std::vector<size_t> qubits{};
    ::std::vector<double> parameters{};
    ::Eigen::Matrix<Complex, Eigen::Dynamic, Eigen::Dynamic> custom_matrix{};
};

// Quantum measurement results
struct QuantumMeasurement {
    size_t outcome{0};
    ::std::vector<double> probabilities{};
    double fidelity{0.0};
    ::std::vector<Complex> amplitudes{};
    ::std::vector<size_t> measured_qubits{};
};

// Quantum error correction data
struct ErrorSyndrome {
    ::std::vector<size_t> error_qubits{};
    ::std::vector<GateType> correction_gates{};
    double error_rate{0.0};
    bool requires_recovery{false};
};

// Quantum security metrics
struct QuantumSecurityMetrics {
    double entanglement{0.0};
    double coherence{0.0};
    double error_rate{0.0};
    double fidelity{0.0};
    size_t circuit_depth{0};
    size_t num_qubits{0};
};

// Circuit configuration
struct QuantumCircuitConfig {
    size_t num_qubits{1};
    size_t max_depth{1};
    double error_rate{0.0};
    bool use_error_correction{false};
    ::std::vector<GateType> allowed_gates{};
    size_t num_measurements{0};
};

// Constants for quantum operations
constexpr double QUANTUM_ERROR_THRESHOLD = 1e-6;
constexpr size_t DEFAULT_QUBIT_COUNT = 8;
constexpr size_t MAX_ENTANGLEMENT_DEPTH = 100;

} // namespace quantum
} // namespace quids

// Hash specialization must be in std namespace
namespace std {
    template<>
    struct hash<quids::quantum::GateMatrix> {
        size_t operator()(const quids::quantum::GateMatrix& gate) const {
            size_t seed = 0;
            for (Eigen::Index i = 0; i < gate.size(); ++i) {
                auto elem = gate.data()[i];
                seed ^= std::hash<double>()(elem.real()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                seed ^= std::hash<double>()(elem.imag()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
} 