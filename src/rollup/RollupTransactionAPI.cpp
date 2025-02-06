#include "rollup/RollupTransactionAPI.h"
#include "rollup/EnhancedRollupMLModel.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

using namespace std::chrono;

RollupTransactionAPI::RollupTransactionAPI(
    std::shared_ptr<EnhancedRollupMLModel> ml_model,
    size_t num_worker_threads
) : ml_model_(ml_model),
    should_stop_(false),
    last_metrics_update_(std::chrono::system_clock::now()) {
    
    worker_threads_.reserve(num_worker_threads);
    for (size_t i = 0; i < num_worker_threads; ++i) {
        worker_threads_.emplace_back(&RollupTransactionAPI::workerThread, this);
    }
}

RollupTransactionAPI::~RollupTransactionAPI() {
    stopProcessing();
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

std::string RollupTransactionAPI::submitTransaction(const Transaction& tx) {
    std::string validation_result = validateTransactionWithMessage(tx);
    if (!validation_result.empty()) {
        return validation_result;  // Return the error message
    }

    auto start = steady_clock::now();
    
    TransactionBatch batch;
    batch.transactions.push_back(tx);
    batch.timestamp = system_clock::now();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        batch_queue_.push(std::move(batch));
    }
    queue_cv_.notify_one();

    auto end = steady_clock::now();
    recordLatency(duration_cast<microseconds>(end - start));
    
    return calculateTransactionHash(tx);
}

bool RollupTransactionAPI::submitBatch(const std::vector<Transaction>& transactions) {
    if (transactions.empty()) {
        return false;
    }

    auto start = steady_clock::now();

    TransactionBatch batch;
    batch.transactions = transactions;
    batch.timestamp = system_clock::now();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        batch_queue_.push(std::move(batch));
    }
    queue_cv_.notify_one();

    auto end = steady_clock::now();
    recordLatency(duration_cast<microseconds>(end - start));

    return true;
}

void RollupTransactionAPI::startProcessing() {
    should_stop_ = false;
}

void RollupTransactionAPI::stopProcessing() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        should_stop_ = true;
    }
    queue_cv_.notify_all();
}

void RollupTransactionAPI::workerThread() {
    while (true) {
        TransactionBatch batch;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return should_stop_ || !batch_queue_.empty();
            });

            if (should_stop_ && batch_queue_.empty()) {
                break;
            }

            batch = std::move(batch_queue_.front());
            batch_queue_.pop();
        }

        processBatch(batch);
    }
}

bool RollupTransactionAPI::validateTransaction(const Transaction& tx) const {
    return validateTransactionWithMessage(tx).empty();
}

std::string RollupTransactionAPI::validateTransactionWithMessage(const Transaction& tx) const {
    if (tx.sender.empty()) {
        return "Invalid transaction: Empty sender";
    }
    if (tx.recipient.empty()) {
        return "Invalid transaction: Empty recipient";
    }
    if (tx.amount == 0) {
        return "Invalid transaction: Zero amount";
    }

    // Create OpenSSL message digest context
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return "Invalid transaction: Failed to create message digest context";
    }

    // Initialize SHA-256 hashing
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "Invalid transaction: Failed to initialize SHA-256";
    }

    // Hash transaction data
    std::string data = tx.sender + tx.recipient + std::to_string(tx.amount);
    if (EVP_DigestUpdate(mdctx, data.c_str(), data.length()) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "Invalid transaction: Failed to update digest";
    }

    // Get hash result
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "Invalid transaction: Failed to finalize digest";
    }

    EVP_MD_CTX_free(mdctx);
    return "";  // Empty string means validation passed
}

bool RollupTransactionAPI::processBatch(const TransactionBatch& batch) {
    auto start = steady_clock::now();
    bool success = true;

    for (const auto& tx : batch.transactions) {
        if (!validateTransaction(tx)) {
            success = false;
            break;
        }
        recordLatency(duration_cast<microseconds>(steady_clock::now() - start));
    }

    auto end = steady_clock::now();
    recordProofTime(duration_cast<microseconds>(end - start));
    return success;
}

std::string RollupTransactionAPI::calculateTransactionHash(const Transaction& tx) const {
    // Create OpenSSL message digest context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP context");
    }

    // Initialize SHA-256 hashing
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize SHA256");
    }

    // Hash transaction data
    std::string data = tx.sender + tx.recipient + std::to_string(tx.amount);
    if (EVP_DigestUpdate(ctx, data.c_str(), data.length()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update digest");
    }

    // Get hash result
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(ctx);

    // Convert hash to hex string
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

RollupPerformanceMetrics RollupTransactionAPI::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return current_metrics_;
}

void RollupTransactionAPI::resetMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_ = RollupPerformanceMetrics();
    current_metrics_.total_transactions = 0;
    current_metrics_.tx_throughput = 0;
    current_metrics_.avg_tx_latency = 0;
    last_metrics_update_ = system_clock::now();
}

void RollupTransactionAPI::setMLModel(std::shared_ptr<EnhancedRollupMLModel> model) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    ml_model_ = model;
}

void RollupTransactionAPI::optimizeParameters() {
    if (ml_model_) {
        std::vector<std::pair<std::string, double>> objective_weights = {
            {"throughput", 0.4},
            {"energy_efficiency", 0.3},
            {"latency", 0.3}
        };
        CrossChainState chain_state;  // Default state
        ml_model_->optimizeParameters(getPerformanceMetrics(), chain_state, objective_weights);
    }
}

bool RollupTransactionAPI::isOverloaded() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return batch_queue_.size() > 1000; // Arbitrary threshold
}

void RollupTransactionAPI::recordLatency(microseconds latency) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.avg_tx_latency = 
        (current_metrics_.avg_tx_latency * current_metrics_.total_transactions + latency.count() / 1000000.0) / 
        (current_metrics_.total_transactions + 1);
    current_metrics_.total_transactions++;
    
    // Calculate throughput based on a sliding window
    auto now = system_clock::now();
    auto duration = duration_cast<microseconds>(now - last_metrics_update_).count() / 1000000.0;
    if (duration > 0) {
        // Use a sliding window of 1 second for throughput calculation
        if (duration > 1.0) {
            last_metrics_update_ = now - seconds(1);
            duration = 1.0;
        }
        current_metrics_.tx_throughput = current_metrics_.total_transactions / duration;
    }
}

void RollupTransactionAPI::recordProofTime(microseconds time) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.proof_generation_time = 
        (current_metrics_.proof_generation_time + time.count() / 1000000.0) / 2;
}

void RollupTransactionAPI::recordVerificationTime(microseconds time) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.verification_time = 
        (current_metrics_.verification_time + time.count() / 1000000.0) / 2;
} 