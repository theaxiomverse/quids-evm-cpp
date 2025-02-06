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

private:
    std::array<uint64_t, 4> data_;  // Little-endian representation
};

} // namespace evm 