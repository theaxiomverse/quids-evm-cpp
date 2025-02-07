#include "evm/ExternalLink.hpp"
#include "evm/ProofVerification.hpp"

// OpenSSL headers
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/core.h>
#include <openssl/core_names.h>
#include <openssl/provider.h>
#include <openssl/params.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

// Other includes
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <stdexcept>
#include <algorithm>

namespace evm {

class ExternalLink::Impl {
public:
    struct CacheEntry {
        ExternalData data;
        std::chrono::system_clock::time_point expiry;
    };
    
    struct RateLimit {
        uint32_t requests_per_second;
        uint32_t daily_quota;
        uint32_t requests_today;
        std::queue<std::chrono::system_clock::time_point> request_times;
    };
    
    std::unordered_map<std::string, std::string> trusted_sources_;
    std::unordered_map<std::string, RateLimit> rate_limits_;
    std::unordered_map<std::string, CacheEntry> cache_;
    bool caching_enabled_{false};
    std::chrono::seconds cache_duration_{3600};  // Default 1 hour
    std::vector<uint8_t> proof_buffer_;
    
    mutable std::mutex mutex_;
    CURL* curl_{nullptr};
    SSL_CTX* ssl_ctx_{nullptr};
    OSSL_PROVIDER* default_provider_{nullptr};
    OSSL_PROVIDER* legacy_provider_{nullptr};
};

ExternalLink::ExternalLink(
    const LinkConfig& config,
    std::shared_ptr<StateManager> state_manager,
    std::shared_ptr<ProofVerifier> proof_verifier
) : config_(config),
    state_manager_(state_manager),
    proof_verifier_(proof_verifier),
    impl_(std::make_unique<Impl>()) {
    
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    impl_->curl_ = curl_easy_init();
    if (!impl_->curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    // Initialize OpenSSL providers
    impl_->default_provider_ = OSSL_PROVIDER_load(nullptr, "default");
    impl_->legacy_provider_ = OSSL_PROVIDER_load(nullptr, "legacy");
    if (!impl_->default_provider_ || !impl_->legacy_provider_) {
        throw std::runtime_error("Failed to load OpenSSL providers");
    }
    
    // Create SSL context with TLS 1.3
    impl_->ssl_ctx_ = SSL_CTX_new(TLS_client_method());
    if (!impl_->ssl_ctx_) {
        throw std::runtime_error("Failed to create SSL context");
    }
    
    // Set secure defaults
    SSL_CTX_set_verify(impl_->ssl_ctx_, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(impl_->ssl_ctx_, 4);
    SSL_CTX_set_min_proto_version(impl_->ssl_ctx_, TLS1_3_VERSION);
    
    // Set additional security options
    SSL_CTX_set_options(impl_->ssl_ctx_, SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_mode(impl_->ssl_ctx_, SSL_MODE_AUTO_RETRY);
}

ExternalLink::~ExternalLink() {
    if (impl_->ssl_ctx_) {
        SSL_CTX_free(impl_->ssl_ctx_);
    }
    if (impl_->curl_) {
        curl_easy_cleanup(impl_->curl_);
    }
    if (impl_->legacy_provider_) {
        OSSL_PROVIDER_unload(impl_->legacy_provider_);
    }
    if (impl_->default_provider_) {
        OSSL_PROVIDER_unload(impl_->default_provider_);
    }
    curl_global_cleanup();
}

ExternalLink::DataResponse ExternalLink::fetch_data(const DataRequest& request) {
    if (!validate_request(request)) {
        return DataResponse{
            .success = false,
            .data = {},
            .proof = {},
            .error_message = "Invalid request"
        };
    }
    
    // Check cache first
    auto cached_data = fetch_from_cache(request);
    if (!cached_data.data.empty()) {
        return DataResponse{
            .success = true,
            .data = cached_data.data,
            .proof = cached_data.proof,
            .error_message = ""
        };
    }
    
    // Check rate limits
    if (!check_rate_limit(request.endpoint) || !check_quota(request.endpoint)) {
        return DataResponse{
            .success = false,
            .data = {},
            .proof = {},
            .error_message = "Rate limit exceeded"
        };
    }
    
    // Fetch data from external source
    ExternalData external_data;
    try {
        // Set up request
        curl_easy_setopt(impl_->curl_, CURLOPT_URL, request.endpoint.c_str());
        curl_easy_setopt(impl_->curl_, CURLOPT_SSL_CTX_FUNCTION, impl_->ssl_ctx_);
        
        // Set timeout
        curl_easy_setopt(impl_->curl_, CURLOPT_TIMEOUT_MS, config_.max_response_time_ms);
        
        // Prepare response buffer
        std::string response_data;
        curl_easy_setopt(impl_->curl_, CURLOPT_WRITEFUNCTION, 
            [](char* ptr, size_t size, size_t nmemb, void* userdata) {
                auto& response = *static_cast<std::string*>(userdata);
                response.append(ptr, size * nmemb);
                return size * nmemb;
            });
        curl_easy_setopt(impl_->curl_, CURLOPT_WRITEDATA, &response_data);
        
        // Perform request
        CURLcode res = curl_easy_perform(impl_->curl_);
        if (res != CURLE_OK) {
            return DataResponse{
                .success = false,
                .data = {},
                .proof = {},
                .error_message = curl_easy_strerror(res)
            };
        }

        // Parse response
        external_data.data = std::vector<uint8_t>(response_data.begin(), response_data.end());
        external_data.proof = impl_->proof_buffer_;
        external_data.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        external_data.source = request.endpoint;

    } catch (const std::exception& e) {
        return DataResponse{
            .success = false,
            .data = {},
            .proof = {},
            .error_message = e.what()
        };
    }
    
    // Verify data if required
    if (config_.require_proof && !verify_external_data(external_data)) {
        handle_verification_failure(DataResponse{
            .success = false,
            .data = external_data.data,
            .proof = external_data.proof,
            .error_message = "Data verification failed"
        });
        return DataResponse{
            .success = false,
            .data = {},
            .proof = {},
            .error_message = "Data verification failed"
        };
    }
    
    // Update cache
    if (impl_->caching_enabled_) {
        update_cache(request, external_data);
    }
    
    return DataResponse{
        .success = true,
        .data = external_data.data,
        .proof = external_data.proof,
        .error_message = ""
    };
}

std::future<ExternalLink::DataResponse> ExternalLink::fetch_data_async(const DataRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return fetch_data(request);
    });
}

void ExternalLink::fetch_data_with_callback(const DataRequest& request, DataCallback callback) {
    std::thread([this, request, callback]() {
        auto response = fetch_data(request);
        callback(response);
    }).detach();
}

bool ExternalLink::verify_external_data(const ExternalData& data) {
    // Verify source is trusted
    {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        if (impl_->trusted_sources_.find(data.source) == impl_->trusted_sources_.end()) {
            return false;
        }
    }
    
    // Verify proof if present
    if (!data.proof.empty()) {
        return proof_verifier_->verify_zk_proof(data.proof, data.data);
    }
    
    return true;
}

bool ExternalLink::add_trusted_source(const std::string& source, const std::string& public_key) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->trusted_sources_[source] = public_key;
    return true;
}

bool ExternalLink::check_rate_limit(const std::string& source) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    auto& limit = impl_->rate_limits_[source];
    
