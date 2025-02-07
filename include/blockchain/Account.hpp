#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <map>
#include "blockchain/Transaction.hpp"

namespace quids {
namespace blockchain {

struct Account {
    std::string address;
    uint64_t balance{0};
    uint64_t nonce{0};
    std::vector<uint8_t> code;
    bool is_contract{false};
    
    Account() = default;
    explicit Account(const std::string& addr) : address(addr) {}
};

} // namespace blockchain
} // namespace quids 