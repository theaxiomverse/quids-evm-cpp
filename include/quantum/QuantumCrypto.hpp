#pragma once

#include <vector>
#include <memory>
#include <string>
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumProof.hpp"

namespace quids {
namespace quantum {

// Forward declarations
class QuantumKeyDistribution;

// Post-quantum signature schemes
enum class SignatureScheme {
    FALCON512,
    FALCON1024,
    SPHINCS_SHA2_128F,
    MLDSA44
};

// Quantum key types
struct QuantumKey {
    std::vector<uint8_t> key_material;
    QuantumState entangled_state{1};  // Initialize with 1 qubit
    double security_parameter{0.0};
    size_t effective_length{0};
};

// Quantum signature scheme
struct QuantumSignature {
    std::vector<uint8_t> signature{};
    QuantumProof proof{};
    double verification_score{0.0};
};

// Quantum encryption parameters
struct QuantumEncryptionParams {
    size_t key_size{256};
    size_t num_rounds{100};
    double noise_threshold{0.01};
    bool use_error_correction{true};
    size_t security_parameter{128};
};

// Quantum cryptographic primitives
class QuantumCrypto {
public:
    explicit QuantumCrypto(const QuantumEncryptionParams& params = QuantumEncryptionParams());
    ~QuantumCrypto();

    // Disable copy and move
    QuantumCrypto(const QuantumCrypto&) = delete;
    QuantumCrypto& operator=(const QuantumCrypto&) = delete;
    QuantumCrypto(QuantumCrypto&&) = delete;
    QuantumCrypto& operator=(QuantumCrypto&&) = delete;

    // Post-quantum signature operations
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generateKeypair(SignatureScheme scheme);
    std::vector<uint8_t> sign(const std::vector<uint8_t>& message, const std::vector<uint8_t>& private_key);
    bool verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature, const std::vector<uint8_t>& public_key);

    // Key generation and distribution
    QuantumKey generateQuantumKey(size_t key_length);
    bool distributeKey(const std::string& recipient_id, const QuantumKey& key);
    
    // Encryption/decryption
    std::vector<uint8_t> encryptQuantum(const std::vector<uint8_t>& plaintext,
                                      const QuantumKey& key);
    std::vector<uint8_t> decryptQuantum(const std::vector<uint8_t>& ciphertext,
                                      const QuantumKey& key);
    
    // Quantum signature operations
    QuantumSignature signQuantum(const std::vector<uint8_t>& message,
                               const QuantumKey& signing_key);
    bool verifyQuantumSignature(const std::vector<uint8_t>& message,
                              const QuantumSignature& signature,
                              const QuantumKey& verification_key);
    
    // Security verification
    double measureSecurityLevel(const QuantumKey& key) const;
    bool checkQuantumSecurity(const QuantumState& state) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal helper functions
    QuantumState prepareEncryptionState(const std::vector<uint8_t>& data);
    QuantumMeasurement measureEncryptedState(const QuantumState& state);
    bool validateQuantumParameters(const QuantumEncryptionParams& params) const;
    void updateSecurityMetrics(const QuantumState& state);
    
    // Constants
    static constexpr size_t MIN_KEY_SIZE = 256;
    static constexpr double MIN_SECURITY_THRESHOLD = 0.99;
    static constexpr size_t MAX_ROUNDS = 1000;
};

// Helper functions for quantum cryptographic operations
namespace utils {
    // Key management
    QuantumKey deriveQuantumKey(const QuantumState& state);
    bool validateKeyMaterial(const QuantumKey& key);
    
    // Signature utilities
    QuantumProof generateSignatureProof(const std::vector<uint8_t>& message,
                                      const QuantumKey& key);
    double verifySignatureProof(const QuantumProof& proof,
                              const std::vector<uint8_t>& message);
    
    // Security analysis
    double estimateQuantumSecurity(const QuantumState& state);
    bool detectQuantumTampering(const QuantumMeasurement& measurement);
}

} // namespace quantum
} // namespace quids 