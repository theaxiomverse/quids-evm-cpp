#include "quantum/QuantumCrypto.hpp"
#include "quantum/QuantumState.hpp"
#include "quantum/OpenSSLPaths.hpp"
#include <stdexcept>
#include <cmath>
#include <random>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/provider.h>
#include <openssl/core_names.h>
#include <openssl/rand.h>
#include <cstring>

namespace {
    void check_openssl_error() {
        unsigned long err = ERR_get_error();
        if (err) {
            char buf[256];
            ERR_error_string_n(err, buf, sizeof(buf));
            throw std::runtime_error(std::string("OpenSSL error: ") + buf);
        }
    }

    // Define algorithm IDs
    constexpr int EVP_PKEY_DILITHIUM5 = 0x0420;
    constexpr int EVP_PKEY_FALCON512 = 0x0430;
    constexpr int EVP_PKEY_SPHINCS_BLAKE3 = 0x0440;

    struct OpenSSLInit {
        OSSL_PROVIDER *default_provider{nullptr};
        OSSL_PROVIDER *oqs_provider{nullptr};
        OSSL_LIB_CTX *lib_ctx{nullptr};

        OpenSSLInit() {
            // Create a new library context
            lib_ctx = OSSL_LIB_CTX_new();
            if (!lib_ctx) {
                throw std::runtime_error("Failed to create OpenSSL library context");
            }

            // Set the modules directory and config file before loading providers
            if (setenv("OPENSSL_MODULES", quids::quantum::openssl::MODULES_DIR, 1) != 0) {
                OSSL_LIB_CTX_free(lib_ctx);
                throw std::runtime_error("Failed to set OPENSSL_MODULES");
            }

            if (setenv("OPENSSL_CONF", quids::quantum::openssl::CONF_FILE, 1) != 0) {
                OSSL_LIB_CTX_free(lib_ctx);
                throw std::runtime_error("Failed to set OPENSSL_CONF");
            }

            // Load default provider
            default_provider = OSSL_PROVIDER_load(lib_ctx, "default");
            if (!default_provider) {
                OSSL_LIB_CTX_free(lib_ctx);
                throw std::runtime_error("Failed to load OpenSSL default provider");
            }

            // Load the OQS provider
            oqs_provider = OSSL_PROVIDER_load(lib_ctx, "oqsprovider");
            if (!oqs_provider) {
                OSSL_PROVIDER_unload(default_provider);
                OSSL_LIB_CTX_free(lib_ctx);
                throw std::runtime_error("Failed to load OpenSSL OQS provider");
            }

            // Verify OQS provider is available
            if (!OSSL_PROVIDER_available(lib_ctx, "oqsprovider")) {
                OSSL_PROVIDER_unload(oqs_provider);
                OSSL_PROVIDER_unload(default_provider);
                OSSL_LIB_CTX_free(lib_ctx);
                throw std::runtime_error("OpenSSL OQS provider not available");
            }

            // Initialize OpenSSL algorithms
            if (!OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG | OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, nullptr)) {
                OSSL_PROVIDER_unload(oqs_provider);
                OSSL_PROVIDER_unload(default_provider);
                OSSL_LIB_CTX_free(lib_ctx);
                throw std::runtime_error("Failed to initialize OpenSSL");
            }
        }

        ~OpenSSLInit() {
            if (oqs_provider) OSSL_PROVIDER_unload(oqs_provider);
            if (default_provider) OSSL_PROVIDER_unload(default_provider);
            if (lib_ctx) OSSL_LIB_CTX_free(lib_ctx);
            OPENSSL_cleanup();
        }

        OSSL_LIB_CTX* get_lib_ctx() const { return lib_ctx; }
    };

    static OpenSSLInit openssl_init;
}

namespace quids {
namespace quantum {

// Define algorithm identifiers for post-quantum schemes
namespace {
    // These values are defined by OpenSSL's OQS provider
    constexpr int EVP_PKEY_DILITHIUM5 = 0x0420;
    constexpr int EVP_PKEY_FALCON512 = 0x0430;
    constexpr int EVP_PKEY_SPHINCS_BLAKE3 = 0x0440;
}

class QuantumCrypto::Impl {
public:
    explicit Impl(const QuantumEncryptionParams& params)
        : params_(params)
        , current_state_(StateVector::Zero(1)) {  // Initialize with 1 qubit
        if (!validateParameters(params)) {
            throw std::invalid_argument("Invalid quantum encryption parameters");
        }
    }

