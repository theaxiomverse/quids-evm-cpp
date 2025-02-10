#include <gtest/gtest.h>
#include "consensus/OptimizedPOBPC.hpp"
#include <thread>
#include <future>

class POBPCTest : public ::testing::Test {
protected:
    void SetUp() override {
        ::quids::consensus::OptimizedPOBPC::BatchConfig config;
        config.max_transactions = 100;
        config.batch_interval = std::chrono::milliseconds(1000);
        config.witness_count = 7;
        config.consensus_threshold = 0.67;
        
        pobpc_ = std::make_unique<::quids::consensus::OptimizedPOBPC>(config);
        
        // Register test witnesses
        for (size_t i = 0; i < 10; ++i) {
            auto [pk, sk] = generateWitnessKeys();
            witness_keys_.push_back({pk, sk});
            pobpc_->registerWitness("witness_" + std::to_string(i), pk);
        }
    }
    
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generateWitnessKeys() {
        quids::quantum::QuantumCrypto qcrypto;
        return qcrypto.QGenerateKeypair(QSignatureScheme::DILITHIUM5);
    }
    
    std::vector<uint8_t> createTestTransaction() {
        return std::vector<uint8_t>(64, 0x42);  // Simple test transaction
    }
    
    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> witness_keys_;
    std::unique_ptr<quids::consensus::OptimizedPOBPC> pobpc_;
};

TEST_F(POBPCTest, BatchCreationTest) {
    // Add transactions
    for (size_t i = 0; i < 50; ++i) {
        EXPECT_TRUE(pobpc_->addTransaction(createTestTransaction()));
    }
    
    // Generate batch proof
    auto proof = pobpc_->generateBatchProof();
    
    EXPECT_EQ(proof.transaction_count, 50);
    EXPECT_FALSE(proof.proof_data.empty());
    EXPECT_FALSE(proof.batch_hash.empty());
    EXPECT_GT(proof.timestamp, 0);
}

TEST_F(POBPCTest, WitnessSelectionTest) {
    auto witnesses = pobpc_->selectWitnesses();
    
    EXPECT_EQ(witnesses.size(), 7);  // Config witness_count
    
    // Verify all witnesses have valid public keys
    for (const auto& witness : witnesses) {
        EXPECT_FALSE(witness.public_key.empty());
        EXPECT_GT(witness.reliability_score, 0.0);
    }
}

TEST_F(POBPCTest, ConsensusAchievementTest) {
    // Create a batch
    for (size_t i = 0; i < 10; ++i) {
        pobpc_->addTransaction(createTestTransaction());
    }
    
    auto proof = pobpc_->generateBatchProof();
    
    // Get witnesses
    auto witnesses = pobpc_->selectWitnesses();
    
    // Submit votes from witnesses
    size_t votes = 0;
    quantum::QuantumCrypto qcrypto;
    
    for (size_t i = 0; i < witnesses.size(); ++i) {
        auto signature = qcrypto.QSign(proof.batch_hash, witness_keys_[i].second);
        if (pobpc_->submitWitnessVote("witness_" + std::to_string(i), signature, proof)) {
            votes++;
        }
    }
    
    // Verify consensus
    EXPECT_TRUE(pobpc_->hasReachedConsensus(proof));
    EXPECT_GT(pobpc_->calculateConsensusConfidence(proof), 0.67);
}

TEST_F(POBPCTest, BatchVerificationTest) {
    // Create and verify a batch
    for (size_t i = 0; i < 10; ++i) {
        pobpc_->addTransaction(createTestTransaction());
    }
    
    auto proof = pobpc_->generateBatchProof();
    EXPECT_TRUE(pobpc_->verifyBatchProof(proof));
    
    // Modify proof and verify it fails
    proof.proof_data[0] ^= 1;
    EXPECT_FALSE(pobpc_->verifyBatchProof(proof));
}

