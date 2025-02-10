#include "evm/uint256.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <array>
#include <iostream>
#include <cstdint>

namespace evm {

namespace {
    uint8_t hex_char_to_value(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        throw std::invalid_argument("Invalid hex character");
    }
} // namespace

uint256_t uint256_t::operator+(const uint256_t& other) const {
    uint256_t result;
    uint64_t carry = 0;
    
    for (size_t i = 0; i < 4; ++i) {
        uint64_t sum = data_[i] + other.data_[i] + carry;
        carry = (sum < data_[i]) || (sum == data_[i] && other.data_[i] > 0);
        result.data_[i] = sum;
    }
    
    if (carry) {
        throw std::overflow_error("uint256_t addition overflow");
    }
    
    return result;
}

uint256_t uint256_t::operator-(const uint256_t& other) const {
    uint256_t result;
    uint64_t borrow = 0;
    
    for (size_t i = 0; i < 4; ++i) {
        uint64_t diff = data_[i] - other.data_[i] - borrow;
        borrow = (diff > data_[i]) || (diff == data_[i] && other.data_[i] > 0);
        result.data_[i] = diff;
    }
    
    if (borrow) {
        throw std::underflow_error("uint256_t subtraction underflow");
    }
    
    return result;
}

uint256_t uint256_t::operator*(const uint256_t& other) const {
    uint256_t result;
    
    for (size_t i = 0; i < 4; ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < 4 - i; ++j) {
            // Split multiplication into 32-bit chunks to avoid overflow
            uint64_t a_low = data_[i] & 0xFFFFFFFF;
            uint64_t a_high = data_[i] >> 32;
            uint64_t b_low = other.data_[j] & 0xFFFFFFFF;
            uint64_t b_high = other.data_[j] >> 32;
            
            // Compute partial products
            uint64_t low = a_low * b_low;
            uint64_t mid1 = a_low * b_high;
            uint64_t mid2 = a_high * b_low;
            uint64_t high = a_high * b_high;
            
            // Combine results
            uint64_t combined_low = low + (mid1 << 32) + (mid2 << 32);
            uint64_t overflow = (combined_low < low) ? 1 : 0;
            uint64_t combined_high = high + (mid1 >> 32) + (mid2 >> 32) + overflow;
            
            // Add to result with carry
            uint64_t sum = combined_low + result.data_[i + j] + carry;
            carry = combined_high + (sum < combined_low ? 1 : 0);
            result.data_[i + j] = sum;
        }
        if (carry) {
            throw std::overflow_error("uint256_t multiplication overflow");
        }
    }
    
    return result;
}

uint256_t uint256_t::operator/(const uint256_t& other) const {
    if (other == uint256_t(0)) {
        throw std::domain_error("Division by zero");
    }
    
    uint256_t quotient;
    uint256_t remainder;
    
    for (int i = 255; i >= 0; --i) {
        remainder = remainder * uint256_t(2);
        remainder.data_[0] |= (data_[i / 64] >> (i % 64)) & 1;
        
        if (remainder >= other) {
            remainder = remainder - other;
            quotient.data_[i / 64] |= uint64_t(1) << (i % 64);
        }
    }
    
    return quotient;
}

bool uint256_t::operator==(const uint256_t& other) const {
    return data_ == other.data_;
}

bool uint256_t::operator<(const uint256_t& other) const {
    for (int i = 3; i >= 0; --i) {
        if (data_[i] < other.data_[i]) return true;
        if (data_[i] > other.data_[i]) return false;
    }
    return false;
}

uint256_t uint256_t::operator|(const uint256_t& other) const {
    uint256_t result;
    for (size_t i = 0; i < 4; ++i) {
        result.data_[i] = data_[i] | other.data_[i];
    }
    return result;
}

uint256_t uint256_t::operator&(const uint256_t& other) const {
    uint256_t result;
    for (size_t i = 0; i < 4; ++i) {
        result.data_[i] = data_[i] & other.data_[i];
    }
    return result;
}

uint256_t uint256_t::operator^(const uint256_t& other) const {
    uint256_t result;
    for (size_t i = 0; i < 4; ++i) {
        result.data_[i] = data_[i] ^ other.data_[i];
    }
    return result;
}

uint256_t& uint256_t::operator|=(const uint256_t& other) {
    for (size_t i = 0; i < 4; ++i) {
        data_[i] |= other.data_[i];
    }
    return *this;
}

uint256_t& uint256_t::operator&=(const uint256_t& other) {
    for (size_t i = 0; i < 4; ++i) {
        data_[i] &= other.data_[i];
    }
    return *this;
}

uint256_t& uint256_t::operator^=(const uint256_t& other) {
    for (size_t i = 0; i < 4; ++i) {
        data_[i] ^= other.data_[i];
    }
    return *this;
}

