#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <array>

class Transaction {
public:
    struct Signature {
        std::array<uint8_t, 64> r;
        std::array<uint8_t, 64> s;
        uint8_t v;
    };

    Transaction() : amount(0), nonce(0) {}

    Transaction(
        const std::string& sender,
        const std::string& recipient,
        uint64_t amount,
        uint64_t nonce
    );

    // Core functionality
    bool sign(const std::array<uint8_t, 32>& private_key);
    bool verify() const;
    std::array<uint8_t, 32> compute_hash() const;
    std::string to_string() const;

    // Serialization
    std::vector<uint8_t> serialize() const;
    static Transaction deserialize(const std::vector<uint8_t>& data);

    // Comparison operators
    bool operator==(const Transaction& other) const;
    bool operator!=(const Transaction& other) const;

    // Public fields
    std::string sender;
    std::string recipient;
    uint64_t amount;
    uint64_t nonce;
    std::vector<uint8_t> signature;

private:
    // Helper methods for ED25519 signature operations
    bool verify_ed25519_signature(const std::vector<uint8_t>& public_key) const;
    void sign_ed25519(const std::array<uint8_t, 32>& private_key);
}; 