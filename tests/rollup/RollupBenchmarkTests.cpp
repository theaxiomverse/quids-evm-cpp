#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <random>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "rollup/RollupStateTransition.hpp"
#include "rollup/ParallelProcessor.hpp"
#include "quantum/QuantumCrypto.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "evm/EVMExecutor.hpp"
#include "rollup/Transaction.hpp"
#include "rollup/StateManager.hpp"
#include "rollup/RollupBenchmark.hpp"
#include "blockchain/Transaction.hpp"
#include <memory>
#include <vector>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace quids::rollup;  // Use rollup namespace
using namespace quids::blockchain;

namespace quids {
namespace rollup {
namespace test {

class RollupBenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize OpenSSL
        if (OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, nullptr) != 1) {
            throw std::runtime_error("Failed to initialize OpenSSL");
        }
        
        // Initialize components
        zkp_generator_ = std::make_shared<QZKPGenerator>();
        state_manager_ = std::make_unique<StateManager>();
        rollup_ = std::make_unique<RollupStateTransition>(zkp_generator_);
        
        // Initialize with default config
        ParallelProcessor::Config config;
        config.num_threads = 4;
        config.batch_size = 100;
        processor_ = std::make_unique<ParallelProcessor>(config);
        
        // Initialize test accounts with ED25519 keys
        for (int i = 0; i < 10; i++) {
            // Generate deterministic seed for testing
            unsigned char seed[32];
            for (int j = 0; j < 32; j++) {
                seed[j] = static_cast<unsigned char>((i * 32 + j) % 256);
            }
            
            // Generate ED25519 key pair
            EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, seed, 32);
            if (!pkey) {
                throw std::runtime_error("Failed to create private key");
            }
            
            // Extract public key
            unsigned char pubkey[32];
            size_t pubkey_len = 32;
            if (EVP_PKEY_get_raw_public_key(pkey, pubkey, &pubkey_len) != 1) {
                EVP_PKEY_free(pkey);
                throw std::runtime_error("Failed to get public key");
            }
            
            // Create hex string from public key for address
            std::stringstream ss;
            for (size_t j = 0; j < pubkey_len; j++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pubkey[j]);
            }
            std::string address = ss.str();
            
            // Store private key for signing
            std::array<uint8_t, 32> private_key;
            std::memcpy(private_key.data(), seed, 32);
            account_keys_[address] = private_key;
            
            // Initialize account in state manager
            StateManager::Account account;
            account.address = address;
            account.balance = 1000000;  // Large initial balance
            account.nonce = 0;
            state_manager_->add_account(address, account);
            
            EVP_PKEY_free(pkey);
        }
    }

    void TearDown() override {
        processor_->stop();
        // Cleanup OpenSSL
        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();
    }

    quids::blockchain::Transaction generateTransaction() {
        // Get random sender and recipient from initialized accounts
        auto accounts = state_manager_->get_accounts_snapshot();
        std::vector<std::string> addresses;
        for (const auto& [addr, _] : accounts) {
            addresses.push_back(addr);
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, addresses.size() - 1);
        
        // Get sender and recipient addresses
        std::string sender = addresses[dis(gen)];
        std::string recipient;
        do {
            recipient = addresses[dis(gen)];
        } while (recipient == sender);  // Ensure sender != recipient
        
        // Get current nonce for sender
        auto sender_account = state_manager_->get_account(sender);
        // Create transaction with a reasonable amount and next nonce
        quids::blockchain::Transaction tx(sender, recipient, 100);
        tx.setNonce(sender_account->nonce + 1);
        
        // Get private key and sign transaction
        auto it = account_keys_.find(sender);
        if (it == account_keys_.end()) {
            throw std::runtime_error("Private key not found for sender: " + sender);
        }
        
        if (!tx.sign(it->second)) {
            throw std::runtime_error("Failed to sign transaction");
        }
        
        return tx;
    }

    std::vector<rollup::Transaction> generateBatch(size_t size) {
        std::vector<rollup::Transaction> batch;
        for (size_t i = 0; i < size; i++) {
            rollup::Transaction tx;  // Explicitly use rollup::Transaction
            tx.setAmount(1000);
            tx.setSender("sender" + std::to_string(i));
            tx.setRecipient("recipient" + std::to_string(i));
            tx.setNonce(i);
            std::vector<uint8_t> sig(64, 1);
            tx.setSignature(sig);
            batch.push_back(tx);
        }
        return batch;
    }

    std::vector<blockchain::Transaction> convertToBlockchainTxs(
        const std::vector<rollup::Transaction>& rollup_txs
    ) {
        std::vector<blockchain::Transaction> blockchain_txs;
        blockchain_txs.reserve(rollup_txs.size());
        
        for (const auto& tx : rollup_txs) {
            blockchain::Transaction blockchain_tx;
            blockchain_tx.setSender(tx.getSender());
            blockchain_tx.setRecipient(tx.getRecipient());
            blockchain_tx.setAmount(tx.getAmount());
            blockchain_tx.setNonce(tx.getNonce());
            blockchain_tx.setGasPrice(tx.getGasPrice());
            blockchain_tx.setGasLimit(tx.getGasLimit());
            blockchain_tx.setData(tx.getData());
            blockchain_tx.setSignature(tx.getSignature());
            blockchain_txs.push_back(blockchain_tx);
        }
        
        return blockchain_txs;
    }

    void runBenchmark(size_t target_tps, seconds duration) {
        auto start_time = steady_clock::now();
        size_t total_tx = 0;
        size_t failed_tx = 0;
        
        while (steady_clock::now() - start_time < duration) {
            auto batch = generateBatch(target_tps / 10); // Process in smaller chunks
            auto blockchain_batch = convertToBlockchainTxs(batch);
            RollupBenchmark benchmark;
            benchmark.processBatch(blockchain_batch);
            
            total_tx += benchmark.getTotalTxCount();
            failed_tx += benchmark.getFailedTxCount();
        }
        
        auto end_time = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(end_time - start_time);
        
        double actual_tps = (total_tx * 1000.0) / elapsed.count();
        double failure_rate = static_cast<double>(failed_tx) / (total_tx + failed_tx);
        
        std::cout << "Benchmark Results:\n"
                  << "Total transactions: " << total_tx << "\n"
                  << "Failed transactions: " << failed_tx << "\n"
                  << "Average TPS: " << actual_tps << "\n"
                  << "Failure rate: " << (failure_rate * 100) << "%\n";

        EXPECT_GT(actual_tps, target_tps * 0.8); // Should achieve at least 80% of target
    }

    std::shared_ptr<QZKPGenerator> zkp_generator_;
    std::unique_ptr<StateManager> state_manager_;
    std::unique_ptr<RollupStateTransition> rollup_;
    std::unique_ptr<ParallelProcessor> processor_;
    std::unordered_map<std::string, std::array<uint8_t, 32>> account_keys_;
};

