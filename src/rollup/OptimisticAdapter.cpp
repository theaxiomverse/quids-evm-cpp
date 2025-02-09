#include "rollup/OptimisticAdapter.hpp"
#include "zkp/QZKPGenerator.hpp"
#include <chrono>

using quids::zkp::QZKPGenerator;
using quids::rollup::StateManager;
using quids::rollup::StateTransitionProof;

OptimisticAdapter::OptimisticProof OptimisticAdapter::convert_to_optimistic(
    const StateTransitionProof& zk_proof) {
    OptimisticProof optimistic_proof;
    optimistic_proof.zk_proof = zk_proof;
    optimistic_proof.challenge_period_end = std::chrono::system_clock::now() + 
                                          std::chrono::hours(24 * 7);
    optimistic_proof.has_fraud_proof = false;
    return optimistic_proof;
}

bool OptimisticAdapter::verify_optimistic_proof(
    const OptimisticProof& proof,
    const StateManager& state_manager
) {
    (void)state_manager;  // Suppress unused parameter warning

    // Proceed with the rest of the function
    StateManager temp_state;
    
    for (const auto& tx : proof.zk_proof.transactions) {
        if (!temp_state.apply_transaction(tx)) {
            return false;
        }
    }
    
    return true;
}

bool OptimisticAdapter::verify_state_transition(
    const StateTransitionProof& proof,
    const StateManager& state_manager
) {
    // Get and convert current pre-state root to array
    auto current_state_root_vec = state_manager.get_state_root();
    if (current_state_root_vec.size() < 32) {
        return false;
    }
    std::array<uint8_t, 32> current_state_root;
    std::copy_n(current_state_root_vec.begin(), 32, current_state_root.begin());
    if (proof.pre_state_root != current_state_root) {
        return false;
    }

    // Apply all transactions to a temporary state
    StateManager temp_state;
    for (const auto& tx : proof.transactions) {
        if (!temp_state.apply_transaction(tx)) {
            return false;
        }
    }
    
    // Verify the post-state root matches
    auto state_root_vec = temp_state.get_state_root();
    if (state_root_vec.size() < 32) {
        return false;
    }
    std::array<uint8_t, 32> state_root_array;
    std::copy_n(state_root_vec.begin(), 32, state_root_array.begin());
    return proof.post_state_root == state_root_array;
}

// OptimisticAdapter::OptimisticProof OptimisticAdapter::generate_proof(
//     const std::vector<quids::blockchain::Transaction>& transactions
// ) {
//     // Implementation code...
// } 