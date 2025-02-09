#include "rollup/ParallelProcessor.hpp"
#include "evm/Memory.hpp"
#include "evm/Stack.hpp"
#include "evm/Storage.hpp"
#include "evm/Address.hpp"
#include "evm/EVMExecutor.hpp"

#include "blockchain/Transaction.hpp"
#include <future>

#include <algorithm>
#include <chrono>
#include "rollup/ParallelProcessor.hpp"

namespace quids {
namespace rollup {

using blockchain::Transaction;
using quids::evm::EVMExecutor;
using ::evm::Address;
using quids::EVMConfig;

// Remove duplicate struct definition since it's now in the header
// struct ContractCall { ... }

struct ProcessingMetrics {
    std::mutex mutex;
    uint64_t failed_transactions{0};
    uint64_t failed_contracts{0};
    
    void update_transaction(bool success, std::chrono::microseconds latency) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!success) failed_transactions++;
        // Store latency for metrics if needed
        (void)latency;
    }
    
    void update_contract(bool success, std::chrono::microseconds latency) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!success) failed_contracts++;
        // Store latency for metrics if needed
        (void)latency;
    }
};

class ParallelProcessor::Impl {
public:
    ProcessingMetrics metrics;
    std::mutex contract_states_mutex_;
    std::mutex account_states_mutex_;
    std::queue<ContractCall> contract_queue_;
    std::mutex contract_queue_mutex_;
    std::condition_variable contract_queue_cv_;
    std::unordered_map<Address, ContractState> contract_states_;
    std::unordered_map<std::string, AccountState> account_states_;
};

ParallelProcessor::ParallelProcessor(const Config& config)
    : should_stop_(false)
    , config_({
        .num_worker_threads = config.num_threads,
        .max_queue_size = config.batch_size,
        .enable_contract_parallelization = true,
        .max_parallel_contracts = 4,
        .max_batch_size = config.batch_size,
        .max_gas_per_block = 15000000,
        .target_block_time_ms = 2000
    }) {
    startWorkers();
}

ParallelProcessor::~ParallelProcessor() {
    stopWorkers();
}

void ParallelProcessor::start() {
    should_stop_ = false;
    startWorkers();
}

void ParallelProcessor::stop() {
    should_stop_ = true;
    transaction_queue_cv_.notify_all();
}

void ParallelProcessor::startWorkers() {
    should_stop_ = false;
    
    // Start worker threads
    for (size_t i = 0; i < config_.num_worker_threads; ++i) {
        worker_threads_.emplace_back(&ParallelProcessor::workerThread, this);
    }
    
    // Start contract worker threads
    if (config_.enable_contract_parallelization) {
        for (size_t i = 0; i < config_.max_parallel_contracts; ++i) {
            contract_worker_threads_.emplace_back(&ParallelProcessor::contractWorkerThread, this);
        }
    }
}

