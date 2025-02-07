#include "rollup/L1Bridge.hpp"
#include <blake3.h>

void L1Bridge::submit_commitment(const L1Commitment& commitment) {
    // In a real implementation, this would submit the commitment to L1
    // For now, we just verify the commitment locally
    verify_commitment(commitment);
}

bool L1Bridge::verify_commitment(const L1Commitment& commitment) {
    // Verify proof size matches batch size
    if (commitment.aggregated_proof.size() != commitment.batch_size * 32) {
        return false;
    }
    
    // Verify state root and batch hash are valid
    bool has_valid_roots = false;
    for (size_t i = 0; i < 32; i++) {
        if (commitment.state_root[i] != 0) {
            has_valid_roots = true;
        }
        if (commitment.batch_hash[i] != 0) {
            has_valid_roots = true;
        }
    }
    
    if (!has_valid_roots) {
        return false;
    }
    
    // Verify the aggregated proof
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    
    // Hash the state root and batch hash together
    blake3_hasher_update(&hasher, commitment.state_root.data(), 32);
    blake3_hasher_update(&hasher, commitment.batch_hash.data(), 32);
    
    std::array<uint8_t, 32> expected_hash;
    blake3_hasher_finalize(&hasher, expected_hash.data(), 32);
    
    // Verify the first 32 bytes of the aggregated proof match our expected hash
    return std::memcmp(expected_hash.data(), commitment.aggregated_proof.data(), 32) == 0;
}

void L1Bridge::send_message_to_l2(const std::vector<uint8_t>& message) {
    pending_messages_.push(message);
}

void L1Bridge::process_l2_messages() {
    while (!pending_messages_.empty()) {
        auto message = pending_messages_.front();
        pending_messages_.pop();
        // Process message...
    }
} 