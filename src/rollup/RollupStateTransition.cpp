#include "rollup/RollupStateTransition.hpp"
#include "blockchain/Transaction.hpp"
#include <Eigen/Dense>
#include <stdexcept>
#include <blake3.h>
#include <openssl/sha.h>

namespace quids {
namespace rollup {

RollupStateTransition::RollupStateTransition(std::shared_ptr<QZKPGenerator> zkp_generator)
    : zkp_generator_(std::move(zkp_generator)) {}

StateTransitionProof RollupStateTransition::generate_transition_proof(
    const std::vector<blockchain::Transaction>& batch,
    const StateManager& state_manager
) {
    // Pre-allocate vectors to avoid resizing
    std::vector<uint8_t> proof_data;
    proof_data.reserve(1024);  // Reserve reasonable size

    // Use quantum state with optimal size
    size_t num_qubits = std::min(
        static_cast<size_t>(std::log2(batch.size() * 256)) + 1,
        static_cast<size_t>(24)  // Max reasonable size
    );
    quantum::QuantumState state(num_qubits);

    // Batch encode transactions
    auto encoded_state = encode_batch_to_quantum_state(batch);
    
    // Generate proof in parallel if batch is large enough
    auto proof = batch.size() > 100 ? 
        zkp_generator_->generate_proof_parallel(encoded_state) :
        zkp_generator_->generate_proof(encoded_state);

    return StateTransitionProof{
        state_manager.get_state_root(),
        compute_post_state_root(batch, state_manager),
        batch,
        std::move(proof.proof_data),
        static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()),
        batch_number_++,
        compute_batch_hash(batch)
    };
}

bool RollupStateTransition::verify_transition(
    const StateManager& pre_state,
    const StateManager& post_state,
    const std::vector<blockchain::Transaction>& transactions
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

bool RollupStateTransition::validate_batch(
    [[maybe_unused]] const std::vector<blockchain::Transaction>& batch
) const {
    // TODO: Implement batch validation
    return true;
}

bool RollupStateTransition::verify_batch_ordering(
    [[maybe_unused]] const std::vector<blockchain::Transaction>& batch
) const {
    // TODO: Implement batch ordering verification
    return true;
}

quantum::QuantumState RollupStateTransition::encode_batch_to_quantum_state(
    const std::vector<blockchain::Transaction>& batch
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
    const std::vector<blockchain::Transaction>& transactions
) const {
    // Verify transaction sequence is valid (e.g., nonces are sequential)
    for (size_t i = 1; i < transactions.size(); i++) {
        if (transactions[i].nonce <= transactions[i-1].nonce) {
            return false;
        }
    }
    return true;
}

std::array<uint8_t, 32> RollupStateTransition::compute_post_state_root(
    const std::vector<blockchain::Transaction>& batch,
    const StateManager& state_manager
) {
    StateManager temp_state = state_manager;
    for (const auto& tx : batch) {
        if (!temp_state.apply_transaction(tx)) {
            throw std::runtime_error("Invalid transaction in batch");
        }
    }
    return temp_state.get_state_root();
}

std::array<uint8_t, 32> RollupStateTransition::compute_batch_hash(
    const std::vector<blockchain::Transaction>& batch
) {
    std::array<uint8_t, 32> hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    
    for (const auto& tx : batch) {
        auto tx_data = tx.serialize();
        blake3_hasher_update(&hasher, tx_data.data(), tx_data.size());
    }
    
    blake3_hasher_finalize(&hasher, hash.data(), hash.size());
    return hash;
}

} // namespace rollup
} // namespace quids 