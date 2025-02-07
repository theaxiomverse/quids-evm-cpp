#include "rollup/RollupStateTransition.hpp"
#include <Eigen/Dense>
#include <stdexcept>

namespace quids {
namespace rollup {

RollupStateTransition::RollupStateTransition(std::shared_ptr<QZKPGenerator> zkp_generator)
    : zkp_generator_(std::move(zkp_generator)) {}

StateTransitionProof RollupStateTransition::generate_transition_proof(
    const std::vector<Transaction>& batch,
    const StateManager& state_manager
) {
    // Create quantum state from batch
    auto batch_state = encode_batch_to_quantum_state(batch);
    
    // Generate pre-state root
    auto pre_state_root = state_manager.get_state_root();
    
    // Apply transactions and get post state
    StateManager temp_state = state_manager;  // Create temporary copy
    for (const auto& tx : batch) {
        if (!temp_state.apply_transaction(tx)) {
            throw std::runtime_error("Invalid transaction in batch");
        }
    }
    
    // Generate post-state root
    auto post_state_root = temp_state.get_state_root();
    
    // Generate quantum proof
    auto quantum_proof = zkp_generator_->generate_proof(batch_state);
    
    return StateTransitionProof{
        pre_state_root,
        post_state_root,
        batch,
        quantum_proof.proof_data,
        static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()),  // timestamp
        0,  // batch_number
        {}  // batch_hash
    };
}

bool RollupStateTransition::verify_transition(
    const StateManager& pre_state,
    const StateManager& post_state,
    const std::vector<Transaction>& transactions
) {
    // Verify pre-state matches
    if (pre_state.get_state_root() != post_state.get_state_root()) {
        return false;
    }
    
    // Apply transactions to pre-state
    StateManager temp_state = pre_state;
    for (const auto& tx : transactions) {
        if (!temp_state.apply_transaction(tx)) {
            return false;
        }
    }
    
    // Verify final state matches
    return temp_state.get_state_root() == post_state.get_state_root();
}

quantum::QuantumState RollupStateTransition::encode_batch_to_quantum_state(
    const std::vector<Transaction>& batch
) const {
    // Convert batch to quantum state vector
    const size_t state_size = batch.size() * 256; // 256 bits per tx
    Eigen::VectorXcd state_vector(state_size);
    
    for (size_t i = 0; i < batch.size(); i++) {
        auto tx_data = batch[i].serialize();
        for (size_t j = 0; j < tx_data.size(); j++) {
            double amplitude = tx_data[j] / 255.0; // Normalize to [0,1]
            state_vector(i * 256 + j) = std::complex<double>(amplitude, 0);
        }
    }
    
    return quantum::QuantumState(state_vector);
}

bool RollupStateTransition::verify_transaction_sequence(
    const std::vector<Transaction>& transactions
) const {
    // Verify transaction sequence is valid (e.g., nonces are sequential)
    for (size_t i = 1; i < transactions.size(); i++) {
        if (transactions[i].nonce <= transactions[i-1].nonce) {
            return false;
        }
    }
    return true;
}

} // namespace rollup
} // namespace quids 