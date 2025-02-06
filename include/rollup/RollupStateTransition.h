#pragma once
#include <Eigen/Dense>
#include <vector>
#include <array>
#include "quantum/QuantumState.h"
#include "blockchain/Transaction.h"
#include "StateManager.h"
#include "zkp/QZKPGenerator.h"
#include <memory>

struct StateTransitionProof {
    std::array<uint8_t, 32> pre_state_root;
    std::array<uint8_t, 32> post_state_root;
    std::vector<Transaction> transactions;
    std::vector<uint8_t> proof_data;
};

class RollupStateTransition {
public:
    RollupStateTransition(std::shared_ptr<QZKPGenerator> zkp_generator);
    
    StateTransitionProof generate_transition_proof(
        const std::vector<Transaction>& batch,
        const StateManager& state_manager
    );

    bool verify_transition(
        const StateManager& pre_state,
        const StateManager& post_state,
        const std::vector<Transaction>& transactions
    );

private:
    std::shared_ptr<QZKPGenerator> zkp_generator_;
    quantum::QuantumState encode_batch_to_quantum_state(const std::vector<Transaction>& batch);
    
    bool verify_transaction_sequence(
        const std::vector<Transaction>& transactions
    );
}; 