#pragma once

#include <vector>
#include <complex>
#include <memory>
#include <Eigen/Dense>

namespace quids {
namespace quantum {

// Forward declarations
class QuantumCircuit;

// Type aliases for quantum state representations
using StateVector = Eigen::VectorXcd;
using GateMatrix = Eigen::MatrixXcd;
using Complex = std::complex<double>;
using QuantumState = StateVector;  // Changed from std::vector<double>
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
    GateType type;
    std::vector<size_t> qubits;
    std::vector<double> parameters;
    GateMatrix custom_matrix;
};

// Quantum measurement results
struct QuantumMeasurement {
    size_t outcome;
    std::vector<double> probabilities;
    double fidelity;
    std::vector<Complex> amplitudes;
    std::vector<size_t> measured_qubits;
};

// Quantum error correction data
struct ErrorSyndrome {
    std::vector<size_t> error_qubits;
    std::vector<GateType> correction_gates;
    double error_rate;
    bool requires_recovery;
};

// Quantum proof for consensus
struct QuantumProof {
    QuantumState initial_state;
    std::vector<GateOperation> circuit_operations;
    std::vector<QuantumMeasurement> measurements;
    double verification_score;
    ErrorSyndrome error_data;
};

// Quantum security metrics
struct QuantumSecurityMetrics {
    double entanglement;
    double coherence;
    double error_rate;
    double fidelity;
    size_t circuit_depth;
    size_t num_qubits;
};

// Circuit configuration
struct QuantumCircuitConfig {
    size_t num_qubits;
    size_t max_depth;
    double error_rate;
    bool use_error_correction;
    std::vector<GateType> allowed_gates;
    size_t num_measurements;
};

// Constants for quantum operations
constexpr double QUANTUM_ERROR_THRESHOLD = 1e-6;
constexpr size_t DEFAULT_QUBIT_COUNT = 8;
constexpr size_t MAX_ENTANGLEMENT_DEPTH = 100;

// Helper functions
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
}

} // namespace quantum
} // namespace quids 