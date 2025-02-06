#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include "zkp/QZKPGenerator.h"

class CrossRollupBridge {
public:
    struct CrossRollupMessage {
        uint32_t source_chain_id;
        uint32_t destination_chain_id;
        std::vector<uint8_t> payload;
        QZKPGenerator::Proof validity_proof;
    };
    
    void send_message(const CrossRollupMessage& message);
    bool verify_incoming_message(const CrossRollupMessage& message);
    
private:
    std::vector<std::array<uint8_t, 32>> message_hashes_;
}; 