#pragma once
#include <vector>
#include <complex>
#include <array>
#include "quantum/QuantumState.h"
#include <cstdint>

class QZKPGenerator {
public:
    struct Proof {
        std::vector<size_t> measurement_qubits;
        std::vector<bool> measurement_outcomes;
        std::vector<double> phase_angles;
        std::vector<uint8_t> proof_data;
    };
    
    QZKPGenerator();
    
    // Core proof generation and verification
    Proof generate_proof(const quantum::QuantumState& state);
    bool verify_proof(const Proof& proof, const quantum::QuantumState& state);
    
    // AI optimization interface
    void updateOptimalParameters(
        const std::vector<double>& phase_angles,
        size_t measurement_qubits
    );
    
    std::vector<double> getOptimalPhaseAngles() const;
    size_t getOptimalMeasurementQubits() const;
    
private:
    std::vector<size_t> generate_random_measurements(size_t n_qubits);
    std::vector<double> generate_random_phases();
    void apply_random_transformations(quantum::QuantumState& state, const std::vector<double>& phases);
    bool verify_measurements(
        const std::vector<bool>& expected,
        const std::vector<bool>& actual,
        double tolerance = 0.1  // Allow 10% error rate for quantum measurements
    );
    
    // Optimal parameters storage
    std::vector<double> optimal_phase_angles_;
    size_t optimal_measurement_qubits_;
    double best_verification_time_;
    double best_success_rate_;
};

struct QZKPProof {
    std::vector<uint8_t> proof_data;
};

QZKPProof generate_commitment(const quantum::QuantumState& state);
std::vector<uint8_t> sign_proof(const Eigen::MatrixXcd& matrix);
std::vector<uint8_t> sign_proof(const std::vector<uint8_t>& proof_data); 