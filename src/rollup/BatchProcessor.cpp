#include "rollup/BatchProcessor.hpp"

namespace quids {
namespace rollup {

BatchProcessor::BatchProcessor(
    std::shared_ptr<quids::rollup::StateManager> state_manager,
    const BatchConfig& config
) : state_manager_(state_manager),
    config_(config),
    should_stop_(false) {
    // Start worker threads
    for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
        worker_threads_.emplace_back([this] { process_batches(); });
    }
}

BatchProcessor::~BatchProcessor() {
    stop();
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void BatchProcessor::submit_transaction(const quids::blockchain::Transaction& tx) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_transactions_.push(tx);
    cv_.notify_one();
}

void BatchProcessor::process_batches() {
    while (!should_stop_) {
        process_batch();
    }
}

void BatchProcessor::stop() {
    should_stop_ = true;
    cv_.notify_all();
}

void BatchProcessor::process_batch() {
    auto batch = create_batch();
    if (batch.empty()) {
        return;
    }
    
    // Process each transaction in the batch
    for (const auto& tx : batch) {
        bool success = state_manager_->apply_transaction(tx);
        if (!success) {
            // Log or handle failed transaction
            // For now we continue processing the batch even if one transaction fails
            continue;
        }
    }
}

std::vector<quids::blockchain::Transaction> BatchProcessor::create_batch() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait for minimum batch size or timeout
    cv_.wait_for(lock, config_.max_wait_time, [this] {
        return pending_transactions_.size() >= config_.min_batch_size || should_stop_;
    });
    
    if (should_stop_ || pending_transactions_.empty()) {
        return {};
    }
    
    std::vector<quids::blockchain::Transaction> batch;
    while (!pending_transactions_.empty() && batch.size() < config_.max_batch_size) {
        batch.push_back(pending_transactions_.front());
        pending_transactions_.pop();
    }
    
    return batch;
}

} // namespace rollup
} // namespace quids 