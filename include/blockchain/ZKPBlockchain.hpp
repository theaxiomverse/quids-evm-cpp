#pragma once
#include "quantum/QuantumState.hpp"
#include "zkp/QZKPVerifier.hpp"
#include "blockchain/Transaction.hpp"

namespace quids {
namespace blockchain {

class ZKPBlockchain {
public:
    struct Block {
        struct Header {
            std::array<uint8_t, 32> previous_hash;
            std::array<uint8_t, 32> merkle_root;
            uint64_t timestamp;
            quids::zkp::QZKPGenerator::Proof proof;
        };
        
        std::vector<Transaction> transactions;
        Header header;
    };
    
    bool validate_block(const Block& block) {
        // Classical validation
        if (!verify_merkle_root(block)) return false;
        
        // Quantum ZKP validation
        auto verification = verifier_.verify_proof(
            block.header.proof, 
            current_state_hash_
        );
        
        return verification.result == quids::zkp::QZKPVerifier::VerificationResult::VALID;
    }
    
private:
    quids::zkp::QZKPVerifier verifier_;
    std::array<uint8_t, 32> current_state_hash_;
    
    bool verify_merkle_root(const Block& block) const;
};

} // namespace blockchain
} // namespace quids 