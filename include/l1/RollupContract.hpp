#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>
#include "rollup/StateManager.hpp"
#include "rollup/RollupStateTransition.hpp"

class RollupContract {
public:
    struct ContractConfig {
        std::string l1_rpc_url;
        std::string contract_address;
        std::string private_key_path;
        uint64_t chain_id;
        uint64_t confirmation_blocks;
    };

    struct DepositEvent {
        std::string l1_address;
        std::string l2_address;
        uint64_t amount;
        uint64_t timestamp;
    };

    struct WithdrawalEvent {
        std::string l2_address;
        std::string l1_address;
        uint64_t amount;
        uint64_t timestamp;
    };

    explicit RollupContract(const ContractConfig& config);

    // State commitment
    bool submit_state_commitment(
        const std::array<uint8_t, 32>& state_root,
        const StateTransitionProof& proof
    );

    // Deposits and withdrawals
    std::vector<DepositEvent> get_pending_deposits();
    bool process_withdrawal(const WithdrawalEvent& withdrawal);
    
    // Fraud proof
    bool submit_fraud_proof(
        const StateTransitionProof& invalid_proof,
        const StateManager& correct_state
    );
    
    // Validator management
    bool register_validator(const std::string& validator_address);
    bool remove_validator(const std::string& validator_address);
    std::vector<std::string> get_active_validators() const;
    
    // Emergency functions
    bool trigger_emergency_shutdown();
    bool is_emergency_mode() const;

private:
    ContractConfig config_;
    std::string operator_address_;
    
    bool verify_l1_transaction(const std::string& tx_hash, uint64_t wait_blocks);
    void monitor_events();
    void handle_deposit_event(const DepositEvent& event);
}; 