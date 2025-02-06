#pragma once
#include "blockchain/Transaction.h"
#include "StateManager.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

class BatchProcessor {
public:
    struct BatchConfig {
        size_t max_batch_size;
        std::chrono::milliseconds max_wait_time;
        size_t min_batch_size;
    };

    explicit BatchProcessor(
        std::shared_ptr<StateManager> state_manager,
        const BatchConfig& config
    );
    
    ~BatchProcessor();
    
    void submit_transaction(const Transaction& tx);
    void process_batches();
    void stop();
    
private:
    std::shared_ptr<StateManager> state_manager_;
    BatchConfig config_;
    std::queue<Transaction> pending_transactions_;
    std::vector<std::thread> worker_threads_;
    bool should_stop_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    
    void process_batch();
    std::vector<Transaction> create_batch();
}; 