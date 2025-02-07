#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include "rollup/RollupPerformanceMetrics.hpp"
#include "rollup/EnhancedRollupMLModel.hpp"
#include "rollup/RollupTypes.hpp"

namespace quids {
namespace rollup {

// Forward declarations
class EnhancedRollupMLModel;

struct Transaction {
    std::vector<uint8_t> data;
    uint64_t nonce;
    std::string sender;
    std::string recipient;
    uint64_t value;
    std::vector<uint8_t> signature;
    uint64_t gas_limit;
    uint64_t gas_price;
    std::chrono::system_clock::time_point timestamp;
    
    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] std::vector<uint8_t> serialize() const;
    static Transaction deserialize(const std::vector<uint8_t>& data);
};

struct TransactionBatch {
    std::vector<Transaction> transactions;
    uint64_t batch_id;
    uint64_t timestamp;
    std::string validator;
    std::vector<uint8_t> merkle_root;
    
    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] size_t size() const { return transactions.size(); }
    [[nodiscard]] std::vector<uint8_t> compute_merkle_root() const;
};

struct TransactionResult {
    bool success;
    std::string error_message;
    uint64_t gas_used;
    std::vector<uint8_t> result_data;
    std::chrono::system_clock::time_point timestamp;
    std::vector<uint8_t> receipt_hash;
};

struct OptimizationResult {
    bool success{false};
    std::vector<double> parameters;
    double expected_improvement{0.0};
    
    explicit operator bool() const { return success; }
};

class RollupTransactionAPI {
public:
    // Constructor and destructor
    explicit RollupTransactionAPI(
        std::shared_ptr<EnhancedRollupMLModel> ml_model,
        size_t num_worker_threads = 4
    );
    ~RollupTransactionAPI();

    // Disable copy
    RollupTransactionAPI(const RollupTransactionAPI&) = delete;
    RollupTransactionAPI& operator=(const RollupTransactionAPI&) = delete;

    // Disable move since we have a mutex member
    RollupTransactionAPI(RollupTransactionAPI&&) noexcept = delete;
    RollupTransactionAPI& operator=(RollupTransactionAPI&&) noexcept = delete;

    // Transaction submission and management
    [[nodiscard]] std::string submit_transaction(const Transaction& tx);
    [[nodiscard]] bool submit_batch(const std::vector<Transaction>& transactions);
    
    // Processing control
    void start_processing();
    void stop_processing();
    [[nodiscard]] bool is_processing() const;
    
    // Metrics and optimization
    [[nodiscard]] RollupPerformanceMetrics get_performance_metrics() const;
    void reset_metrics();
    void optimize_parameters();
    
    // ML model management
    void set_ml_model(std::shared_ptr<EnhancedRollupMLModel> model);
    [[nodiscard]] const EnhancedRollupMLModel& get_ml_model() const;
    
    // Transaction validation
    [[nodiscard]] bool validate_transaction(const Transaction& tx) const;
    [[nodiscard]] std::string validate_transaction_with_message(const Transaction& tx) const;
    
    // Batch management
    [[nodiscard]] size_t get_pending_batch_count() const;
    [[nodiscard]] size_t get_processed_batch_count() const;
    void clear_pending_batches();

private:
    void worker_thread();
    [[nodiscard]] bool process_batch(const TransactionBatch& batch);
    [[nodiscard]] std::string calculate_transaction_hash(const Transaction& tx) const;
    [[nodiscard]] bool is_overloaded() const;
    
    // Metrics recording
    void record_latency(std::chrono::microseconds latency);
    void record_proof_time(std::chrono::microseconds time);
    void record_verification_time(std::chrono::microseconds time);
    
    // Member variables
    std::shared_ptr<EnhancedRollupMLModel> ml_model_;
    std::queue<TransactionBatch> batch_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::vector<std::thread> worker_threads_;
    
    mutable RollupPerformanceMetrics current_metrics_;
    mutable std::mutex metrics_mutex_;
    std::chrono::system_clock::time_point last_metrics_update_;
    bool should_stop_;
    
    // Constants
    static constexpr size_t MAX_BATCH_SIZE = 1000;
    static constexpr size_t MAX_QUEUE_SIZE = 10000;
    static constexpr std::chrono::seconds METRICS_UPDATE_INTERVAL{60};
    static constexpr std::chrono::milliseconds WORKER_SLEEP_TIME{100};
};

} // namespace rollup
} // namespace quids 