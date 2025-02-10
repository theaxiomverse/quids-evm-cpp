#include "evm/FloatingPoint.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace evm {

Decimal::Decimal() : value_(0) {}

Decimal::Decimal(const std::string& value) {
    // Parse string with high precision
    std::stringstream ss(value);
    std::string integer_part, decimal_part;
    
    std::getline(ss, integer_part, '.');
    std::getline(ss, decimal_part);
    
    // Handle integer part
    value_ = std::stoull(integer_part);
    value_ *= pow10(DECIMALS);
    
    // Handle decimal part
    if (!decimal_part.empty()) {
        if (decimal_part.length() > DECIMALS) {
            decimal_part = decimal_part.substr(0, DECIMALS);
        } else {
            decimal_part.append(DECIMALS - decimal_part.length(), '0');
        }
        value_ += std::stoull(decimal_part);
    }
    
    normalize();
}

Decimal::Decimal(uint256_t value) : value_(value) {
    normalize();
}

Decimal Decimal::add(const Decimal& other) const {
    if (value_ > std::numeric_limits<uint256_t>::max() - other.value_) {
        throw FloatingPointError("Addition overflow");
    }
    return Decimal(value_ + other.value_);
}

Decimal Decimal::subtract(const Decimal& other) const {
    if (value_ < other.value_) {
        throw FloatingPointError("Subtraction underflow");
    }
    return Decimal(value_ - other.value_);
}

Decimal Decimal::multiply(const Decimal& other) const {
    // Check for overflow before multiplication
    if (value_ != 0 && other.value_ > std::numeric_limits<uint256_t>::max() / value_) {
        throw FloatingPointError("Multiplication overflow");
    }
    
    uint256_t result = value_ * other.value_;
    result /= pow10(DECIMALS);  // Adjust decimal places
    return Decimal(result);
}

Decimal Decimal::divide(const Decimal& other) const {
    if (other.value_ == 0) {
        throw FloatingPointError("Division by zero");
    }
    
    // Scale up for precision
    uint256_t scaled = value_ * pow10(DECIMALS);
    return Decimal(scaled / other.value_);
}

bool Decimal::operator==(const Decimal& other) const {
    return value_ == other.value_;
}

bool Decimal::operator<(const Decimal& other) const {
    return value_ < other.value_;
}

bool Decimal::operator>(const Decimal& other) const {
    return value_ > other.value_;
}

std::string Decimal::toString() const {
    std::stringstream ss;
    uint256_t integer_part = value_ / pow10(DECIMALS);
    uint256_t decimal_part = value_ % pow10(DECIMALS);
    
    ss << integer_part << ".";
    ss << std::setfill('0') << std::setw(DECIMALS) << decimal_part;
    
    // Remove trailing zeros
    std::string result = ss.str();
    while (result.back() == '0' && result.find('.') != std::string::npos) {
        result.pop_back();
    }
    if (result.back() == '.') {
        result.pop_back();
    }
    return result;
}

uint256_t Decimal::toInteger() const {
    return value_ / pow10(DECIMALS);
}

Decimal Decimal::round(size_t decimal_places, RoundingMode mode) const {
    if (decimal_places >= DECIMALS) {
        return *this;
    }
    
    uint256_t scale = pow10(DECIMALS - decimal_places);
    uint256_t scaled_value = value_ / scale;
    
    switch (mode) {
        case RoundingMode::ROUND_DOWN:
            return Decimal(scaled_value * scale);
            
        case RoundingMode::ROUND_UP: {
            if (value_ % scale != 0) {
                scaled_value += 1;
            }
            return Decimal(scaled_value * scale);
        }
        
        case RoundingMode::ROUND_HALF_UP: {
            uint256_t remainder = value_ % scale;
            if (remainder >= scale / 2) {
                scaled_value += 1;
            }
            return Decimal(scaled_value * scale);
        }
    }
    
    throw FloatingPointError("Invalid rounding mode");
}

void Decimal::normalize() {
    // Ensure value doesn't exceed maximum precision
    value_ %= pow10(DECIMALS * 2);
}

uint256_t Decimal::pow10(size_t exponent) {
    static const uint256_t powers[] = {
        uint256_t(1),
        uint256_t(10),
        uint256_t(100),
        uint256_t(1000),
        uint256_t(10000),
        uint256_t(100000),
        uint256_t(1000000),
        uint256_t(10000000),
        uint256_t(100000000),
        uint256_t(1000000000)
    };

    if (exponent < sizeof(powers) / sizeof(powers[0])) {
        return powers[exponent];
    }

    uint256_t result = uint256_t(1);
    for (size_t i = 0; i < exponent; ++i) {
        result *= uint256_t(10);
    }
    return result;
}


} // namespace evm 