TEST_F(POBPCTest, WitnessReliabilityTest) {
    // Create a batch
    for (size_t i = 0; i < 10; ++i) {
        pobpc_->addTransaction(createTestTransaction());
    }
    
    auto proof = pobpc_->generateBatchProof();
    auto witnesses = pobpc_->selectWitnesses();
    quantum::QuantumCrypto qcrypto;
    
    // Submit both valid and invalid votes
    for (size_t i = 0; i < witnesses.size(); ++i) {
        auto signature = qcrypto.QSign(proof.batch_hash, witness_keys_[i].second);
        if (i % 2 == 0) {
            // Submit valid vote
            pobpc_->submitWitnessVote("witness_" + std::to_string(i), signature, proof);
        } else {
            // Submit invalid vote
            signature[0] ^= 1;
            pobpc_->submitWitnessVote("witness_" + std::to_string(i), signature, proof);
        }
    }
    
    // Verify reliability scores were updated
    auto updated_witnesses = pobpc_->selectWitnesses();
    for (const auto& witness : updated_witnesses) {
        EXPECT_GT(witness.reliability_score, 0.0);
        EXPECT_LE(witness.reliability_score, 1.0);
    }
}

TEST_F(POBPCTest, ConcurrentOperationsTest) {
    const size_t THREAD_COUNT = 4;
    const size_t TRANSACTIONS_PER_THREAD = 25;
    
    std::vector<std::future<void>> futures;
    std::atomic<bool> failed{false};
    
    // Submit transactions concurrently
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            try {
                for (size_t j = 0; j < TRANSACTIONS_PER_THREAD && !failed; ++j) {
                    if (!pobpc_->addTransaction(createTestTransaction())) {
                        failed = true;
                        break;
                    }
                }
            } catch (...) {
                failed = true;
            }
        }));
    }
    
    // Wait for all threads
    for (auto& future : futures) {
        future.wait();
    }
    
    EXPECT_FALSE(failed);
    
    // Generate and verify batch
    auto proof = pobpc_->generateBatchProof();
    EXPECT_EQ(proof.transaction_count, THREAD_COUNT * TRANSACTIONS_PER_THREAD);
    EXPECT_TRUE(pobpc_->verifyBatchProof(proof));
}

TEST_F(POBPCTest, PerformanceTest) {
    const size_t BATCH_SIZES[] = {10, 50, 100};
    
    for (size_t batch_size : BATCH_SIZES) {
        // Fill batch
        for (size_t i = 0; i < batch_size; ++i) {
            pobpc_->addTransaction(createTestTransaction());
        }
        
        // Measure proof generation time
        auto start = std::chrono::high_resolution_clock::now();
        auto proof = pobpc_->generateBatchProof();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Batch size " << batch_size 
                  << " proof generation time: " << duration.count() 
                  << "ms" << std::endl;
        
        // Measure verification time
        start = std::chrono::high_resolution_clock::now();
        EXPECT_TRUE(pobpc_->verifyBatchProof(proof));
        end = std::chrono::high_resolution_clock::now();
        
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Batch size " << batch_size 
                  << " verification time: " << duration.count() 
                  << "ms" << std::endl;
    }
}

TEST_F(POBPCTest, MetricsTest) {
    // Generate several batches
    for (size_t batch = 0; batch < 5; ++batch) {
        for (size_t i = 0; i < 20; ++i) {
            pobpc_->addTransaction(createTestTransaction());
        }
        
        auto proof = pobpc_->generateBatchProof();
        auto witnesses = pobpc_->selectWitnesses();
        
        // Submit witness votes
        quantum::QuantumCrypto qcrypto;
        for (size_t i = 0; i < witnesses.size(); ++i) {
            auto signature = qcrypto.QSign(proof.batch_hash, witness_keys_[i].second);
            pobpc_->submitWitnessVote("witness_" + std::to_string(i), signature, proof);
        }
    }
    
    // Verify metrics
    auto metrics = pobpc_->getMetrics();
    EXPECT_GT(metrics.avg_batch_time, 0.0);
    EXPECT_GT(metrics.avg_verification_time, 0.0);
    EXPECT_EQ(metrics.total_batches_processed, 5);
    EXPECT_EQ(metrics.total_transactions_processed, 100);
    EXPECT_GT(metrics.witness_participation_rate, 0.0);
} 