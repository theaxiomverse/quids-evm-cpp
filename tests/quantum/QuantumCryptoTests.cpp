#include <gtest/gtest.h>
#include "quantum/QuantumCrypto.hpp"
#include <vector>
#include <random>

namespace quids {
namespace quantum {
namespace test {

class QuantumCryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
        QuantumEncryptionParams params;
        params.key_size = 3072;
        params.num_rounds = 10;
        params.noise_threshold = 0.1;
        params.security_parameter = 256;
        params.use_error_correction = true;  // Add this for better security
        qcrypto_ = std::make_unique<QuantumCrypto>(params);
    }

    void TearDown() override {
        qcrypto_.reset();
    }

    std::unique_ptr<QuantumCrypto> qcrypto_;
};

TEST_F(QuantumCryptoTest, BasicEncryptionTest) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    auto key = qcrypto_->generateQuantumKey(256);
    
    EXPECT_FALSE(key.key_material.empty());
    EXPECT_GT(key.security_parameter, 0.0);
    
    auto encrypted = qcrypto_->encryptQuantum(data, key);
    EXPECT_FALSE(encrypted.empty());
    EXPECT_NE(encrypted, data);
    
    auto decrypted = qcrypto_->decryptQuantum(encrypted, key);
    EXPECT_EQ(decrypted, data);
}

TEST_F(QuantumCryptoTest, ClassicalSignatureTest) {
    std::vector<uint8_t> message = {'t', 'e', 's', 't'};
    auto key = qcrypto_->generateQuantumKey(32);
    
    // Sign message
    auto signature = qcrypto_->signQuantum(message, key);
    
    // Modify message
    message[0] = 'T';  // Change first character
    
    // Verify should fail for modified message
    bool is_valid = qcrypto_->verifyQuantumSignature(message, signature, key);
    EXPECT_FALSE(is_valid) << "Signature verification should fail for modified message";
}

TEST_F(QuantumCryptoTest, QuantumSignatureTest) {
    // Generate a keypair first for the quantum signature
    auto [public_key, private_key] = qcrypto_->generateKeypair(SignatureScheme::FALCON512);
    
    // Generate quantum key with sufficient size
    auto signing_key = qcrypto_->generateQuantumKey(2048);  // Increased size
    signing_key.key_material = private_key;  // Use the Falcon private key
    
    auto verification_key = qcrypto_->generateQuantumKey(2048);
    verification_key.key_material = public_key;  // Use the Falcon public key
    
    std::vector<uint8_t> message(32, 0);
    std::generate(message.begin(), message.end(), std::rand);
    
    // Sign the message
    auto signature = qcrypto_->signQuantum(message, signing_key);
    EXPECT_FALSE(signature.signature.empty());
    EXPECT_GT(signature.verification_score, 0.9);
    
    // Verify the signature
    bool result = qcrypto_->verifyQuantumSignature(message, signature, verification_key);
    EXPECT_TRUE(result);
}

TEST_F(QuantumCryptoTest, SecurityLevelTest) {
    // Generate quantum key with sufficient size
    auto key = qcrypto_->generateQuantumKey(2048);
    
    // Generate proper Falcon keys for the quantum key
    auto [public_key, private_key] = qcrypto_->generateKeypair(SignatureScheme::FALCON512);
    key.key_material = private_key;  // Use Falcon private key for better security measurement
    
    // Prepare a maximally entangled state
    Eigen::VectorXcd state_vector = Eigen::VectorXcd::Zero(8);
    const std::complex<double> amplitude(1.0 / std::sqrt(8.0), 0.0);
    for (Eigen::Index i = 0; i < state_vector.size(); ++i) {
        state_vector(i) = amplitude;
    }
    key.entangled_state = QuantumState(state_vector);
    
    double security = qcrypto_->measureSecurityLevel(key);
    EXPECT_GT(security, 0.9);
    
    bool is_secure = qcrypto_->checkQuantumSecurity(key.entangled_state);
    EXPECT_TRUE(is_secure);
}

TEST_F(QuantumCryptoTest, TestKeyGeneration) {
    QuantumKey key;
    key.entangled_state = QuantumState(8);  // Initialize with 8 qubits
    
    // Create equal superposition state
    const std::complex<double> amplitude(1.0 / std::sqrt(8.0), 0.0);
    Eigen::VectorXcd state_vector = Eigen::VectorXcd::Constant(8, amplitude);
    key.entangled_state = QuantumState(state_vector);
    
    // Test the state
    EXPECT_EQ(key.entangled_state.size(), 8);
}

} // namespace test
} // namespace quantum
} // namespace quids 