#include <string>
#include <zstd.h>  // This should now be found in the system include path
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <ranges>
#include "rollup/DataCompressor.h"

namespace {
    // Helper function to check OpenSSL errors
    void check_openssl_error() {
        unsigned long err = ERR_get_error();
        if (err) {
            char buf[256];
            ERR_error_string_n(err, buf, sizeof(buf));
            throw std::runtime_error(std::string("OpenSSL error: ") + buf);
        }
    }

    // Helper function to compute SHA256 hash
    std::array<uint8_t, 32> compute_sha256(const uint8_t* data, size_t size) {
        std::array<uint8_t, 32> hash;
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            check_openssl_error();
        }

        if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
            EVP_MD_CTX_free(ctx);
            check_openssl_error();
        }

        if (!EVP_DigestUpdate(ctx, data, size)) {
            EVP_MD_CTX_free(ctx);
            check_openssl_error();
        }

        unsigned int hash_len;
        if (!EVP_DigestFinal_ex(ctx, hash.data(), &hash_len)) {
            EVP_MD_CTX_free(ctx);
            check_openssl_error();
        }

        EVP_MD_CTX_free(ctx);
        return hash;
    }
}

DataCompressor::CompressedBatch 
DataCompressor::compress_batch(std::span<const Transaction> transactions) {
    if (transactions.empty()) {
        throw std::invalid_argument("Cannot compress empty transaction batch");
    }

    // Estimate bound for compressed size
    size_t const data_size = transactions.size_bytes();
    size_t compressed_size = ZSTD_compressBound(data_size);
    std::vector<uint8_t> compressed_data(compressed_size);
    
    // Compress the transactions
    size_t actual_size = ZSTD_compress(
        compressed_data.data(), compressed_size,
        transactions.data(), data_size,
        COMPRESSION_LEVEL
    );
    
    if (ZSTD_isError(actual_size)) {
        throw std::runtime_error(std::string("Compression error: ") + ZSTD_getErrorName(actual_size));
    }
    
    compressed_data.resize(actual_size);
    
    // Generate compression hash using the helper function
    auto hash = compute_sha256(compressed_data.data(), compressed_data.size());
    
    return CompressedBatch{
        .compressed_data = std::move(compressed_data),
        .original_size = data_size,
        .hash = hash
    };
}

std::vector<Transaction> 
DataCompressor::decompress_batch(const CompressedBatch& compressed) {
    if (compressed.original_size == 0 || compressed.compressed_data.empty()) {
        throw std::invalid_argument("Invalid compressed batch");
    }

    // Verify hash
    auto computed_hash = compute_sha256(compressed.compressed_data.data(), compressed.compressed_data.size());
    if (computed_hash != compressed.hash) {
        throw std::runtime_error("Hash verification failed");
    }

    // Decompress data
    std::vector<Transaction> transactions(compressed.original_size / sizeof(Transaction));
    
    size_t const decompressed_size = ZSTD_decompress(
        transactions.data(), compressed.original_size,
        compressed.compressed_data.data(), compressed.compressed_data.size()
    );
    
    if (ZSTD_isError(decompressed_size)) {
        throw std::runtime_error(std::string("Decompression error: ") + ZSTD_getErrorName(decompressed_size));
    }
    
    if (decompressed_size != compressed.original_size) {
        throw std::runtime_error("Decompressed size does not match original size");
    }
    
    return transactions;
}

std::string DataCompressor::compress_transaction(const Transaction& tx) {
    // Compress a single transaction
    size_t const estimated_compressed_size = ZSTD_compressBound(sizeof(Transaction));
    std::string compressed_data(estimated_compressed_size, '\0');
    
    size_t const compressed_size = ZSTD_compress(
        compressed_data.data(), estimated_compressed_size,
        &tx, sizeof(Transaction),
        COMPRESSION_LEVEL
    );
    
    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error(std::string("Compression error: ") + ZSTD_getErrorName(compressed_size));
    }
    
    compressed_data.resize(compressed_size);
    return compressed_data;
}

Transaction DataCompressor::decompress_transaction(const std::string& compressed_tx) {
    if (compressed_tx.empty()) {
        throw std::invalid_argument("Empty compressed transaction");
    }

    // Get the original size
    if (ZSTD_getFrameContentSize(compressed_tx.data(), compressed_tx.size()) != sizeof(Transaction)) {
        throw std::runtime_error("Invalid compressed transaction size");
    }
    
    Transaction tx;
    size_t const decompressed_size = ZSTD_decompress(
        &tx, sizeof(Transaction),
        compressed_tx.data(), compressed_tx.size()
    );
    
    if (ZSTD_isError(decompressed_size)) {
        throw std::runtime_error(std::string("Decompression error: ") + ZSTD_getErrorName(decompressed_size));
    }
    
    if (decompressed_size != sizeof(Transaction)) {
        throw std::runtime_error("Decompressed size does not match transaction size");
    }
    
    return tx;
} 