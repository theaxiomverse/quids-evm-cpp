#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include "evm/FloatingPoint.hpp"
#include "evm/Address.hpp"

namespace evm {

// Forward declarations
class StateManager;
class ProofVerifier;

struct ExternalData {
    std::vector<uint8_t> data;
    std::vector<uint8_t> proof;
    uint64_t timestamp;
    std::string source;
};

class ExternalLink {
public:
    struct LinkConfig {
        uint32_t min_confirmations;
        uint32_t max_response_time_ms;
        bool require_proof;
        std::vector<std::string> trusted_sources;
        std::string verification_key;
    };

    struct DataRequest {
        Address address;
        std::vector<uint8_t> key;
        uint64_t timestamp;
        bool require_proof;
        std::string endpoint;  // External API endpoint
        std::string method;    // HTTP method (GET, POST, etc.)
    };

    struct DataResponse {
        bool success;
        std::vector<uint8_t> data;
        std::vector<uint8_t> proof;
        std::string error_message;
    };

    // Callback type for async operations
    using DataCallback = std::function<void(const DataResponse&)>;

    explicit ExternalLink(
        const LinkConfig& config,
        std::shared_ptr<StateManager> state_manager,
        std::shared_ptr<ProofVerifier> proof_verifier
    );

    ~ExternalLink();

    // Synchronous operations
    DataResponse fetch_data(const DataRequest& request);
    bool verify_external_data(const ExternalData& data);
    
    // Asynchronous operations
    std::future<DataResponse> fetch_data_async(const DataRequest& request);
    void fetch_data_with_callback(const DataRequest& request, DataCallback callback);
    
    // Source management
    bool add_trusted_source(const std::string& source, const std::string& public_key);
    bool remove_trusted_source(const std::string& source);
    std::vector<std::string> get_trusted_sources() const;
    
    // Rate limiting and quotas
    void set_rate_limit(const std::string& source, uint32_t requests_per_second);
    void set_quota(const std::string& source, uint32_t daily_quota);
    
    // Caching
    void enable_caching(bool enable);
    void set_cache_duration(std::chrono::seconds duration);
    void clear_cache();

    // Connection management
    void establish_secure_connection(const std::string& endpoint);
    void verify_tls_certificate(const std::string& cert);

    // Error handling
    void handle_timeout(const DataRequest& request);
    void handle_verification_failure(const DataResponse& data);

private:
    LinkConfig config_;
    std::shared_ptr<StateManager> state_manager_;
    std::shared_ptr<ProofVerifier> proof_verifier_;
    
    // Internal helper methods
    bool validate_request(const DataRequest& request);
    bool verify_source(const std::string& source, const std::vector<uint8_t>& signature);
    ExternalData fetch_from_cache(const DataRequest& request);
    void update_cache(const DataRequest& request, const ExternalData& data);
    
    // Rate limiting
    bool check_rate_limit(const std::string& source);
    bool check_quota(const std::string& source);
    
    std::string endpoint_;
    bool is_connected_{false};
    uint64_t timeout_ms_{5000};  // Default 5 second timeout
    
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace evm 