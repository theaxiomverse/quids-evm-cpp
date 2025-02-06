#pragma once
#include "quantum/QuantumState.h"
#include "zkp/QZKPVerifier.h"

class ZKPBlockchain {
public:
    struct Block {
        struct Header {
            std::array<uint8_t, 32> previous_hash;
            std::array<uint8_t, 32> merkle_root;
            uint64_t timestamp;
            ZKPGenerator::Proof proof;
        };
        
        std::vector<Transaction> transactions;
        Header header;
    };
    
    bool validate_block(const Block& block) {
        // Classical validation
        if (!verify_merkle_root(block)) return false;
        
        // Quantum ZKP validation
        return verifier_.verify_proof(
            block.header.proof, 
            current_state_hash()
        );
    }
    
private:
    QZKPVerifier verifier_;
    std::array<uint8_t, 32> current_state_hash_;
}; 