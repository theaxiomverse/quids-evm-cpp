#include "rollup/CrossRollupBridge.hpp"
#include <openssl/evp.h>
#include <cstring>
#include <stdexcept>

namespace {
std::array<uint8_t, 32> compute_message_hash(const quids::rollup::CrossRollupBridge::CrossRollupMessage& message) {
    std::array<uint8_t, 32> hash;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP context");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize SHA256");
    }

    // Hash message fields
    if (EVP_DigestUpdate(ctx, &message.source_chain_id, sizeof(message.source_chain_id)) != 1 ||
        EVP_DigestUpdate(ctx, &message.destination_chain_id, sizeof(message.destination_chain_id)) != 1 ||
        EVP_DigestUpdate(ctx, message.payload.data(), message.payload.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update digest");
    }

    unsigned int hash_len;
    if (EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(ctx);
    return hash;
}
}  // namespace

namespace quids {
namespace rollup {

void CrossRollupBridge::send_message(const CrossRollupMessage& message) {
    // Hash the message for verification
    auto message_hash = compute_message_hash(message);
    
    // Store message hash for verification
    message_hashes_.push_back(message_hash);
}

bool CrossRollupBridge::verify_incoming_message(const CrossRollupMessage& message) {
    // Hash the message
    auto message_hash = compute_message_hash(message);
    
    // Verify message hash exists
    return std::find(message_hashes_.begin(), message_hashes_.end(), message_hash) != message_hashes_.end();
}

} // namespace rollup
} // namespace quids 