#pragma once
#include <string>
#include <vector>
#include <array>
#include <concepts>
#include <span>
#include "blockchain/Transaction.h"

class DataCompressor {
public:
    struct CompressedBatch {
        std::vector<uint8_t> compressed_data;
        size_t original_size;
        std::array<uint8_t, 32> hash;

        // C++20 default comparison operators
        auto operator<=>(const CompressedBatch&) const = default;
    };

    // Compress a batch of transactions into a compressed batch
    static CompressedBatch compress_batch(std::span<const Transaction> transactions);
    
    // Decompress a compressed batch back into a vector of transactions
    static std::vector<Transaction> decompress_batch(const CompressedBatch& compressed);
    
private:
    // Helper methods for compression/decompression
    static std::string compress_transaction(const Transaction& tx);
    static Transaction decompress_transaction(const std::string& compressed_tx);

    // C++20 constexpr constants
    static constexpr size_t COMPRESSION_LEVEL = 1;
    static constexpr size_t HASH_SIZE = 32;
}; 