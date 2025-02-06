#include <gtest/gtest.h>
#include "quantum/QuantumCrypto.h"
#include <vector>
#include <chrono>
#include <random>
#include <thread>

class QuantumCryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
        qcrypto_ = std::make_unique<quantum::QuantumCrypto>();
    }

    std::vector<uint8_t> generateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }

    void benchmarkOperation(const std::string& name, size_t iterations, std::function<void()> operation) {
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            operation();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_time = static_cast<double>(duration.count()) / iterations;
        std::cout << name << " average time: " << avg_time << " microseconds" << std::endl;
    }

    std::unique_ptr<quantum::QuantumCrypto> qcrypto_;
};

TEST_F(QuantumCryptoTest, DilithiumSignatureTest) {
    auto message = generateRandomData(1024);
    
    // Generate keypair
    auto [public_key, private_key] = qcrypto_->QGenerateKeypair(
        quantum::QSignatureScheme::DILITHIUM5);
    
    // Sign message
    auto signature = qcrypto_->QSign(message, private_key);
    
    // Verify signature
    EXPECT_TRUE(qcrypto_->QVerify(message, signature, public_key));
    
    // Verify fails with modified message
    message[0] ^= 1;
    EXPECT_FALSE(qcrypto_->QVerify(message, signature, public_key));
}

TEST_F(QuantumCryptoTest, FalconSignatureTest) {
    auto message = generateRandomData(1024);
    
    auto [public_key, private_key] = qcrypto_->QGenerateKeypair(
        quantum::QSignatureScheme::FALCON512);
    
    auto signature = qcrypto_->QSign(message, private_key);
    EXPECT_TRUE(qcrypto_->QVerify(message, signature, public_key));
    
    // Test signature malleability
    signature[0] ^= 1;
    EXPECT_FALSE(qcrypto_->QVerify(message, signature, public_key));
}

TEST_F(QuantumCryptoTest, SphincsSignatureTest) {
    auto message = generateRandomData(1024);
    
    auto [public_key, private_key] = qcrypto_->QGenerateKeypair(
        quantum::QSignatureScheme::SPHINCS_BLAKE3);
    
    auto signature = qcrypto_->QSign(message, private_key);
    EXPECT_TRUE(qcrypto_->QVerify(message, signature, public_key));
}

TEST_F(QuantumCryptoTest, Blake3HashTest) {
    auto data = generateRandomData(1024);
    auto hash1 = qcrypto_->QHash(data);
    
    // Test collision resistance
    data[0] ^= 1;
    auto hash2 = qcrypto_->QHash(data);
    EXPECT_NE(hash1, hash2);
    
    // Test deterministic output
    EXPECT_EQ(qcrypto_->QHash(data), qcrypto_->QHash(data));
}

TEST_F(QuantumCryptoTest, QuantumResistantHashTest) {
    auto data = generateRandomData(2048);
    auto hash = qcrypto_->QHashQuantumResistant(data);
    
    // Verify output size (512 bits = 64 bytes)
    EXPECT_EQ(hash.size(), 64);
}

TEST_F(QuantumCryptoTest, KyberKeyExchangeTest) {
    // Generate keypair for party A
    auto [public_key_a, private_key_a] = qcrypto_->QGenerateKyberKeypair();
    
    // Party B encapsulates shared secret
    auto [shared_secret_b, ciphertext] = qcrypto_->QEncapsulateKyber(public_key_a);
    
    // Party A decapsulates shared secret
    auto shared_secret_a = qcrypto_->QDecapsulateKyber(ciphertext, private_key_a);
    
    // Verify shared secrets match
    EXPECT_EQ(shared_secret_a, shared_secret_b);
}

