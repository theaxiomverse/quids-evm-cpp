#include "crypto/falcon_signature.hpp"
#include <stdexcept>
#include <cstring>
#include <memory>

quids::crypto::falcon::FalconSignature::FalconSignature(size_t N_)
{
    N = N_;
    
    // Handle different Falcon parameter sets
    switch (N) {
        case 512:
            pklen = ::falcon_utils::compute_pkey_len<512>();
            sklen = ::falcon_utils::compute_skey_len<512>();
            break;
        case 1024:
            pklen = ::falcon_utils::compute_pkey_len<1024>();
            sklen = ::falcon_utils::compute_skey_len<1024>();
            break;
        default:
            throw std::invalid_argument("Invalid Falcon parameter N. Supported values are 512 and 1024.");
    }
    
    // Allocate memory for keys and signature
    public_key = new uint8_t[pklen]();  // Zero initialize
    secret_key = new uint8_t[sklen]();  // Zero initialize
    signature = new uint8_t[N == 512 ? 666 : 1280]();  // Pre-allocate signature buffer
    message = new uint8_t[msglen]();    // Zero initialize
}

quids::crypto::falcon::FalconSignature::~FalconSignature()
{
    if (public_key) {
        std::memset(public_key, 0, pklen);  // Secure cleanup
        delete[] public_key;
    }
    if (secret_key) {
        std::memset(secret_key, 0, sklen);  // Secure cleanup
        delete[] secret_key;
    }
    if (signature) {
        delete[] signature;
    }
    if (message) {
        std::memset(message, 0, msglen);  // Secure cleanup
        delete[] message;
    }
}

auto quids::crypto::falcon::FalconSignature::generate_key_pair() -> std::pair<std::string, std::string>
{
    // Clear any existing key data
    std::memset(public_key, 0, pklen);
    std::memset(secret_key, 0, sklen);

    switch (N) {
        case 512:
            ::falcon::keygen<512>(public_key, secret_key);
            break;
        case 1024:
            ::falcon::keygen<1024>(public_key, secret_key);
            break;
    }
    return std::make_pair(std::string(reinterpret_cast<char*>(public_key), pklen),
                         std::string(reinterpret_cast<char*>(secret_key), sklen));
}

auto quids::crypto::falcon::FalconSignature::sign_message(const std::string& message_str) -> std::string
{
    if (message_str.length() > msglen) {
        throw std::invalid_argument("Message too long");
    }

    // Copy message to internal buffer and zero-pad
    std::memset(message, 0, msglen);  // Clear entire buffer first
    std::memcpy(message, message_str.data(), message_str.length());
    
    // Clear signature buffer before signing
    size_t sig_len = (N == 512) ? 666 : 1280;
    std::memset(signature, 0, sig_len);
    
    bool success = false;
    switch (N) {
        case 512:
            success = ::falcon::sign<512>(secret_key, message, message_str.length(), signature);  // Use actual length
            break;
        case 1024:
            success = ::falcon::sign<1024>(secret_key, message, message_str.length(), signature);  // Use actual length
            break;
    }

    if (!success) {
        throw std::runtime_error("Signature generation failed");
    }

    return std::string(reinterpret_cast<char*>(signature), sig_len);
}

auto quids::crypto::falcon::FalconSignature::verify_signature(
    const std::string& message_str, const std::string& signature_str) -> bool
{
    if (message_str.length() > msglen) {
        throw std::invalid_argument("Message too long");
    }

    // Copy and zero-pad message
    std::memset(message, 0, msglen);  // Clear entire buffer first
    std::memcpy(message, message_str.data(), message_str.length());

    // Create a mutable copy of the signature using RAII
    size_t sig_len = (N == 512) ? 666 : 1280;
    if (signature_str.length() != sig_len) {
        return false;  // Invalid signature length
    }

    std::unique_ptr<uint8_t[]> sig_buffer(new uint8_t[sig_len]);
    std::memset(sig_buffer.get(), 0, sig_len);  // Clear buffer
    std::memcpy(sig_buffer.get(), signature_str.data(), signature_str.length());

    bool result = false;
    try {
        switch (N) {
            case 512:
                result = ::falcon::verify<512>(
                    public_key,
                    message,
                    message_str.length(),  // Use actual message length
                    sig_buffer.get()
                );
                break;
            case 1024:
                result = ::falcon::verify<1024>(
                    public_key,
                    message,
                    message_str.length(),  // Use actual message length
                    sig_buffer.get()
                );
                break;
        }
    } catch (...) {
        return false;  // Any verification exception means invalid signature
    }

    return result;
}

auto quids::crypto::falcon::FalconSignature::import_key_pair(
    const std::string& public_key_str, const std::string& secret_key_str) -> bool
{
    if (public_key_str.length() != pklen || secret_key_str.length() != sklen) {
        throw std::invalid_argument("Invalid key length");
    }

    std::memcpy(public_key, public_key_str.data(), pklen);
    std::memcpy(secret_key, secret_key_str.data(), sklen);
    return true;
}

auto quids::crypto::falcon::FalconSignature::export_key_pair() -> std::pair<std::string, std::string>
{
    return std::make_pair(
        std::string(reinterpret_cast<char*>(public_key), pklen),
        std::string(reinterpret_cast<char*>(secret_key), sklen)
    );
}

