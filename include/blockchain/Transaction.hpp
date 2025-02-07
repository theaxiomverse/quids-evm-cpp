#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <memory>
#include <chrono>
#include <optional>

namespace quids {
namespace blockchain {

class Transaction {
public:
    struct Signature {
        std::array<uint8_t, 64> r;
        std::array<uint8_t, 64> s;
        uint8_t v;
        
        // Default constructor
        Signature() : r{}, s{}, v(0) {}
        
        // Constructor with values
        Signature(
            const std::array<uint8_t, 64>& r_,
            const std::array<uint8_t, 64>& s_,
            uint8_t v_
        ) : r(r_), s(s_), v(v_) {}
        
        // Comparison operators
        bool operator==(const Signature& other) const {
            return r == other.r && s == other.s && v == other.v;
        }
        
        bool operator!=(const Signature& other) const {
            return !(*this == other);
        }
    };

    // Constructors
    Transaction() : amount(0), nonce(0), gas_limit(0), gas_price(0), timestamp(std::chrono::system_clock::now()) {}

    Transaction(
        std::string sender_,
        std::string recipient_,
        uint64_t amount_,
        uint64_t nonce_,
        uint64_t gas_limit_ = 21000,
        uint64_t gas_price_ = 1
    ) : sender(std::move(sender_)),
        recipient(std::move(recipient_)),
        amount(amount_),
        nonce(nonce_),
        gas_limit(gas_limit_),
        gas_price(gas_price_),
        timestamp(std::chrono::system_clock::now())
    {}

    // Rule of 5
    Transaction(const Transaction&) = default;
    Transaction& operator=(const Transaction&) = default;
    Transaction(Transaction&&) noexcept = default;
    Transaction& operator=(Transaction&&) noexcept = default;
    ~Transaction() = default;

    // Core functionality
    [[nodiscard]] bool sign(const std::array<uint8_t, 32>& private_key);
    [[nodiscard]] bool verify() const;
    [[nodiscard]] std::array<uint8_t, 32> compute_hash() const;
    [[nodiscard]] std::string to_string() const;

    // Serialization
    [[nodiscard]] std::vector<uint8_t> serialize() const;
    [[nodiscard]] static std::optional<Transaction> deserialize(const std::vector<uint8_t>& data);

    // Validation
    [[nodiscard]] bool is_valid() const {
        return !sender.empty() &&
               !recipient.empty() &&
               amount > 0 &&
               gas_limit >= MIN_GAS_LIMIT &&
               gas_price > 0 &&
               signature.size() == SIGNATURE_SIZE;
    }

    // Gas calculations
    [[nodiscard]] uint64_t calculate_gas_cost() const {
        return gas_limit * gas_price;
    }

    [[nodiscard]] uint64_t calculate_total_cost() const {
        return amount + calculate_gas_cost();
    }

    // Comparison operators
    bool operator==(const Transaction& other) const {
        return sender == other.sender &&
               recipient == other.recipient &&
               amount == other.amount &&
               nonce == other.nonce &&
               gas_limit == other.gas_limit &&
               gas_price == other.gas_price &&
               signature == other.signature;
    }

    bool operator!=(const Transaction& other) const {
        return !(*this == other);
    }

    // Public fields
    std::string sender;
    std::string recipient;
    uint64_t amount;
    uint64_t nonce;
    uint64_t gas_limit;
    uint64_t gas_price;
    std::vector<uint8_t> signature;
    std::chrono::system_clock::time_point timestamp;

private:
    // Helper methods for ED25519 signature operations
    [[nodiscard]] bool verify_ed25519_signature(const std::vector<uint8_t>& public_key) const;
    void sign_ed25519(const std::array<uint8_t, 32>& private_key);

    // Constants
    static constexpr size_t SIGNATURE_SIZE = 64;
    static constexpr uint64_t MIN_GAS_LIMIT = 21000;
    static constexpr uint64_t MAX_GAS_LIMIT = 15000000;
    static constexpr size_t MAX_DATA_SIZE = 128 * 1024;  // 128KB
};

} // namespace blockchain
} // namespace quids 