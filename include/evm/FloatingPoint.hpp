#pragma once

#include "evm/uint256.hpp"
#include <limits>
#include <string>
#include <vector>
#include <stdexcept>

namespace evm {

using uint256_t = ::evm::uint256_t;

class Decimal {
public:
    // Represents decimal numbers with 18 decimal places (wei precision)
    static constexpr size_t DECIMALS = 18;
    
    Decimal();
    explicit Decimal(const std::string& value);
    explicit Decimal(uint256_t value);
    
    // Core operations
    Decimal add(const Decimal& other) const;
    Decimal subtract(const Decimal& other) const;
    Decimal multiply(const Decimal& other) const;
    Decimal divide(const Decimal& other) const;
    
    // Comparison
    bool operator==(const Decimal& other) const;
    bool operator<(const Decimal& other) const;
    bool operator>(const Decimal& other) const;
    
    // Conversion
    std::string toString() const;
    uint256_t toInteger() const;
    
    // Rounding modes
    enum class RoundingMode {
        ROUND_DOWN,
        ROUND_UP,
        ROUND_HALF_UP
    };
    
    Decimal round(size_t decimal_places, RoundingMode mode = RoundingMode::ROUND_HALF_UP) const;

private:
    // Internal representation: integer * 10^-DECIMALS
    uint256_t value_;
    
    void normalize();
    static uint256_t pow10(size_t exponent);
};

class FloatingPointError : public std::runtime_error {
public:
    explicit FloatingPointError(const std::string& message) 
        : std::runtime_error(message) {}
};

} // namespace evm 