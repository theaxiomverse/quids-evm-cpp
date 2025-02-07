#pragma once

#include "rollup/Transaction.hpp"
#include <vector>
#include <cstdint>

namespace quids {
namespace rollup {

class RollupBenchmark {
public:
    RollupBenchmark() = default;
    
    void processBatch(const std::vector<Transaction>& txs);
    bool validateAndProcess(const Transaction& tx);
    
    // Getters for metrics
    double getTPS() const { return tps_; }
    uint64_t getFailedTxCount() const { return failed_tx_count_; }
    uint64_t getTotalTxCount() const { return total_tx_count_; }

private:
    double tps_{0.0};
    uint64_t failed_tx_count_{0};
    uint64_t total_tx_count_{0};
};

} // namespace rollup
} // namespace quids 