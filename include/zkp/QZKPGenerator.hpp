#pragma once

#include <vector>
#include <complex>
#include <array>
#include <memory>
#include <chrono>
#include <random>
#include <optional>
#include "quantum/QuantumState.hpp"
#include <Eigen/Dense>

namespace quids {
namespace zkp {

class QZKPGenerator {
public:
    struct Proof {
        std::vector<uint8_t> proof_data;
        std::vector<std::complex<double>> commitment;
        std::vector<size_t> measurement_qubits;
        std::vector<double> phase_angles;
        std::vector<bool> measurement_outcomes;
        std::chrono::system_clock::time_point timestamp;
        bool is_valid{false};
    };
    
    // Constructor and destructor
    QZKPGenerator();
    ~QZKPGenerator() = default;
    
    // Rule of 5
    QZKPGenerator(const QZKPGenerator&) = delete;
    QZKPGenerator& operator=(const QZKPGenerator&) = delete;
    QZKPGenerator(QZKPGenerator&&) noexcept = default;
    QZKPGenerator& operator=(QZKPGenerator&&) noexcept = default;
    // Core proof generation and verification
    [[nodiscard]] Proof generate_proof(const quantum::QuantumState& state);
    [[nodiscard]] bool verify_proof(const Proof& proof, const quantum::QuantumState& state) const;
    [[nodiscard]] bool verify_share(const quantum::QuantumState& state, const std::array<uint8_t, 32>& commitment);
    
    // AI optimization interface
    void update_optimal_parameters(
        const std::vector<double>& phase_angles,
        size_t measurement_qubits
    );
    
    [[nodiscard]] std::vector<double> get_optimal_phase_angles() const;
    [[nodiscard]] size_t get_optimal_measurement_qubits() const;
    
    // Performance metrics
    [[nodiscard]] double get_average_proof_time() const { return avg_proof_time_; }
    [[nodiscard]] double get_success_rate() const { return success_rate_; }
    [[nodiscard]] size_t get_total_proofs() const { return total_proofs_; }
    
    // Add these declarations
    Proof generate_proof_parallel(const quantum::QuantumState& state);
    
protected:
    std::vector<uint8_t> generate_partial_proof(
        const quantum::QuantumState& state,
        size_t start_idx,
        size_t end_idx
    );
    
    Proof combine_partial_proofs(
        const std::vector<std::vector<uint8_t>>& partial_proofs
    );
    
private:
    [[nodiscard]] std::vector<size_t> generate_random_measurements(size_t n_qubits);
    [[nodiscard]] std::vector<double> generate_random_phases();
    void apply_random_transformations(quantum::QuantumState& state, const std::vector<double>& phases) const;
    [[nodiscard]] bool verify_measurements(
        const std::vector<bool>& expected,
        const std::vector<bool>& actual,
        double tolerance = 0.1  // Allow 10% error rate for quantum measurements
    ) const;
    
    // Optimal parameters storage
    std::vector<double> optimal_phase_angles_;
    size_t optimal_measurement_qubits_;
    double best_verification_time_;
    double best_success_rate_;
    
    // Performance tracking
    double avg_proof_time_{0.0};
    double success_rate_{1.0};
    size_t total_proofs_{0};
    
    // Random number generation
    std::unique_ptr<std::mt19937> rng_;
    
    // Constants
    static constexpr size_t MIN_QUBITS = 8;
    static constexpr size_t MAX_QUBITS = 1024;
    static constexpr double MIN_PHASE_ANGLE = -M_PI;
    static constexpr double MAX_PHASE_ANGLE = M_PI;
    static constexpr size_t MAX_PROOF_SIZE = 1024 * 1024;  // 1MB
};

struct QZKPProof {
    std::vector<uint8_t> proof_data;
    std::chrono::system_clock::time_point timestamp;
    
    // Default constructor
    QZKPProof() = default;
    
    // Constructor with data
    explicit QZKPProof(std::vector<uint8_t> data)
        : proof_data(std::move(data))
        , timestamp(std::chrono::system_clock::now())
    {}
    
    // Rule of 5
    QZKPProof(const QZKPProof&) = default;
    QZKPProof& operator=(const QZKPProof&) = default;
    QZKPProof(QZKPProof&&) noexcept = default;
    QZKPProof& operator=(QZKPProof&&) noexcept = default;
    ~QZKPProof() = default;
    
    [[nodiscard]] bool is_valid() const {
        return !proof_data.empty();
    }
};

// Free functions
[[nodiscard]] QZKPProof generate_commitment(const quantum::QuantumState& state);
[[nodiscard]] std::vector<uint8_t> sign_proof(const Eigen::MatrixXcd& matrix);
[[nodiscard]] std::vector<uint8_t> sign_proof(const std::vector<uint8_t>& proof_data);

} // namespace zkp
} // namespace quids 