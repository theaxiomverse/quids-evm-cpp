#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <memory>
#include <rocksdb/db.h>
#include "rollup/RollupTransactionAPI.hpp"
#include "crypto/Ed25519Verifier.hpp"

class RealisticPerformanceTest : public ::testing::Test {
protected:
    struct RealTransaction {
        std::array<unsigned char, 32> sender;
        std::array<unsigned char, 32> receiver;
        uint64_t amount;
        std::array<unsigned char, 64> signature;
        std::vector<uint8_t> data;
        
        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> buf;
            buf.insert(buf.end(), sender.begin(), sender.end());
            buf.insert(buf.end(), receiver.begin(), receiver.end());
            auto* amount_ptr = reinterpret_cast<const uint8_t*>(&amount);
            buf.insert(buf.end(), amount_ptr, amount_ptr + sizeof(amount));
            buf.insert(buf.end(), signature.begin(), signature.end());
            buf.insert(buf.end(), data.begin(), data.end());
            return buf;
        }
    };

    void SetUp() override {
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::DB::Open(options, "/tmp/benchmark_db", &db_);
        
        verifier_ = std::make_unique<Ed25519Verifier>();
        tx_api_ = std::make_unique<RollupTransactionAPI>(RollupTransactionAPI::Config{
            .enable_verification = true,
            .persistence_mode = PersistenceMode::EveryTransaction
        });
    }

    void TearDown() override {
        delete db_;
    }

    std::vector<RealTransaction> generateSignedTransactions(size_t count) {
        std::vector<RealTransaction> txs;
        txs.reserve(count);
        
        ed25519_secret_key sk;
        ed25519_public_key pk;
        ed25519_create_keypair(&sk, &pk);
        
        for (size_t i = 0; i < count; ++i) {
            RealTransaction tx;
            tx.sender = pk;
            tx.receiver = pk; // Send to self for simplicity
            tx.amount = i % 1000;
            tx.data = std::vector<uint8_t>(1024, 0xAA); // 1KB payload
            
            // Sign transaction
            auto serialized = tx.serialize();
            ed25519_sign(&serialized[0], serialized.size(), &sk, &pk, tx.signature.data());
            
            txs.push_back(tx);
        }
        return txs;
    }

    rocksdb::DB* db_;
    std::unique_ptr<Ed25519Verifier> verifier_;
    std::unique_ptr<RollupTransactionAPI> tx_api_;
    size_t success_count_ = 0;
    size_t failed_count_ = 0;
};

TEST_F(RealisticPerformanceTest, EndToEndThroughput) {
    const size_t total_transactions = 100000;
    auto transactions = generateSignedTransactions(total_transactions);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Process transactions with real components
    for (const auto& tx : transactions) {
        // 1. Cryptographic verification
        if (!verifier_->verify(tx.serialize(), tx.signature)) {
            failed_count_++;
            continue;
        }
        
        // 2. State update simulation
        auto write_batch = rocksdb::WriteBatch();
        write_batch.Put("state", tx.serialize());
        
        // 3. Persistence
        rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &write_batch);
        if (!status.ok()) {
            failed_count_++;
            continue;
        }
        
        success_count_++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double tps = (success_count_ * 1000.0) / duration.count();
    
    std::cout << "\nRealistic Performance Metrics:\n"
              << "==============================\n"
              << "Successful TXs: " << success_count_ << "\n"
              << "Failed TXs: " << failed_count_ << "\n"
              << "Total Time: " << duration.count() << "ms\n"
              << "TPS: " << tps << "\n"
              << "Verification Fail Rate: " 
              << (100.0 * failed_count_ / total_transactions) << "%\n";
    
    // Realistic expectations for i3-12100F
    EXPECT_GT(tps, 50000.0);  // 50k TPS with real crypto and persistence
    EXPECT_LT(failed_count_, 5);  // <0.005% failure rate
}

TEST_F(RealisticPerformanceTest, SignatureVerificationThroughput) {
    const size_t total_verifications = 1000000;
    auto transactions = generateSignedTransactions(total_verifications);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& tx : transactions) {
        if (!verifier_->verify(tx.serialize(), tx.signature)) {
            failed_count_++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double vps = (total_verifications * 1000.0) / duration.count();
    
    std::cout << "\nCrypto Performance:\n"
              << "====================\n"
              << "Verifications/s: " << vps << "\n"
              << "Failure Rate: " << (100.0 * failed_count_ / total_verifications) << "%\n";
    
    // Ed25519 verification baseline
    EXPECT_GT(vps, 150000.0);  // ~150k verifications/sec on 4C/8T
} 