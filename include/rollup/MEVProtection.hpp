#pragma once

#include "blockchain/Transaction.hpp"
#include <array>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>

namespace quids {
namespace rollup {

class MEVProtection {
public:
    struct OrderingCommitment {
        uint64_t timestamp;
        std::array<uint8_t, 32> batch_hash;
    };

    MEVProtection() noexcept;
    ~MEVProtection();

    // Disable copy
    MEVProtection(const MEVProtection&) = delete;
    MEVProtection& operator=(const MEVProtection&) = delete;

    // Move operations
    MEVProtection(MEVProtection&&) noexcept = default;
    MEVProtection& operator=(MEVProtection&&) noexcept = default;

    void add_transaction(const blockchain::Transaction& tx);
    std::vector<blockchain::Transaction> get_optimal_ordering();
    [[nodiscard]] OrderingCommitment create_ordering_commitment(
        const std::vector<blockchain::Transaction>& transactions
    ) const;
    
    [[nodiscard]] double estimate_profit(const blockchain::Transaction& tx) const;

    [[nodiscard]] bool detect_sandwich_attack(
        const blockchain::Transaction& target,
        const std::vector<blockchain::Transaction>& batch
    ) const;

    [[nodiscard]] bool detect_frontrunning(
        const blockchain::Transaction& tx1,
        const blockchain::Transaction& tx2
    ) const;

    [[nodiscard]] std::array<uint8_t, 32> compute_fairness_hash(
        const std::vector<blockchain::Transaction>& transactions
    ) const;

    [[nodiscard]] std::vector<uint8_t> compute_transaction_hash(
        const blockchain::Transaction& tx
    ) const;

    [[nodiscard]] double calculate_transaction_value(const blockchain::Transaction& tx) const;
    [[nodiscard]] bool is_high_value_transaction(const blockchain::Transaction& tx) const;
    void set_high_value_threshold(double threshold);

    void finalize_batch();

private:
    struct Impl {
        double high_value_threshold{1000.0};
        std::unordered_map<std::string, std::chrono::system_clock::time_point> last_transaction_time;
    };
    std::unique_ptr<Impl> impl_;
};

} // namespace rollup
} // namespace quids 