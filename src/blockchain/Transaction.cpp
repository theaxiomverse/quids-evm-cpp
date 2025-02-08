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
        char err_buf[256];
        unsigned long err = ERR_get_error();
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        return err_buf;
    }

    // RAII wrapper for OpenSSL initialization
    class OpenSSLGuard {
    public:
        OpenSSLGuard() {
            OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr);
            legacy_provider_ = OSSL_PROVIDER_load(nullptr, "legacy");
            default_provider_ = OSSL_PROVIDER_load(nullptr, "default");
        }

        ~OpenSSLGuard() {
            if (legacy_provider_) OSSL_PROVIDER_unload(legacy_provider_);
            if (default_provider_) OSSL_PROVIDER_unload(default_provider_);
            OPENSSL_cleanup();
        }

    private:
        OSSL_PROVIDER* legacy_provider_;
        OSSL_PROVIDER* default_provider_;
    };

    // Static initialization
    static const OpenSSLGuard openssl_guard;
}

namespace quids {
namespace blockchain {

bool Transaction::sign(const std::array<uint8_t, 32>& private_key) {
    static OpenSSLGuard openssl_guard;
    
    EVP_PKEY* pkey = nullptr;
    EVP_MD_CTX* md_ctx = nullptr;
    bool success = false;

    try {
        // Create key
        pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, 
            private_key.data(), private_key.size());
        if (!pkey) {
            throw std::runtime_error("Failed to create private key: " + get_openssl_error());
        }

        // Create signing context
        md_ctx = EVP_MD_CTX_new();
        if (!md_ctx) {
            throw std::runtime_error("Failed to create signing context: " + get_openssl_error());
        }

        // Initialize signing operation
        if (EVP_DigestSignInit(md_ctx, nullptr, nullptr, nullptr, pkey) != 1) {
            throw std::runtime_error("Failed to initialize signing: " + get_openssl_error());
        }

        // Calculate message hash
        auto hash = compute_hash();

        // Sign the hash
        size_t sig_len = 64; // ED25519 signatures are always 64 bytes
        signature.resize(sig_len);
        
        if (EVP_DigestSign(md_ctx, signature.data(), &sig_len, 
            hash.data(), hash.size()) != 1) {
            throw std::runtime_error("Failed to sign: " + get_openssl_error());
        }

        success = true;
    }
    catch (const std::exception& e) {
        // Log error
        success = false;
    }

    // Cleanup
    if (pkey) EVP_PKEY_free(pkey);
    if (md_ctx) EVP_MD_CTX_free(md_ctx);

    return success;
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
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    // Hash sender
    const auto& sender = getSender();
    blake3_hasher_update(&hasher, sender.data(), sender.size());

    // Hash recipient
    const auto& recipient = getRecipient();
    blake3_hasher_update(&hasher, recipient.data(), recipient.size());

    // Hash amount
    const auto amount = getAmount();
    blake3_hasher_update(&hasher, &amount, sizeof(amount));

    // Hash nonce
    const auto nonce = getNonce();
    blake3_hasher_update(&hasher, &nonce, sizeof(nonce));

    std::array<uint8_t, 32> hash;
    blake3_hasher_finalize(&hasher, hash.data(), hash.size());
    return hash;
}

std::string Transaction::to_string() const {
    std::stringstream ss;
    ss << "Transaction{"
       << "from: " << getSender()
       << ", to: " << getRecipient()
       << ", value: " << getAmount()
       << ", nonce: " << getNonce()
       << "}";
    return ss.str();
}

std::vector<uint8_t> Transaction::serialize() const {
    std::vector<uint8_t> result;
    
    // Add sender
    const auto& sender = getSender();
    result.insert(result.end(), sender.begin(), sender.end());
    
    // Add recipient
    const auto& recipient = getRecipient();
    result.insert(result.end(), recipient.begin(), recipient.end());
    
    // Add amount
    const auto amount = getAmount();
    const uint8_t* amount_bytes = reinterpret_cast<const uint8_t*>(&amount);
    result.insert(result.end(), amount_bytes, amount_bytes + sizeof(amount));
    
    // Add nonce
    const auto nonce = getNonce();
    const uint8_t* nonce_bytes = reinterpret_cast<const uint8_t*>(&nonce);
    result.insert(result.end(), nonce_bytes, nonce_bytes + sizeof(nonce));
    
    return result;
}

std::optional<Transaction> Transaction::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint64_t) * 2) {
        return std::nullopt;
    }
    
    Transaction tx;
    size_t offset = 0;
    
    // Extract sender
    std::string sender(reinterpret_cast<const char*>(data.data() + offset));
    tx.setSender(sender);
    offset += sender.size() + 1;
    
    // Extract recipient
    std::string recipient(reinterpret_cast<const char*>(data.data() + offset));
    tx.setRecipient(recipient);
    offset += recipient.size() + 1;
    
    // Extract amount
    uint64_t amount;
    std::memcpy(&amount, data.data() + offset, sizeof(amount));
    tx.setAmount(amount);
    offset += sizeof(amount);
    
    // Extract nonce
    uint64_t nonce;
    std::memcpy(&nonce, data.data() + offset, sizeof(nonce));
    tx.setNonce(nonce);
    
    return tx;
}

bool Transaction::is_valid() const {
    // Basic validation rules
    if (getSender().empty() || getRecipient().empty()) {
        return false;
    }

    // Amount must be positive
    if (getAmount() == 0) {
        return false;
    }

    // Add more validation rules as needed
    return true;
}

uint64_t Transaction::calculate_gas_cost() const {
    // Base cost for any transaction
    uint64_t cost = gas_limit;
    
    // Add cost based on data size
    cost += data.size() * 16; // 16 gas per byte of data
    
    return cost;
}

uint64_t Transaction::calculate_total_cost() const {
    return calculate_gas_cost() * gas_price + getAmount();
}

} // namespace blockchain
} // namespace quids 