    ~Impl() {
        if (oqs_provider) OSSL_PROVIDER_unload(oqs_provider);
        if (default_provider) OSSL_PROVIDER_unload(default_provider);
        OPENSSL_cleanup();
    }

    bool validateParameters(const QuantumEncryptionParams& params) const {
        return params.key_size >= MIN_KEY_SIZE &&
               params.security_parameter > 0;
    }

    QuantumEncryptionParams params_;
    StateVector current_state_;
    OSSL_PROVIDER* default_provider{nullptr};
    OSSL_PROVIDER* oqs_provider{nullptr};
};

QuantumCrypto::QuantumCrypto(const QuantumEncryptionParams& params)
    : impl_(std::make_unique<Impl>(params)) {}

QuantumCrypto::~QuantumCrypto() = default;

QuantumKey QuantumCrypto::generateQuantumKey(size_t key_length) {
    if (key_length < MIN_KEY_SIZE) {
        throw std::invalid_argument("Key length must be at least 256 bits");
    }

    QuantumKey key;
    key.key_material.resize(key_length / 8);
    key.security_parameter = impl_->params_.security_parameter;
    key.entangled_state = StateVector::Zero(key_length);
    key.effective_length = key_length;
    
    // Generate quantum-resistant key material
    if (RAND_bytes(key.key_material.data(), key.key_material.size()) != 1) {
        throw std::runtime_error("Failed to generate secure random bytes");
    }

    // Create entangled state
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(0.0, 1.0);
    
    for (Eigen::Index i = 0; i < key.entangled_state.size(); ++i) {
        key.entangled_state(i) = std::complex<double>(dist(gen), dist(gen));
    }
    
    // Normalize the state
    key.entangled_state.normalize();

    return key;
}

bool QuantumCrypto::distributeKey(const std::string& recipient_id, const QuantumKey& key) {
    if (key.key_material.empty() || recipient_id.empty()) {
        return false;
    }

    // TODO: Implement quantum key distribution protocol
    return true;
}

std::vector<uint8_t> QuantumCrypto::encryptQuantum(
    const std::vector<uint8_t>& plaintext,
    const QuantumKey& key) {
    
    if (plaintext.empty() || key.key_material.empty()) {
        throw std::invalid_argument("Invalid input for quantum encryption");
    }

    std::vector<uint8_t> ciphertext;
    ciphertext.reserve(plaintext.size());

    // XOR encryption with quantum key
    for (size_t i = 0; i < plaintext.size(); ++i) {
        ciphertext.push_back(plaintext[i] ^ key.key_material[i % key.key_material.size()]);
    }

    return ciphertext;
}

std::vector<uint8_t> QuantumCrypto::decryptQuantum(
    const std::vector<uint8_t>& ciphertext,
    const QuantumKey& key) {
    
    if (ciphertext.empty() || key.key_material.empty()) {
        throw std::invalid_argument("Invalid input for quantum decryption");
    }

    std::vector<uint8_t> plaintext;
    plaintext.reserve(ciphertext.size());

    // XOR decryption with quantum key
    for (size_t i = 0; i < ciphertext.size(); ++i) {
        plaintext.push_back(ciphertext[i] ^ key.key_material[i % key.key_material.size()]);
    }

    return plaintext;
}

QuantumSignature QuantumCrypto::signQuantum(const std::vector<uint8_t>& message,
                                          const QuantumKey& signing_key) {
    QuantumSignature sig;
    sig.signature = sign(message, signing_key.key_material);
    sig.verification_score = 0.95;  // Simulated quantum verification score
    sig.proof = utils::generateSignatureProof(message, signing_key);
    return sig;
}

bool QuantumCrypto::verifyQuantumSignature(const std::vector<uint8_t>& message,
                                         const QuantumSignature& signature,
                                         const QuantumKey& verification_key) {
    if (!verify(message, signature.signature, verification_key.key_material)) {
        return false;
    }
    double proof_score = utils::verifySignatureProof(signature.proof, message);
    return proof_score >= impl_->params_.noise_threshold;
}

double QuantumCrypto::measureSecurityLevel(const QuantumKey& key) const {
    return utils::estimateQuantumSecurity(key.entangled_state);
}

bool QuantumCrypto::checkQuantumSecurity(const QuantumState& state) const {
    double security_level = utils::estimateQuantumSecurity(state);
    return security_level >= MIN_SECURITY_THRESHOLD;
}

QuantumState QuantumCrypto::prepareEncryptionState([[maybe_unused]] const std::vector<uint8_t>& data) {
    // TODO: Implement quantum state preparation
    return QuantumState(1); // Placeholder
}

QuantumMeasurement QuantumCrypto::measureEncryptedState([[maybe_unused]] const QuantumState& state) {
    // TODO: Implement quantum measurement
    return QuantumMeasurement(); // Placeholder
}

bool QuantumCrypto::validateQuantumParameters(const QuantumEncryptionParams& params) const {
    return impl_->validateParameters(params);
}

void QuantumCrypto::updateSecurityMetrics(const QuantumState& state) {
    impl_->current_state_ = state;
}

namespace utils {

QuantumKey deriveQuantumKey([[maybe_unused]] const QuantumState& state) {
    // TODO: Implement quantum key derivation
    return QuantumKey();
}

bool validateKeyMaterial(const QuantumKey& key) {
    return !key.key_material.empty() && key.security_parameter > 0.0;
}

QuantumProof generateSignatureProof(const std::vector<uint8_t>& message,
                                  const QuantumKey& key) {
    // Placeholder implementation
    return QuantumProof{};
}

double verifySignatureProof(const QuantumProof& proof,
                          const std::vector<uint8_t>& message) {
    // Placeholder implementation
    return 0.95;
}

double estimateQuantumSecurity(const QuantumState& state) {
    // Placeholder implementation
    return 0.95;
}

bool detectQuantumTampering(const QuantumMeasurement& measurement) {
    // Placeholder implementation
    return false;
}

} // namespace utils

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> 
QuantumCrypto::generateKeypair(SignatureScheme scheme) {
    const char* alg_name = nullptr;
    switch (scheme) {
        case SignatureScheme::FALCON512:
            alg_name = "falcon512";
            break;
        case SignatureScheme::FALCON1024:
            alg_name = "falcon1024";
            break;
        case SignatureScheme::SPHINCS_SHA2_128F:
            alg_name = "sphincssha2128fsimple";
            break;
        case SignatureScheme::MLDSA44:
            alg_name = "mldsa44";
            break;
        default:
            throw std::invalid_argument("Unsupported signature scheme");
    }

    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(openssl_init.get_lib_ctx(), alg_name, nullptr);
    if (!ctx) {
        throw std::runtime_error("Failed to create key context");
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize key generation");
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to generate key pair");
    }

    // Get the public key
    unsigned char* pubkey_buf = nullptr;
    size_t pubkey_len = 0;
    if (EVP_PKEY_get_raw_public_key(pkey, nullptr, &pubkey_len) <= 0) {
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to get public key length");
    }

    pubkey_buf = static_cast<unsigned char*>(OPENSSL_malloc(pubkey_len));
    if (!pubkey_buf) {
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to allocate memory for public key");
    }

    if (EVP_PKEY_get_raw_public_key(pkey, pubkey_buf, &pubkey_len) <= 0) {
        OPENSSL_free(pubkey_buf);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to get public key");
    }

    // Get the private key
    unsigned char* privkey_buf = nullptr;
    size_t privkey_len = 0;
    if (EVP_PKEY_get_raw_private_key(pkey, nullptr, &privkey_len) <= 0) {
        OPENSSL_free(pubkey_buf);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to get private key length");
    }

    privkey_buf = static_cast<unsigned char*>(OPENSSL_malloc(privkey_len));
    if (!privkey_buf) {
        OPENSSL_free(pubkey_buf);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to allocate memory for private key");
    }

    if (EVP_PKEY_get_raw_private_key(pkey, privkey_buf, &privkey_len) <= 0) {
        OPENSSL_free(privkey_buf);
        OPENSSL_free(pubkey_buf);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to get private key");
    }

    // Convert to vectors and clean up
    std::vector<uint8_t> public_key(pubkey_buf, pubkey_buf + pubkey_len);
    std::vector<uint8_t> private_key(privkey_buf, privkey_buf + privkey_len);

    OPENSSL_free(privkey_buf);
    OPENSSL_free(pubkey_buf);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);

