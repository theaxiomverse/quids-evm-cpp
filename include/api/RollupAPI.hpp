#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "rollup/RollupTransactionAPI.hpp"
#include "rollup/StateManager.hpp"
#include "l1/RollupContract.hpp"

namespace quids {
namespace api {

using json = nlohmann::json;

class RollupAPI {
public:
    struct APIConfig {
        uint16_t rpc_port;
        std::string rpc_host;
        bool enable_ssl;
        std::string ssl_cert_path;
        std::string ssl_key_path;
        std::vector<std::string> allowed_origins;
    };

    struct APIResponse {
        bool success;
        json data;
        std::string error_message;
    };

    explicit RollupAPI(
        const APIConfig& config,
        std::shared_ptr<RollupTransactionAPI> tx_api,
        std::shared_ptr<StateManager> state_manager,
        std::shared_ptr<RollupContract> l1_contract
    );

    ~RollupAPI() = default;

    // Disable copy and move
    RollupAPI(const RollupAPI&) = delete;
    RollupAPI& operator=(const RollupAPI&) = delete;
    RollupAPI(RollupAPI&&) = delete;
    RollupAPI& operator=(RollupAPI&&) = delete;

    // Server lifecycle
    void start();
    void stop();

    // Transaction endpoints
    APIResponse submit_transaction(const json& params);
    APIResponse get_transaction(const json& params);
    APIResponse get_transaction_receipt(const json& params);
    
    // Account endpoints
    APIResponse get_account_balance(const json& params);
    APIResponse get_account_nonce(const json& params);
    APIResponse get_account_transactions(const json& params);
    
    // Block endpoints
    APIResponse get_block_by_number(const json& params);
    APIResponse get_latest_block(const json& params);
    APIResponse get_block_transactions(const json& params);
    
    // State endpoints
    APIResponse get_state_root(const json& params);
    APIResponse get_proof(const json& params);
    
    // Bridge endpoints
    APIResponse initiate_deposit(const json& params);
    APIResponse initiate_withdrawal(const json& params);
    APIResponse get_bridge_events(const json& params);
    
    // Validator endpoints
    APIResponse register_validator(const json& params);
    APIResponse get_validator_set(const json& params);
    
    // Metrics endpoints
    APIResponse get_network_metrics(const json& params);
    APIResponse get_performance_metrics(const json& params);

private:
    APIConfig config_;
    std::shared_ptr<RollupTransactionAPI> tx_api_;
    std::shared_ptr<StateManager> state_manager_;
    std::shared_ptr<RollupContract> l1_contract_;
    
    // Internal helper methods
    void setup_routes();
    APIResponse handle_request(const std::string& method, const json& params);
    bool validate_params(const json& params, const std::vector<std::string>& required);

    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace api
} // namespace quids 