void ParallelProcessor::stopWorkers() {
    should_stop_ = true;
    
    // Wake up all worker threads
    transaction_queue_cv_.notify_all();
    contract_queue_cv_.notify_all();
    
    // Join worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Join contract worker threads
    for (auto& thread : contract_worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    contract_worker_threads_.clear();
}

bool ParallelProcessor::submitTransaction(const Transaction& tx) {
    if (should_stop_) return false;
    
    std::unique_lock<std::mutex> lock(transaction_queue_mutex_);
    if (transaction_queue_.size() >= config_.max_queue_size) {
        return false;
    }
    
    transaction_queue_.push(tx);
    lock.unlock();
    transaction_queue_cv_.notify_one();
    return true;
}

bool ParallelProcessor::submitBatch(const std::vector<Transaction>& batch) {
    if (should_stop_) return false;
    
    // Split batch into independent sub-batches
    auto independent_batches = createIndependentBatches(batch);
    
    // Process each sub-batch
    std::vector<std::future<bool>> results;
    for (const auto& sub_batch : independent_batches) {
        results.push_back(std::async(std::launch::async, [this, sub_batch]() {
            return processBatch(sub_batch);
        }));
    }
    
    // Wait for all sub-batches to complete
    bool all_success = true;
    for (auto& result : results) {
        all_success &= result.get();
    }
    
    return all_success;
}

ParallelProcessor::ContractResult ParallelProcessor::executeContract(const ContractCall& call) {
    if (should_stop_) {
        return std::async(std::launch::deferred, []() {
            return quids::evm::EVMExecutor::ExecutionResult{
                false, {}, 0, "Processor stopped"
            };
        });
    }
    
    // Add to contract queue
    std::unique_lock<std::mutex> lock(impl_->contract_queue_mutex_);
    impl_->contract_queue_.push(call);
    lock.unlock();
    impl_->contract_queue_cv_.notify_one();
    
    // Return future for result
    return std::async(std::launch::async, [this, call]() {
        return this->executeContractInternal(call);
    });
}

void ParallelProcessor::workerThread() {
    while (!should_stop_) {
        Transaction tx;
        {
            std::unique_lock<std::mutex> lock(transaction_queue_mutex_);
            transaction_queue_cv_.wait(lock, [this]() {
                return !transaction_queue_.empty() || should_stop_;
            });
            
            if (should_stop_) break;
            
            tx = transaction_queue_.front();
            transaction_queue_.pop();
        }
        
        processTransaction(tx);
    }
}

void ParallelProcessor::contractWorkerThread() {
    while (!should_stop_) {
        ContractCall call;
        {
            std::unique_lock<std::mutex> lock(impl_->contract_queue_mutex_);
            impl_->contract_queue_cv_.wait(lock, [this]() {
                return !impl_->contract_queue_.empty() || should_stop_;
            });
            
            if (should_stop_) break;
            
            call = impl_->contract_queue_.front();
            impl_->contract_queue_.pop();
        }
        
        executeContractInternal(call);
    }
}

bool ParallelProcessor::processTransaction(const Transaction& tx) {
    auto start = std::chrono::high_resolution_clock::now();
    bool success = false;
    
    try {
        // Get account state
        std::unique_lock<std::mutex> account_lock(impl_->account_states_mutex_);
        auto& account_state = impl_->account_states_[tx.getSender()];
        account_lock.unlock();
        
        // Process transaction
        {
            std::lock_guard<std::mutex> lock(account_state.mutex);
            if (tx.getNonce() == account_state.nonce) {
                // Process transaction logic here
                account_state.nonce++;
                success = true;
            } else if (tx.getNonce() > account_state.nonce) {
                // Queue transaction for later processing
                account_state.pending_transactions.push(tx);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        impl_->metrics.update_transaction(
            success,
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        );
        
    } catch (const std::exception&) {
        // Handle error
        std::lock_guard<std::mutex> lock(impl_->metrics.mutex);
        impl_->metrics.failed_transactions++;
        return false;
    }
    
    return success;
}

bool ParallelProcessor::processBatch(const std::vector<Transaction>& batch) {
    bool all_success = true;
    std::vector<std::future<bool>> results;
    
    // Process transactions in parallel
    for (const auto& tx : batch) {
        results.push_back(std::async(
            std::launch::async,
            &ParallelProcessor::processTransaction,
            this,
            tx
        ));
    }
    
    // Wait for all transactions to complete
    for (auto& result : results) {
        all_success &= result.get();
    }
    
    return all_success;
}

EVMExecutor::ExecutionResult ParallelProcessor::executeContractInternal(const ContractCall& call) {
    auto start = std::chrono::high_resolution_clock::now();
    EVMExecutor::ExecutionResult result;
    bool success = false;
    
    try {
        // Get contract state
        std::unique_lock<std::mutex> contract_lock(impl_->contract_states_mutex_);
        auto& contract_state = impl_->contract_states_[call.contract_address];
        contract_lock.unlock();
        
        // Execute contract
        {
            std::lock_guard<std::mutex> lock(contract_state.mutex);
            if (!contract_state.is_executing) {
                contract_state.is_executing = true;
                auto* executor = getAvailableExecutor();
                result = executor->execute_contract(
                    call.contract_address,
                    call.input,
                    {},  // input data
                    call.gas_limit
                );
                returnExecutor(executor);
                contract_state.is_executing = false;
                success = result.success;
            } else {
                // Queue call for later execution
                contract_state.pending_calls.push(call);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        impl_->metrics.update_contract(
            success,
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        );
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(impl_->metrics.mutex);
        impl_->metrics.failed_contracts++;
        result.success = false;
        result.error_message = e.what();
    }
    
    return result;
}

bool ParallelProcessor::hasDependency(
    const Transaction& tx1,
    const Transaction& tx2
) const {
    // Check for dependencies between transactions using getters
    return tx1.getSender() == tx2.getSender() ||
           tx1.getSender() == tx2.getRecipient() ||
           tx1.getRecipient() == tx2.getSender();
}

std::vector<std::vector<Transaction>> ParallelProcessor::createIndependentBatches(
    const std::vector<Transaction>& transactions
) {
    std::vector<std::vector<Transaction>> batches;
    std::vector<bool> assigned(transactions.size(), false);
    
    for (size_t i = 0; i < transactions.size(); ++i) {
        if (assigned[i]) continue;
        
        std::vector<Transaction> batch;
        batch.push_back(transactions[i]);
        assigned[i] = true;
        
        for (size_t j = i + 1; j < transactions.size(); ++j) {
            if (assigned[j]) continue;
            
            bool has_dependency = false;
            for (const auto& tx : batch) {
                if (hasDependency(tx, transactions[j])) {
                    has_dependency = true;
                    break;
                }
            }
            
            if (!has_dependency) {
                batch.push_back(transactions[j]);
                assigned[j] = true;
            }
        }
        
        batches.push_back(std::move(batch));
    }
    
    return batches;
}

quids::evm::EVMExecutor* ParallelProcessor::getAvailableExecutor() {
    std::lock_guard<std::mutex> lock(evm_executors_mutex_);
    // Simple round-robin allocation
    static size_t current = 0;
    auto* executor = evm_executors_[current].get();
    current = (current + 1) % evm_executors_.size();
    return executor;
}

void ParallelProcessor::returnExecutor(quids::evm::EVMExecutor* executor) {
    (void)executor;
    // No-op for now, could implement more sophisticated pooling
}

void ParallelProcessor::process(const ::evm::Address& contract_address) {
    // Convert address to string for lookup
    ContractState& contract_state = impl_->contract_states_[contract_address];
    
    // Process any pending calls
    while (!contract_state.pending_calls.empty()) {
        auto call = contract_state.pending_calls.front();
        contract_state.pending_calls.pop();
        executeContractInternal(call);
    }
}

} // namespace rollup
} // namespace quids 