#pragma once

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <array>
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumCrypto.hpp"
#include "utils/LockFreeQueue.hpp"

namespace quids {
namespace consensus {

struct BatchConfig {
    size_t max_transactions;
    size_t witness_count;
    double consensus_threshold;
    bool use_quantum_proofs;
    size_t batch_size;
    size_t num_parallel_verifiers;
    size_t quantum_circuit_depth;
    bool enable_error_correction;
};

class OptimizedPOBPC {
public:
    struct WitnessInfo {
        std::string node_id;
        std::vector<uint8_t> public_key;
        std::atomic<double> reliability_score;
        std::atomic<uint64_t> last_active;
        quantum::QuantumState quantum_state;
        std::atomic<size_t> successful_validations{0};
        std::atomic<size_t> total_validations{0};
    };

    struct BatchProof {
        uint64_t timestamp;
        size_t transaction_count;
        std::vector<uint8_t> batch_hash;
        std::vector<uint8_t> proof_data;
        std::vector<std::vector<uint8_t>> witness_signatures;
        quantum::QuantumProof quantum_proof;
        std::vector<quantum::QuantumMeasurement> quantum_measurements;
    };

    struct ConsensusMetrics {
        std::atomic<double> avg_batch_time{0.0};
        std::atomic<double> avg_verification_time{0.0};
        std::atomic<uint64_t> total_batches_processed{0};
        std::atomic<uint64_t> total_transactions_processed{0};
        std::atomic<double> witness_participation_rate{0.0};
        std::atomic<double> quantum_security_score{0.0};
        std::atomic<double> avg_quantum_fidelity{1.0};
        std::atomic<uint64_t> error_corrections{0};
    };

    explicit OptimizedPOBPC(const BatchConfig& config);
    ~OptimizedPOBPC();

    // Disable copy and move
    OptimizedPOBPC(const OptimizedPOBPC&) = delete;
    OptimizedPOBPC& operator=(const OptimizedPOBPC&) = delete;
    OptimizedPOBPC(OptimizedPOBPC&&) = delete;
    OptimizedPOBPC& operator=(OptimizedPOBPC&&) = delete;

    // Core consensus operations
    bool addTransaction(const std::vector<uint8_t>& transaction);
    BatchProof generateBatchProof();
    bool verifyBatchProof(const BatchProof& proof);

    // Witness management
    bool registerWitness(const std::string& node_id, const std::vector<uint8_t>& public_key);
    std::vector<WitnessInfo> selectWitnesses();
    bool submitWitnessVote(const std::string& witness_id,
                          const std::vector<uint8_t>& signature,
                          const BatchProof& proof);

    // Consensus status
    bool hasReachedConsensus(const BatchProof& proof);
    double calculateConsensusConfidence(const BatchProof& proof);
    ConsensusMetrics getMetrics() const;

    // Configuration
    void updateConfig(const BatchConfig& config);
    BatchConfig getConfig() const;

private:
    // Forward declaration of implementation
    class Impl;
    std::unique_ptr<Impl> impl_;

    // SIMD-optimized operations
    alignas(64) struct BatchBuffer {
        static constexpr size_t BUFFER_SIZE = 1024;
        std::array<std::vector<uint8_t>, BUFFER_SIZE> transactions;
        std::atomic<size_t> count{0};
        std::atomic<bool> processing{false};
    };

    // Lock-free transaction management
    utils::LockFreeQueue<std::vector<uint8_t>> transactionQueue_;
    
    // Quantum-enhanced security
    struct QuantumSecurityContext {
        quantum::QuantumState consensus_state;
        quantum::QuantumCircuit verification_circuit;
        std::vector<quantum::QuantumMeasurement> measurements;
        std::atomic<double> entanglement_score{0.0};
        std::atomic<double> coherence_score{0.0};
    };

    // Internal helper functions
    std::vector<uint8_t> createBatchHash(const std::vector<std::vector<uint8_t>>& transactions);
    bool verifyWitnessSignature(const std::string& witness_id,
                               const std::vector<uint8_t>& signature,
                               const std::vector<uint8_t>& message);
    void updateWitnessReliability(const std::string& witness_id, bool successful_validation);
    std::vector<WitnessInfo> selectWitnessesRandomly(size_t count);
    bool validateBatchStructure(const BatchProof& proof);
    void recordMetrics(const BatchProof& proof, std::chrono::microseconds processing_time);

    // SIMD-optimized batch processing
    void processBatchSIMD(std::vector<std::vector<uint8_t>>& batch);
    void verifyBatchSIMD(const BatchProof& proof, std::vector<bool>& results);

    // Quantum-enhanced verification
    quantum::QuantumProof generateQuantumProof(const std::vector<std::vector<uint8_t>>& transactions);
    bool verifyQuantumProof(const quantum::QuantumProof& proof, const std::vector<uint8_t>& batch_hash);
    void updateQuantumState(const BatchProof& proof);
    double calculateQuantumSecurityScore(const BatchProof& proof) const;

    // Error correction
    void applyQuantumErrorCorrection();
    bool detectQuantumErrors();
    void recoverFromErrors();

    // Constants
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t SIMD_WIDTH = 8;
    static constexpr double MIN_RELIABILITY_THRESHOLD = 0.5;
    static constexpr size_t MAX_QUANTUM_DEPTH = 100;
    static constexpr double ERROR_THRESHOLD = 1e-6;
};

} // namespace consensus
} // namespace quids 