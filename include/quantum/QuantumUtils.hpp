#pragma once

#include <Eigen/Dense>
#include <complex>
#include <vector>
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumState.hpp"
#include "quantum/QuantumProof.hpp"

namespace quids {
namespace quantum {

// Utility functions for quantum operations
namespace utils {
    // State vector manipulation
    StateVector tensor_product(const StateVector& a, const StateVector& b);
    StateVector partial_trace(const StateVector& state, size_t qubit_index, size_t num_qubits);
    
    // Quantum measurements
    std::pair<size_t, double> measure_qubit(StateVector& state, size_t qubit_index);
    std::vector<double> get_measurement_probabilities(const StateVector& state);
    
    // State preparation
    StateVector create_bell_pair();
    StateVector create_ghz_state(size_t num_qubits);
    StateVector create_w_state(size_t num_qubits);
    
    // Error correction
    StateVector apply_error_correction(const StateVector& state, const ErrorSyndrome& syndrome);
    ErrorSyndrome detect_errors(const StateVector& state);
    
    // Quantum metrics
    double calculate_fidelity(const StateVector& state1, const StateVector& state2);
    double calculate_trace_distance(const StateVector& state1, const StateVector& state2);
    double calculate_von_neumann_entropy(const StateVector& state);
    
    // Gate operations
    GateMatrix create_controlled_gate(const GateMatrix& gate);
    GateMatrix create_toffoli_gate();
    GateMatrix create_swap_gate();
    
    // Helper functions
    size_t get_state_dimension(size_t num_qubits);
    std::vector<size_t> get_computational_basis_states(size_t num_qubits);
    bool is_unitary(const GateMatrix& gate);
    
    // SIMD-optimized operations
    namespace simd {
        void apply_single_qubit_gate(StateVector& state, const GateMatrix& gate, size_t qubit_index);
        void apply_two_qubit_gate(StateVector& state, const GateMatrix& gate, size_t qubit1, size_t qubit2);
        void apply_controlled_gate(StateVector& state, const GateMatrix& gate, size_t control, size_t target);
    }
}

namespace detail {

double calculateFidelity(const QuantumState& state1, const QuantumState& state2);
double calculateEntanglement(const QuantumState& state);
ErrorSyndrome detectErrors(const QuantumState& state);
QuantumState correctErrors(const QuantumState& state, const ErrorSyndrome& syndrome);
double calculateQuantumSecurity(const QuantumState& state);

} // namespace detail

} // namespace quantum
} // namespace quids 