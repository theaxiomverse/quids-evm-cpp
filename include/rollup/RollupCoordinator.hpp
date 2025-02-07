#pragma once
#include <memory>
#include "StateManager.hpp"
#include "BatchProcessor.hpp"
#include "zkp/QZKPGenerator.hpp"

class RollupCoordinator {
public:
    struct RollupConfig {
        uint32_t max_batch_size{1000};
        uint32_t target_proof_time_ms{2000};
        uint32_t max_pending_batches{10};
        bool enable_compression{true};
    };

    explicit RollupCoordinator(const RollupConfig& config);

    // Batch management
    void submit_transaction(const Transaction& tx);
    void process_pending_batches();
    
    // State commitment
    std::array<uint8_t, 32> generate_state_commitment();
    
    // L1 interaction
    void submit_batch_to_l1(const std::vector<Transaction>& batch,
                           const ZKPGenerator::Proof& proof);
    
    // Proof generation
    ZKPGenerator::Proof generate_batch_proof(
        const std::vector<Transaction>& batch,
        const std::array<uint8_t, 32>& previous_state_root);

private:
    RollupConfig config_;
    std::unique_ptr<StateManager> state_manager_;
    std::unique_ptr<BatchProcessor> batch_processor_;
    std::unique_ptr<ZKPGenerator> zkp_generator_;
    
    QuantumState prepare_batch_state(const std::vector<Transaction>& batch);
    void validate_batch(const std::vector<Transaction>& batch);
}; 