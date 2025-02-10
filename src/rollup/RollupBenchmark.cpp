#include "rollup/RollupBenchmark.hpp"
#include <chrono>
#include <random>
#include <algorithm>
#include <iostream>
#include <thread>
#include <iomanip>
#include <sstream>
#include <omp.h>

namespace quids {
namespace rollup {

RollupBenchmark::RollupBenchmark(size_t num_transactions)
    : num_transactions_(num_transactions), total_tx_count_(num_transactions), start_time_(std::chrono::steady_clock::now()) {
    initialize_transactions();
}

void RollupBenchmark::initialize_transactions() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> amount_dist(1, 1000000);
    std::uniform_int_distribution<uint64_t> address_dist(1, 1000);

    transactions_.reserve(num_transactions_);
    for (size_t i = 0; i < num_transactions_; ++i) {
        blockchain::Transaction tx;
        tx.setSender("0x" + std::to_string(address_dist(gen)));
        tx.setRecipient("0x" + std::to_string(address_dist(gen)));
        tx.setAmount(amount_dist(gen));
        tx.setNonce(i);
        transactions_.push_back(tx);
    }
}

BenchmarkResult RollupBenchmark::run_benchmark() {
    BenchmarkResult result;
    auto start_time = std::chrono::high_resolution_clock::now();

    // Process transactions
    for (const auto& tx : transactions_) {
        process_transaction(tx);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    result.total_time = duration.count();
    result.transactions_per_second = static_cast<double>(num_transactions_) /
        (result.total_time / 1000.0);
    result.total_gas_used = total_gas_;
    result.max_gas_used = max_gas_;
    result.successful_transactions = transaction_count_;
    result.failed_transactions = failed_tx_count_;
    
    return result;
}

void RollupBenchmark::process_transaction(const blockchain::Transaction& tx) {
    // Simulate transaction processing
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    // Update metrics
    total_value_ += tx.getAmount();
    transaction_count_++;
    
    // Record gas usage
    uint64_t gas_used = tx.calculate_gas_cost();
    total_gas_ += gas_used;
    
    uint64_t current_max = max_gas_;
    while (gas_used > current_max) {
        if (max_gas_.compare_exchange_weak(current_max, gas_used)) {
            break;
        }
    }
}

double RollupBenchmark::get_average_value() const {
    size_t count = transaction_count_;
    if (count == 0) return 0.0;
    return static_cast<double>(total_value_) / count;
}

uint64_t RollupBenchmark::get_total_gas() const {
    return total_gas_;
}

uint64_t RollupBenchmark::get_max_gas() const {
    return max_gas_;
}

size_t RollupBenchmark::get_transaction_count() const {
    return transaction_count_;
}

void RollupBenchmark::processBatch(const std::vector<blockchain::Transaction>& batch) {
    if (batch.empty()) return;

    // Use all available cores
    #ifdef _OPENMP
    omp_set_num_threads(std::thread::hardware_concurrency());
    #endif

    auto start_time = std::chrono::high_resolution_clock::now();
    
    const size_t batch_size = batch.size();
    std::vector<bool> results(batch_size);
    
    #pragma omp parallel for simd schedule(guided)
    for (size_t i = 0; i < batch_size; i++) {
        results[i] = validateAndProcess(batch[i]);
    }
    
    size_t successful = 0;
    #pragma omp simd reduction(+:successful)
    for (size_t i = 0; i < batch_size; i++) {
        successful += results[i] ? 1 : 0;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    
    tps_ = static_cast<double>(successful) / (duration.count() / 1e6);

    // Process the batch and update counts
    total_tx_count_ += batch_size;
}

bool RollupBenchmark::validateAndProcess(const blockchain::Transaction& tx) {
    total_tx_count_++;
    
    try {
        if (!tx.is_valid()) {
            failed_tx_count_++;
            return false;
        }

        process_transaction(tx);
        return true;
    } catch (...) {
        failed_tx_count_++;
        return false;
    }
}

void RollupBenchmark::run_parallel() {
    omp_set_num_threads(std::thread::hardware_concurrency());
    // ... rest of code ...
}

size_t RollupBenchmark::getTotalTxCount() const {
    return total_tx_count_;
}

size_t RollupBenchmark::getFailedTxCount() const {
    return failed_tx_count_;
}

double RollupBenchmark::get_tps() const {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    return static_cast<double>(total_tx_count_) / (elapsed_seconds + 1e-9); // Avoid division by zero
}

RollupPerformanceMetrics RollupBenchmark::getMetrics() const {
    return metrics_;  // Assuming metrics_ is a member variable
}

} // namespace rollup
} // namespace quids 