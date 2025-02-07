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

namespace {
    void check_openssl_error() {
        unsigned long err = ERR_get_error();
        if (err) {
            char buf[256];
            ERR_error_string_n(err, buf, sizeof(buf));
            throw std::runtime_error(std::string("OpenSSL error: ") + buf);
        }
    }

    // Initialize OpenSSL providers
    struct OpenSSLInit {
        OpenSSLInit() {
            // Set the OpenSSL configuration file path
            if (!OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr)) {
                throw std::runtime_error("Failed to initialize OpenSSL");
            }

            // Set the modules directory
            if (setenv("OPENSSL_MODULES", "/usr/local/Cellar/openssl@3/3.4.0/lib/ossl-modules", 1) != 0) {
                throw std::runtime_error("Failed to set OPENSSL_MODULES");
            }

            // Set the config file
            if (setenv("OPENSSL_CONF", "/usr/local/etc/openssl/openssl.cnf", 1) != 0) {
                throw std::runtime_error("Failed to set OPENSSL_CONF");
            }

            // Load providers
            auto default_provider = OSSL_PROVIDER_load(nullptr, "default");
            if (!default_provider) {
                throw std::runtime_error("Failed to load OpenSSL default provider");
            }

            auto oqs_provider = OSSL_PROVIDER_load(nullptr, "oqsprovider");
            if (!oqs_provider) {
                OSSL_PROVIDER_unload(default_provider);
                throw std::runtime_error("Failed to load OpenSSL OQS provider");
            }

            if (!OSSL_PROVIDER_available(nullptr, "oqsprovider")) {
                OSSL_PROVIDER_unload(oqs_provider);
                OSSL_PROVIDER_unload(default_provider);
                throw std::runtime_error("OpenSSL OQS provider not available");
            }
        }
        ~OpenSSLInit() {
            EVP_cleanup();
        }
    } openssl_init;
}

namespace quids {
namespace blockchain {

bool Transaction::sign(const std::array<uint8_t, 32>& private_key) {
    try {
        // Create message to sign (hash of transaction data)
        auto hash = compute_hash();
        
        // Create signing context
        EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, 
                                                      private_key.data(), private_key.size());
        if (!pkey) {
            return false;
        }
        
        // Create signature
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            EVP_PKEY_free(pkey);
            return false;
        }
        
        if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) != 1) {
            EVP_PKEY_free(pkey);
            EVP_MD_CTX_free(ctx);
            return false;
        }
        
        size_t sig_len = 64;  // ED25519 signature length
        signature.resize(sig_len);
        
        if (EVP_DigestSign(ctx, signature.data(), &sig_len, hash.data(), hash.size()) != 1) {
            EVP_PKEY_free(pkey);
            EVP_MD_CTX_free(ctx);
            return false;
        }
        
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        return true;
        
    } catch (...) {
        return false;
    }
}

bool Transaction::verify() const {
    try {
        // Convert sender address from hex to bytes for public key
        std::vector<uint8_t> pub_key;
        pub_key.reserve(sender.length() / 2);
        
        for (size_t i = 0; i < sender.length(); i += 2) {
            std::string byte = sender.substr(i, 2);
            pub_key.push_back(std::stoi(byte, nullptr, 16));
        }

        return verify_ed25519_signature(pub_key);
    } catch (const std::exception& e) {
        return false;
    }
}

bool Transaction::verify_ed25519_signature(const std::vector<uint8_t>& public_key) const {
    try {
        // Create verification context using ED25519
        EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                                     public_key.data(), public_key.size());
        if (!pkey) {
            return false;
        }

        // Create verification context
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            EVP_PKEY_free(pkey);
            return false;
        }

        // Initialize verification
        if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) != 1) {
            EVP_PKEY_free(pkey);
            EVP_MD_CTX_free(ctx);
            return false;
        }

        // Calculate transaction hash
        auto hash = compute_hash();

        // Verify the signature
        int ret = EVP_DigestVerify(ctx, signature.data(), signature.size(),
                                  hash.data(), hash.size());

        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);

        return ret == 1;
    } catch (...) {
        return false;
    }
}

std::array<uint8_t, 32> Transaction::compute_hash() const {
    std::array<uint8_t, 32> hash;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        check_openssl_error();
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(ctx);
        check_openssl_error();
    }

    // Hash all transaction fields
    if (!EVP_DigestUpdate(ctx, sender.data(), sender.size())) {
        EVP_MD_CTX_free(ctx);
        check_openssl_error();
    }
    if (!EVP_DigestUpdate(ctx, recipient.data(), recipient.size())) {
        EVP_MD_CTX_free(ctx);
        check_openssl_error();
    }
    if (!EVP_DigestUpdate(ctx, &amount, sizeof(amount))) {
        EVP_MD_CTX_free(ctx);
        check_openssl_error();
    }
    if (!EVP_DigestUpdate(ctx, &nonce, sizeof(nonce))) {
        EVP_MD_CTX_free(ctx);
        check_openssl_error();
    }

    unsigned int hash_len;
    if (!EVP_DigestFinal_ex(ctx, hash.data(), &hash_len)) {
        EVP_MD_CTX_free(ctx);
        check_openssl_error();
    }

    EVP_MD_CTX_free(ctx);
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
    std::vector<uint8_t> data;
    
    // Serialize sender
    data.insert(data.end(), sender.begin(), sender.end());
    data.push_back(0);  // null terminator
    
    // Serialize recipient
    data.insert(data.end(), recipient.begin(), recipient.end());
    data.push_back(0);
    
    // Serialize amount and nonce
    data.insert(data.end(), 
               reinterpret_cast<const uint8_t*>(&amount),
               reinterpret_cast<const uint8_t*>(&amount) + sizeof(amount));
    data.insert(data.end(),
               reinterpret_cast<const uint8_t*>(&nonce),
               reinterpret_cast<const uint8_t*>(&nonce) + sizeof(nonce));
               
    // Serialize signature
    data.insert(data.end(), signature.begin(), signature.end());
    
    return data;
}

std::optional<Transaction> Transaction::deserialize(const std::vector<uint8_t>& data) {
    try {
        size_t pos = 0;
        
        // Deserialize sender
        std::string sender;
        while (pos < data.size() && data[pos] != 0) {
            sender.push_back(data[pos++]);
        }
        if (pos >= data.size()) return std::nullopt;
        pos++;  // Skip null terminator
        
        // Deserialize recipient
        std::string recipient;
        while (pos < data.size() && data[pos] != 0) {
            recipient.push_back(data[pos++]);
        }
        if (pos >= data.size()) return std::nullopt;
        pos++;
        
        // Deserialize amount and nonce
        if (pos + sizeof(uint64_t) * 2 > data.size()) return std::nullopt;
        
        uint64_t amount = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += sizeof(uint64_t);
        
        uint64_t nonce = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += sizeof(uint64_t);
        
        // Create transaction
        Transaction tx(sender, recipient, amount, nonce, 21000, 1);
        
        // Deserialize signature if present
        if (pos < data.size()) {
            tx.signature.assign(data.begin() + pos, data.end());
        }
        
        return tx;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

} // namespace blockchain
} // namespace quids 