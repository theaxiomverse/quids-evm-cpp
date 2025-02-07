#pragma once

#include <array>
#include <string>
#include <vector>

class Transaction {
public:
    std::string sender;
    std::string recipient;
    uint64_t amount;
    uint64_t nonce;
    std::vector<uint8_t> signature;

    // Compute hash of transaction data
    std::array<uint8_t, 32> compute_hash() const;

    // Sign transaction with private key
    void sign(const std::array<uint8_t, 32>& private_key);

    // Verify transaction signature
    bool verify() const;

    // String representation for debugging
    std::string to_string() const;
}; 