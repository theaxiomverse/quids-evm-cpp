#pragma once
#include <queue>
#include <memory>
#include "RollupStateTransition.hpp"

class L2BlockProcessor {
public:
    struct L2Block {
        uint64_t block_number;
        std::vector<Transaction> transactions;
        std::array<uint8_t, 32> state_root;
        std::array<uint8_t, 32> previous_hash;
        ZKPGenerator::Proof block_proof;
        uint64_t timestamp;
    };

    L2Block create_block(const std::vector<Transaction>& batch);
    bool verify_block(const L2Block& block);
    void apply_block(const L2Block& block);
    
    // L1 synchronization
    void submit_block_to_l1(const L2Block& block);
    void process_l1_events();
    
private:
    std::unique_ptr<StateManager> state_manager_;
    std::unique_ptr<RollupStateTransition> state_transition_;
    uint64_t current_block_number_{0};
    
    std::array<uint8_t, 32> calculate_block_hash(const L2Block& block);
    bool verify_block_proof(const L2Block& block);
}; 