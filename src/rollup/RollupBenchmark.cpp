#include "rollup/RollupBenchmark.hpp"
#include "rollup/Transaction.hpp"
#include <chrono>
#include <omp.h>
#include <iostream>
#include <thread>
#include <iomanip>
#include <sstream>

namespace quids {
namespace rollup {

void RollupBenchmark::processBatch(const std::vector<Transaction>& txs) {
    if (txs.empty()) return;

    // Use all available cores
    #ifdef _OPENMP
    omp_set_num_threads(std::thread::hardware_concurrency());
    #endif

    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Pre-allocate results vector to avoid resizing
    const size_t batch_size = txs.size();
    std::vector<bool> results(batch_size);
    
    // Process in parallel chunks with SIMD hints
    #pragma omp parallel for simd schedule(guided)
    for (size_t i = 0; i < batch_size; i++) {
        results[i] = validateAndProcess(txs[i]);
    }
    
    // Count successes using SIMD
    size_t processed = 0;
    #pragma omp simd reduction(+:processed)
    for (size_t i = 0; i < batch_size; i++) {
        processed += results[i] ? 1 : 0;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double seconds = duration.count() / 1e6;
    tps_ = processed / seconds;

    // Format large numbers with commas
    auto format_number = [](double num) -> std::string {
        std::stringstream ss;
        ss.imbue(std::locale(""));  // Use system locale for number formatting
        ss << std::fixed << std::setprecision(0) << num;
        return ss.str();
    };

    // Debug output with formatted numbers
    std::cout << "\nBenchmark Details:\n";
    std::cout << "Batch size: " << format_number(batch_size) << "\n";
    std::cout << "Processed: " << format_number(processed) << "\n";
    std::cout << "Time taken: " << std::fixed << std::setprecision(6) << seconds << "s\n";
    std::cout << "Raw TPS: " << format_number(processed / seconds) << "\n";
    std::cout << "Parallel TPS: " << format_number(tps_) << "\n\n";
}

bool RollupBenchmark::validateAndProcess(const Transaction& tx) {
    total_tx_count_++;
    
    try {
        if (!tx.isValid()) {
            failed_tx_count_++;
            return false;
        }

        // Remove critical section - use atomic operations if needed
        return true;
    } catch (...) {
        failed_tx_count_++;
        return false;
    }
}

} // namespace rollup
} // namespace quids 