    auto now = std::chrono::system_clock::now();
    
    // Remove old requests
    while (!limit.request_times.empty() && 
           now - limit.request_times.front() > std::chrono::seconds(1)) {
        limit.request_times.pop();
    }
    
    if (limit.request_times.size() >= limit.requests_per_second) {
        return false;
    }
    
    limit.request_times.push(now);
    return true;
}

bool ExternalLink::check_quota(const std::string& source) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    auto& limit = impl_->rate_limits_[source];
    
    auto now = std::chrono::system_clock::now();
    auto today = std::chrono::time_point_cast<std::chrono::hours>(now);
    
    // Reset daily quota if it's a new day
    if (now - today > std::chrono::hours(24)) {
        limit.requests_today = 0;
    }
    
    if (limit.requests_today >= limit.daily_quota) {
        return false;
    }
    
    limit.requests_today++;
    return true;
}

ExternalData ExternalLink::fetch_from_cache(const DataRequest& request) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    auto it = impl_->cache_.find(request.endpoint);
    if (it != impl_->cache_.end() && 
        std::chrono::system_clock::now() < it->second.expiry) {
        return it->second.data;
    }
    return {};
}

void ExternalLink::update_cache(const DataRequest& request, const ExternalData& data) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->cache_[request.endpoint] = {
        data,
        std::chrono::system_clock::now() + impl_->cache_duration_
    };
}

