#pragma once

#include <vector>
#include <cstddef>

struct QuantumParameters {
    // From EnhancedRollupMLModel.h
    double entanglement_strength;
    size_t qubits_per_transaction;
    double error_correction_overhead;
    double quantum_memory_capacity;
    
    // From RollupMLModel.h
    std::vector<double> phase_angles;
    size_t num_qubits;
    double entanglement_degree;
    bool use_quantum_execution;
    
    // Default constructor
    QuantumParameters()
        : entanglement_strength(0.0)
        , qubits_per_transaction(0)
        , error_correction_overhead(0.0)
        , quantum_memory_capacity(0.0)
        , phase_angles{0.0}
        , num_qubits(0)
        , entanglement_degree(0.0)
        , use_quantum_execution(false)
    {}
    
    // Constructor for EnhancedRollupMLModel style initialization
    QuantumParameters(
        double entanglement_strength_,
        size_t qubits_per_transaction_,
        double error_correction_overhead_,
        double quantum_memory_capacity_
    )
        : entanglement_strength(entanglement_strength_)
        , qubits_per_transaction(qubits_per_transaction_)
        , error_correction_overhead(error_correction_overhead_)
        , quantum_memory_capacity(quantum_memory_capacity_)
        , phase_angles{0.0}
        , num_qubits(qubits_per_transaction_)
        , entanglement_degree(entanglement_strength_)
        , use_quantum_execution(true)
    {}
    
    // Constructor for RollupMLModel style initialization
    QuantumParameters(
        const std::vector<double>& phase_angles_,
        size_t num_qubits_,
        double entanglement_degree_,
        bool use_quantum_execution_
    )
        : entanglement_strength(entanglement_degree_)
        , qubits_per_transaction(num_qubits_)
        , error_correction_overhead(0.0)
        , quantum_memory_capacity(0.0)
        , phase_angles(phase_angles_)
        , num_qubits(num_qubits_)
        , entanglement_degree(entanglement_degree_)
        , use_quantum_execution(use_quantum_execution_)
    {}
}; 