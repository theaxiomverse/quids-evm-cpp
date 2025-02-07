#include "api/RollupAPI.hpp"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <thread>

class RollupAPI::Impl {
public:
    httplib::Server server;
    std::thread server_thread;
    bool running{false};
};

RollupAPI::RollupAPI(
    const APIConfig& config,
    std::shared_ptr<RollupTransactionAPI> tx_api,
    std::shared_ptr<StateManager> state_manager,
    std::shared_ptr<RollupContract> l1_contract
) : config_(config),
    tx_api_(tx_api),
    state_manager_(state_manager),
    l1_contract_(l1_contract),
    impl_(std::make_unique<Impl>()) {
    setup_routes();
}

void RollupAPI::start() {
    if (impl_->running) return;
    
    // Configure SSL if enabled
    if (config_.enable_ssl) {
        impl_->server.set_ssl_cert(config_.ssl_cert_path.c_str());
        impl_->server.set_ssl_key(config_.ssl_key_path.c_str());
    }
    
    // Configure CORS
    impl_->server.set_cors(config_.allowed_origins, "POST, GET, OPTIONS",
                          "Content-Type, Authorization");
    
    // Start server in a separate thread
    impl_->server_thread = std::thread([this]() {
        impl_->running = true;
        impl_->server.listen(config_.rpc_host.c_str(), config_.rpc_port);
    });
    
    spdlog::info("RPC server started on {}:{}", config_.rpc_host, config_.rpc_port);
}

void RollupAPI::stop() {
    if (!impl_->running) return;
    
    impl_->server.stop();
    if (impl_->server_thread.joinable()) {
        impl_->server_thread.join();
    }
    impl_->running = false;
    
    spdlog::info("RPC server stopped");
}

void RollupAPI::setup_routes() {
    // Transaction endpoints
    impl_->server.Post("/tx/submit", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto params = json::parse(req.body);
            auto response = submit_transaction(params);
            res.set_content(response.data.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
        }
    });
    
    impl_->server.Get("/tx/:hash", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json params = {{"hash", req.path_params.at("hash")}};
            auto response = get_transaction(params);
            res.set_content(response.data.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
        }
    });
    
    // Account endpoints
    impl_->server.Get("/account/:address/balance", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json params = {{"address", req.path_params.at("address")}};
            auto response = get_account_balance(params);
            res.set_content(response.data.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
        }
    });
    
    // Block endpoints
    impl_->server.Get("/block/:number", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json params = {{"number", std::stoull(req.path_params.at("number"))}};
            auto response = get_block_by_number(params);
            res.set_content(response.data.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
        }
    });
    
    // Bridge endpoints
    impl_->server.Post("/bridge/deposit", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto params = json::parse(req.body);
            auto response = initiate_deposit(params);
            res.set_content(response.data.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
        }
    });
}

APIResponse RollupAPI::submit_transaction(const json& params) {
    if (!validate_params(params, {"sender", "recipient", "amount", "signature"})) {
        return {false, nullptr, "Missing required parameters"};
    }
    
    Transaction tx;
    tx.sender = params["sender"];
    tx.recipient = params["recipient"];
    tx.amount = params["amount"];
    // TODO: Parse signature
    
    auto result = tx_api_->submitTransaction(tx);
    return {!result.empty(), {{"tx_hash", result}}, ""};
}

APIResponse RollupAPI::get_account_balance(const json& params) {
    if (!validate_params(params, {"address"})) {
        return {false, nullptr, "Missing address parameter"};
    }
    
    try {
        auto account = state_manager_->get_account(params["address"]);
        return {true, {{"balance", account.balance}}, ""};
    } catch (const std::exception& e) {
        return {false, nullptr, e.what()};
    }
}

APIResponse RollupAPI::initiate_deposit(const json& params) {
    if (!validate_params(params, {"l1_address", "l2_address", "amount"})) {
        return {false, nullptr, "Missing required parameters"};
    }
    
    DepositEvent event{
        params["l1_address"],
        params["l2_address"],
        params["amount"],
        std::chrono::system_clock::now().time_since_epoch().count()
    };
    
    auto deposits = l1_contract_->get_pending_deposits();
    return {true, {{"event_id", deposits.size()}}, ""};
}

bool RollupAPI::validate_params(const json& params, const std::vector<std::string>& required) {
    for (const auto& param : required) {
        if (!params.contains(param)) {
            return false;
        }
    }
    return true;
} 