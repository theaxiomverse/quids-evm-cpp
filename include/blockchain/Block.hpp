#pragma once
#include "blockchain/Transaction.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace quids {
namespace blockchain {

struct Block {
    uint64_t number{0};
    std::string hash;
    std::string prev_hash;
    uint64_t timestamp{0};
    std::vector<Transaction> transactions;
    
    bool verify() const;
};

} // namespace blockchain
} // namespace quids 