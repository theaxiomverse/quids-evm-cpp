#pragma once

#include <vector>
#include <cstdint>
#include <atomic>
#include "blockchain/Transaction.hpp"

namespace quids {
namespace rollup {

struct BenchmarkResult {
    uint64_t total_time{0};  // in milliseconds
    double transactions_per_second{0.0};
    uint64_t total_gas_used{0};
    uint64_t max_gas_used{0};
    size_t successful_transactions{0};
    size_t failed_transactions{0};
};

class RollupBenchmark {
public:
    explicit RollupBenchmark(size_t num_transactions);
    ~RollupBenchmark() = default;

    // Core benchmark functionality
    BenchmarkResult run_benchmark();
    void processBatch(const std::vector<blockchain::Transaction>& txs);

    // Metrics
    [[nodiscard]] double get_average_value() const;
    [[nodiscard]] uint64_t get_total_gas() const;
    [[nodiscard]] uint64_t get_max_gas() const;
    [[nodiscard]] size_t get_transaction_count() const;
    [[nodiscard]] double get_tps() const { return tps_; }

private:
    void initialize_transactions();
    void process_transaction(const blockchain::Transaction& tx);
    bool validateAndProcess(const blockchain::Transaction& tx);

    // Transaction storage
    std::vector<blockchain::Transaction> transactions_;
    size_t num_transactions_{0};

    // Metrics
    std::atomic<uint64_t> total_value_{0};
    std::atomic<uint64_t> total_gas_{0};
    std::atomic<uint64_t> max_gas_{0};
    std::atomic<size_t> transaction_count_{0};
    std::atomic<size_t> total_tx_count_{0};
    std::atomic<size_t> failed_tx_count_{0};
    double tps_{0.0};
};

} // namespace rollup
} // namespace quids 