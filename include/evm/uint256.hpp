#pragma once

#include <ios>
#include <iomanip>
#include <iostream>
#include <string>
#include <cstdint>
#include <boost/multiprecision/cpp_int.hpp>
#include <string>

namespace evm {

using uint256_t = boost::multiprecision::uint256_t;

class Uint256 {
private:
    uint256_t value_;

public:
    Uint256() : value_(0) {}
    Uint256(uint64_t value) : value_(value) {}
    Uint256(const std::string& value) : value_(value) {}
    Uint256(const uint256_t& value) : value_(value) {} // ✅ Explicit conversion constructor

    // ✅ Define explicit conversion operator
    operator uint256_t() const { return value_; }

    Uint256 operator+(const Uint256& other) const { return Uint256(value_ + other.value_); }
    Uint256 operator-(const Uint256& other) const { return Uint256(value_ - other.value_); }
    Uint256 operator*(const Uint256& other) const { return Uint256(value_ * other.value_); }
    Uint256 operator/(const Uint256& other) const { return Uint256(value_ / other.value_); } // ✅ Fix implicit conversion

    bool operator==(const Uint256& other) const { return value_ == other.value_; }
    bool operator<(const Uint256& other) const { return value_ < other.value_; }
    bool operator>(const Uint256& other) const { return value_ > other.value_; }

    std::string to_string() const { return value_.str(); }
};

} // namespace evm
