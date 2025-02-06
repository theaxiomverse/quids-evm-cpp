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
#include "rollup/RollupPerformanceMetrics.h"
#include "rollup/EnhancedRollupMLModel.h"

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
};

struct TransactionBatch {
    std::vector<Transaction> transactions;
    uint64_t batch_id;
    uint64_t timestamp;
    std::string validator;
};

struct TransactionResult {
    bool success;
    std::string error_message;
    uint64_t gas_used;
    std::vector<uint8_t> result_data;
};

class RollupTransactionAPI {
public:
    struct Transaction {
        std::string sender;
        std::string recipient;
        uint64_t amount;
        std::string signature;
        std::vector<uint8_t> data;
    };

    struct TransactionBatch {
        std::vector<Transaction> transactions;
        std::chrono::system_clock::time_point timestamp;
    };

    RollupTransactionAPI(
        std::shared_ptr<EnhancedRollupMLModel> ml_model,
        size_t num_worker_threads = 4
    );

    ~RollupTransactionAPI();

    std::string submitTransaction(const Transaction& tx);
    bool submitBatch(const std::vector<Transaction>& transactions);
    
    void startProcessing();
    void stopProcessing();
    
    RollupPerformanceMetrics getPerformanceMetrics() const;
    void resetMetrics();
    
    void setMLModel(std::shared_ptr<EnhancedRollupMLModel> model);
    void optimizeParameters();

    bool validateTransaction(const Transaction& tx) const;
    std::string validateTransactionWithMessage(const Transaction& tx) const;

private:
    void workerThread();
    bool processBatch(const TransactionBatch& batch);
    std::string calculateTransactionHash(const Transaction& tx) const;
    bool isOverloaded() const;
    
    void recordLatency(std::chrono::microseconds latency);
    void recordProofTime(std::chrono::microseconds time);
    void recordVerificationTime(std::chrono::microseconds time);

    std::shared_ptr<EnhancedRollupMLModel> ml_model_;
    std::queue<TransactionBatch> batch_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::vector<std::thread> worker_threads_;
    
    mutable RollupPerformanceMetrics current_metrics_;
    mutable std::mutex metrics_mutex_;
    std::chrono::system_clock::time_point last_metrics_update_;
    bool should_stop_;
}; 