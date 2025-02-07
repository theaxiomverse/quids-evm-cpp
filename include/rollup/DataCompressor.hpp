#pragma once

#include <span>
#include <string>
#include <vector>
#include "blockchain/Transaction.hpp"

namespace quids {
namespace rollup {

struct CompressedBatch {
    std::string compressed_data;
    size_t original_size;
};

class DataCompressor {
public:
    static CompressedBatch compress_batch(std::span<const quids::blockchain::Transaction> transactions);

    static std::vector<quids::blockchain::Transaction> decompress_batch(const CompressedBatch& compressed);

private:
    static std::string compress_transaction(const quids::blockchain::Transaction& tx);
    static quids::blockchain::Transaction decompress_transaction(const std::string& compressed_tx);
};

} // namespace rollup
} // namespace quids 