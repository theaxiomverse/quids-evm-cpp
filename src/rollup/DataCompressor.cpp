#include "rollup/DataCompressor.hpp"
#include <zstd.h>
#include <stdexcept>
#include <cstring>

namespace quids {
namespace rollup {

CompressedBatch DataCompressor::compress_batch(std::span<const quids::blockchain::Transaction> transactions) {
    CompressedBatch batch;
    batch.original_size = transactions.size() * sizeof(quids::blockchain::Transaction);

    // Estimate compressed size
    size_t const estimated_compressed_size = ZSTD_compressBound(batch.original_size);
    std::string compressed_data;
    compressed_data.resize(estimated_compressed_size);

    // Compress the transactions
    size_t const compressed_size = ZSTD_compress(
        compressed_data.data(), estimated_compressed_size,
        transactions.data(), batch.original_size,
        1  // compression level
    );

    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error(std::string("ZSTD compression failed: ") + ZSTD_getErrorName(compressed_size));
    }

    // Resize to actual compressed size
    compressed_data.resize(compressed_size);
    batch.compressed_data = std::move(compressed_data);

    return batch;
}

std::vector<quids::blockchain::Transaction> 
DataCompressor::decompress_batch(const CompressedBatch& compressed) {
    // Get decompressed size
    size_t const decompressed_size = ZSTD_getFrameContentSize(
        compressed.compressed_data.data(), 
        compressed.compressed_data.size()
    );

    if (decompressed_size != compressed.original_size) {
        throw std::runtime_error("Decompressed size does not match original size");
    }

    std::vector<quids::blockchain::Transaction> transactions(compressed.original_size / sizeof(quids::blockchain::Transaction));

    // Decompress
    size_t const actual_decompressed_size = ZSTD_decompress(
        transactions.data(), decompressed_size,
        compressed.compressed_data.data(), compressed.compressed_data.size()
    );

    if (ZSTD_isError(actual_decompressed_size)) {
        throw std::runtime_error(std::string("ZSTD decompression failed: ") + ZSTD_getErrorName(actual_decompressed_size));
    }

    if (actual_decompressed_size != decompressed_size) {
        throw std::runtime_error("Actual decompressed size does not match expected size");
    }

    return transactions;
}

std::string DataCompressor::compress_transaction(const quids::blockchain::Transaction& tx) {
    // Estimate compressed size
    size_t const estimated_compressed_size = ZSTD_compressBound(sizeof(quids::blockchain::Transaction));
    std::string compressed_data;
    compressed_data.resize(estimated_compressed_size);

    // Compress the transaction
    size_t const compressed_size = ZSTD_compress(
        compressed_data.data(), estimated_compressed_size,
        &tx, sizeof(quids::blockchain::Transaction),
        1  // compression level
    );

    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error(std::string("ZSTD compression failed: ") + ZSTD_getErrorName(compressed_size));
    }

    // Resize to actual compressed size
    compressed_data.resize(compressed_size);
    return compressed_data;
}

quids::blockchain::Transaction DataCompressor::decompress_transaction(const std::string& compressed_tx) {
    // Get decompressed size
    size_t const decompressed_size = ZSTD_getFrameContentSize(
        compressed_tx.data(), 
        compressed_tx.size()
    );

    if (decompressed_size != sizeof(quids::blockchain::Transaction)) {
        throw std::runtime_error("Decompressed size does not match Transaction size");
    }

    quids::blockchain::Transaction tx;

    // Decompress
    size_t const actual_decompressed_size = ZSTD_decompress(
        &tx, sizeof(quids::blockchain::Transaction),
        compressed_tx.data(), compressed_tx.size()
    );

    if (ZSTD_isError(actual_decompressed_size)) {
        throw std::runtime_error(std::string("ZSTD decompression failed: ") + ZSTD_getErrorName(actual_decompressed_size));
    }

    if (actual_decompressed_size != sizeof(quids::blockchain::Transaction)) {
        throw std::runtime_error("Actual decompressed size does not match Transaction size");
    }

    return tx;
}

} // namespace rollup
} // namespace quids 