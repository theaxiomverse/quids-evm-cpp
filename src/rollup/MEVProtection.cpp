#include "rollup/MEVProtection.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <stdexcept>

using quids::blockchain::Transaction;

MEVProtection::OrderingCommitment MEVProtection::create_ordering_commitment(
    const std::vector<Transaction>& transactions
) {
    OrderingCommitment commitment;
    commitment.transactions = transactions;
    commitment.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    commitment.batch_hash = compute_fairness_hash(transactions);
    return commitment;
}

void MEVProtection::add_transaction(const Transaction& tx) {
    pending_transactions_.push_back(tx);
}

std::vector<Transaction> MEVProtection::get_optimal_ordering() {
    // Sort transactions by profit estimate
    std::vector<Transaction> ordered = pending_transactions_;
    std::sort(ordered.begin(), ordered.end(),
        [this](const Transaction& a, const Transaction& b) {
            return estimate_profit(a) > estimate_profit(b);
        });
    return ordered;
}

std::array<uint8_t, 32> MEVProtection::compute_fairness_hash(
    const std::vector<Transaction>& transactions
) {
    std::array<uint8_t, 32> hash;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP context");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize SHA256");
    }

    // Hash each transaction in order
    for (const auto& tx : transactions) {
        // Hash transaction fields
        if (EVP_DigestUpdate(ctx, tx.sender.data(), tx.sender.size()) != 1 ||
            EVP_DigestUpdate(ctx, tx.recipient.data(), tx.recipient.size()) != 1 ||
            EVP_DigestUpdate(ctx, &tx.amount, sizeof(tx.amount)) != 1 ||
            EVP_DigestUpdate(ctx, &tx.nonce, sizeof(tx.nonce)) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to update digest");
        }
    }

    unsigned int hash_len;
    if (EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(ctx);
    return hash;
}

double MEVProtection::estimate_profit(const quids::blockchain::Transaction& tx) {
    // Simple profit estimate based on transaction amount
    return static_cast<double>(tx.amount);
}

bool MEVProtection::detect_frontrunning(const quids::blockchain::Transaction& tx1, const quids::blockchain::Transaction& tx2) {
    // Simple frontrunning detection based on profit estimates
    return estimate_profit(tx1) < estimate_profit(tx2);
} 