TEST_F(RollupBenchmarkTest, ThroughputTest) {
    const size_t NUM_TRANSACTIONS = 10000000; // 10M transactions
    const size_t BATCH_SIZE = 100000; // Process in 100K batches
    
    // Pre-allocate the full vector
    std::vector<rollup::Transaction> transactions(NUM_TRANSACTIONS);
    
    // Generate transactions in parallel safely
    #pragma omp parallel for schedule(guided)
    for (size_t i = 0; i < NUM_TRANSACTIONS; i++) {
        // Create transaction directly in the pre-allocated slot
        transactions[i].setAmount(1000);
        transactions[i].setSender("sender" + std::to_string(i));
        transactions[i].setRecipient("recipient" + std::to_string(i));
        transactions[i].setNonce(i);
        std::vector<uint8_t> sig(64, 1);
        transactions[i].setSignature(sig);
    }

    RollupBenchmark benchmark;
    
    // Process in batches
    for (size_t i = 0; i < NUM_TRANSACTIONS; i += BATCH_SIZE) {
        size_t end = std::min(i + BATCH_SIZE, NUM_TRANSACTIONS);
        std::vector<rollup::Transaction> batch(
            transactions.begin() + i,
            transactions.begin() + end
        );
        benchmark.processBatch(convertToBlockchainTxs(batch));
    }

    double tps = benchmark.get_tps();
    auto format_number = [](double num) -> std::string {
        std::stringstream ss;
        ss.imbue(std::locale(""));  // Use system locale for number formatting
        ss << std::fixed << std::setprecision(0) << num;
        return ss.str();
    };

    std::cout << "\nBenchmark Results:\n";
    std::cout << "Total Transactions: " << format_number(NUM_TRANSACTIONS) << "\n";
    std::cout << "Batch Size: " << format_number(BATCH_SIZE) << "\n";
    std::cout << "Final TPS: " << format_number(tps) << "\n\n";
    EXPECT_GT(tps, 100000);
}

TEST_F(RollupBenchmarkTest, ConsensusLatencyTest) {
    auto batch = generateBatch(4);  // Small power of 2 batch size
    auto start_time = steady_clock::now();
    
    auto blockchain_batch = convertToBlockchainTxs(batch);
    auto proof = rollup_->generate_transition_proof(blockchain_batch, *state_manager_);
    
    auto end_time = steady_clock::now();
    auto latency = duration_cast<milliseconds>(end_time - start_time);
    
    std::cout << "Consensus latency: " << latency.count() << "ms\n";
}

