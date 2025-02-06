#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "blockchain/Transaction.h"
#include "rollup/StateManager.h"
#include "rollup/RollupStateTransition.h"

class PersistentStorage {
public:
    struct StorageConfig {
        std::string db_path;
        size_t cache_size_mb;
        bool enable_compression;
        uint32_t max_open_files;
    };

    struct BlockHeader {
        uint64_t number;
        std::array<uint8_t, 32> state_root;
        std::array<uint8_t, 32> previous_hash;
        uint64_t timestamp;
    };

    struct Config {
        std::string db_path;
        size_t cache_size_mb{512};
        bool create_if_missing{true};
    };

    explicit PersistentStorage(const Config& config);
    ~PersistentStorage();

    // Disable copy
    PersistentStorage(const PersistentStorage&) = delete;
    PersistentStorage& operator=(const PersistentStorage&) = delete;

    // Enable move
    PersistentStorage(PersistentStorage&&) noexcept;
    PersistentStorage& operator=(PersistentStorage&&) noexcept;

    // State management
    bool store_state_update(
        const BlockHeader& header,
        const StateTransitionProof& proof
    );
    
    std::optional<StateManager> load_state_at_block(uint64_t block_number);
    BlockHeader get_latest_block() const;
    
    // Transaction history
    bool store_transaction(
        const Transaction& tx,
        const std::array<uint8_t, 32>& block_hash
    );
    
    std::vector<Transaction> get_block_transactions(
        const std::array<uint8_t, 32>& block_hash
    );
    std::optional<Transaction> get_transaction_by_hash(const std::array<uint8_t, 32>& tx_hash);
    
    // Account history
    std::vector<Transaction> get_account_transactions(
        const std::string& address,
        uint64_t start_block,
        uint64_t end_block
    );
    
    // Proof storage
    bool store_fraud_proof(
        const StateTransitionProof& invalid_proof,
        const StateManager& correct_state
    );
    
    // Maintenance
    void compact_database();
    void backup_database(const std::string& backup_path);

    void cache_proof([[maybe_unused]] const StateTransitionProof& proof);

private:
    struct Impl;  // Forward declaration
    std::unique_ptr<Impl> impl_;
    Config config_;
    
    bool init_database();
    void create_tables();
    void migrate_schema();
}; 