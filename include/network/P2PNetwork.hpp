#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <optional>
#include <atomic>
#include "blockchain/Transaction.hpp"
#include "rollup/StateTransitionProof.hpp"

namespace quids {
namespace network {

class P2PNetwork {
public:
    struct NetworkConfig {
        std::string bootstrap_nodes;
        uint16_t listen_port;
        std::string network_id;
        size_t max_peers;
        std::chrono::seconds peer_timeout;
        bool enable_discovery;
        
        NetworkConfig()
            : listen_port(30303)
            , max_peers(50)
            , peer_timeout(std::chrono::seconds(60))
            , enable_discovery(true)
        {}
    };
    
    struct NodeInfo {
        std::string node_id;
        std::string address;
        uint16_t port;
        std::chrono::system_clock::time_point last_seen;
        bool is_validator;
        
        bool operator==(const NodeInfo& other) const {
            return node_id == other.node_id;
        }
    };
    
    using MessageHandler = std::function<void(const std::string&, const std::vector<uint8_t>&)>;
    
    // Constructor and destructor
    explicit P2PNetwork(const NetworkConfig& config);
    ~P2PNetwork();
    
    // Rule of 5
    P2PNetwork(const P2PNetwork&) = delete;
    P2PNetwork& operator=(const P2PNetwork&) = delete;
    P2PNetwork(P2PNetwork&&) noexcept = default;
    P2PNetwork& operator=(P2PNetwork&&) noexcept = default;
    
    // Network operations
    void start();
    void stop();
    [[nodiscard]] bool connect_to_peer(const std::string& peer_address);
    void broadcast_transaction(const Transaction& tx);
    void broadcast_state_update(const StateTransitionProof& proof);
    [[nodiscard]] std::vector<NodeInfo> get_connected_peers() const;
    
    // Message handling
    void register_message_handler(const std::string& topic, MessageHandler handler);
    [[nodiscard]] bool register_as_validator(const std::string& validator_key);
    
    // Peer discovery
    void discover_peers();
    
private:
    // Private implementation (PIMPL)
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Internal event handlers
    void handle_peer_connection(const std::string& peer_id);
    void handle_peer_disconnection(const std::string& peer_id);
    [[nodiscard]] std::string generate_node_id();
    
    // Performance metrics
    std::atomic<size_t> total_messages_sent_{0};
    std::atomic<size_t> total_messages_received_{0};
    std::atomic<size_t> failed_messages_{0};
    
    // Constants
    static constexpr size_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024; // 10MB
    static constexpr size_t MIN_PEERS = 3;
    static constexpr std::chrono::seconds DISCOVERY_INTERVAL{60};
};

} // namespace network
} // namespace quids 