#pragma once
#include "blockchain/Block.hpp"
#include "blockchain/Transaction.hpp"
#include "blockchain/Account.hpp"
#include "evm/EVMExecutor.hpp"
#include "node/QuidsConfig.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

namespace quids {
namespace blockchain {

class Chain {
public:
    explicit Chain(const ChainConfig& config, evm::EVMExecutor* evm_executor);
    ~Chain() = default;

    // Chain operations
    bool addBlock(const Block& block);
    bool addTransaction(const Transaction& tx);
    
    // State queries
    uint64_t getHeight() const { return current_height_; }
    Block getLatestBlock() const;
    std::vector<Transaction> getPendingTransactions() const;
    Account getAccount(const std::string& address) const;

private:
    uint64_t current_height_{0};
    evm::EVMExecutor* evm_executor_;
    std::vector<Block> chain_;
    std::vector<Transaction> pending_transactions_;
    std::unordered_map<std::string, Account> accounts_;
    ChainConfig config_;
};

} // namespace blockchain
} // namespace quids 