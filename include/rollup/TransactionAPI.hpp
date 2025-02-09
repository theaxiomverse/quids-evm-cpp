#pragma once

#include "blockchain/Transaction.hpp"
#include <vector>

namespace quids {
namespace rollup {

class TransactionAPI {
public:
    virtual ~TransactionAPI() = default;
    virtual bool submitTransaction(const blockchain::Transaction& tx) = 0;
    virtual bool submitBatch(const std::vector<blockchain::Transaction>& batch) = 0;
};

} // namespace rollup
} // namespace quids 