#pragma once

#include <array>
#include <functional>

namespace evm {

struct Address {
    std::array<uint8_t, 20> bytes;
    
    bool operator==(const Address& other) const;
    bool operator<(const Address& other) const;
};

} // namespace evm

// Add hash function for Address
namespace std {
    template<>
    struct hash<evm::Address> {
        size_t operator()(const evm::Address& addr) const {
            size_t h = 0;
            for (auto b : addr.bytes) {
                h = h * 31 + b;
            }
            return h;
        }
    };
} 