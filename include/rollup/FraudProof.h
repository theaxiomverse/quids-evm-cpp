#pragma once
#include "StateManager.h"
#include "zkp/QZKPGenerator.h"
#include "quantum/QuantumState.h"
#include <memory>

class FraudProof {
public:
    struct StateProof {
        std::unique_ptr<StateManager> pre_state;
        std::unique_ptr<StateManager> post_state;
        
        StateProof() = default;
        
        StateProof(const StateManager& pre, const StateManager& post)
            : pre_state(std::make_unique<StateManager>(pre)),
              post_state(std::make_unique<StateManager>(post)) {}
              
        StateProof(StateProof&&) = default;
        StateProof& operator=(StateProof&&) = default;
    };
    
    struct InvalidTransitionProof {
        std::array<uint8_t, 32> pre_state_root;
        std::array<uint8_t, 32> post_state_root;
        std::vector<Transaction> transactions;
        StateProof state_proof;
        QZKPGenerator::Proof validity_proof;
    };
    
    struct FraudVerificationResult {
        bool is_valid;
        std::string message;
    };
    
    FraudProof(std::shared_ptr<QZKPGenerator> zkp_generator)
        : zkp_generator_(zkp_generator) {}
    
    InvalidTransitionProof generate_fraud_proof(
        std::unique_ptr<StateManager> pre_state,
        std::unique_ptr<StateManager> post_state,
        const std::vector<Transaction>& transactions
    );
    
    FraudVerificationResult verify_fraud_proof(
        const InvalidTransitionProof& proof
    );
    
private:
    std::shared_ptr<QZKPGenerator> zkp_generator_;
    
    bool verify_state_roots(const InvalidTransitionProof& proof);
    bool verify_state_transition(const InvalidTransitionProof& proof);
    bool verify_zkp_proof(const InvalidTransitionProof& proof);
    StateProof generate_state_proof(
        const StateManager& pre_state,
        const StateManager& post_state
    );
    quantum::QuantumState encode_state_diff(
        const StateManager& pre_state,
        const StateManager& post_state
    );
}; 