#pragma once

#include <vector>
#include <memory>
#include <random>
#include <chrono>
#include <atomic>
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumCrypto.hpp"
#include "evm/FloatingPoint.hpp"

namespace quids {
namespace consensus {

class POBPC {
public:
    struct BatchConfig {
        size_t max_transactions{100};
        std::chrono::milliseconds batch_interval{1000};  // 1 second
        size_t witness_count{7};  // Number of witnesses per batch
        double consensus_threshold{0.67};  // 2/3 majority
        bool use_quantum_proofs{true};
        size_t quantum_circuit_depth{20};
        bool enable_error_correction{true};
    };

    struct BatchProof {
        std::vector<uint8_t> proof_data;
        std::vector<uint8_t> batch_hash;
        uint64_t timestamp;
        size_t transaction_count;
        std::vector<std::vector<uint8_t>> witness_signatures;
        QuantumProof quantum_proof;
    };

    struct WitnessInfo {
        std::string node_id;
        std::vector<uint8_t> public_key;
        std::atomic<double> reliability_score;
        std::atomic<uint64_t> last_active;
        QuantumState quantum_state;
    };

    // Constructor
    explicit POBPC(const BatchConfig& config);
    ~POBPC();

    // Disable copy and move
    POBPC(const POBPC&) = delete;
    POBPC& operator=(const POBPC&) = delete;
    POBPC(POBPC&&) = delete;
    POBPC& operator=(POBPC&&) = delete;

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
        std::atomic<double> avg_batch_time{0.0};
        std::atomic<double> avg_verification_time{0.0};
        std::atomic<uint64_t> total_batches_processed{0};
        std::atomic<uint64_t> total_transactions_processed{0};
        std::atomic<double> witness_participation_rate{0.0};
        std::atomic<double> quantum_security_score{0.0};
    };

    ConsensusMetrics getMetrics() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    BatchConfig config_;

    // Internal helper functions
    std::vector<uint8_t> createBatchHash(const std::vector<std::vector<uint8_t>>& transactions);
    bool verifyWitnessSignature(const std::string& witness_id,
                               const std::vector<uint8_t>& signature,
                               const std::vector<uint8_t>& message);
    void updateWitnessReliability(const std::string& witness_id, bool successful_validation);
    std::vector<WitnessInfo> selectWitnessesRandomly(size_t count);
    bool validateBatchStructure(const BatchProof& proof);
    void recordMetrics(const BatchProof& proof, std::chrono::microseconds processing_time);

    // Constants
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t SIMD_WIDTH = 8;
    static constexpr double MIN_RELIABILITY_THRESHOLD = 0.5;
};

} // namespace consensus
} // namespace quids 