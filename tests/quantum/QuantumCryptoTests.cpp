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
    // Generate a keypair using FALCON-512
    auto [private_key, public_key] = qcrypto_->generateKeypair(SignatureScheme::FALCON512);
    ASSERT_FALSE(private_key.empty());
    ASSERT_FALSE(public_key.empty());

    // Test message signing
    std::vector<uint8_t> message = {'t', 'e', 's', 't'};
    auto signature = qcrypto_->sign(message, private_key);
    ASSERT_FALSE(signature.empty());

    // Verify the signature
    bool is_valid = qcrypto_->verify(message, signature, public_key);
    ASSERT_TRUE(is_valid);

    // Test with modified message
    message[0] = 'x';
    is_valid = qcrypto_->verify(message, signature, public_key);
    ASSERT_FALSE(is_valid);
}

TEST_F(QuantumCryptoTest, QuantumSignatureTest) {
    std::vector<uint8_t> message = {1, 2, 3, 4, 5};
    
    // Generate signing and verification keys
    auto signing_key = qcrypto_->generateQuantumKey(256);
    auto verification_key = signing_key;  // In a real system, this would be derived differently
    
    // Sign the message
    auto signature = qcrypto_->signQuantum(message, signing_key);
    EXPECT_FALSE(signature.signature.empty());
    EXPECT_GT(signature.verification_score, 0.9);
    
    // Verify the signature with both message and verification key
    bool result = qcrypto_->verifyQuantumSignature(message, signature, verification_key);
    EXPECT_TRUE(result);
}

TEST_F(QuantumCryptoTest, SecurityLevelTest) {
    auto key = qcrypto_->generateQuantumKey(256);
    double security = qcrypto_->measureSecurityLevel(key);
    EXPECT_GT(security, 0.9);
    
    bool is_secure = qcrypto_->checkQuantumSecurity(key.entangled_state);
    EXPECT_TRUE(is_secure);
}

} // namespace test
} // namespace quantum
} // namespace quids 