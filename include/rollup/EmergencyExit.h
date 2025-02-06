#pragma once
#include <memory>
#include "rollup/StateManager.h"
#include "zkp/QZKPGenerator.h"
#include "quantum/QuantumState.h"

class EmergencyExit {
public:
    struct ExitProof {
        std::string account_address;
        uint64_t balance;
        std::array<uint8_t, 32> state_root;
        QZKPGenerator::Proof validity_proof;
    };
    
    EmergencyExit(
        std::unique_ptr<StateManager>& state_manager,
        std::shared_ptr<QZKPGenerator> zkp_generator
    );
    
    ExitProof generate_exit_proof(
        const std::string& address,
        const StateManager& state
    );
    
    bool verify_exit_proof(const ExitProof& proof);
    void process_emergency_exit(const ExitProof& proof);
    
    // Get a const reference to the state manager
    const StateManager& get_state() const { return *state_manager_; }
    
private:
    std::unique_ptr<StateManager> state_manager_;
    std::shared_ptr<QZKPGenerator> zkp_generator_;
    
    quantum::QuantumState encode_account_state(const StateManager::Account& account);
}; 