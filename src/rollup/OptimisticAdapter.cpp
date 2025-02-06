#include "rollup/OptimisticAdapter.h"
#include "zkp/QZKPGenerator.h"

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
    const StateManager& state_manager) {
    if (std::chrono::system_clock::now() < proof.challenge_period_end) {
        return false;  // Challenge period not over yet
    }
    
    if (proof.has_fraud_proof) {
        return false;  // Fraud proof exists
    }
    
    return verify_state_transition(proof.zk_proof, state_manager);
}

bool OptimisticAdapter::verify_state_transition(
    const StateTransitionProof& proof,
    const StateManager& state_manager
) {
    // Verify the pre-state root matches current state
    if (proof.pre_state_root != state_manager.get_state_root()) {
        return false;
    }
    
    // Create a temporary state manager for verification
    StateManager temp_state = state_manager;
    
    // Apply all transactions
    for (const auto& tx : proof.transactions) {
        if (!temp_state.apply_transaction(tx)) {
            return false;
        }
    }
    
    // Verify the post-state root matches
    return proof.post_state_root == temp_state.get_state_root();
} 