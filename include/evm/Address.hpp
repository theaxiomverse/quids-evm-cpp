#pragma once

#include <array>
#include <functional>
#include <cstdint>


namespace evm {

struct Address {
    std::array<uint8_t, 20> bytes;
    
    // Constructors
    Address() = default;
    ~Address() = default;
    
    // Comparison operators
    bool operator==(const Address& other) const;
    bool operator<(const Address& other) const;
    bool operator!=(const Address& other) const { return !(*this == other); }
    bool operator>(const Address& other) const { return other < *this; }
    bool operator<=(const Address& other) const { return !(other < *this); }
    bool operator>=(const Address& other) const { return !(*this < other); }
};

} // namespace evm


// Add hash function for Address
namespace std {
    template<>
    struct hash<evm::Address> {
        size_t operator()(const evm::Address& addr) const noexcept {
            size_t h = 0;
            for (auto b : addr.bytes) {
                h = h * 31 + b;
            }
            return h;
        }
    };
} 