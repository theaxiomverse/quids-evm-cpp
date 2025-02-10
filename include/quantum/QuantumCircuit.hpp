#pragma once

#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumState.hpp"
#include "quantum/QuantumGates.hpp"
#include "quantum/QuantumParameters.hpp"
#include "quantum/QuantumOperations.hpp"
#include "quantum/QuantumUtils.hpp"
#include <vector>
#include <memory>

namespace quids::quantum {

class QuantumCircuit {
public:
    // Constructors
    QuantumCircuit();
    explicit QuantumCircuit(const QuantumCircuitConfig& config);
    ~QuantumCircuit();

    // Rule of 5
    QuantumCircuit(const QuantumCircuit&) = delete;
    QuantumCircuit& operator=(const QuantumCircuit&) = delete;
    QuantumCircuit(QuantumCircuit&&) noexcept = default;
    QuantumCircuit& operator=(QuantumCircuit&&) noexcept = default;

    // State management
    void loadState(const ::quids::quantum::QuantumState& state);
    ::quids::quantum::QuantumState getState() const;
    
    // Gate operations
    void applyGate(::quids::quantum::GateType gate, const ::std::vector<size_t>& qubits);
    void applyCustomGate(const ::quids::quantum::GateMatrix& gate, const ::std::vector<size_t>& qubits);
    void applyControlledGate(::quids::quantum::GateType gate, size_t control, size_t target);
    
    // Measurements
    ::quids::quantum::QuantumMeasurement measure(const ::quids::quantum::QuantumState& state);
    ::std::vector<bool> measureAll();
    
    // Circuit optimization
    void optimize();
    void decompose();
    
private:
    ::quids::quantum::StateVector stateVector_;
    ::std::vector<::std::vector<::quids::quantum::GateOperation>> layers_;
    ::quids::quantum::QuantumCircuitConfig config_;
    
    // Error correction
    struct ErrorCorrection {
        bool enabled{false};
        ::std::vector<::quids::quantum::GateOperation> correctionOperations;
        double errorThreshold{0.01};
        size_t syndromeQubits{3};
    } errorCorrection_;
    
    // Private helper methods
    void initialize();
    void validate() const;
    void applyLayer(const ::std::vector<::quids::quantum::GateOperation>& layer);
    ::quids::quantum::GateMatrix constructGateMatrix(::quids::quantum::GateType gate) const;
    void applyGateToState(const ::quids::quantum::GateMatrix& gate, const ::std::vector<size_t>& qubits);
    
    double calculateStateOverlap(const ::quids::quantum::StateVector& state1, 
                               const ::quids::quantum::StateVector& state2) const;
};

} // namespace quids::quantum 