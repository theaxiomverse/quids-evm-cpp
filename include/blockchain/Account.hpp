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
    Account(const std::string& addr, uint64_t bal) : address(addr), balance(bal) {}
    Account(const std::string& addr, const std::vector<uint8_t>& code_, uint64_t bal = 0)
        : address(addr), balance(bal), code(code_) {}
};

} // namespace blockchain
} // namespace quids 