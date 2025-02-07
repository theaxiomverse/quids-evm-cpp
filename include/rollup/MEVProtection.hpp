#pragma once
#include "blockchain/Transaction.hpp"
#include <vector>
#include <unordered_map>
#include <array>
#include <cstdint>

using quids::blockchain::Transaction;

class MEVProtection {
public:
    struct OrderingCommitment {
        std::array<uint8_t, 32> batch_hash;
        std::vector<Transaction> transactions;
        uint64_t timestamp;
    };
    
    void add_transaction(const Transaction& tx);
    std::vector<Transaction> get_optimal_ordering();
    OrderingCommitment create_ordering_commitment(
        const std::vector<Transaction>& transactions
    );
    
    std::array<uint8_t, 32> compute_fairness_hash(
        const std::vector<Transaction>& transactions
    );

private:
    std::vector<Transaction> pending_transactions_;
    std::unordered_map<std::string, double> profit_estimates_;
    
    double estimate_profit(const Transaction& tx);
    bool detect_frontrunning(const Transaction& tx1, const Transaction& tx2);
}; 