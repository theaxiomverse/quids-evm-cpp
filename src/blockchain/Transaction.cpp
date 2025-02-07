#include "blockchain/Transaction.hpp"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/core.h>
#include <openssl/core_names.h>
#include <openssl/provider.h>
#include <openssl/params.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <openssl/sha.h>
#include <blake3.h>
#include <spdlog/spdlog.h>

namespace {
    // Thread-safe OpenSSL error checking
    std::string get_openssl_error() {
        std::string error_msg;
        unsigned long err;
        while ((err = ERR_get_error()) != 0) {
            char buf[256];
            ERR_error_string_n(err, buf, sizeof(buf));
            if (!error_msg.empty()) {
                error_msg += "; ";
            }
            error_msg += buf;
        }
        return error_msg;
    }

    // RAII wrapper for OpenSSL initialization
    class OpenSSLGuard {
    public:
        OpenSSLGuard() {
            if (!OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG | 
                                   OPENSSL_INIT_ADD_ALL_CIPHERS | 
                                   OPENSSL_INIT_ADD_ALL_DIGESTS, nullptr)) {
                throw std::runtime_error("Failed to initialize OpenSSL: " + get_openssl_error());
            }

            default_provider_ = OSSL_PROVIDER_load(nullptr, "default");
            if (!default_provider_) {
                throw std::runtime_error("Failed to load OpenSSL default provider: " + get_openssl_error());
            }
        }

        ~OpenSSLGuard() {
            if (default_provider_) {
                OSSL_PROVIDER_unload(default_provider_);
            }
            OPENSSL_cleanup();
        }

    private:
        OSSL_PROVIDER* default_provider_{nullptr};
    };

    // Static initialization
    static const OpenSSLGuard openssl_guard;
}

namespace quids {
namespace blockchain {

bool Transaction::sign(const std::array<uint8_t, 32>& private_key) {
    // Create message to sign (hash of transaction data with context)
    auto hash = compute_hash();

    // Sign using ED25519
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519,
        nullptr,
        private_key.data(),
        private_key.size()
    );
    
    if (!pkey) {
        return false;
    }

    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        return false;
    }

    if (EVP_DigestSignInit(md_ctx, nullptr, nullptr, nullptr, pkey) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(md_ctx);
        return false;
    }

    size_t sig_len = SIGNATURE_SIZE;
    signature.resize(sig_len);
    
    if (EVP_DigestSign(md_ctx, signature.data(), &sig_len, 
                       hash.data(), hash.size()) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(md_ctx);
        return false;
    }

    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(md_ctx);
    return true;
}

bool Transaction::verify() const {
    try {
        // Basic transaction validation
        if (from.empty() || to.empty()) {
            return false;
        }

        // Verify signature if present
        if (!signature.empty()) {
            // TODO: Implement signature verification
        }

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Transaction verification failed: {}", e.what());
        return false;
    }
}

std::array<uint8_t, 32> Transaction::compute_hash() const {
    std::array<uint8_t, 32> hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    // Add domain separation context
    const char* CONTEXT = "QUIDS_TRANSACTION_V1";
    blake3_hasher_update(&hasher, CONTEXT, strlen(CONTEXT));

    // Add fields with domain separation
    const uint8_t DOMAIN_SENDER = 0x01;
    blake3_hasher_update(&hasher, &DOMAIN_SENDER, 1);
    blake3_hasher_update(&hasher, sender.data(), sender.size());

    const uint8_t DOMAIN_RECIPIENT = 0x02;
    blake3_hasher_update(&hasher, &DOMAIN_RECIPIENT, 1);
    blake3_hasher_update(&hasher, recipient.data(), recipient.size());

    const uint8_t DOMAIN_AMOUNT = 0x03;
    blake3_hasher_update(&hasher, &DOMAIN_AMOUNT, 1);
    blake3_hasher_update(&hasher, &amount, sizeof(amount));

    const uint8_t DOMAIN_NONCE = 0x04;
    blake3_hasher_update(&hasher, &DOMAIN_NONCE, 1);
    blake3_hasher_update(&hasher, &nonce, sizeof(nonce));

    const uint8_t DOMAIN_GAS_LIMIT = 0x05;
    blake3_hasher_update(&hasher, &DOMAIN_GAS_LIMIT, 1);
    blake3_hasher_update(&hasher, &gas_limit, sizeof(gas_limit));

    const uint8_t DOMAIN_GAS_PRICE = 0x06;
    blake3_hasher_update(&hasher, &DOMAIN_GAS_PRICE, 1);
    blake3_hasher_update(&hasher, &gas_price, sizeof(gas_price));

    const uint8_t DOMAIN_TIMESTAMP = 0x07;
    auto ts = timestamp.time_since_epoch().count();
    blake3_hasher_update(&hasher, &DOMAIN_TIMESTAMP, 1);
    blake3_hasher_update(&hasher, &ts, sizeof(ts));

    blake3_hasher_finalize(&hasher, hash.data(), hash.size());
    return hash;
}

std::string Transaction::to_string() const {
    std::stringstream ss;
    ss << "Transaction{sender=" << sender
       << ", recipient=" << recipient
       << ", amount=" << amount
       << ", nonce=" << nonce
       << ", signature_size=" << signature.size()
       << "}";
    return ss.str();
}

std::vector<uint8_t> Transaction::serialize() const {
    std::vector<uint8_t> result;
    
    // Serialize sender
    result.insert(result.end(), sender.begin(), sender.end());
    result.push_back(0); // null terminator
    
    // Serialize recipient
    result.insert(result.end(), recipient.begin(), recipient.end());
    result.push_back(0);
    
    // Serialize amount and nonce
    result.insert(result.end(), 
        reinterpret_cast<const uint8_t*>(&amount),
        reinterpret_cast<const uint8_t*>(&amount) + sizeof(amount));
    result.insert(result.end(),
        reinterpret_cast<const uint8_t*>(&nonce),
        reinterpret_cast<const uint8_t*>(&nonce) + sizeof(nonce));
    
    return result;
}

std::optional<Transaction> Transaction::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint64_t) * 2) {
        return std::nullopt;
    }

    Transaction tx;
    size_t pos = 0;

    // Deserialize sender
    while (pos < data.size() && data[pos] != 0) {
        tx.sender.push_back(data[pos++]);
    }
    if (pos >= data.size()) return std::nullopt;
    pos++; // Skip null terminator

    // Deserialize recipient
    while (pos < data.size() && data[pos] != 0) {
        tx.recipient.push_back(data[pos++]);
    }
    if (pos >= data.size()) return std::nullopt;
    pos++;

    // Deserialize amount and nonce
    if (pos + sizeof(uint64_t) * 2 <= data.size()) {
        std::memcpy(&tx.amount, &data[pos], sizeof(uint64_t));
        pos += sizeof(uint64_t);
        std::memcpy(&tx.nonce, &data[pos], sizeof(uint64_t));
        return tx;
    }

    return std::nullopt;
}

} // namespace blockchain
} // namespace quids 