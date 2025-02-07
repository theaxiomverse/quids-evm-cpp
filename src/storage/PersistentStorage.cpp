#include "storage/PersistentStorage.hpp"
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <sstream>
#include <filesystem>

namespace quids {
namespace storage {

struct PersistentStorage::Impl {
    std::unique_ptr<rocksdb::DB> db;
    std::string data_dir;

    explicit Impl(const std::string& dir) : data_dir(dir) {
        rocksdb::Options options;
        options.create_if_missing = true;
        options.compression = rocksdb::kLZ4Compression;
        options.max_background_jobs = 4;
        options.write_buffer_size = 64 * 1024 * 1024; // 64MB
        options.target_file_size_base = 64 * 1024 * 1024; // 64MB
        
        rocksdb::DB* db_ptr = nullptr;
        rocksdb::Status status = rocksdb::DB::Open(options, data_dir, &db_ptr);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open database: " + status.ToString());
        }
        db.reset(db_ptr);
    }

    std::string makeKey(const std::string& prefix, const std::string& id) {
        return prefix + ":" + id;
    }
};

PersistentStorage::PersistentStorage(const std::string& data_dir) 
    : impl_(std::make_unique<Impl>(data_dir)) {}

PersistentStorage::~PersistentStorage() = default;

bool PersistentStorage::storeTransaction(const blockchain::Transaction& tx) {
    auto serialized = tx.serialize();
    auto hash = tx.compute_hash();
    std::string key = impl_->makeKey("tx", std::string(hash.begin(), hash.end()));
    
    rocksdb::Status status = impl_->db->Put(
        rocksdb::WriteOptions(),
        key,
        rocksdb::Slice(reinterpret_cast<const char*>(serialized.data()), serialized.size())
    );
    
    return status.ok();
}

std::optional<blockchain::Transaction> PersistentStorage::loadTransaction(
    const std::array<uint8_t, 32>& tx_hash
) {
    std::string key = impl_->makeKey("tx", std::string(tx_hash.begin(), tx_hash.end()));
    std::string tx_data;
    
    rocksdb::Status status = impl_->db->Get(
        rocksdb::ReadOptions(),
        key,
        &tx_data
    );
    
    if (status.ok()) {
        std::vector<uint8_t> data(tx_data.begin(), tx_data.end());
        return blockchain::Transaction::deserialize(data);
    }
    
    return std::nullopt;
}

bool PersistentStorage::storeProof(uint64_t block_number, const rollup::StateTransitionProof& proof) {
    auto serialized = proof.serialize();
    std::string key = impl_->makeKey("proof", std::to_string(block_number));
    
    rocksdb::Status status = impl_->db->Put(
        rocksdb::WriteOptions(),
        key,
        rocksdb::Slice(reinterpret_cast<const char*>(serialized.data()), serialized.size())
    );
    
    return status.ok();
}

std::optional<rollup::StateTransitionProof> PersistentStorage::loadProof(uint64_t block_number) {
    std::string key = impl_->makeKey("proof", std::to_string(block_number));
    std::string proof_data;
    
    rocksdb::Status status = impl_->db->Get(
        rocksdb::ReadOptions(),
        key,
        &proof_data
    );
    
    if (status.ok()) {
        std::vector<uint8_t> data(proof_data.begin(), proof_data.end());
        return rollup::StateTransitionProof::deserialize(data);
    }
    
    return std::nullopt;
}

bool PersistentStorage::storeBlockData(uint64_t block_number, const std::vector<uint8_t>& data) {
    std::string key = impl_->makeKey("block", std::to_string(block_number));
    
    rocksdb::Status status = impl_->db->Put(
        rocksdb::WriteOptions(),
        key,
        rocksdb::Slice(reinterpret_cast<const char*>(data.data()), data.size())
    );
    
    return status.ok();
}

std::optional<std::vector<uint8_t>> PersistentStorage::loadBlockData(uint64_t block_number) {
    std::string key = impl_->makeKey("block", std::to_string(block_number));
    std::string block_data;
    
    rocksdb::Status status = impl_->db->Get(
        rocksdb::ReadOptions(),
        key,
        &block_data
    );
    
    if (status.ok()) {
        return std::vector<uint8_t>(block_data.begin(), block_data.end());
    }
    
    return std::nullopt;
}

} // namespace storage
} // namespace quids 