#pragma once
#include "blockchain/Transaction.hpp"
#include "rollup/StateManager.hpp"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace quids {
namespace rollup {

class BatchProcessor {
public:
    struct BatchConfig {
        size_t max_batch_size;
        std::chrono::milliseconds max_wait_time;
        size_t min_batch_size;
    };

    explicit BatchProcessor(
        std::shared_ptr<quids::rollup::StateManager> state_manager,
        const BatchConfig& config
    );
    
    ~BatchProcessor();
    
    void submit_transaction(const quids::blockchain::Transaction& tx);
    void process_batches();
    void stop();
    
private:
    std::shared_ptr<quids::rollup::StateManager> state_manager_;
    BatchConfig config_;
    std::queue<quids::blockchain::Transaction> pending_transactions_;
    std::vector<std::thread> worker_threads_;
    bool should_stop_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    
    void process_batch();
    std::vector<quids::blockchain::Transaction> create_batch();
};

} // namespace rollup
} // namespace quids 