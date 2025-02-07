#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <array>
#include "blockchain/Transaction.hpp"
#include "rollup/StateManager.hpp"
#include "rollup/RollupStateTransition.hpp"

namespace quids {
namespace storage {

class PersistentStorage {
public:
    struct StorageConfig {
        std::string db_path;
        size_t cache_size_mb;
        bool enable_compression;
        uint32_t max_open_files;
        bool create_if_missing{true};
        size_t write_buffer_size{64 * 1024 * 1024};  // 64MB default
        size_t block_cache_size{256 * 1024 * 1024};  // 256MB default
        uint32_t max_background_jobs{4};
    };

    struct BlockHeader {
        uint64_t number;
        std::array<uint8_t, 32> state_root;
        std::array<uint8_t, 32> previous_hash;
        uint64_t timestamp;
        std::array<uint8_t, 32> transactions_root;
        std::array<uint8_t, 32> receipts_root;
        uint64_t gas_used;
        uint64_t gas_limit;
    };

    // Constructor and destructor
    explicit PersistentStorage(const StorageConfig& config);
    ~PersistentStorage();

    // Disable copy
    PersistentStorage(const PersistentStorage&) = delete;
    PersistentStorage& operator=(const PersistentStorage&) = delete;

    // Enable move
    PersistentStorage(PersistentStorage&&) noexcept;
    PersistentStorage& operator=(PersistentStorage&&) noexcept;

    // State management
    [[nodiscard]] bool store_state_update(
        const BlockHeader& header,
        const rollup::StateTransitionProof& proof
    );
    
    [[nodiscard]] std::optional<rollup::StateManager> load_state_at_block(uint64_t block_number);
    [[nodiscard]] BlockHeader get_latest_block() const;
    
    // Transaction history
    [[nodiscard]] bool store_transaction(
        const blockchain::Transaction& tx,
        const std::array<uint8_t, 32>& block_hash
    );
    
    [[nodiscard]] std::vector<blockchain::Transaction> get_block_transactions(
        const std::array<uint8_t, 32>& block_hash
    );

    [[nodiscard]] std::optional<blockchain::Transaction> get_transaction_by_hash(
        const std::array<uint8_t, 32>& tx_hash
    );
    
    // Account history
    [[nodiscard]] std::vector<blockchain::Transaction> get_account_transactions(
        const std::string& address,
        uint64_t start_block,
        uint64_t end_block
    );
    
    // Proof storage and verification
    [[nodiscard]] bool store_fraud_proof(
        const rollup::StateTransitionProof& invalid_proof,
        const rollup::StateManager& correct_state
    );
    
    void cache_proof(const rollup::StateTransitionProof& proof);
    [[nodiscard]] bool verify_cached_proof(const std::array<uint8_t, 32>& proof_hash);
    
    // Database maintenance
    void compact_database();
    void backup_database(const std::string& backup_path);
    void optimize_for_reads();
    void optimize_for_writes();
    
    // Metrics and diagnostics
    [[nodiscard]] size_t get_database_size() const;
    [[nodiscard]] std::string get_statistics() const;
    [[nodiscard]] double get_compression_ratio() const;

private:
    // Forward declaration of implementation
    class Impl;
    std::unique_ptr<Impl> impl_;
    StorageConfig config_;
    
    // Internal helper methods
    bool init_database();
    void create_tables();
    void migrate_schema();
    void validate_config();
    
    // Cache management
    void flush_cache();
    void clear_cache();
    void resize_cache(size_t new_size_mb);
    
    // Constants
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t MAX_KEY_SIZE = 256;
    static constexpr size_t MAX_VALUE_SIZE = 1024 * 1024;  // 1MB
    static constexpr uint32_t SCHEMA_VERSION = 1;
};

} // namespace storage
} // namespace quids 