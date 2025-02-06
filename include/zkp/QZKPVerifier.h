#pragma once
#include "zkp/Types.h"
#include "zkp/QZKPGenerator.h"
#include <vector>
#include <string>
#include "quantum/QuantumState.h"

namespace zkp {

class QZKPVerifier {
public:
    enum class VerificationResult {
        VALID,
        INVALID,
        INCONCLUSIVE
    };
    
    struct VerificationDetails {
        VerificationResult result;
        std::string message;
        double confidence_score;
        bool measurements_match;
        double fidelity;
        std::vector<double> phase_angles;
        size_t total_measurements;
        size_t matching_measurements;
    };
    
    // Main verification methods
    VerificationDetails verify_proof(
        const quantum::QuantumState& claimed_state,
        const QZKPGenerator::Proof& proof
    );
    
    VerificationDetails verify_entanglement(
        const quantum::QuantumState& state,
        const EntanglementProof& proof
    );
    
    // Configuration methods
    void set_confidence_threshold(double threshold);
    void set_measurement_tolerance(double tolerance);
    void set_fidelity_threshold(double threshold);
    
    // Getters for verification parameters
    double get_confidence_threshold() const { return confidence_threshold_; }
    double get_measurement_tolerance() const { return measurement_tolerance_; }
    double get_fidelity_threshold() const { return fidelity_threshold_; }
    
    // Constructor
    QZKPVerifier() noexcept;
    
private:
    bool verify_measurement_consistency(
        const std::vector<bool>& recorded_measurements,
        const std::vector<bool>& state_measurements,
        size_t& matching_count
    );
    
    double calculate_confidence_score(
        const QZKPGenerator::Proof& proof,
        const quantum::QuantumState& state,
        double& fidelity,
        size_t& total_measurements,
        size_t& matching_measurements
    );
    
    double calculate_entanglement_fidelity(
        const quantum::QuantumState& state,
        const EntanglementProof& proof
    );
    
    // Verification parameters
    double confidence_threshold_{0.95};
    double measurement_tolerance_{0.1};
    double fidelity_threshold_{0.9};
    
    // Last verification details
    VerificationDetails last_verification_;
};

} // namespace zkp 