    return {std::move(private_key), std::move(public_key)};
}

std::vector<uint8_t> QuantumCrypto::sign(const std::vector<uint8_t>& message,
                                        const std::vector<uint8_t>& private_key) {
    if (message.empty() || private_key.empty()) {
        throw std::invalid_argument("Invalid input for signing");
    }

    // Create a new private key object
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(openssl_init.get_lib_ctx(), "falcon512", nullptr);
    if (!ctx) {
        throw std::runtime_error("Failed to create key context");
    }

    try {
        if (EVP_PKEY_fromdata_init(ctx) <= 0) {
            throw std::runtime_error("Failed to initialize fromdata");
        }

        OSSL_PARAM params[] = {
            OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PRIV_KEY,
                                            const_cast<unsigned char*>(private_key.data()),
                                            private_key.size()),
            OSSL_PARAM_construct_end()
        };

        if (EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_PRIVATE_KEY, params) <= 0) {
            throw std::runtime_error("Failed to load private key data");
        }

        EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
        if (!md_ctx) {
            throw std::runtime_error("Failed to create message digest context");
        }

        if (EVP_DigestSignInit_ex(md_ctx, nullptr, "falcon512", openssl_init.get_lib_ctx(), nullptr, pkey, nullptr) <= 0) {
            EVP_MD_CTX_free(md_ctx);
            throw std::runtime_error("Failed to initialize signing");
        }

