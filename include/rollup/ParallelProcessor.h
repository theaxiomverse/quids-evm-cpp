#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <unordered_map>
#include <atomic>
#include "blockchain/Transaction.h"
#include "evm/EVMExecutor.h"

class ParallelProcessor {
public:
    struct Config {
        size_t num_worker_threads{std::thread::hardware_concurrency()};
        size_t max_queue_size{10000};
        size_t batch_size{1000};
        bool enable_contract_parallelization{true};
        size_t max_parallel_contracts{100};
    };
    
    explicit ParallelProcessor(const Config& config);
    ~ParallelProcessor();
    
    // Transaction processing
    bool submitTransaction(const Transaction& tx);
    bool submitBatch(const std::vector<Transaction>& batch);
    
    // Contract execution
    struct ContractCall {
        evm::Address contract_address;
        std::vector<uint8_t> input;
        uint64_t gas_limit{500000};  // Default gas limit
    };
    
    using ContractResult = std::future<evm::EVMExecutor::ExecutionResult>;
    ContractResult executeContract(const ContractCall& call);
    
    // Batch contract execution
    std::vector<ContractResult> executeContractBatch(
        const std::vector<ContractCall>& calls
    );
    
    // Status and metrics
    struct Metrics {
        size_t processed_transactions{0};
        size_t processed_contracts{0};
        size_t failed_transactions{0};
        size_t failed_contracts{0};
        uint64_t total_processing_time_ms{0};
        uint64_t avg_transaction_time_us{0};
        uint64_t avg_contract_time_us{0};
        
        std::mutex mutex;
        
        void update_transaction(bool success, std::chrono::microseconds time) {
            std::lock_guard<std::mutex> lock(mutex);
            processed_transactions++;
            if (!success) failed_transactions++;
            total_processing_time_ms += time.count() / 1000;
            avg_transaction_time_us = (avg_transaction_time_us * (processed_transactions - 1) + time.count()) / processed_transactions;
        }
        
        void update_contract(bool success, std::chrono::microseconds time) {
            std::lock_guard<std::mutex> lock(mutex);
            processed_contracts++;
            if (!success) failed_contracts++;
            avg_contract_time_us = (avg_contract_time_us * (processed_contracts - 1) + time.count()) / processed_contracts;
        }
        
        void reset() {
            std::lock_guard<std::mutex> lock(mutex);
            processed_transactions = 0;
            processed_contracts = 0;
            failed_transactions = 0;
            failed_contracts = 0;
            total_processing_time_ms = 0;
            avg_transaction_time_us = 0;
            avg_contract_time_us = 0;
        }
    };
    
    const Metrics& getMetrics() const { return metrics_; }
    void resetMetrics() { metrics_.reset(); }
    
    void start();
    void stop();
    
private:
    // Worker thread management
    void startWorkers();
    void stopWorkers();
    void workerThread();
    void contractWorkerThread();
    
    // Transaction processing internals
    bool processTransaction(const Transaction& tx);
    bool processBatch(const std::vector<Transaction>& batch);
    
    // Contract execution internals
    evm::EVMExecutor::ExecutionResult executeContractInternal(
        const ContractCall& call
    );
    
    // Dependency tracking
    bool hasDependency(const Transaction& tx1, const Transaction& tx2) const;
    std::vector<std::vector<Transaction>> createIndependentBatches(
        const std::vector<Transaction>& transactions
    );
    
    // State management
    struct AccountState {
        std::mutex mutex;
        uint64_t nonce{0};
        std::queue<Transaction> pending_transactions;
    };
    
    std::unordered_map<std::string, AccountState> account_states_;
    std::mutex account_states_mutex_;
    
    // Contract state management
    struct ContractState {
        std::mutex mutex;
        std::queue<ContractCall> pending_calls;
        bool is_executing{false};
    };
    
    std::unordered_map<evm::Address, ContractState> contract_states_;
    std::mutex contract_states_mutex_;
    
    // Thread management
    std::vector<std::thread> worker_threads_;
    std::vector<std::thread> contract_worker_threads_;
    std::atomic<bool> should_stop_{false};
    
    // Work queues
    std::queue<Transaction> transaction_queue_;
    std::mutex transaction_queue_mutex_;
    std::condition_variable transaction_queue_cv_;
    
    std::queue<ContractCall> contract_queue_;
    std::mutex contract_queue_mutex_;
    std::condition_variable contract_queue_cv_;
    
    // Configuration and metrics
    Config config_;
    Metrics metrics_;
    
    // EVM executor pool
    std::vector<std::unique_ptr<evm::EVMExecutor>> evm_executors_;
    std::mutex evm_executors_mutex_;
    
    // Helper methods
    evm::EVMExecutor* getAvailableExecutor();
    void returnExecutor(evm::EVMExecutor* executor);
    
    void updateTransactionMetrics(bool success, std::chrono::microseconds time) {
        metrics_.update_transaction(success, time);
    }
    
    void updateContractMetrics(bool success, std::chrono::microseconds time) {
        metrics_.update_contract(success, time);
    }
}; 