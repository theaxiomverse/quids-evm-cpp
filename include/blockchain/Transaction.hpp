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
    // Basic transaction data
    std::string from;
    std::string to;
    uint64_t value{0};
    uint64_t gas_price{0};
    uint64_t gas_limit{21000};
    std::vector<uint8_t> data;
    std::vector<uint8_t> signature;
    uint64_t nonce{0};
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::array<uint8_t, 32> hash;

    // Constants
    static constexpr size_t SIGNATURE_SIZE = 64;
    static constexpr uint64_t MIN_GAS_LIMIT = 21000;
    static constexpr uint64_t MAX_GAS_LIMIT = 15000000;
    static constexpr size_t MAX_DATA_SIZE = 128 * 1024;  // 128KB

    // Core functionality
    bool sign(const std::array<uint8_t, 32>& private_key);
    bool verify() const;
    std::array<uint8_t, 32> compute_hash() const;
    std::string to_string() const;
    std::vector<uint8_t> serialize() const;
    static std::optional<Transaction> deserialize(const std::vector<uint8_t>& data);

    // Constructors and rule of 5
    Transaction() = default;
    Transaction(const std::string& from_, const std::string& to_, uint64_t value_) 
        : from(from_), to(to_), value(value_) {}
    Transaction(const Transaction&) = default;
    Transaction& operator=(const Transaction&) = default;
    Transaction(Transaction&&) noexcept = default;
    Transaction& operator=(Transaction&&) noexcept = default;
    ~Transaction() = default;

    // Validation
    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] uint64_t calculate_gas_cost() const;
    [[nodiscard]] uint64_t calculate_total_cost() const;

    // Getters
    const std::string& getSender() const { return from; }
    const std::string& getRecipient() const { return to; }
    uint64_t getAmount() const { return value; }
    uint64_t getNonce() const { return nonce; }
    uint64_t getGasPrice() const { return gas_price; }
    uint64_t getGasLimit() const { return gas_limit; }
    const std::vector<uint8_t>& getData() const { return data; }
    const std::vector<uint8_t>& getSignature() const { return signature; }

    // Setters
    void setSender(const std::string& s) { from = s; }
    void setRecipient(const std::string& r) { to = r; }
    void setAmount(uint64_t a) { value = a; }
    void setNonce(uint64_t n) { nonce = n; }
    void setGasPrice(uint64_t gp) { gas_price = gp; }
    void setGasLimit(uint64_t gl) { gas_limit = gl; }
    void setData(const std::vector<uint8_t>& d) { data = d; }
    void setSignature(const std::vector<uint8_t>& sig) { signature = sig; }

    // For sorting in mempool
    bool operator<(const Transaction& other) const {
        return value > other.value; // Higher amount = higher priority
    }

    // Comparison operators
    bool operator==(const Transaction& other) const {
        return from == other.from &&
               to == other.to &&
               value == other.value &&
               nonce == other.nonce &&
               gas_limit == other.gas_limit &&
               gas_price == other.gas_price &&
               signature == other.signature;
    }

    bool operator!=(const Transaction& other) const {
        return !(*this == other);
    }

private:
    [[nodiscard]] bool verify_ed25519_signature(const std::vector<uint8_t>& public_key) const;
    void sign_ed25519(const std::array<uint8_t, 32>& private_key);
};

} // namespace blockchain
} // namespace quids 