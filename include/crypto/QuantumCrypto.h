#pragma once

#include <oqs/oqs.h>
#include <blake3.h>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace quantum {

class QuantumCrypto {
public:
    // Quantum-safe signature schemes
    enum class QSignatureScheme {
        DILITHIUM2,
        DILITHIUM3,
        DILITHIUM5,
        FALCON512,
        FALCON1024,
        SPHINCS_SHAKE256
    };

    // Constructor with default scheme
    explicit QuantumCrypto(QSignatureScheme scheme = QSignatureScheme::DILITHIUM5);
    ~QuantumCrypto();

    // Key generation
    std::vector<uint8_t> QGenerateKeypair();
    std::vector<uint8_t> QGetPublicKey() const;
    
    // Quantum-safe signatures
    std::vector<uint8_t> QSign(const std::vector<uint8_t>& message);
    bool QVerify(const std::vector<uint8_t>& message, 
                const std::vector<uint8_t>& signature,
                const std::vector<uint8_t>& public_key);

    // Blake3 hashing
    static std::vector<uint8_t> QHash(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> QHashWithKey(const std::vector<uint8_t>& data,
                                           const std::vector<uint8_t>& key);

    // Utility functions
    static std::string schemeToString(QSignatureScheme scheme);
    static size_t getSignatureSize(QSignatureScheme scheme);
    static size_t getPublicKeySize(QSignatureScheme scheme);
    static size_t getPrivateKeySize(QSignatureScheme scheme);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    QSignatureScheme scheme_;
    
    void initializeOQS();
    void cleanupOQS();
};

// Exception class for quantum crypto operations
class QuantumCryptoError : public std::runtime_error {
public:
    explicit QuantumCryptoError(const std::string& message) 
        : std::runtime_error(message) {}
};

} // namespace quantum 