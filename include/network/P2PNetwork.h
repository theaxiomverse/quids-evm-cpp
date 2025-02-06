#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "blockchain/Transaction.h"
#include "rollup/RollupStateTransition.h"

class P2PNetwork {
public:
    struct NodeInfo {
        std::string id;
        std::string ip;
        uint16_t port;
        bool is_validator;
    };

    struct NetworkConfig {
        uint16_t listen_port;
        std::vector<std::string> bootstrap_nodes;
        std::string node_key_path;
        size_t max_peers;
    };

    explicit P2PNetwork(const NetworkConfig& config);
    
    // Core networking
    void start();
    void stop();
    bool connect_to_peer(const std::string& peer_address);
    void broadcast_transaction(const Transaction& tx);
    void broadcast_state_update(const StateTransitionProof& proof);
    
    // Node discovery
    std::vector<NodeInfo> get_connected_peers() const;
    void add_bootstrap_node(const std::string& address);
    
    // Message handlers
    using MessageHandler = std::function<void(const std::vector<uint8_t>&, const std::string&)>;
    void register_message_handler(const std::string& topic, MessageHandler handler);
    
    // Validator specific
    bool register_as_validator(const std::string& validator_key);
    std::vector<NodeInfo> get_active_validators() const;

private:
    NetworkConfig config_;
    std::string node_id_;
    bool is_validator_;
    
    void handle_peer_connection(const std::string& peer_id);
    void handle_peer_disconnection(const std::string& peer_id);
    void discover_peers();
    void maintain_connections();
}; 