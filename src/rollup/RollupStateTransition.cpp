#include "rollup/RollupStateTransition.hpp"
#include "blockchain/Transaction.hpp"
#include <Eigen/Dense>
#include <stdexcept>
#include <blake3.h>
#include <openssl/sha.h>
#include <chrono>
#include <memory>
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <omp.h>
#include "rollup/StateManager.hpp"
#include "quantum/QuantumState.hpp"
#include "evm/uint256.hpp"


namespace quids {
namespace rollup {

RollupStateTransition::RollupStateTransition(std::shared_ptr<QZKPGenerator> zkp_generator)
    : zkp_generator_(std::move(zkp_generator)) {}

StateTransitionProof RollupStateTransition::generate_transition_proof(
    const std::vector<blockchain::Transaction>& batch,
    const StateManager& state_manager
) {
    // (1) Generate ZKP proof data (assume generate_proof takes a quantum state as input)
    // For example, assume you have already created your quantum state from the batch.
    quantum::QuantumState quantum_state = encode_batch_to_quantum_state(batch); // your implementation
    auto proofResult = zkp_generator_->generate_proof(quantum_state);
    std::vector<uint8_t> local_proof_data = proofResult.proof_data;

    // (2) Convert the pre-state root from the state manager (which returns a std::vector<uint8_t>)
    auto vec_root = state_manager.get_state_root();
    if(vec_root.size() < 32)
        throw std::runtime_error("State root size is invalid");
    std::array<uint8_t, 32> pre_state_root{};
    std::copy_n(vec_root.begin(), 32, pre_state_root.begin());

    // (3) Compute the post-state root and the batch hash (both returning std::array<uint8_t, 32>)
    std::array<uint8_t, 32> post_state_root = compute_post_state_root(batch, state_manager);
    std::array<uint8_t, 32> batch_hash = compute_batch_hash(batch);

    // (4) Get the timestamp and batch number (ensure these are uint64_t)
    uint64_t timestamp = static_cast<uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    uint64_t batch_number = batch_number_++; // assuming batch_number_ is defined as uint64_t

    // (5) Now build the StateTransitionProof by assigning each field explicitly.
    StateTransitionProof proof;
    proof.pre_state_root = pre_state_root;
    proof.post_state_root = post_state_root;
    proof.transactions = batch;
    proof.proof_data = std::move(local_proof_data);
    proof.timestamp = timestamp;
    proof.batch_number = batch_number;
    proof.batch_hash = batch_hash;

    return proof;
}

bool RollupStateTransition::verify_transition(
    const StateManager& pre_state,
    const StateManager& post_state,
    const std::vector<blockchain::Transaction>& txs
) {
    // 1. Verify initial state matches
    if (pre_state.get_state_root() != post_state.get_previous_root()) {
        return false;
    }

    // 2. Apply transactions to pre-state copy
    auto temp_state = pre_state.clone();
    for (const auto& tx : txs) {
        if (!temp_state->apply_transaction(tx)) {
            return false;
        }
    }

    // 3. Verify final state matches post_state
    return temp_state->get_state_root() == post_state.get_state_root();
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
    // Convert vector to array
    auto vec_root = state_manager.get_state_root();
    std::array<uint8_t, 32> arr_root;
    std::copy_n(vec_root.begin(), 32, arr_root.begin());
    return arr_root;
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