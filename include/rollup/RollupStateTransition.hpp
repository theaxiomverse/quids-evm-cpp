#pragma once

#include <Eigen/Dense>
#include <vector>
#include <array>
#include <memory>
#include "quantum/QuantumState.hpp"
#include "blockchain/Transaction.hpp"
#include "rollup/StateManager.hpp"
#include "zkp/QZKPGenerator.hpp"

namespace quids {
namespace rollup {

using quids::zkp::QZKPGenerator;

struct StateTransitionProof {
    std::array<uint8_t, 32> pre_state_root;
    std::array<uint8_t, 32> post_state_root;
    std::vector<blockchain::Transaction> transactions;
    std::vector<uint8_t> proof_data;
    uint64_t timestamp;
    uint64_t batch_number;
    std::array<uint8_t, 32> batch_hash;
};

class RollupStateTransition {
public:
    // Constructor and destructor
    explicit RollupStateTransition(std::shared_ptr<QZKPGenerator> zkp_generator);
    ~RollupStateTransition() = default;

    // Disable copy
    RollupStateTransition(const RollupStateTransition&) = delete;
    RollupStateTransition& operator=(const RollupStateTransition&) = delete;

    // Enable move
    RollupStateTransition(RollupStateTransition&&) noexcept = default;
    RollupStateTransition& operator=(RollupStateTransition&&) noexcept = default;

    // Core functionality
    [[nodiscard]] StateTransitionProof generate_transition_proof(
        const std::vector<blockchain::Transaction>& batch,
        const StateManager& state_manager
    );

    [[nodiscard]] bool verify_transition(
        const StateManager& pre_state,
        const StateManager& post_state,
        const std::vector<blockchain::Transaction>& transactions
    );

    // Batch validation
    [[nodiscard]] bool validate_batch(const std::vector<blockchain::Transaction>& batch) const;
    [[nodiscard]] bool verify_batch_ordering(const std::vector<blockchain::Transaction>& batch) const;
    
    // Proof verification
    [[nodiscard]] bool verify_proof(const StateTransitionProof& proof) const;
    [[nodiscard]] bool verify_state_roots(
        const std::array<uint8_t, 32>& pre_root,
        const std::array<uint8_t, 32>& post_root
    ) const;

private:
    std::shared_ptr<QZKPGenerator> zkp_generator_;
    uint64_t batch_number_{0};  // Add batch number counter
    
    // Internal helper methods
    [[nodiscard]] quantum::QuantumState encode_batch_to_quantum_state(
        const std::vector<blockchain::Transaction>& batch
    ) const;
    
    [[nodiscard]] bool verify_transaction_sequence(
        const std::vector<blockchain::Transaction>& transactions
    ) const;
    
    [[nodiscard]] std::array<uint8_t, 32> compute_post_state_root(
        const std::vector<blockchain::Transaction>& batch,
        const StateManager& state_manager
    );
    
    [[nodiscard]] std::array<uint8_t, 32> compute_batch_hash(
        const std::vector<blockchain::Transaction>& batch
    );
    
    void validate_proof_data(const std::vector<uint8_t>& proof_data) const;
    
    // Constants
    static constexpr size_t MAX_BATCH_SIZE = 1000;
    static constexpr size_t MIN_BATCH_SIZE = 1;
    static constexpr uint64_t MAX_PROOF_SIZE = 1024 * 1024;  // 1MB
};

} // namespace rollup
} // namespace quids 