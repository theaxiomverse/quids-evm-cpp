#include "zkp/QZKPVerifier.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace quids {
namespace zkp {

QZKPVerifier::QZKPVerifier() noexcept 
    : confidence_threshold_(0.95),
      measurement_tolerance_(0.1),
      fidelity_threshold_(0.9) {}

void QZKPVerifier::set_confidence_threshold(double threshold) {
    if (threshold > 0.0 && threshold <= 1.0) {
        confidence_threshold_ = threshold;
    }
}

void QZKPVerifier::set_measurement_tolerance(double tolerance) {
    if (tolerance > 0.0 && tolerance <= 1.0) {
        measurement_tolerance_ = tolerance;
    }
}

void QZKPVerifier::set_fidelity_threshold(double threshold) {
    if (threshold > 0.0 && threshold <= 1.0) {
        fidelity_threshold_ = threshold;
    }
}

QZKPVerifier::VerificationDetails QZKPVerifier::verify_proof(
    const quantum::QuantumState& claimed_state,
    const QZKPGenerator::Proof& proof
) {
    VerificationDetails details;
    details.phase_angles = proof.phase_angles;
    
    // Verify measurement consistency
    size_t matching_count = 0;
    auto state_measurements = claimed_state.get_measurement_outcomes();
    details.measurements_match = verify_measurement_consistency(
        proof.measurement_outcomes, 
        state_measurements,
        matching_count
    );
    
    if (!details.measurements_match) {
        details.result = VerificationResult::INVALID;
        details.message = "Measurement outcomes do not match";
        details.confidence_score = 0.0;
        details.fidelity = 0.0;
        details.total_measurements = proof.measurement_outcomes.size();
        details.matching_measurements = matching_count;
        last_verification_ = details;
        return details;
    }
    
    // Calculate confidence score and fidelity
    details.confidence_score = calculate_confidence_score(
        proof, 
        claimed_state,
        details.fidelity,
        details.total_measurements,
        details.matching_measurements
    );
    
    // Verify minimum confidence threshold
    if (details.confidence_score < confidence_threshold_) {
        details.result = VerificationResult::INCONCLUSIVE;
        details.message = "Confidence score too low";
        last_verification_ = details;
        return details;
    }
    
    details.result = VerificationResult::VALID;
    details.message = "Proof verified successfully";
    last_verification_ = details;
    return details;
}

bool QZKPVerifier::verify_measurement_consistency(
    const std::vector<bool>& proof_measurements,
    const std::vector<bool>& state_measurements,
    size_t& matching_count
) const {
    if (proof_measurements.size() != state_measurements.size()) {
        matching_count = 0;
        return false;
    }
    
    matching_count = 0;
    for (size_t i = 0; i < proof_measurements.size(); ++i) {
        if (proof_measurements[i] == state_measurements[i]) {
            matching_count++;
        }
    }
    
    double match_ratio = static_cast<double>(matching_count) / proof_measurements.size();
    return match_ratio >= (1.0 - measurement_tolerance_);
}

double QZKPVerifier::calculate_confidence_score(
    const QZKPGenerator::Proof& proof,
    const quantum::QuantumState& state,
    double& fidelity,
    size_t& total_measurements,
    size_t& matching_measurements
) const {
    // Get state vector
    auto state_vector = state.get_state_vector();
    total_measurements = proof.measurement_outcomes.size();
    matching_measurements = 0;
    
    // Calculate fidelity between proof and claimed state
    fidelity = 0.0;
    for (size_t i = 0; i < proof.measurement_outcomes.size(); i++) {
        if (proof.measurement_outcomes[i] == state.get_measurement_outcomes()[i]) {
            matching_measurements++;
            fidelity += 1.0;
        }
    }
    fidelity /= total_measurements;
    
    // Apply phase angle corrections
    double phase_contribution = 0.0;
    for (double angle : proof.phase_angles) {
        phase_contribution += std::cos(angle);
    }
    phase_contribution /= proof.phase_angles.size();
    
    // Calculate final confidence score
    double measurement_confidence = static_cast<double>(matching_measurements) / total_measurements;
    double confidence = (fidelity + phase_contribution + measurement_confidence) / 3.0;
    
    return confidence;
}

QZKPVerifier::VerificationDetails QZKPVerifier::verify_entanglement(
    const quantum::QuantumState& state,
    const EntanglementProof& proof
) {
    VerificationDetails details;
    
    // Calculate entanglement fidelity
    details.fidelity = calculate_entanglement_fidelity(state, proof);
    
    if (details.fidelity < fidelity_threshold_) {
        details.result = VerificationResult::INVALID;
        details.message = "Insufficient entanglement fidelity";
        details.confidence_score = details.fidelity;
    } else {
        details.result = VerificationResult::VALID;
        details.message = "Entanglement verified successfully";
        details.confidence_score = details.fidelity;
    }
    
    last_verification_ = details;
    return details;
}

double QZKPVerifier::calculate_entanglement_fidelity(
    const quantum::QuantumState& state,
    const EntanglementProof& proof
) const {
    // Get entanglement matrix
    auto entanglement_matrix = state.generate_entanglement();
    
    // Calculate trace fidelity between state and proof
    double trace_fidelity = 0.0;
    size_t dim = static_cast<size_t>(std::sqrt(entanglement_matrix.rows()));
    
    for (size_t i = 0; i < dim; ++i) {
        for (size_t j = 0; j < dim; ++j) {
            trace_fidelity += std::norm(entanglement_matrix(i, j));
        }
    }
    
    return trace_fidelity / (dim * dim);
}

} // namespace zkp 
} // namespace quids 