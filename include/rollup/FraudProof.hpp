#pragma once
#include "StateManager.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "quantum/QuantumState.hpp"
#include <memory>

namespace quids {
namespace rollup {

class FraudProof {
public:
    struct StateProof {
        std::unique_ptr<quids::rollup::StateManager> pre_state;
        std::unique_ptr<quids::rollup::StateManager> post_state;
        
        StateProof() = default;
        
        StateProof(const quids::rollup::StateManager& pre, const quids::rollup::StateManager& post)
            : pre_state(std::make_unique<quids::rollup::StateManager>(pre)),
              post_state(std::make_unique<quids::rollup::StateManager>(post)) {}
              
        StateProof(StateProof&&) = default;
        StateProof& operator=(StateProof&&) = default;
    };
    
    struct InvalidTransitionProof {
        std::array<uint8_t, 32> pre_state_root;
        std::array<uint8_t, 32> post_state_root;
        std::vector<quids::blockchain::Transaction> transactions;
        StateProof state_proof;
        quids::zkp::QZKPGenerator::Proof validity_proof;
    };
    
    struct FraudVerificationResult {
        bool is_valid;
        std::string message;
    };
    
    FraudProof(std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator)
        : zkp_generator_(zkp_generator) {}
    
    InvalidTransitionProof generate_fraud_proof(
        std::unique_ptr<quids::rollup::StateManager> pre_state,
        std::unique_ptr<quids::rollup::StateManager> post_state,
        const std::vector<quids::blockchain::Transaction>& transactions
    );
    
    FraudVerificationResult verify_fraud_proof(
        const InvalidTransitionProof& proof
    );
    
private:
    std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator_;
    
    bool verify_state_roots(const InvalidTransitionProof& proof);
    bool verify_state_transition(const InvalidTransitionProof& proof);
    bool verify_zkp_proof(const InvalidTransitionProof& proof);
    StateProof generate_state_proof(
        const quids::rollup::StateManager& pre_state,
        const quids::rollup::StateManager& post_state
    );
    quantum::QuantumState encode_state_diff(
        const quids::rollup::StateManager& pre_state,
        const quids::rollup::StateManager& post_state
    );
};

} // namespace rollup
} // namespace quids 