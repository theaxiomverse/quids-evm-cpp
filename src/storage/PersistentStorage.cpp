#include "storage/PersistentStorage.hpp"
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/backup_engine.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <stdexcept>

namespace quids {
namespace storage {

namespace {
    const std::string BLOCK_CF = "blocks";
    const std::string TX_CF = "transactions";
    const std::string STATE_CF = "state";
    const std::string PROOF_CF = "proofs";
}

// Implementation details
struct PersistentStorage::Impl {
    std::unique_ptr<::rocksdb::DB> db;
    std::unique_ptr<::rocksdb::ColumnFamilyHandle> block_cf;
    std::unique_ptr<::rocksdb::ColumnFamilyHandle> tx_cf;
    std::unique_ptr<::rocksdb::ColumnFamilyHandle> state_cf;
    std::unique_ptr<::rocksdb::ColumnFamilyHandle> proof_cf;
};

PersistentStorage::PersistentStorage(const StorageConfig& config)
    : impl_(std::make_unique<Impl>()), config_(config) {
    validate_config();
    init_database();
}

PersistentStorage::~PersistentStorage() = default;
PersistentStorage::PersistentStorage(PersistentStorage&&) noexcept = default;
PersistentStorage& PersistentStorage::operator=(PersistentStorage&&) noexcept = default;

bool PersistentStorage::init_database() {
    rocksdb::Options options;
    options.create_if_missing = config_.create_if_missing;
    options.max_open_files = -1;  // Use system limit
    
    // Configure table options
    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = rocksdb::NewLRUCache(config_.cache_size_mb * 1024 * 1024);
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
    
    // Define column families
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        BLOCK_CF, rocksdb::ColumnFamilyOptions()));
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        TX_CF, rocksdb::ColumnFamilyOptions()));
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        STATE_CF, rocksdb::ColumnFamilyOptions()));
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        PROOF_CF, rocksdb::ColumnFamilyOptions()));
    
    // Open DB with column families
    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::DB* db_raw = nullptr;
    auto status = rocksdb::DB::Open(options, config_.db_path, column_families, &handles, &db_raw);
    
    if (!status.ok()) {
        return false;
    }
    
    // Take ownership of handles
    impl_->db.reset(db_raw);
    impl_->block_cf.reset(handles[1]);
    impl_->tx_cf.reset(handles[2]);
    impl_->state_cf.reset(handles[3]);
    impl_->proof_cf.reset(handles[4]);
    
    return true;
}

bool PersistentStorage::store_state_update(
    const BlockHeader& header,
    const rollup::StateTransitionProof& proof
) {
    rocksdb::WriteBatch batch;
    rocksdb::WriteOptions write_options;
    
    // Store block header
    std::string block_key = "block:" + std::to_string(header.number);
    batch.Put(impl_->block_cf.get(), 
              rocksdb::Slice(block_key),
              rocksdb::Slice(reinterpret_cast<const char*>(&header), sizeof(header)));
    
    // Store state root
    std::string state_key = "state:" + std::to_string(header.number);
    batch.Put(impl_->state_cf.get(),
              rocksdb::Slice(state_key),
              rocksdb::Slice(reinterpret_cast<const char*>(header.state_root.data()), 32));
    
    // Write batch
    auto status = impl_->db->Write(write_options, &batch);
    return status.ok();
}

std::optional<rollup::StateManager> PersistentStorage::load_state_at_block(uint64_t block_number) {
    std::string state_key = "state:" + std::to_string(block_number);
    std::string state_data;
    
    auto status = impl_->db->Get(rocksdb::ReadOptions(),
                                impl_->state_cf.get(),
                                state_key, &state_data);
    
    if (!status.ok()) {
        return std::nullopt;
    }
    
    // Create StateManager from state data
    rollup::StateManager state_manager;
    // TODO: Initialize state manager from state data
    return state_manager;
}

bool PersistentStorage::store_transaction(
    const blockchain::Transaction& tx,
    const std::array<uint8_t, 32>& block_hash
) {
    rocksdb::WriteBatch batch;
    rocksdb::WriteOptions write_options;
    
    // Store transaction
    auto tx_data = tx.serialize();
    batch.Put(impl_->tx_cf.get(), 
              rocksdb::Slice(reinterpret_cast<const char*>(block_hash.data()), block_hash.size()),
              rocksdb::Slice(reinterpret_cast<const char*>(tx_data.data()), tx_data.size()));
    
    auto status = impl_->db->Write(write_options, &batch);
    return status.ok();
}

std::vector<blockchain::Transaction> PersistentStorage::get_block_transactions(
    const std::array<uint8_t, 32>& block_hash
) {
    std::vector<blockchain::Transaction> transactions;
    
    // Iterate over transactions in block
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions(), impl_->tx_cf.get()));
    for (it->Seek(rocksdb::Slice(reinterpret_cast<const char*>(block_hash.data()), block_hash.size()));
         it->Valid() && it->key().starts_with(rocksdb::Slice(reinterpret_cast<const char*>(block_hash.data()), block_hash.size()));
         it->Next()) {
        
        // Get transaction data
        std::string tx_data = it->value().ToString();
        blockchain::Transaction tx;
        if (tx.deserialize(std::vector<uint8_t>(tx_data.begin(), tx_data.end()))) {
            transactions.push_back(std::move(tx));
        }
    }
    
    return transactions;
}

void PersistentStorage::compact_database() {
    impl_->db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);
}

void PersistentStorage::backup_database(const std::string& backup_path) {
    rocksdb::BackupEngineOptions backup_options(backup_path);
    rocksdb::BackupEngine* backup_engine = nullptr;
    
    auto status = rocksdb::BackupEngine::Open(
        rocksdb::Env::Default(), backup_options, &backup_engine);
    
    if (status.ok()) {
        status = backup_engine->CreateNewBackup(impl_->db.get());
        if (!status.ok()) {
            spdlog::error("Failed to create backup: {}", status.ToString());
        }
        delete backup_engine;
    } else {
        spdlog::error("Failed to open backup engine: {}", status.ToString());
    }
}

void PersistentStorage::cache_proof(
    [[maybe_unused]] const rollup::StateTransitionProof& proof) {
    // TODO: Implement proof caching
}

void PersistentStorage::validate_config() {
    if (config_.db_path.empty()) {
        throw std::invalid_argument("Database path cannot be empty");
    }
    
    if (config_.cache_size_mb == 0) {
        throw std::invalid_argument("Cache size must be greater than 0");
    }
    
    if (!std::filesystem::exists(config_.db_path) && !config_.create_if_missing) {
        throw std::runtime_error("Database path does not exist and create_if_missing is false");
    }
}

} // namespace storage
} // namespace quids 