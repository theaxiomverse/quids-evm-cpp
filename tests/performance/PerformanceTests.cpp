#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <future>
#include "rollup/RollupTransactionAPI.hpp"
#include "evm/EVMExecutor.hpp"
#include "consensus/POBPC.hpp"

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize components
        evm_executor_ = std::make_unique<evm::EVMExecutor>();
        
        rollup::RollupTransactionAPI::Config config;
        config.num_worker_threads = std::thread::hardware_concurrency();
        config.batch_size = 1000;
        config.enable_parallel_processing = true;
        
        tx_api_ = std::make_unique<rollup::RollupTransactionAPI>(config);
    }
    
    Transaction createTestTransaction(size_t data_size = 100) {
        Transaction tx;
        tx.sender = "0x" + std::string(40, '1');
        tx.recipient = "0x" + std::string(40, '2');
        tx.amount = 1000000;
        tx.nonce = nonce_counter_++;
        tx.data = std::vector<uint8_t>(data_size, 0x42);
        return tx;
    }
    
    std::vector<Transaction> generateTestTransactions(size_t count) {
        std::vector<Transaction> transactions;
        transactions.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            transactions.push_back(createTestTransaction());
        }
        return transactions;
    }
    
    double measureTPS(size_t num_transactions, size_t batch_size) {
        auto transactions = generateTestTransactions(num_transactions);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t processed = 0;
        while (processed < transactions.size()) {
            size_t batch_end = std::min(processed + batch_size, transactions.size());
            std::vector<Transaction> batch(
                transactions.begin() + processed,
                transactions.begin() + batch_end
            );
            tx_api_->submitBatch(batch);
            processed = batch_end;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return (num_transactions * 1000.0) / duration.count();
    }
    
    std::unique_ptr<evm::EVMExecutor> evm_executor_;
    std::unique_ptr<rollup::RollupTransactionAPI> tx_api_;
    std::atomic<uint64_t> nonce_counter_{0};
};

TEST_F(PerformanceTest, TransactionThroughputTest) {
    const std::vector<size_t> batch_sizes = {100, 500, 1000, 5000};
    const size_t total_transactions = 100000;
    
    std::cout << "\nTransaction Throughput Test Results:" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    
    for (size_t batch_size : batch_sizes) {
        double tps = measureTPS(total_transactions, batch_size);
        std::cout << "Batch Size: " << batch_size 
                  << ", TPS: " << std::fixed << std::setprecision(2) << tps << std::endl;
        
        // Basic throughput assertions
        EXPECT_GT(tps, 1000.0);  // Minimum acceptable TPS
    }
}

TEST_F(PerformanceTest, ContractCompilationTest) {
    const std::string test_contract = R"(
        pragma solidity ^0.8.0;
        contract TestContract {
            uint256 private value;
            
            function setValue(uint256 _value) public {
                value = _value;
            }
            
            function getValue() public view returns (uint256) {
                return value;
            }
        }
    )";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Compile contract 100 times to get average
    const size_t iterations = 100;
    for (size_t i = 0; i < iterations; ++i) {
        auto result = evm_executor_->compile_contract(test_contract);
        EXPECT_FALSE(result.bytecode.empty());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_compilation_time = duration.count() / static_cast<double>(iterations);
    std::cout << "\nContract Compilation Performance:" << std::endl;
    std::cout << "Average compilation time: " << avg_compilation_time << " microseconds" << std::endl;
    
    EXPECT_LT(avg_compilation_time, 1000000.0);  // Should compile in under 1 second
}

TEST_F(PerformanceTest, ContractExecutionTest) {
    // Deploy test contract
    const std::string test_contract = R"(
        pragma solidity ^0.8.0;
        contract TestContract {
            uint256[] private values;
            
            function addValue(uint256 _value) public {
                values.push(_value);
            }
            
            function processValues() public view returns (uint256) {
                uint256 sum = 0;
                for (uint i = 0; i < values.length; i++) {
                    sum += values[i];
                }
                return sum;
            }
        }
    )";
    
    auto compilation_result = evm_executor_->compile_contract(test_contract);
    ASSERT_FALSE(compilation_result.bytecode.empty());
    
    evm::EVMContext context;
    context.gas_left = 10000000;
    
    auto deployment_result = evm_executor_->deploy_contract(
        compilation_result.bytecode,
        std::vector<uint8_t>(),
        context
    );
    ASSERT_TRUE(deployment_result.success);
    
    // Measure execution time for different operations
    const size_t iterations = 1000;
    
    // Test addValue
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        auto input = evm::EvmAbiCompat::encodeFunctionCall(
            "addValue(uint256)",
            {std::vector<uint8_t>{42}}
        );
        auto result = evm_executor_->execute_contract(
            deployment_result.contract_address,
            input,
            context
        );
        ASSERT_TRUE(result.success);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::microseconds>(end - start);
    
    double avg_execution_time = duration.count() / static_cast<double>(iterations);
    std::cout << "\nContract Execution Performance:" << std::endl;
    std::cout << "Average execution time (addValue): " 
              << avg_execution_time << " microseconds" << std::endl;
    
    // Test processValues
    start = std::chrono::high_resolution_clock::now();
    auto input = evm::EvmAbiCompat::encodeFunctionCall(
        "processValues()",
        std::vector<std::vector<uint8_t>>()
    );
    auto result = evm_executor_->execute_contract(
        deployment_result.contract_address,
        input,
        context
    );
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::microseconds>(end - start);
    
    std::cout << "Execution time (processValues): " 
              << duration.count() << " microseconds" << std::endl;
    
    EXPECT_TRUE(result.success);
    EXPECT_LT(avg_execution_time, 1000.0);  // Should execute in under 1ms
}

TEST_F(PerformanceTest, ParallelProcessingTest) {
    const size_t num_threads = std::thread::hardware_concurrency();
    const size_t transactions_per_thread = 10000;
    const size_t total_transactions = num_threads * transactions_per_thread;
    
    std::vector<std::future<void>> futures;
    std::atomic<size_t> successful_transactions{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Launch threads
    for (size_t i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            auto transactions = generateTestTransactions(transactions_per_thread);
            for (const auto& tx : transactions) {
                if (tx_api_->submitTransaction(tx)) {
                    successful_transactions++;
                }
            }
        }));
    }
    
    // Wait for all threads
    for (auto& future : futures) {
        future.wait();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::milliseconds>(end - start);
    
    double tps = (successful_transactions * 1000.0) / duration.count();
    
    std::cout << "\nParallel Processing Performance:" << std::endl;
    std::cout << "Number of threads: " << num_threads << std::endl;
    std::cout << "Successful transactions: " << successful_transactions << std::endl;
    std::cout << "TPS: " << std::fixed << std::setprecision(2) << tps << std::endl;
    
    EXPECT_EQ(successful_transactions, total_transactions);
    EXPECT_GT(tps, 5000.0);  // Should achieve at least 5k TPS with parallel processing
} 