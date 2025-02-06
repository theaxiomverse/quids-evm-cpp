#pragma once
#include <Eigen/Dense>  // Fixed include path
#include <vector>
#include <complex>
#include <memory>

namespace quantum {

class QuantumState {
public:
    // Constructor for n-qubit state initialized to |0...0>
    explicit QuantumState(size_t num_qubits);
    
    // Constructor from state vector
    explicit QuantumState(const Eigen::VectorXcd& state_vector);
    
    // Copy constructor
    QuantumState(const QuantumState& other);
    
    // Move constructor
    QuantumState(QuantumState&& other) noexcept;
    
    // Copy assignment
    QuantumState& operator=(const QuantumState& other);
    
    // Move assignment
    QuantumState& operator=(QuantumState&& other) noexcept;
    
    // Destructor
    ~QuantumState();

    // Core quantum operations
    Eigen::MatrixXcd generate_entanglement() const;
    std::vector<Eigen::MatrixXcd> create_layers() const;
    double calculate_coherence() const;
    double calculate_entropy() const;
    
    // State preparation
    void prepare_state();
    void apply_quantum_transform();
    
    // Getters
    size_t get_num_qubits() const;
    const Eigen::VectorXcd& normalized_vector() const;
    const Eigen::MatrixXcd& entanglement_matrix() const;
    double get_coherence() const;
    double get_entropy() const;
    
    // Quantum operations
    void apply_hadamard(size_t qubit);
    void apply_phase(size_t qubit, double angle);
    void apply_cnot(size_t control, size_t target);
    void apply_measurement(size_t qubit);
    void apply_single_qubit_gate(size_t qubit, const Eigen::Matrix2cd& gate);
    void normalize();
    
    // State access
    Eigen::VectorXcd get_state_vector() const;
    std::vector<bool> get_measurement_outcomes() const;
    
private:
    void generate_entanglement_matrix();
    void validate_state() const;
    
    Eigen::MatrixXcd create_single_qubit_gate(
        const Eigen::Matrix2cd& gate,
        size_t target_qubit
    ) const;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace quantum 