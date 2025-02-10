#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace quids {

namespace blockchain {
class Transaction;
}

namespace rollup {
class StateTransitionProof;
}

namespace network {

struct NetworkConfig {
    uint16_t listen_port{0};
    std::string stun_server;
    uint16_t stun_port{3478};
    bool enable_upnp{true};
    bool enable_nat_pmp{true};
    size_t max_peers{100};
    std::vector<std::string> bootstrap_nodes;
};

class P2PNetwork {
public:
    struct NodeInfo {
        std::string id;
        std::string address;
        uint16_t port{0};
        bool is_validator{false};
    };

    using MessageHandler = std::function<void(const std::vector<uint8_t>&, const std::string&)>;

    explicit P2PNetwork(const NetworkConfig& config);
    ~P2PNetwork();

    // Prevent copying
    P2PNetwork(const P2PNetwork&) = delete;
    P2PNetwork& operator=(const P2PNetwork&) = delete;

    // Allow moving
    P2PNetwork(P2PNetwork&&) noexcept = default;
    P2PNetwork& operator=(P2PNetwork&&) noexcept = default;

    // Network management
    void start();
    void stop();
    bool connect_to_peer(const std::string& peer_address);
    std::vector<NodeInfo> get_connected_peers() const;

    // Message handling
    void register_message_handler(const std::string& topic, MessageHandler handler);
    void broadcast_transaction(const blockchain::Transaction& tx);
    void broadcast_state_update(const rollup::StateTransitionProof& proof);

    // Validator management
    bool register_as_validator(const std::string& validator_key);

private:
    void discover_peers();
    void handle_peer_connection(const std::string& peer_address);
    void handle_peer_disconnection(const std::string& peer_address);
    std::string generate_node_id();

    NetworkConfig config_;
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace network
} // namespace quids 