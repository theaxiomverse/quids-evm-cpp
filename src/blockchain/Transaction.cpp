#include "blockchain/Transaction.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/provider.h>
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
            OSSL_PROVIDER_load(nullptr, "default");
            OSSL_PROVIDER_load(nullptr, "oqsprovider");
        }
        ~OpenSSLInit() {
            EVP_cleanup();
        }
    } openssl_init;
}

Transaction::Transaction(
    const std::string& sender,
    const std::string& recipient,
    uint64_t amount,
    uint64_t nonce
) : sender(sender), recipient(recipient), amount(amount), nonce(nonce) {}

bool Transaction::sign(const std::array<uint8_t, 32>& private_key) {
    try {
        // Create the signing context using Dilithium5
        std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> 
            ctx(EVP_PKEY_CTX_new_from_name(nullptr, "dilithium5", nullptr), 
                EVP_PKEY_CTX_free);
        if (!ctx) {
            check_openssl_error();
            return false;
        }

        // Initialize key generation
        if (EVP_PKEY_keygen_init(ctx.get()) <= 0) {
            check_openssl_error();
            return false;
        }

        // Generate the key pair
        EVP_PKEY* pkey = nullptr;
        if (EVP_PKEY_keygen(ctx.get(), &pkey) <= 0) {
            check_openssl_error();
            return false;
        }
        std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> 
            key(pkey, EVP_PKEY_free);

        // Create signing context
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> 
            md_ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
        if (!md_ctx) {
            check_openssl_error();
            return false;
        }

        // Initialize signing operation
        if (EVP_DigestSignInit_ex(md_ctx.get(), nullptr, "SHA3-256",
                                 nullptr, nullptr, key.get(), nullptr) <= 0) {
            check_openssl_error();
            return false;
        }

        // Calculate transaction hash
        auto hash = compute_hash();

        // Sign the hash
        size_t sig_len = 0;
        if (EVP_DigestSign(md_ctx.get(), nullptr, &sig_len,
                          hash.data(), hash.size()) <= 0) {
            check_openssl_error();
            return false;
        }

        signature.resize(sig_len);
        if (EVP_DigestSign(md_ctx.get(), signature.data(), &sig_len,
                          hash.data(), hash.size()) <= 0) {
            check_openssl_error();
            return false;
        }

        return true;
    } catch (const std::exception& e) {
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
    // Create verification context using Dilithium5
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> 
        ctx(EVP_PKEY_CTX_new_from_name(nullptr, "dilithium5", nullptr), 
            EVP_PKEY_CTX_free);
    if (!ctx) {
        std::cerr << "Failed to create key context" << std::endl;
        return false;
    }

    // Initialize verification
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> 
        md_ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!md_ctx) {
        std::cerr << "Failed to create message digest context" << std::endl;
        return false;
    }

    // Initialize verification operation
    if (EVP_DigestVerifyInit_ex(md_ctx.get(), nullptr, "SHA3-256",
                               nullptr, nullptr, ctx.get(), nullptr) <= 0) {
        std::cerr << "Failed to initialize verification" << std::endl;
        return false;
    }

    // Calculate transaction hash
    auto hash = compute_hash();

    // Verify the signature
    int ret = EVP_DigestVerify(md_ctx.get(), signature.data(), signature.size(),
                              hash.data(), hash.size());
    
    if (ret < 0) {
        std::cerr << "Verification error" << std::endl;
        return false;
    }

    return ret == 1;
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

Transaction Transaction::deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    
    // Deserialize sender
    std::string sender;
    while (data[pos] != 0) {
        sender.push_back(data[pos++]);
    }
    pos++;  // Skip null terminator
    
    // Deserialize recipient
    std::string recipient;
    while (data[pos] != 0) {
        recipient.push_back(data[pos++]);
    }
    pos++;
    
    // Deserialize amount and nonce
    uint64_t amount = *reinterpret_cast<const uint64_t*>(&data[pos]);
    pos += sizeof(uint64_t);
    uint64_t nonce = *reinterpret_cast<const uint64_t*>(&data[pos]);
    pos += sizeof(uint64_t);
    
    // Create transaction
    Transaction tx(sender, recipient, amount, nonce);
    
    // Deserialize signature
    std::copy(data.begin() + pos, data.begin() + pos + 64, std::back_inserter(tx.signature));
    
    return tx;
}

bool Transaction::operator==(const Transaction& other) const {
    return sender == other.sender &&
           recipient == other.recipient &&
           amount == other.amount &&
           nonce == other.nonce &&
           signature == other.signature;
}

bool Transaction::operator!=(const Transaction& other) const {
    return !(*this == other);
} 