void ExternalLink::enable_caching(bool enable) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->caching_enabled_ = enable;
}

void ExternalLink::set_cache_duration(std::chrono::seconds duration) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->cache_duration_ = duration;
}

void ExternalLink::clear_cache() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->cache_.clear();
}

bool ExternalLink::validate_request(const DataRequest& request) {
    if (request.endpoint.empty() || request.method.empty()) {
        return false;
    }
    
    // Validate URL format
    if (request.endpoint.substr(0, 8) != "https://") {
        return false;
    }
    
    // Validate timestamp
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    if (std::abs(static_cast<int64_t>(now - request.timestamp)) > 
        static_cast<int64_t>(config_.max_response_time_ms)) {
        return false;
    }
    
    return true;
}

void ExternalLink::establish_secure_connection(const std::string& endpoint) {
    // Create new SSL object
    SSL* ssl = SSL_new(impl_->ssl_ctx_);
    if (!ssl) {
        throw std::runtime_error("Failed to create SSL object");
    }
    
    // Create BIO for network operations
    BIO* bio = BIO_new_ssl_connect(impl_->ssl_ctx_);
    if (!bio) {
        SSL_free(ssl);
        throw std::runtime_error("Failed to create BIO");
    }
    
    try {
        // Set connection hostname
        if (BIO_set_conn_hostname(bio, endpoint.c_str()) != 1) {
            throw std::runtime_error("Failed to set connection hostname");
        }
        
        // Set SSL object to BIO
        SSL* ssl_bio;
        BIO_get_ssl(bio, &ssl_bio);
        if (!ssl_bio) {
            throw std::runtime_error("Failed to get SSL from BIO");
        }
        
        // Set TLS SNI (Server Name Indication)
        if (SSL_set_tlsext_host_name(ssl_bio, endpoint.c_str()) != 1) {
            throw std::runtime_error("Failed to set TLS hostname");
        }
        
        // Perform TLS handshake
        if (BIO_do_connect(bio) != 1 || BIO_do_handshake(bio) != 1) {
            throw std::runtime_error("Failed to establish TLS connection");
        }
        
        // Verify the certificate
        X509* cert = SSL_get_peer_certificate(ssl_bio);
        if (!cert) {
            throw std::runtime_error("Failed to get peer certificate");
        }
        
        // Certificate verification result
        long verify_result = SSL_get_verify_result(ssl_bio);
        if (verify_result != X509_V_OK) {
            X509_free(cert);
            throw std::runtime_error("Certificate verification failed");
        }
        
        X509_free(cert);
    } catch (const std::exception& e) {
        BIO_free_all(bio);
        SSL_free(ssl);
        throw;
    }
    
    // Clean up
    BIO_free_all(bio);
    SSL_free(ssl);
}

void ExternalLink::verify_tls_certificate(const std::string& cert) {
    // Implement certificate verification logic
}

void ExternalLink::handle_timeout(const DataRequest& request) {
    // Implement timeout handling logic
}

void ExternalLink::handle_verification_failure(const DataResponse& response) {
    // Implement verification failure handling logic
}

} // namespace evm 