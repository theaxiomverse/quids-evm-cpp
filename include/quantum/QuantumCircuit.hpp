#pragma once

#include <vector>
#include <complex>
#include <memory>
#include <Eigen/Dense>
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumGates.hpp"
#include "quantum/QuantumUtils.hpp"
#include "quantum/QuantumState.hpp"

namespace quids {
namespace quantum {

class QuantumCircuit {
public:
    explicit QuantumCircuit(const QuantumCircuitConfig& config);
    ~QuantumCircuit() = default;

    // Disable copy to prevent quantum state duplication
    QuantumCircuit(const QuantumCircuit&) = delete;
    QuantumCircuit& operator=(const QuantumCircuit&) = delete;

    // Allow move semantics
    QuantumCircuit(QuantumCircuit&&) noexcept = default;
    QuantumCircuit& operator=(QuantumCircuit&&) noexcept = default;

    // State management
    void resetState();
    void loadState(const QuantumState& state);
    QuantumState getState() const;

    // Circuit operations
    void applyGate(GateType gate, const std::vector<size_t>& qubits);
    void applyHadamard(size_t qubit);
    void applyCNOT(size_t control, size_t target);

    // Measurement
    QuantumMeasurement measure(const QuantumState& state);
    std::vector<size_t> measureAll();

    // Error correction
    void applyErrorCorrection();

    // Circuit analysis
    size_t getNumQubits() const;

private:
    // Internal state
    StateVector stateVector_;
    std::vector<std::vector<GateOperation>> layers_;
    QuantumCircuitConfig config_;

    // Error correction
    struct ErrorCorrection {
        std::vector<size_t> syndromeQubits;
        std::vector<GateOperation> correctionOperations;
    } errorCorrection_;

    // Cache for frequently used gates (aligned for SIMD)
    alignas(64) struct GateCache {
        Eigen::Matrix2cd hadamard;
        Eigen::Matrix2cd pauliX;
        Eigen::Matrix2cd pauliY;
        Eigen::Matrix2cd pauliZ;
        Eigen::Matrix4cd cnot;
    } gateCache_;

    // Internal helper functions
    void initializeGateCache();
    GateMatrix constructGateMatrix(GateType gate) const;
    void applyGateToState(const GateMatrix& gate, const std::vector<size_t>& qubits);
    void performErrorCorrection();
    double calculateStateOverlap(const StateVector& state1, const StateVector& state2) const;

    // Constants
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr double ERROR_THRESHOLD = 1e-10;
    static constexpr size_t MAX_QUBITS = 32;
};

} // namespace quantum
} // namespace quids 