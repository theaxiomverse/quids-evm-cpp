#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <ostream>

namespace evm {

class uint256_t {
public:
    uint256_t() : data_{0, 0, 0, 0} {}
    
    // Integer constructors - remove explicit to allow implicit conversion
    uint256_t(uint64_t value) : data_{value, 0, 0, 0} {}
    uint256_t(int value) : data_{static_cast<uint64_t>(value), 0, 0, 0} {}
    uint256_t(unsigned int value) : data_{static_cast<uint64_t>(value), 0, 0, 0} {}
    
    // Assignment operators
    uint256_t& operator=(uint64_t value) {
        data_[0] = value;
        data_[1] = 0;
        data_[2] = 0;
        data_[3] = 0;
        return *this;
    }
    
    // Arithmetic operators
    uint256_t operator+(const uint256_t& other) const;
    uint256_t operator-(const uint256_t& other) const;
    uint256_t operator*(const uint256_t& other) const;
    uint256_t operator/(const uint256_t& other) const;
    uint256_t operator%(const uint256_t& other) const;
    uint256_t operator<<(int shift) const;
    uint256_t operator>>(int shift) const;
    uint256_t operator|(const uint256_t& other) const;
    uint256_t operator&(const uint256_t& other) const;
    uint256_t operator^(const uint256_t& other) const;

    // Compound assignment operators
    uint256_t& operator+=(const uint256_t& other);
    uint256_t& operator-=(const uint256_t& other);
    uint256_t& operator*=(const uint256_t& other);
    uint256_t& operator/=(const uint256_t& other);
    uint256_t& operator%=(const uint256_t& other);
    uint256_t& operator<<=(int shift);
    uint256_t& operator>>=(int shift);
    uint256_t& operator|=(const uint256_t& other);
    uint256_t& operator&=(const uint256_t& other);
    uint256_t& operator^=(const uint256_t& other);

    // Comparison operators
    bool operator==(const uint256_t& other) const;
    bool operator!=(const uint256_t& other) const { return !(*this == other); }
    bool operator<(const uint256_t& other) const;
    bool operator>(const uint256_t& other) const { return other < *this; }
    bool operator<=(const uint256_t& other) const { return !(other < *this); }
    bool operator>=(const uint256_t& other) const { return !(*this < other); }
    
    // Integer comparison operators
    bool operator==(uint64_t value) const { return data_[0] == value && data_[1] == 0 && data_[2] == 0 && data_[3] == 0; }
    bool operator!=(uint64_t value) const { return !(*this == value); }
    bool operator<(uint64_t value) const { return data_[1] == 0 && data_[2] == 0 && data_[3] == 0 && data_[0] < value; }
    bool operator>(uint64_t value) const { return data_[1] > 0 || data_[2] > 0 || data_[3] > 0 || data_[0] > value; }
    bool operator<=(uint64_t value) const { return !(*this > value); }
    bool operator>=(uint64_t value) const { return !(*this < value); }

    // Stream operator for output
    friend std::ostream& operator<<(std::ostream& os, const uint256_t& value);

    // Data access
    const std::array<uint64_t, 4>& data() const { return data_; }

    // Hash function for std::unordered_map
    size_t hash() const {
        size_t h = 0;
        for (const auto& word : data_) {
            h ^= std::hash<uint64_t>{}(word) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    static uint256_t from_string(const std::string& str);
    static uint256_t from_hex_string(const std::string& hex_str);

    // Add this public constructor declaration
    explicit uint256_t(const std::array<uint64_t, 4>& data);

    std::string to_string() const;
    std::string to_string(int base) const;

private:
    std::array<uint64_t, 4> data_;  // Little-endian representation
};

} // namespace evm 

namespace std {
template<>
struct hash<evm::uint256_t> {
    size_t operator()(const evm::uint256_t& value) const {
        return value.hash();
    }
};
} 