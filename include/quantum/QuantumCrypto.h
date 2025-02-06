#pragma once

#include <vector>
#include <string>
#include <array>
#include <memory>
#include <tuple>
#include "quantum/QuantumState.h"

namespace quantum {

// Forward declarations
class QuantumState;

enum class QSignatureScheme {
    DILITHIUM5,
    FALCON512,
    SPHINCS_BLAKE3
};

class QuantumCrypto {
public:
    // Constructor
    QuantumCrypto();
    ~QuantumCrypto() = default;

    // Key generation
    std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> QGenerateKeypair(QSignatureScheme scheme);
    std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> QGenerateKyberKeypair();

    // Signature operations
    std::vector<uint8_t> QSign(const std::vector<uint8_t>& message,
                              const std::vector<uint8_t>& privateKey);

    bool QVerify(const std::vector<uint8_t>& message,
                const std::vector<uint8_t>& signature,
                const std::vector<uint8_t>& publicKey);

    // Hash functions
    std::vector<uint8_t> QHash(const std::vector<uint8_t>& data);
    std::vector<uint8_t> QHashQuantumResistant(const std::vector<uint8_t>& data);

    // Key exchange
    std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> QEncapsulateKyber(
        const std::vector<uint8_t>& publicKey);

    std::vector<uint8_t> QDecapsulateKyber(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& privateKey);

private:
    std::unique_ptr<QuantumState> state_;
};

} // namespace quantum 