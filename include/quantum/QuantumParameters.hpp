#pragma once

#include <vector>
#include <cstddef>
#include <string>
#include <chrono>

namespace quids {
namespace rollup {

struct QuantumParameters {
    // Core quantum parameters
    double entanglement_strength{0.0};
    size_t qubits_per_transaction{0};
    double error_correction_overhead{0.0};
    double quantum_memory_capacity{0.0};
    
    // Circuit parameters
    std::vector<double> phase_angles;
    size_t num_qubits{0};
    double entanglement_degree{0.0};
    bool use_quantum_execution{false};
    
    // Advanced parameters
    double coherence_time{0.0};
    double gate_fidelity{0.99};
    size_t error_correction_cycles{0};
    double noise_threshold{0.001};
    
    // Resource management
    size_t max_parallel_gates{1};
    double memory_overhead_factor{1.0};
    size_t redundancy_level{1};
    
    // Timing parameters
    std::chrono::nanoseconds gate_time{100};
    std::chrono::microseconds measurement_time{1};
    std::chrono::nanoseconds swap_time{200};
    
    // Default constructor
    QuantumParameters() = default;
    
    // Constructor for enhanced quantum parameters
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
        , num_qubits(qubits_per_transaction_)
        , entanglement_degree(entanglement_strength_)
        , use_quantum_execution(true)
        , coherence_time(1000.0)  // 1 microsecond default
        , gate_fidelity(0.99)
        , error_correction_cycles(10)
        , noise_threshold(0.001)
        , max_parallel_gates(qubits_per_transaction_ / 2)
        , memory_overhead_factor(2.0)
        , redundancy_level(3)
    {
        phase_angles.resize(num_qubits, 0.0);
    }
    
    // Constructor for basic quantum parameters
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
        , coherence_time(1000.0)  // 1 microsecond default
        , gate_fidelity(0.99)
        , error_correction_cycles(10)
        , noise_threshold(0.001)
        , max_parallel_gates(num_qubits_ / 2)
        , memory_overhead_factor(2.0)
        , redundancy_level(3)
        , gate_time(100)
        , measurement_time(1)
        , swap_time(200)
    {}
    
    // Utility methods
    [[nodiscard]] bool is_valid() const {
        return num_qubits > 0 &&
               entanglement_strength >= 0.0 &&
               entanglement_strength <= 1.0 &&
               error_correction_overhead >= 0.0 &&
               quantum_memory_capacity > 0.0 &&
               gate_fidelity > 0.0 &&
               gate_fidelity <= 1.0 &&
               noise_threshold > 0.0 &&
               noise_threshold < 1.0;
    }
    
    [[nodiscard]] double calculate_resource_requirements() const {
        return num_qubits * memory_overhead_factor * redundancy_level;
    }
    
    [[nodiscard]] std::chrono::nanoseconds estimate_execution_time() const {
        auto total_time = gate_time * num_qubits * error_correction_cycles;
        total_time += measurement_time * num_qubits;
        total_time += swap_time * (num_qubits / 2);
        return total_time;
    }
    
    // Constants
    static constexpr size_t MAX_QUBITS = 1024;
    static constexpr double MIN_GATE_FIDELITY = 0.9;
    static constexpr double MAX_ERROR_RATE = 0.01;
    static constexpr size_t MAX_ERROR_CORRECTION_CYCLES = 100;
};

} // namespace rollup
} // namespace quids 