#include "blockchain/Block.hpp"
#include <spdlog/spdlog.h>
#include <openssl/sha.h>

namespace quids {
namespace blockchain {

bool Block::verify() const {
    try {
        // Verify block hash
        if (hash.empty() || prev_hash.empty()) {
            return false;
        }

        // Verify transactions
        for (const auto& tx : transactions) {
            if (!tx.verify()) {
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Block verification failed: {}", e.what());
        return false;
    }
}

} // namespace blockchain
} // namespace quids 