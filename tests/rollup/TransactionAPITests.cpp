#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "rollup/RollupTransactionAPI.h"
#include "rollup/EnhancedRollupMLModel.h"

using namespace std::chrono_literals;

class TransactionAPITests : public ::testing::Test {
protected:
    void SetUp() override {
        EnhancedMLParameters params;
        params.num_layers = 3;
        params.hidden_size = 128;
        params.learning_rate = 0.001;
        params.dropout_rate = 0.2;
        
        ml_model_ = std::make_shared<EnhancedRollupMLModel>(params);
        api_ = std::make_unique<RollupTransactionAPI>(ml_model_);
    }
    
    std::unique_ptr<RollupTransactionAPI> api_;
    std::shared_ptr<EnhancedRollupMLModel> ml_model_;
};

TEST_F(TransactionAPITests, SubmitSingleTransaction) {
    RollupTransactionAPI::Transaction tx;
    tx.sender = "sender123";
    tx.recipient = "recipient456";
    tx.amount = 1000;
    tx.signature = "valid_signature";
    tx.data = std::vector<uint8_t>(100, 0x42);
    
    auto result = api_->submitTransaction(tx);
    EXPECT_FALSE(result.empty());
}

TEST_F(TransactionAPITests, SubmitBatch) {
    std::vector<RollupTransactionAPI::Transaction> batch;
    for (int i = 0; i < 10; i++) {
        RollupTransactionAPI::Transaction tx;
        tx.sender = "sender" + std::to_string(i);
        tx.recipient = "recipient" + std::to_string(i);
        tx.amount = 1000 + i;
        tx.signature = "valid_signature_" + std::to_string(i);
        tx.data = std::vector<uint8_t>(100, 0x42);
        batch.push_back(std::move(tx));
    }
    
    auto results = api_->submitBatch(batch);
    EXPECT_TRUE(results);
}

TEST_F(TransactionAPITests, ConcurrentBatchSubmission) {
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < 5; i++) {
        std::vector<RollupTransactionAPI::Transaction> batch;
        for (int j = 0; j < 10; j++) {
            RollupTransactionAPI::Transaction tx;
            tx.sender = "sender" + std::to_string(i * 10 + j);
            tx.recipient = "recipient" + std::to_string(i * 10 + j);
            tx.amount = 1000 + i * 10 + j;
            tx.signature = "valid_signature_" + std::to_string(i * 10 + j);
            tx.data = std::vector<uint8_t>(100, 0x42);
            batch.push_back(std::move(tx));
        }
        
        futures.push_back(std::async(std::launch::async, [this, b = std::move(batch)]() mutable {
            return api_->submitBatch(std::move(b));
        }));
    }
    
    for (auto& future : futures) {
        EXPECT_TRUE(future.get());
    }
}

TEST_F(TransactionAPITests, TestMLOptimization) {
    // Process some transactions to gather metrics
    for (int i = 0; i < 5; ++i) {
        std::vector<RollupTransactionAPI::Transaction> batch;
        for (int j = 0; j < 10; j++) {
            RollupTransactionAPI::Transaction tx;
            tx.sender = "sender" + std::to_string(i * 10 + j);
            tx.recipient = "recipient" + std::to_string(i * 10 + j);
            tx.amount = 1000 + i * 10 + j;
            tx.signature = "valid_signature_" + std::to_string(i * 10 + j);
            tx.data = std::vector<uint8_t>(100, 0x42);
            batch.push_back(std::move(tx));
        }
        api_->submitBatch(std::move(batch));
        std::this_thread::sleep_for(100ms);
    }
    
    // Get and verify performance metrics
    auto metrics = api_->getPerformanceMetrics();
    EXPECT_GT(metrics.tx_throughput, 0);
    EXPECT_GT(metrics.avg_tx_latency, 0.0);
    
    // Get metrics and optimize
    api_->optimizeParameters();
    
    // Allow optimization to take effect and reset metrics
    std::this_thread::sleep_for(500ms);
    api_->resetMetrics();
    
    // Process more transactions
    std::vector<RollupTransactionAPI::Transaction> batch;
    for (int i = 0; i < 10; i++) {
        RollupTransactionAPI::Transaction tx;
        tx.sender = "sender" + std::to_string(i);
        tx.recipient = "recipient" + std::to_string(i);
        tx.amount = 1000 + i;
        tx.signature = "valid_signature_" + std::to_string(i);
        tx.data = std::vector<uint8_t>(100, 0x42);
        batch.push_back(std::move(tx));
    }
    auto results = api_->submitBatch(std::move(batch));
    EXPECT_TRUE(results);
    
    // Get and verify updated metrics
    auto metrics1 = api_->getPerformanceMetrics();
    EXPECT_GT(metrics1.tx_throughput, metrics.tx_throughput);
    EXPECT_GT(metrics1.avg_tx_latency, 0.0);
}

TEST_F(TransactionAPITests, TestInvalidTransactions) {
    // Test empty transaction
    RollupTransactionAPI::Transaction tx;
    auto result = api_->submitTransaction(tx);
    EXPECT_FALSE(result.empty());
    
    // Test invalid signature
    tx.sender = "sender123";
    tx.recipient = "recipient456";
    tx.amount = 1000;
    tx.signature = "invalid_signature";
    result = api_->submitTransaction(tx);
    EXPECT_FALSE(result.empty());
    
    // Test zero amount
    tx.signature = "valid_signature";
    tx.amount = 0;
    result = api_->submitTransaction(tx);
    EXPECT_FALSE(result.empty());
}

TEST_F(TransactionAPITests, TestMetricsReset) {
    // Process some transactions
    std::vector<RollupTransactionAPI::Transaction> batch;
    for (int i = 0; i < 10; i++) {
        RollupTransactionAPI::Transaction tx;
        tx.sender = "sender" + std::to_string(i);
        tx.recipient = "recipient" + std::to_string(i);
        tx.amount = 1000 + i;
        tx.signature = "valid_signature_" + std::to_string(i);
        tx.data = std::vector<uint8_t>(100, 0x42);
        batch.push_back(std::move(tx));
    }
    api_->submitBatch(std::move(batch));
    
    // Get metrics and verify they're non-zero
    auto metrics1 = api_->getPerformanceMetrics();
    EXPECT_GT(metrics1.tx_throughput, 0);
    EXPECT_GT(metrics1.avg_tx_latency, 0.0);
    
    // Reset metrics
    api_->resetMetrics();
    
    // Verify metrics are reset
    auto metrics2 = api_->getPerformanceMetrics();
    EXPECT_EQ(metrics2.tx_throughput, 0);
    EXPECT_EQ(metrics2.avg_tx_latency, 0.0);
} 