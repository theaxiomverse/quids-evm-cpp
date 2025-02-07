#pragma once
#include <array>
#include <vector>
#include <queue>
#include <memory>
#include <cstdint>
#include "zkp/QZKPGenerator.hpp"
#include "rollup/ProofAggregator.hpp"

using quids::zkp::QZKPGenerator;

class L1Bridge {
public:
    struct L1Commitment {
        std::array<uint8_t, 32> state_root;
        std::array<uint8_t, 32> batch_hash;
        size_t batch_size;
        std::vector<uint8_t> aggregated_proof;
    };

    void submit_commitment(const L1Commitment& commitment);
    bool verify_commitment(const L1Commitment& commitment);
    
    // L1 -> L2 message passing
    void send_message_to_l2(const std::vector<uint8_t>& message);
    void process_l2_messages();
    
private:
    std::unique_ptr<ProofAggregator> proof_aggregator_;
    std::queue<std::vector<uint8_t>> pending_messages_;
    
    bool verify_state_transition(
        const std::array<uint8_t, 32>& pre_state_root,
        const std::array<uint8_t, 32>& post_state_root,
        const quids::zkp::QZKPGenerator::Proof& proof
    );
}; 