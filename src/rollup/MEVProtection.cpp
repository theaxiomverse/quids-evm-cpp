#include "rollup/MEVProtection.hpp"
#include <blake3.h>
#include <chrono>
#include <spdlog/spdlog.h>

namespace quids {
namespace rollup {

// Define Impl struct in the cpp file
struct Impl {
    double high_value_threshold{1000.0};
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_transaction_time;
};

MEVProtection::MEVProtection() noexcept : impl_(std::make_unique<Impl>()) {}
MEVProtection::~MEVProtection() = default;

MEVProtection::OrderingCommitment MEVProtection::create_ordering_commitment(
    const std::vector<blockchain::Transaction>& transactions
) const {
    OrderingCommitment commitment;
    commitment.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    commitment.batch_hash = compute_fairness_hash(transactions);
    return commitment;
}

double MEVProtection::estimate_profit(const blockchain::Transaction& tx) const {
    // Basic profit estimation based on transaction value and gas price
    double base_value = static_cast<double>(tx.value);
    double gas_cost = static_cast<double>(tx.gas_price * tx.gas_limit);
    return base_value - gas_cost;
}

bool MEVProtection::detect_sandwich_attack(
    const blockchain::Transaction& target,
    const std::vector<blockchain::Transaction>& batch
) const {
    if (batch.size() < 3) return false;

    for (size_t i = 1; i < batch.size() - 1; i++) {
        if (batch[i].hash == target.hash) {
            // Check for similar transactions before and after
            const auto& front = batch[i-1];
            const auto& back = batch[i+1];
            
            if (front.from == back.from && front.to == back.to) {
                return true;
            }
        }
    }
    return false;
}

bool MEVProtection::detect_frontrunning(
    const blockchain::Transaction& tx1,
    const blockchain::Transaction& tx2
) const {
    // Check if transactions are targeting the same contract/address
    if (tx1.to != tx2.to) {
        return false;
    }

    // Check if tx1 has a significantly higher gas price (>20% higher)
    if (tx1.gas_price <= tx2.gas_price * 1.2) {
        return false;
    }

    // Check timestamp proximity (within 2 blocks / ~30 seconds)
    constexpr int64_t FRONTRUN_TIME_THRESHOLD = 30000; // 30 seconds in milliseconds
    int64_t time_diff = std::abs(
        tx1.timestamp.time_since_epoch().count() - 
        tx2.timestamp.time_since_epoch().count()
    );
    if (time_diff > FRONTRUN_TIME_THRESHOLD) {
        return false;
    }

    // Check for similar transaction characteristics
    bool similar_value = std::abs(static_cast<double>(tx1.value) - static_cast<double>(tx2.value)) < 
                        (static_cast<double>(tx2.value) * 0.1); // 10% threshold
    bool similar_gas_limit = std::abs(static_cast<double>(tx1.gas_limit) - static_cast<double>(tx2.gas_limit)) <
                            (static_cast<double>(tx2.gas_limit) * 0.1); // 10% threshold

    // If transactions have similar characteristics but different senders, likely frontrunning
    return similar_value && similar_gas_limit && (tx1.from != tx2.from);
}

std::array<uint8_t, 32> MEVProtection::compute_fairness_hash(
    const std::vector<blockchain::Transaction>& transactions
) const {
    std::array<uint8_t, 32> hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    for (const auto& tx : transactions) {
        auto tx_hash = compute_transaction_hash(tx);
        blake3_hasher_update(&hasher, tx_hash.data(), tx_hash.size());
    }

    blake3_hasher_finalize(&hasher, hash.data(), hash.size());
    return hash;
}

std::vector<uint8_t> MEVProtection::compute_transaction_hash(
    const blockchain::Transaction& tx
) const {
    std::vector<uint8_t> hash(BLAKE3_OUT_LEN);
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    // Hash from address
    blake3_hasher_update(&hasher, tx.from.data(), tx.from.size());

    // Hash to address
    blake3_hasher_update(&hasher, tx.to.data(), tx.to.size());

    // Hash value (convert to bytes first)
    auto value_bytes = reinterpret_cast<const uint8_t*>(&tx.value);
    blake3_hasher_update(&hasher, value_bytes, sizeof(tx.value));

    // Hash gas price
    auto gas_price_bytes = reinterpret_cast<const uint8_t*>(&tx.gas_price);
    blake3_hasher_update(&hasher, gas_price_bytes, sizeof(tx.gas_price));

    // Hash gas limit
    auto gas_limit_bytes = reinterpret_cast<const uint8_t*>(&tx.gas_limit);
    blake3_hasher_update(&hasher, gas_limit_bytes, sizeof(tx.gas_limit));

    // Hash nonce
    auto nonce_bytes = reinterpret_cast<const uint8_t*>(&tx.nonce);
    blake3_hasher_update(&hasher, nonce_bytes, sizeof(tx.nonce));

    // Hash input data if present
    if (!tx.data.empty()) {
        blake3_hasher_update(&hasher, tx.data.data(), tx.data.size());
    }

    // Finalize hash
    blake3_hasher_finalize(&hasher, hash.data(), BLAKE3_OUT_LEN);
    return hash;
}

double MEVProtection::calculate_transaction_value(const blockchain::Transaction& tx) const {
    return static_cast<double>(tx.value);
}

bool MEVProtection::is_high_value_transaction(const blockchain::Transaction& tx) const {
    return calculate_transaction_value(tx) >= impl_->high_value_threshold;
}

void MEVProtection::set_high_value_threshold(double threshold) {
    impl_->high_value_threshold = threshold;
}

} // namespace rollup
} // namespace quids 