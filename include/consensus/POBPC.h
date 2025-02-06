#pragma once

#include <vector>
#include <memory>
#include <random>
#include <chrono>
#include "quantum/QuantumCrypto.h"
#include "evm/FloatingPoint.h"

namespace consensus {

class POBPC {
public:
    struct BatchConfig {
        size_t max_transactions{100};
        std::chrono::milliseconds batch_interval{1000};  // 1 second
        size_t witness_count{7};  // Number of witnesses per batch
        double consensus_threshold{0.67};  // 2/3 majority
    };

    struct BatchProof {
        std::vector<uint8_t> proof_data;
        std::vector<uint8_t> batch_hash;
        uint64_t timestamp;
        size_t transaction_count;
        std::vector<std::vector<uint8_t>> witness_signatures;
    };

    struct WitnessInfo {
        std::string node_id;
        std::vector<uint8_t> public_key;
        double reliability_score;
        uint64_t last_active;
    };

    // Constructor
    explicit POBPC(const BatchConfig& config);
    ~POBPC();

    // Batch management
    bool addTransaction(const std::vector<uint8_t>& transaction);
    BatchProof generateBatchProof();
    bool verifyBatchProof(const BatchProof& proof);

    // Witness management
    bool registerWitness(const std::string& node_id, const std::vector<uint8_t>& public_key);
    std::vector<WitnessInfo> selectWitnesses();
    bool submitWitnessVote(const std::string& witness_id, 
                          const std::vector<uint8_t>& signature,
                          const BatchProof& proof);

    // Consensus verification
    bool hasReachedConsensus(const BatchProof& proof);
    double calculateConsensusConfidence(const BatchProof& proof);

    // Performance metrics
    struct ConsensusMetrics {
        double avg_batch_time;
        double avg_verification_time;
        size_t total_batches_processed;
        size_t total_transactions_processed;
        double witness_participation_rate;
    };

    ConsensusMetrics getMetrics() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    BatchConfig config_;

    // Internal methods
    std::vector<uint8_t> createBatchHash(const std::vector<std::vector<uint8_t>>& transactions);
    bool verifyWitnessSignature(const std::string& witness_id,
                               const std::vector<uint8_t>& signature,
                               const std::vector<uint8_t>& message);
    void updateWitnessReliability(const std::string& witness_id, bool successful_validation);
    std::vector<WitnessInfo> selectWitnessesRandomly(size_t count);
    bool validateBatchStructure(const BatchProof& proof);
    void recordMetrics(const BatchProof& proof, std::chrono::microseconds processing_time);
};

// Helper class for ZK batch proofs
class BatchProofGenerator {
public:
    BatchProofGenerator();
    ~BatchProofGenerator();

    std::vector<uint8_t> generateProof(const std::vector<std::vector<uint8_t>>& transactions);
    bool verifyProof(const std::vector<uint8_t>& proof,
                    const std::vector<uint8_t>& batch_hash);

private:
    quantum::QuantumCrypto qcrypto_;
    
    std::vector<uint8_t> createCommitment(const std::vector<std::vector<uint8_t>>& transactions);
    std::vector<uint8_t> generateZKProof(const std::vector<uint8_t>& commitment);
};

} // namespace consensus 