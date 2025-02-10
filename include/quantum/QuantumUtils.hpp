#pragma once

#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumState.hpp"
#include <Eigen/Dense>
#include <vector>
#include <complex>
#include <utility>
#include "quantum/QuantumProof.hpp"

namespace quids::quantum {
namespace utils {

    // Use fully qualified types
    ::quids::quantum::StateVector tensor_product(
        const ::quids::quantum::StateVector& a, 
        const ::quids::quantum::StateVector& b
    );

    ::quids::quantum::StateVector partial_trace(
        const ::quids::quantum::StateVector& state, 
        size_t qubit_index, 
        size_t num_qubits
    );

    ::std::pair<size_t, double> measure_qubit(
        ::quids::quantum::StateVector& state, 
        size_t qubit_index
    );

    ::std::vector<double> get_measurement_probabilities(
        const ::quids::quantum::StateVector& state
    );

    // State preparation functions
    ::quids::quantum::StateVector create_bell_pair();
    ::quids::quantum::StateVector create_ghz_state(size_t num_qubits);
    ::quids::quantum::StateVector create_w_state(size_t num_qubits);

    // Error correction
    ::quids::quantum::StateVector apply_error_correction(
        const ::quids::quantum::StateVector& state, 
        const ::quids::quantum::ErrorSyndrome& syndrome
    );

    ::quids::quantum::ErrorSyndrome detect_errors(
        const ::quids::quantum::StateVector& state
    );

    // Quantum state metrics
    double calculate_fidelity(
        const ::quids::quantum::StateVector& state1, 
        const ::quids::quantum::StateVector& state2
    );

    double calculate_trace_distance(
        const ::quids::quantum::StateVector& state1, 
        const ::quids::quantum::StateVector& state2
    );

    // Quantum metrics
    double calculate_von_neumann_entropy(const ::quids::quantum::StateVector& state);
    
    // Gate operations
    ::quids::quantum::GateMatrix create_controlled_gate(const ::quids::quantum::GateMatrix& gate);
    ::quids::quantum::GateMatrix create_toffoli_gate();
    ::quids::quantum::GateMatrix create_swap_gate();
    
    // Helper functions
    size_t get_state_dimension(size_t num_qubits);
    ::std::vector<size_t> get_computational_basis_states(size_t num_qubits);
    bool is_unitary(const ::quids::quantum::GateMatrix& gate);
    
    // SIMD-optimized operations
    namespace simd {
        void apply_single_qubit_gate(::quids::quantum::StateVector& state, const ::quids::quantum::GateMatrix& gate, size_t qubit_index);
        void apply_two_qubit_gate(::quids::quantum::StateVector& state, const ::quids::quantum::GateMatrix& gate, size_t qubit1, size_t qubit2);
        void apply_controlled_gate(::quids::quantum::StateVector& state, const ::quids::quantum::GateMatrix& gate, size_t control, size_t target);
    }
}

namespace detail {

double calculateFidelity(const ::quids::quantum::QuantumState& state1, const ::quids::quantum::QuantumState& state2);
double calculateEntanglement(const ::quids::quantum::QuantumState& state);
::quids::quantum::ErrorSyndrome detectErrors(const ::quids::quantum::QuantumState& state);
::quids::quantum::QuantumState correctErrors(const ::quids::quantum::QuantumState& state, const ::quids::quantum::ErrorSyndrome& syndrome);
double calculateQuantumSecurity(const ::quids::quantum::QuantumState& state);

} // namespace detail

} // namespace quids::quantum

namespace quids {
namespace quantum {

} // namespace quantum
} // namespace quids 