uint256_t uint256_t::operator<<(int shift) const {
    if (shift <= 0) return *this;
    if (shift >= 256) return uint256_t(0);

    uint256_t result;
    const int word_shift = shift / 64;
    const int bit_shift = shift % 64;

    if (bit_shift == 0) {
        // Word-aligned shift
        for (int i = 3; i >= word_shift; --i) {
            result.data_[i] = data_[i - word_shift];
        }
    } else {
        // Handle bit shift
        const int carry_shift = 64 - bit_shift;
        for (int i = 3; i >= word_shift + 1; --i) {
            result.data_[i] = (data_[i - word_shift] << bit_shift) | 
                             (data_[i - word_shift - 1] >> carry_shift);
        }
        if (word_shift < 4) {
            result.data_[word_shift] = data_[0] << bit_shift;
        }
    }

    return result;
}

uint256_t uint256_t::operator>>(int shift) const {
    uint256_t result;
    if (shift >= 256) {
        return result;  // All zeros
    }
    
    int word_shift = shift / 64;
    int bit_shift = shift % 64;
    
    for (int i = 0; i < 4 - word_shift; ++i) {
        result.data_[i] = data_[i + word_shift] >> bit_shift;
        if (bit_shift > 0 && i + word_shift + 1 < 4) {
            result.data_[i] |= data_[i + word_shift + 1] << (64 - bit_shift);
        }
    }
    
    return result;
}

uint256_t& uint256_t::operator<<=(int shift) {
    *this = *this << shift;
    return *this;
}

uint256_t& uint256_t::operator>>=(int shift) {
    *this = *this >> shift;
    return *this;
}

uint256_t& uint256_t::operator+=(const uint256_t& other) {
    *this = *this + other;
    return *this;
}

uint256_t& uint256_t::operator-=(const uint256_t& other) {
    *this = *this - other;
    return *this;
}

uint256_t& uint256_t::operator*=(const uint256_t& other) {
    *this = *this * other;
    return *this;
}

uint256_t& uint256_t::operator/=(const uint256_t& other) {
    *this = *this / other;
    return *this;
}

uint256_t& uint256_t::operator%=(const uint256_t& other) {
    *this = *this % other;
    return *this;
}

uint256_t uint256_t::operator%(const uint256_t& other) const {
    if (other == uint256_t(0)) {
        throw std::domain_error("Modulo by zero");
    }
    
    uint256_t quotient = *this / other;
    return *this - (quotient * other);
}

std::ostream& operator<<(std::ostream& os, const uint256_t& value) {
    bool started = false;
    for (int i = 3; i >= 0; --i) {
        if (started || value.data_[i] != 0) {
            if (started) {
                os << std::hex << std::setw(16) << std::setfill('0');
            }
            os << value.data_[i];
            started = true;
        }
    }
    if (!started) os << "0";
    return os;
}

uint256_t uint256_t::from_string(const std::string& str) {
    uint256_t result(0);
    for (char c : str) {
        if (!isdigit(c)) {
            throw std::invalid_argument("Invalid decimal character");
        }
        result = result * uint256_t(10) + uint256_t(static_cast<uint64_t>(c - '0'));
    }
    return result;
}

uint256_t uint256_t::from_hex_string(const std::string& hex_str) {
    std::string s = hex_str;
    size_t start = 0;
    
    // Remove 0x prefix
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        start = 2;
    }

    std::array<uint64_t, 4> data{0, 0, 0, 0};
    size_t pos = 0;
    
    // Process from right to left (little-endian)
    for (int i = 0; i < 4; ++i) {
        uint64_t val = 0;
        for (int j = 0; j < 16; ++j) { // 16 hex chars = 64 bits
            if (start + pos >= s.size()) break;
            
            char c = s[s.size() - 1 - pos];
            val |= static_cast<uint64_t>(hex_char_to_value(c)) << (4 * j);
            pos++;
        }
        data[i] = val;
    }

    return uint256_t(data);
}

uint256_t::uint256_t(const std::array<uint64_t, 4>& data) {
    data_ = data;
}

std::string uint256_t::to_string() const {
    return to_string(10);
}

std::string uint256_t::to_string(int base) const {
    if (base < 2 || base > 16) {
        throw std::invalid_argument("Base must be between 2 and 16");
    }
    
    if (*this == 0) return "0";
    
    std::string result;
    uint256_t num = *this;
    const char* digits = "0123456789abcdef";
    
    while (num > 0) {
        auto rem = num % base;
        result += digits[static_cast<uint8_t>(rem.data_[0])];
        num = num / base;
    }
    
    reverse(result.begin(), result.end());
    return result;
}

} // namespace evm 