#include "rollup/ParallelProcessor.hpp"
#include "evm/Memory.hpp"
#include "evm/Stack.hpp"
#include "evm/Storage.hpp"
#include <algorithm>
#include <chrono>

namespace quids {
namespace rollup {

using quids::blockchain::Transaction;
using quids::evm::EVMExecutor;

ParallelProcessor::ParallelProcessor(const Config& config)
    : config_(config) {
    // Initialize EVM executor pool
    evm_executors_.reserve(config.num_worker_threads);
    for (size_t i = 0; i < config.num_worker_threads; ++i) {
        auto memory = std::make_shared<::evm::Memory>();
        auto stack = std::make_shared<::evm::Stack>();
        auto storage = std::make_shared<::evm::Storage>();
        evm_executors_.push_back(std::make_unique<EVMExecutor>(EVMConfig{}));
    }
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
    stopWorkers();
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

ParallelProcessor::ContractResult ParallelProcessor::executeContract(
    const ContractCall& call
) {
    if (should_stop_) {
        return std::async(std::launch::deferred, []() {
            return evm::EVMExecutor::ExecutionResult{
                false, {}, 0, "Processor stopped"
            };
        });
    }
    
    // Add to contract queue
    std::unique_lock<std::mutex> lock(contract_queue_mutex_);
    contract_queue_.push(call);
    lock.unlock();
    contract_queue_cv_.notify_one();
    
    // Return future for result
    return std::async(std::launch::async, [this, call]() {
        return executeContractInternal(call);
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
            std::unique_lock<std::mutex> lock(contract_queue_mutex_);
            contract_queue_cv_.wait(lock, [this]() {
                return !contract_queue_.empty() || should_stop_;
            });
            
            if (should_stop_) break;
            
            call = contract_queue_.front();
            contract_queue_.pop();
        }
        
        executeContractInternal(call);
    }
}

bool ParallelProcessor::processTransaction(const Transaction& tx) {
    auto start = std::chrono::high_resolution_clock::now();
    bool success = false;
    
    try {
        // Get account state
        std::unique_lock<std::mutex> account_lock(account_states_mutex_);
        auto& account_state = account_states_[tx.sender];
        account_lock.unlock();
        
        // Process transaction
        {
            std::lock_guard<std::mutex> lock(account_state.mutex);
            if (tx.nonce == account_state.nonce) {
                // Process transaction logic here
                account_state.nonce++;
                success = true;
            } else if (tx.nonce > account_state.nonce) {
                // Queue transaction for later processing
                account_state.pending_transactions.push(tx);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        metrics_.update_transaction(
            success,
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        );
        
    } catch (const std::exception&) {
        // Handle error
        std::lock_guard<std::mutex> lock(metrics_.mutex);
        metrics_.failed_transactions++;
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

evm::EVMExecutor::ExecutionResult ParallelProcessor::executeContractInternal(
    const ContractCall& call
) {
    auto start = std::chrono::high_resolution_clock::now();
    evm::EVMExecutor::ExecutionResult result;
    bool success = false;
    
    try {
        // Get contract state
        std::unique_lock<std::mutex> contract_lock(contract_states_mutex_);
        auto& contract_state = contract_states_[call.contract_address];
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
        metrics_.update_contract(
            success,
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        );
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(metrics_.mutex);
        metrics_.failed_contracts++;
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

evm::EVMExecutor* ParallelProcessor::getAvailableExecutor() {
    std::lock_guard<std::mutex> lock(evm_executors_mutex_);
    // Simple round-robin allocation
    static size_t current = 0;
    auto* executor = evm_executors_[current].get();
    current = (current + 1) % evm_executors_.size();
    return executor;
}

void ParallelProcessor::returnExecutor(evm::EVMExecutor* /*executor*/) {
    // No-op for now, could implement more sophisticated pooling
}

} // namespace rollup
} // namespace quids 