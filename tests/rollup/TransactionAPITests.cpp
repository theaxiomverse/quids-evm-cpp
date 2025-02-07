#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "rollup/RollupTransactionAPI.hpp"
#include "rollup/EnhancedRollupMLModel.hpp"
#include <memory>
#include <string>
#include <vector>

namespace quids {
namespace rollup {
namespace test {

using namespace std::chrono_literals;

class TransactionAPITest : public ::testing::Test {
protected:
    void SetUp() override {
        quids::rollup::EnhancedMLParameters params;
        params.num_layers = 3;
        params.hidden_size = 128;
        params.learning_rate = 0.001;
        params.dropout_rate = 0.2;
        
        ml_model_ = std::make_shared<quids::rollup::EnhancedRollupMLModel>(params);
        api_ = std::make_unique<quids::rollup::RollupTransactionAPI>(ml_model_);
    }
    
    std::unique_ptr<quids::rollup::RollupTransactionAPI> api_;
    std::shared_ptr<quids::rollup::EnhancedRollupMLModel> ml_model_;
};

TEST_F(TransactionAPITest, TestSingleTransaction) {
    quids::rollup::Transaction tx;
    tx.sender = "sender_address";
    tx.recipient = "recipient_address";
    tx.value = 1000;
    tx.nonce = 0;
    
    auto result = api_->submit_transaction(tx);
    EXPECT_FALSE(result.empty());
}

TEST_F(TransactionAPITest, TestBatchTransactions) {
    std::vector<quids::rollup::Transaction> batch;
    
    for (int i = 0; i < 5; i++) {
        quids::rollup::Transaction tx;
        tx.sender = "sender_" + std::to_string(i);
        tx.recipient = "recipient_" + std::to_string(i);
        tx.value = 1000 + i;
        tx.nonce = i;
        batch.push_back(tx);
    }
    
    auto results = api_->submit_batch(batch);
    EXPECT_TRUE(results);
}

TEST_F(TransactionAPITest, TestInvalidBatch) {
    try {
        std::vector<quids::rollup::Transaction> batch;
        for (int i = 0; i < 1000; i++) {  // Too many transactions
            quids::rollup::Transaction tx;
            tx.sender = "sender_" + std::to_string(i);
            tx.recipient = "recipient_" + std::to_string(i);
            tx.value = 1000 + i;
            tx.nonce = i;
            batch.push_back(tx);
        }
        
        auto results = api_->submit_batch(batch);
        FAIL() << "Expected an exception for batch size limit";
    } catch (const std::exception& e) {
        EXPECT_TRUE(std::string(e.what()).find("batch size") != std::string::npos);
    }
}

TEST_F(TransactionAPITest, TestMLOptimization) {
    // Process some transactions to gather metrics
    for (int i = 0; i < 5; ++i) {
        std::vector<quids::rollup::Transaction> batch;
        for (int j = 0; j < 10; j++) {
            quids::rollup::Transaction tx;
            tx.sender = "sender_" + std::to_string(i * 10 + j);
            tx.recipient = "recipient_" + std::to_string(i * 10 + j);
            tx.value = 1000 + i * 10 + j;
            tx.nonce = i * 10 + j;
            batch.push_back(tx);
        }
        api_->submit_batch(batch);
        std::this_thread::sleep_for(100ms);
    }
    
    // Get and verify performance metrics
    auto metrics = api_->get_performance_metrics();
    EXPECT_GT(metrics.tx_throughput, 0);
    EXPECT_GT(metrics.avg_tx_latency, 0.0);
    
    // Get metrics and optimize
    api_->optimize_parameters();
    
    // Allow optimization to take effect and reset metrics
    std::this_thread::sleep_for(500ms);
    api_->reset_metrics();
    
    // Process more transactions
    std::vector<quids::rollup::Transaction> batch;
    for (int i = 0; i < 10; i++) {
        quids::rollup::Transaction tx;
        tx.sender = "sender_" + std::to_string(i);
        tx.recipient = "recipient_" + std::to_string(i);
        tx.value = 1000 + i;
        tx.nonce = i;
        batch.push_back(tx);
    }
    auto results = api_->submit_batch(batch);
    EXPECT_TRUE(results);
    
    // Get and verify updated metrics
    auto metrics1 = api_->get_performance_metrics();
    EXPECT_GT(metrics1.tx_throughput, metrics.tx_throughput);
    EXPECT_GT(metrics1.avg_tx_latency, 0.0);
}

TEST_F(TransactionAPITest, TestInvalidTransactions) {
    // Test empty transaction
    quids::rollup::Transaction tx;
    auto result = api_->submit_transaction(tx);
    EXPECT_FALSE(result.empty());
    
    // Test invalid signature
    tx.sender = "sender_address";
    tx.recipient = "recipient_address";
    tx.value = 1000;
    tx.nonce = 0;
    result = api_->submit_transaction(tx);
    EXPECT_FALSE(result.empty());
    
    // Test zero amount
    tx.nonce = 0;
    result = api_->submit_transaction(tx);
    EXPECT_FALSE(result.empty());
}

TEST_F(TransactionAPITest, TestMetricsReset) {
    // Process some transactions
    std::vector<quids::rollup::Transaction> batch;
    for (int i = 0; i < 10; i++) {
        quids::rollup::Transaction tx;
        tx.sender = "sender_" + std::to_string(i);
        tx.recipient = "recipient_" + std::to_string(i);
        tx.value = 1000 + i;
        tx.nonce = i;
        batch.push_back(tx);
    }
    api_->submit_batch(batch);
    
    // Get metrics and verify they're non-zero
    auto metrics1 = api_->get_performance_metrics();
    EXPECT_GT(metrics1.tx_throughput, 0);
    EXPECT_GT(metrics1.avg_tx_latency, 0.0);
    
    // Reset metrics
    api_->reset_metrics();
    
    // Verify metrics are reset
    auto metrics2 = api_->get_performance_metrics();
    EXPECT_EQ(metrics2.tx_throughput, 0);
    EXPECT_EQ(metrics2.avg_tx_latency, 0.0);
}

} // namespace test
} // namespace rollup
} // namespace quids 