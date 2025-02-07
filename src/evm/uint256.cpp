#include "evm/uint256.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace evm {

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
        if (data_[i] != other.data_[i]) {
            return data_[i] < other.data_[i];
        }
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
    uint256_t result;
    if (shift >= 256) {
        return result;  // All zeros
    }
    
    int word_shift = shift / 64;
    int bit_shift = shift % 64;
    
    for (int i = 3; i >= word_shift; --i) {
        result.data_[i] = data_[i - word_shift] << bit_shift;
        if (bit_shift > 0 && i > word_shift) {
            result.data_[i] |= data_[i - word_shift - 1] >> (64 - bit_shift);
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
    std::stringstream ss;
    bool started = false;
    
    // Print in hex format
    ss << "0x";
    for (int i = 3; i >= 0; --i) {
        if (value.data()[i] != 0 || started) {
            if (!started) {
                ss << std::hex << value.data()[i];
                started = true;
            } else {
                ss << std::hex << std::setw(16) << std::setfill('0') << value.data()[i];
            }
        }
    }
    
    if (!started) {
        ss << "0";
    }
    
    os << ss.str();
    return os;
}

} // namespace evm 