TEST_F(QuantumCryptoTest, PerformanceBenchmarks) {
    const size_t ITERATIONS = 100;
    auto message = generateRandomData(1024);
    
    // Dilithium benchmarks
    auto keypair_d = qcrypto_->QGenerateKeypair(quantum::QSignatureScheme::DILITHIUM5);
    auto pk_d = std::get<0>(keypair_d);
    auto sk_d = std::get<1>(keypair_d);
    
    std::vector<uint8_t> sig_d;
    EXPECT_NO_THROW(sig_d = qcrypto_->QSign(message, sk_d));
    EXPECT_NO_THROW(qcrypto_->QVerify(message, sig_d, pk_d));

    benchmarkOperation("Dilithium Sign", ITERATIONS,
        [&]() { qcrypto_->QSign(message, sk_d); });

    benchmarkOperation("Dilithium Verify", ITERATIONS,
        [&]() { qcrypto_->QVerify(message, sig_d, pk_d); });

    // Test Falcon
    auto keypair_f = qcrypto_->QGenerateKeypair(quantum::QSignatureScheme::FALCON512);
    auto pk_f = std::get<0>(keypair_f);
    auto sk_f = std::get<1>(keypair_f);
    
    std::vector<uint8_t> sig_f;
    EXPECT_NO_THROW(sig_f = qcrypto_->QSign(message, sk_f));
    EXPECT_NO_THROW(qcrypto_->QVerify(message, sig_f, pk_f));

    benchmarkOperation("Falcon Sign", ITERATIONS,
        [&]() { qcrypto_->QSign(message, sk_f); });

    benchmarkOperation("Falcon Verify", ITERATIONS,
        [&]() { qcrypto_->QVerify(message, sig_f, pk_f); });

    // Test SPHINCS+
    auto keypair_s = qcrypto_->QGenerateKeypair(quantum::QSignatureScheme::SPHINCS_BLAKE3);
    auto pk_s = std::get<0>(keypair_s);
    auto sk_s = std::get<1>(keypair_s);
    
    std::vector<uint8_t> sig_s;
    EXPECT_NO_THROW(sig_s = qcrypto_->QSign(message, sk_s));
    EXPECT_NO_THROW(qcrypto_->QVerify(message, sig_s, pk_s));

    benchmarkOperation("SPHINCS+ Sign", ITERATIONS,
        [&]() { qcrypto_->QSign(message, sk_s); });

    benchmarkOperation("SPHINCS+ Verify", ITERATIONS,
        [&]() { qcrypto_->QVerify(message, sig_s, pk_s); });

    // Test Kyber
    auto keypair_k = qcrypto_->QGenerateKyberKeypair();
    auto pk_k = std::get<0>(keypair_k);
    auto sk_k = std::get<1>(keypair_k);

    benchmarkOperation("Kyber Encapsulation", ITERATIONS,
        [&]() { qcrypto_->QEncapsulateKyber(pk_k); });

    auto encap_result = qcrypto_->QEncapsulateKyber(pk_k);
    auto shared_secret = std::get<0>(encap_result);
    auto ct = std::get<1>(encap_result);

    benchmarkOperation("Kyber Decapsulation", ITERATIONS,
        [&]() { qcrypto_->QDecapsulateKyber(ct, sk_k); });
}

TEST_F(QuantumCryptoTest, MemoryLeakTest) {
    const size_t ITERATIONS = 1000;
    auto message = generateRandomData(1024);
    
    for (size_t i = 0; i < ITERATIONS; ++i) {
        auto [pk, sk] = qcrypto_->QGenerateKeypair(quantum::QSignatureScheme::DILITHIUM5);
        auto sig = qcrypto_->QSign(message, sk);
        qcrypto_->QVerify(message, sig, pk);
    }
}

TEST_F(QuantumCryptoTest, ThreadSafetyTest) {
    const size_t THREAD_COUNT = 4;
    const size_t ITERATIONS = 100;
    auto message = generateRandomData(1024);
    
    std::vector<std::thread> threads;
    std::atomic<bool> failed{false};
    
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&]() {
            for (size_t j = 0; j < ITERATIONS && !failed; ++j) {
                try {
                    auto [pk, sk] = qcrypto_->QGenerateKeypair(
                        quantum::QSignatureScheme::DILITHIUM5);
                    auto sig = qcrypto_->QSign(message, sk);
                    if (!qcrypto_->QVerify(message, sig, pk)) {
                        failed = true;
                        break;
                    }
                } catch (...) {
                    failed = true;
                    break;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_FALSE(failed);
} 