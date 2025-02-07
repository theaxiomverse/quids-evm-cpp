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
#include "blockchain/Transaction.hpp"
#include "rollup/StateManager.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

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
        
        ParallelProcessor::Config config;
        config.num_worker_threads = 4;
        config.batch_size = 4;  // Small power of 2
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

    Transaction generateTransaction() {
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
        uint64_t nonce = sender_account->nonce + 1;  // Use next nonce
        
        // Create transaction with a reasonable amount
        Transaction tx(sender, recipient, 100, nonce);  // Small amount to avoid balance issues
        
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

    std::vector<Transaction> generateBatch(size_t size) {
        // Ensure batch size * 256 is a power of 2 for quantum state
        size_t adjusted_size = 1;
        while (adjusted_size * 256 < size * 256) {
            adjusted_size *= 2;
        }
        
        std::vector<Transaction> batch;
        batch.reserve(adjusted_size);
        for (size_t i = 0; i < adjusted_size; i++) {
            batch.push_back(generateTransaction());
        }
        return batch;
    }

    void runBenchmark(size_t target_tps, seconds duration) {
        auto start_time = steady_clock::now();
        size_t total_tx = 0;
        size_t failed_tx = 0;
        
        while (steady_clock::now() - start_time < duration) {
            auto batch = generateBatch(4);  // Use small power of 2 batch size
            try {
                auto proof = rollup_->generate_transition_proof(batch, *state_manager_);
                total_tx += batch.size();
            } catch (const std::exception& e) {
                failed_tx += batch.size();
            }
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
    }

    std::shared_ptr<QZKPGenerator> zkp_generator_;
    std::unique_ptr<StateManager> state_manager_;
    std::unique_ptr<RollupStateTransition> rollup_;
    std::unique_ptr<ParallelProcessor> processor_;
    std::unordered_map<std::string, std::array<uint8_t, 32>> account_keys_;
};

TEST_F(RollupBenchmarkTest, ThroughputTest) {
    runBenchmark(10000, 60s);
}

TEST_F(RollupBenchmarkTest, ConsensusLatencyTest) {
    auto batch = generateBatch(4);  // Small power of 2 batch size
    auto start_time = steady_clock::now();
    
    auto proof = rollup_->generate_transition_proof(batch, *state_manager_);
    
    auto end_time = steady_clock::now();
    auto latency = duration_cast<milliseconds>(end_time - start_time);
    
    std::cout << "Consensus latency: " << latency.count() << "ms\n";
}

TEST_F(RollupBenchmarkTest, StressTest) {
    std::vector<size_t> tps_levels = {1000, 5000, 10000, 50000};
    
    for (auto target_tps : tps_levels) {
        std::cout << "\nTesting at " << target_tps << " TPS:\n";
        runBenchmark(target_tps, 10s);
    }
}

TEST_F(RollupBenchmarkTest, ProofGenerationBenchmark) {
    auto batch = generateBatch(4);  // Small power of 2 batch size
    
    auto start_time = steady_clock::now();
    auto proof = rollup_->generate_transition_proof(batch, *state_manager_);
    auto end_time = steady_clock::now();
    
    auto generation_time = duration_cast<milliseconds>(end_time - start_time);
    std::cout << "Proof generation time: " << generation_time.count() << "ms\n";
}

} // namespace test
} // namespace rollup
} // namespace quids 