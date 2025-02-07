#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <optional>
#include "zkp/Types.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "quantum/QuantumState.hpp"
#include <Eigen/Dense>

namespace quids {
namespace zkp {

struct EntanglementProof {
    Eigen::MatrixXcd entanglement_matrix;
    double fidelity;
};

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
        std::chrono::system_clock::time_point timestamp;
        
        // Default constructor
        VerificationDetails()
            : result(VerificationResult::INCONCLUSIVE)
            , confidence_score(0.0)
            , measurements_match(false)
            , fidelity(0.0)
            , total_measurements(0)
            , matching_measurements(0)
            , timestamp(std::chrono::system_clock::now())
        {}
        
        // Constructor with values
        VerificationDetails(
            VerificationResult result_,
            std::string message_,
            double confidence_score_,
            bool measurements_match_,
            double fidelity_,
            std::vector<double> phase_angles_,
            size_t total_measurements_,
            size_t matching_measurements_
        ) : result(result_)
            , message(std::move(message_))
            , confidence_score(confidence_score_)
            , measurements_match(measurements_match_)
            , fidelity(fidelity_)
            , phase_angles(std::move(phase_angles_))
            , total_measurements(total_measurements_)
            , matching_measurements(matching_measurements_)
            , timestamp(std::chrono::system_clock::now())
        {}
        
        // Rule of 5
        VerificationDetails(const VerificationDetails&) = default;
        VerificationDetails& operator=(const VerificationDetails&) = default;
        VerificationDetails(VerificationDetails&&) noexcept = default;
        VerificationDetails& operator=(VerificationDetails&&) noexcept = default;
        ~VerificationDetails() = default;
        
        [[nodiscard]] bool is_valid() const {
            return confidence_score >= 0.0 &&
                   confidence_score <= 1.0 &&
                   fidelity >= 0.0 &&
                   fidelity <= 1.0 &&
                   total_measurements > 0 &&
                   matching_measurements <= total_measurements;
        }
    };
    
    // Constructor and destructor
    QZKPVerifier() noexcept;
    ~QZKPVerifier() = default;
    
    // Rule of 5
    QZKPVerifier(const QZKPVerifier&) = delete;
    QZKPVerifier& operator=(const QZKPVerifier&) = delete;
    QZKPVerifier(QZKPVerifier&&) noexcept = default;
    QZKPVerifier& operator=(QZKPVerifier&&) noexcept = default;
    
    // Main verification methods
    [[nodiscard]] VerificationDetails verify_proof(
        const quantum::QuantumState& claimed_state,
        const QZKPGenerator::Proof& proof
    );
    
    [[nodiscard]] VerificationDetails verify_entanglement(
        const quantum::QuantumState& state,
        const EntanglementProof& proof
    );
    
    // Configuration methods
    void set_confidence_threshold(double threshold);
    void set_measurement_tolerance(double tolerance);
    void set_fidelity_threshold(double threshold);
    
    // Getters for verification parameters
    [[nodiscard]] double get_confidence_threshold() const { return confidence_threshold_; }
    [[nodiscard]] double get_measurement_tolerance() const { return measurement_tolerance_; }
    [[nodiscard]] double get_fidelity_threshold() const { return fidelity_threshold_; }
    [[nodiscard]] const VerificationDetails& get_last_verification() const { return last_verification_; }
    
    // Performance metrics
    [[nodiscard]] double get_average_verification_time() const { return avg_verification_time_; }
    [[nodiscard]] double get_success_rate() const { return success_rate_; }
    [[nodiscard]] size_t get_total_verifications() const { return total_verifications_; }
    
private:
    [[nodiscard]] bool verify_measurement_consistency(
        const std::vector<bool>& recorded_measurements,
        const std::vector<bool>& state_measurements,
        size_t& matching_count
    ) const;
    
    [[nodiscard]] double calculate_confidence_score(
        const QZKPGenerator::Proof& proof,
        const quantum::QuantumState& state,
        double& fidelity,
        size_t& total_measurements,
        size_t& matching_measurements
    ) const;
    
    [[nodiscard]] double calculate_entanglement_fidelity(
        const quantum::QuantumState& state,
        const EntanglementProof& proof
    ) const;
    
    // Verification parameters
    double confidence_threshold_{0.95};
    double measurement_tolerance_{0.1};
    double fidelity_threshold_{0.9};
    
    // Performance tracking
    double avg_verification_time_{0.0};
    double success_rate_{1.0};
    size_t total_verifications_{0};
    
    // Last verification details
    VerificationDetails last_verification_;
    
    // Constants
    static constexpr double MIN_CONFIDENCE = 0.8;
    static constexpr double MAX_TOLERANCE = 0.2;
    static constexpr double MIN_FIDELITY = 0.85;
    static constexpr size_t MIN_MEASUREMENTS = 10;
    static constexpr size_t MAX_PHASE_ANGLES = 1024;
};

} // namespace zkp
} // namespace quids 