TEST_F(RollupBenchmarkTest, StressTestWithTPS) {
    std::vector<size_t> test_tps = {1000, 5000, 10000, 50000};
    
    for (auto target_tps : test_tps) {
        std::cout << "\nTesting at " << target_tps << " TPS:\n";
        
        // Create batch sized for target TPS
        std::vector<rollup::Transaction> transactions;
        const size_t BATCH_SIZE = target_tps * 10; // 10 seconds worth of transactions
        
        for (size_t i = 0; i < BATCH_SIZE; i++) {
            rollup::Transaction tx;
            tx.setAmount(1000);
            tx.setSender("sender" + std::to_string(i));
            tx.setRecipient("recipient" + std::to_string(i));
            tx.setNonce(i);
            std::vector<uint8_t> sig(64, 1);
            tx.setSignature(sig);
            transactions.push_back(tx);
        }

        RollupBenchmark benchmark;
        auto blockchain_batch = convertToBlockchainTxs(transactions);
        benchmark.processBatch(blockchain_batch);

        std::cout << "Benchmark Results:\n";
        std::cout << "Total transactions: " << benchmark.getTotalTxCount() << "\n";
        std::cout << "Failed transactions: " << benchmark.getFailedTxCount() << "\n";
        std::cout << "Average TPS: " << benchmark.get_tps() << "\n";
        std::cout << "Failure rate: " << 
            (100.0 * benchmark.getFailedTxCount() / benchmark.getTotalTxCount()) << "%\n";

        EXPECT_GT(benchmark.get_tps(), target_tps * 0.8); // Should achieve at least 80% of target
    }
}

TEST_F(RollupBenchmarkTest, ProofGenerationBenchmark) {
    auto batch = generateBatch(4);  // Small power of 2 batch size
    
    auto start_time = steady_clock::now();
    auto blockchain_batch = convertToBlockchainTxs(batch);
    auto proof = rollup_->generate_transition_proof(blockchain_batch, *state_manager_);
    auto end_time = steady_clock::now();
    
    auto generation_time = duration_cast<milliseconds>(end_time - start_time);
    std::cout << "Proof generation time: " << generation_time.count() << "ms\n";
}

TEST_F(RollupBenchmarkTest, ProcessSingleBatch) {
    // Create test transactions
    auto batch = generateBatch(10);
    auto blockchain_batch = convertToBlockchainTxs(batch);

    // Create benchmark with batch size
    RollupBenchmark benchmark(blockchain_batch.size());
    
    // Process batch
    benchmark.processBatch(blockchain_batch);
    
    // Verify results
    auto metrics = benchmark.getMetrics();
    EXPECT_GT(metrics.tx_throughput, 0);
    EXPECT_GT(metrics.avg_tx_latency, 0);
    EXPECT_GT(metrics.verification_time, 0);
}

TEST_F(RollupBenchmarkTest, ProcessMultipleBatches) {
    // Create test transactions
    auto batch = generateBatch(20);
    auto blockchain_batch = convertToBlockchainTxs(batch);

    // Create benchmark with batch size
    RollupBenchmark benchmark(blockchain_batch.size());
    
    // Process multiple batches
    for (int i = 0; i < 3; i++) {
        benchmark.processBatch(blockchain_batch);
    }
    
    // Verify results
    auto metrics = benchmark.getMetrics();
    EXPECT_GT(metrics.tx_throughput, 0);
    EXPECT_GT(metrics.avg_tx_latency, 0);
    EXPECT_GT(metrics.verification_time, 0);
}

TEST_F(RollupBenchmarkTest, StressTest) {
    // Create large batch of transactions
    auto batch = generateBatch(1000);
    auto blockchain_batch = convertToBlockchainTxs(batch);

    // Create benchmark with large batch size
    RollupBenchmark benchmark(blockchain_batch.size());
    
    // Process batch
    benchmark.processBatch(blockchain_batch);
    
    // Verify results
    auto metrics = benchmark.getMetrics();
    EXPECT_GT(metrics.tx_throughput, 0);
    EXPECT_GT(metrics.avg_tx_latency, 0);
    EXPECT_GT(metrics.verification_time, 0);
    EXPECT_GT(metrics.success_rate, 0.9);
}

TEST(RollupBenchmarkTests, BasicTest) {
    quids::rollup::RollupBenchmark benchmark(100);
    std::vector<quids::blockchain::Transaction> txs;
    for (size_t i = 0; i < 10; i++) {
        quids::blockchain::Transaction tx;
        tx.setAmount(100);
        tx.setSender("sender" + std::to_string(i));
        tx.setRecipient("recipient" + std::to_string(i));
        tx.setNonce(i);
        txs.push_back(tx);
    }
    benchmark.processBatch(txs);

    std::cout << "Total transactions: " << benchmark.getTotalTxCount() << "\n";
    std::cout << "Failed transactions: " << benchmark.getFailedTxCount() << "\n";
    std::cout << "Average TPS: " << benchmark.get_tps() << "\n";
}

} // namespace test
} // namespace rollup
} // namespace quids 