        size_t sig_len = 0;
        if (EVP_DigestSign(md_ctx, nullptr, &sig_len, message.data(), message.size()) <= 0) {
            EVP_MD_CTX_free(md_ctx);
            throw std::runtime_error("Failed to determine signature length");
        }

        std::vector<uint8_t> signature(sig_len);
        if (EVP_DigestSign(md_ctx, signature.data(), &sig_len, message.data(), message.size()) <= 0) {
            EVP_MD_CTX_free(md_ctx);
            throw std::runtime_error("Failed to create signature");
        }
        signature.resize(sig_len);

        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return signature;
    } catch (...) {
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        throw;
    }
}

bool QuantumCrypto::verify(const std::vector<uint8_t>& message,
                          const std::vector<uint8_t>& signature,
                          const std::vector<uint8_t>& public_key) {
    if (message.empty() || signature.empty() || public_key.empty()) {
        return false;
    }

    // Create a new public key object
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(openssl_init.get_lib_ctx(), "falcon512", nullptr);
    if (!ctx) {
        return false;
    }

    try {
        if (EVP_PKEY_fromdata_init(ctx) <= 0) {
            throw std::runtime_error("Failed to initialize fromdata");
        }

        OSSL_PARAM params[] = {
            OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PUB_KEY,
                                            const_cast<unsigned char*>(public_key.data()),
                                            public_key.size()),
            OSSL_PARAM_construct_end()
        };

        if (EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
            throw std::runtime_error("Failed to load public key data");
        }

        EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
        if (!md_ctx) {
            throw std::runtime_error("Failed to create message digest context");
        }

        if (EVP_DigestVerifyInit_ex(md_ctx, nullptr, "falcon512", openssl_init.get_lib_ctx(), nullptr, pkey, nullptr) <= 0) {
            EVP_MD_CTX_free(md_ctx);
            throw std::runtime_error("Failed to initialize verification");
        }

        int ret = EVP_DigestVerify(md_ctx, signature.data(), signature.size(),
                                  message.data(), message.size());

        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return ret == 1;
    } catch (...) {
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
}

} // namespace quantum
} // namespace quids 