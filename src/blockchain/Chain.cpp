#include "blockchain/Chain.hpp"
#include <spdlog/spdlog.h>

namespace quids {
namespace blockchain {

Chain::Chain(const ChainConfig& config, evm::EVMExecutor* evm_executor)
    : evm_executor_(evm_executor) {
    // Initialize genesis block
    Block genesis;
    chain_.push_back(genesis);
    current_height_ = 1;
}

bool Chain::addBlock(const Block& block) {
    try {
        // Verify block
        if (!block.verify()) {
            return false;
        }

        // Execute transactions
        for (const auto& tx : block.transactions) {
            if (!evm_executor_->execute(tx)) {
                return false;
            }
        }

        chain_.push_back(block);
        current_height_++;
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to add block: {}", e.what());
        return false;
    }
}

bool Chain::addTransaction(const Transaction& tx) {
    try {
        if (!tx.verify()) {
            return false;
        }
        pending_transactions_.push_back(tx);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to add transaction: {}", e.what());
        return false;
    }
}

Block Chain::getLatestBlock() const {
    return chain_.back();
}

std::vector<Transaction> Chain::getPendingTransactions() const {
    return pending_transactions_;
}

} // namespace blockchain
} // namespace quids 