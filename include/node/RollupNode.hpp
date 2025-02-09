#pragma once

#include <memory>
#include <string>
#include "network/P2PNetwork.hpp"
#include "l1/RollupContract.hpp"
#include "storage/PersistentStorage.hpp"
#include "api/RollupAPI.hpp"
#include "rollup/RollupTransactionAPI.hpp"
#include "rollup/StateManager.hpp"

class RollupNode {
public:
    struct NodeConfig {
        std::string data_dir;
        bool is_validator;
        std::string validator_key_path;
        
        P2PNetwork::NetworkConfig network;
        RollupContract::ContractConfig contract;
        PersistentStorage::StorageConfig storage;
        RollupAPI::APIConfig api;
        
        // Performance tuning
        size_t num_worker_threads;
        size_t max_pending_txs;
        size_t batch_size;
        uint64_t batch_timeout_ms;
    };

    explicit RollupNode(const NodeConfig& config);
    
    // Node lifecycle
    void initialize();
    void start();
    void stop();
    
    // Status and metrics
    bool is_synced() const;
    bool is_healthy() const;
    RollupPerformanceMetrics get_metrics() const;
    
    // Node management
    void reload_config();
    void backup_data(const std::string& backup_path);
    void compact_storage();
    
    // Emergency handling
    void handle_emergency_shutdown();
    bool is_emergency_mode() const;

private:
    NodeConfig config_;
    
    // Core components
    std::shared_ptr<quids::network::P2PNetwork> network_;
    std::shared_ptr<quids::rollup::RollupContract> l1_contract_;
    std::shared_ptr<quids::storage::PersistentStorage> storage_;
    std::shared_ptr<quids::api::RollupAPI> api_;
    std::shared_ptr<quids::rollup::RollupTransactionAPI> tx_api_;
    std::shared_ptr<quids::rollup::StateManager> state_manager_;
    
    // Node state
    bool is_initialized_;
    bool is_running_;
    bool is_emergency_;
    
    void setup_components();
    void connect_components();
    void start_services();
    void stop_services();
    
    void handle_new_block(const PersistentStorage::BlockHeader& header);
    void handle_state_update(const StateTransitionProof& proof);
    void handle_network_event(const std::string& event, const std::vector<uint8_t>& data);
}; 