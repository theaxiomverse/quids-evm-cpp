#include "rollup/MEVProtection.hpp"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <stdexcept>
#include <cmath>

namespace quids {
namespace rollup {

using quids::blockchain::Transaction;

MEVProtection::OrderingCommitment MEVProtection::create_ordering_commitment(
    const std::vector<Transaction>& transactions
) {
    OrderingCommitment commitment;
    commitment.transactions = transactions;
    commitment.timestamp = std::chrono::system_clock::now();
    commitment.batch_hash = compute_fairness_hash(transactions);
    return commitment;
}

void MEVProtection::add_transaction(const Transaction& tx) {
    pending_transactions_.push_back(tx);
}

std::vector<Transaction> MEVProtection::get_optimal_ordering() {
    // Sort transactions by estimated profit
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
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    for (const auto& tx : transactions) {
        auto sender = tx.getSender();
        auto recipient = tx.getRecipient();
        auto amount = tx.getAmount();
        auto nonce = tx.getNonce();
        
        SHA256_Update(&sha256, sender.data(), sender.size());
        SHA256_Update(&sha256, recipient.data(), recipient.size());
        SHA256_Update(&sha256, &amount, sizeof(amount));
        SHA256_Update(&sha256, &nonce, sizeof(nonce));
    }

    std::array<uint8_t, 32> hash;
    SHA256_Final(hash.data(), &sha256);
    return hash;
}

double MEVProtection::estimate_profit(const Transaction& tx) {
    return static_cast<double>(tx.getAmount());
}

bool MEVProtection::detect_sandwich_attack(
    const std::vector<Transaction>& transactions
) {
    for (size_t i = 0; i < transactions.size() - 2; ++i) {
        const auto& tx1 = transactions[i];
        const auto& tx2 = transactions[i + 1];
        const auto& tx3 = transactions[i + 2];

        if (tx1.getSender() == tx3.getSender() &&
            tx1.getRecipient() == tx2.getRecipient() &&
            tx2.getRecipient() == tx3.getRecipient()) {
            return true;
        }
    }
    return false;
}

bool MEVProtection::detect_frontrunning(
    const std::vector<Transaction>& transactions
) {
    for (size_t i = 0; i < transactions.size() - 1; ++i) {
        const auto& tx1 = transactions[i];
        const auto& tx2 = transactions[i + 1];

        // Check if transactions target the same contract
        if (tx1.getRecipient() == tx2.getRecipient()) {
            // Check if gas price of tx1 is significantly higher
            if (tx1.gas_price > tx2.gas_price * 1.5) {
                // Check if tx1 was submitted just before tx2
                if (std::abs(static_cast<double>(tx1.timestamp.time_since_epoch().count() - 
                                               tx2.timestamp.time_since_epoch().count())) < 1000) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<uint8_t> MEVProtection::compute_transaction_hash(
    const Transaction& tx
) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    std::vector<uint8_t> hash(EVP_MAX_MD_SIZE);
    unsigned int hash_len;

    if (ctx == nullptr) {
        return hash;
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return hash;
    }

    // Hash sender
    const auto& sender = tx.getSender();
    if (EVP_DigestUpdate(ctx, sender.data(), sender.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        return hash;
    }

    // Hash recipient
    const auto& recipient = tx.getRecipient();
    if (EVP_DigestUpdate(ctx, recipient.data(), recipient.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        return hash;
    }

    // Hash amount
    const auto amount = tx.getAmount();
    if (EVP_DigestUpdate(ctx, &amount, sizeof(amount)) != 1) {
        EVP_MD_CTX_free(ctx);
        return hash;
    }

    if (EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return hash;
    }

    EVP_MD_CTX_free(ctx);
    hash.resize(hash_len);
    return hash;
}

double MEVProtection::calculate_transaction_value(const Transaction& tx) {
    return static_cast<double>(tx.getAmount());
}

bool MEVProtection::is_high_value_transaction(const Transaction& tx) {
    return calculate_transaction_value(tx) > high_value_threshold_;
}

void MEVProtection::set_high_value_threshold(double threshold) {
    high_value_threshold_ = threshold;
}

} // namespace rollup
} // namespace quids 