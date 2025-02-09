#pragma once

#include "blockchain/Transaction.hpp"
#include "evm/Address.hpp"
#include "evm/EVMExecutor.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <atomic>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include "rollup/RollupPerformanceMetrics.hpp"

namespace quids {
namespace rollup {

using quids::rollup::RollupPerformanceMetrics;

// Forward declarations
namespace evm {
class EVMExecutor;
}

struct ParallelProcessorConfig {
    size_t num_worker_threads{4};
    size_t max_queue_size{1000};
    bool enable_contract_parallelization{true};
    size_t max_parallel_contracts{4};
    size_t max_batch_size{100};
    uint64_t max_gas_per_block{15000000};
    uint64_t target_block_time_ms{2000};
};

class ParallelProcessor {
public:
    struct Config {
        size_t num_threads = 4;
        size_t batch_size = 100;
    };

    struct ContractCall {
        ::evm::Address contract_address;
        std::vector<uint8_t> input;
        uint64_t gas_limit;
    };

    struct ContractState {
        uint64_t balance{0};
        std::vector<uint8_t> code;
        std::unordered_map<std::string, std::vector<uint8_t>> storage;
        std::mutex mutex;
        bool is_executing{false};
        std::queue<ContractCall> pending_calls;
    };

    struct AccountState {
        uint64_t balance{0};
        uint64_t nonce{0};
        std::mutex mutex;
        std::queue<blockchain::Transaction> pending_transactions;
    };

    struct ProcessingResult {
        bool success;
        std::string error_message;
        uint64_t gas_used;
    };

    using ContractResult = std::future<quids::evm::EVMExecutor::ExecutionResult>;

    explicit ParallelProcessor(const Config& config);
    ParallelProcessor(const ParallelProcessorConfig &config);
    ~ParallelProcessor();

    void process_batch(const std::vector<blockchain::Transaction>& batch);
    ProcessingResult process_transaction(const blockchain::Transaction& tx);
    void process(const ::evm::Address& contract_address);

    void start();
    void stop();

    ContractResult executeContract(const ContractCall& call);
    quids::evm::EVMExecutor::ExecutionResult executeContractInternal(const ContractCall& call);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Member variables
    std::atomic<bool> should_stop_;
    ParallelProcessorConfig config_;
    std::vector<std::unique_ptr<quids::evm::EVMExecutor>> evm_executors_;
    std::mutex evm_executors_mutex_;
    
    // Thread management
    std::vector<std::thread> worker_threads_;
    std::vector<std::thread> contract_worker_threads_;
    
    // Queue management
    std::queue<blockchain::Transaction> transaction_queue_;
    std::mutex transaction_queue_mutex_;
    std::condition_variable transaction_queue_cv_;
    
    // Contract queue
    std::queue<ContractCall> contract_queue_;
    std::mutex contract_queue_mutex_;
    std::condition_variable contract_queue_cv_;
    
    // State management
    std::unordered_map<std::string, AccountState> account_states_;
    std::mutex account_states_mutex_;
    std::unordered_map<::evm::Address, ContractState> contract_states_;
    std::mutex contract_states_mutex_;

    // Helper methods
    void startWorkers();
    void stopWorkers();
    void workerThread();
    void contractWorkerThread();
    bool submitTransaction(const blockchain::Transaction& tx);
    bool submitBatch(const std::vector<blockchain::Transaction>& batch);
    bool processTransaction(const blockchain::Transaction& tx);
    bool processBatch(const std::vector<blockchain::Transaction>& batch);
    bool hasDependency(const blockchain::Transaction& tx1, const blockchain::Transaction& tx2) const;
    std::vector<std::vector<blockchain::Transaction>> createIndependentBatches(
        const std::vector<blockchain::Transaction>& transactions);
    quids::evm::EVMExecutor* getAvailableExecutor();
    void returnExecutor(quids::evm::EVMExecutor* executor);

    RollupPerformanceMetrics metrics_;
};

} // namespace rollup
} // namespace quids