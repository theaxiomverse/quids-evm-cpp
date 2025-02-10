#include "network/P2PNetwork.hpp"
#include "network/P2PConnection.hpp"
#include <spdlog/spdlog.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace quids {
namespace network {

class P2PNetwork::Impl {
public:
    std::unordered_map<std::string, std::shared_ptr<P2PConnection>> connections;
    std::unordered_map<std::string, MessageHandler> message_handlers;
    std::vector<std::string> connected_peers;
    bool running{false};
    NetworkConfig config;
    std::string node_id;
    bool is_validator{false};
    std::mutex connections_mutex;
    std::mutex handlers_mutex;
};

P2PNetwork::P2PNetwork(const NetworkConfig& config)
    : config_(config), impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    impl_->node_id = generate_node_id();
    impl_->is_validator = false;
}

void P2PNetwork::start() {
    if (impl_->running) return;
    
    // Create P2P connection with config
    P2PConnection::Config conn_config;
    conn_config.port = config_.listen_port;
    conn_config.stun_server = config_.stun_server;
    conn_config.stun_port = config_.stun_port;
    conn_config.enable_upnp = config_.enable_upnp;
    conn_config.enable_nat_pmp = config_.enable_nat_pmp;
    conn_config.max_peers = config_.max_peers;
    
    auto connection = std::make_shared<P2PConnection>(conn_config);
    if (!connection->start()) {
        spdlog::error("Failed to start P2P connection");
        return;
    }
    
    // Store main connection
    {
        std::lock_guard<std::mutex> lock(impl_->connections_mutex);
        impl_->connections["main"] = connection;
    }
    
    // Connect to bootstrap nodes
    for (const auto& addr : config_.bootstrap_nodes) {
        connect_to_peer(addr);
    }
    
    impl_->running = true;
    spdlog::info("P2P network started on port {}", config_.listen_port);
    
    // Start peer discovery
    discover_peers();
}

void P2PNetwork::stop() {
    if (!impl_->running) return;
    
    // Stop all connections
    {
        std::lock_guard<std::mutex> lock(impl_->connections_mutex);
        for (auto& [_, connection] : impl_->connections) {
            connection->stop();
        }
        impl_->connections.clear();
    }
    
    impl_->running = false;
    spdlog::info("P2P network stopped");
}

bool P2PNetwork::connect_to_peer(const std::string& peer_address) {
    if (!impl_->running) return false;

    try {
        // Parse address into IP and port
        auto pos = peer_address.find(':');
        if (pos == std::string::npos) {
            spdlog::error("Invalid peer address format: {}", peer_address);
            return false;
        }
        
        std::string ip = peer_address.substr(0, pos);
        uint16_t port = std::stoi(peer_address.substr(pos + 1));
        
        // Get main connection
        std::shared_ptr<P2PConnection> main_connection;
        {
            std::lock_guard<std::mutex> lock(impl_->connections_mutex);
            auto it = impl_->connections.find("main");
            if (it == impl_->connections.end()) {
                spdlog::error("Main connection not found");
                return false;
            }
            main_connection = it->second;
        }
        
        // Perform NAT traversal and connect
        if (main_connection->perform_nat_traversal(ip, port)) {
            handle_peer_connection(peer_address);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to peer {}: {}", peer_address, e.what());
    }
    return false;
}

void P2PNetwork::broadcast_transaction(const blockchain::Transaction& tx) {
    if (!impl_->running) return;
    
    // Serialize transaction
    std::vector<uint8_t> data = tx.serialize();
    
    // Broadcast to all peers
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    for (auto& [_, connection] : impl_->connections) {
        connection->broadcast(data);
    }
    spdlog::debug("Broadcasted transaction to network");
}

void P2PNetwork::broadcast_state_update(const rollup::StateTransitionProof& proof) {
    if (!impl_->running) return;
    
    // Serialize proof
    std::vector<uint8_t> data;  // TODO: Implement proof serialization
    
    // Broadcast to all peers
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    for (auto& [_, connection] : impl_->connections) {
        connection->broadcast(data);
    }
    spdlog::debug("Broadcasted state update to network");
}

std::vector<P2PNetwork::NodeInfo> P2PNetwork::get_connected_peers() const {
    std::vector<NodeInfo> peers;
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    
    for (const auto& [_, connection] : impl_->connections) {
        for (const auto& peer : connection->get_connected_peers()) {
            NodeInfo info;
            info.id = peer.id;
            info.address = peer.address;
            info.port = peer.port;
            info.is_validator = false;  // TODO: Implement validator detection
            peers.push_back(info);
        }
    }
    return peers;
}

void P2PNetwork::register_message_handler(const std::string& topic, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->handlers_mutex);
    impl_->message_handlers[topic] = std::move(handler);
    
    // Register handler with all connections
    std::lock_guard<std::mutex> conn_lock(impl_->connections_mutex);
    for (auto& [_, connection] : impl_->connections) {
        connection->set_message_handler(
            [this, topic](const std::string& peer_id, uint16_t port, const std::vector<uint8_t>& data) {
                if (auto it = impl_->message_handlers.find(topic); 
                    it != impl_->message_handlers.end()) {
                    it->second(data, topic);
                }
            }
        );
    }
}

bool P2PNetwork::register_as_validator(const std::string& validator_key) {
    // TODO: Implement validator registration with key verification
    impl_->is_validator = true;
    return true;
}

void P2PNetwork::discover_peers() {
    // Start periodic peer discovery
    std::thread([this]() {
        while (impl_->running) {
            // Get peers from all connections
            std::lock_guard<std::mutex> lock(impl_->connections_mutex);
            for (auto& [_, connection] : impl_->connections) {
                auto peers = connection->get_connected_peers();
                for (const auto& peer : peers) {
                    handle_peer_connection(peer.address + ":" + std::to_string(peer.port));
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }).detach();
}

void P2PNetwork::handle_peer_connection(const std::string& peer_address) {
    spdlog::info("New peer connected: {}", peer_address);
    impl_->connected_peers.push_back(peer_address);
}

void P2PNetwork::handle_peer_disconnection(const std::string& peer_address) {
    spdlog::info("Peer disconnected: {}", peer_address);
    auto it = std::find(impl_->connected_peers.begin(), impl_->connected_peers.end(), peer_address);
    if (it != impl_->connected_peers.end()) {
        impl_->connected_peers.erase(it);
    }
}

std::string P2PNetwork::generate_node_id() {
    // Generate random node ID
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << dis(gen);
    return ss.str();
}

} // namespace network
} // namespace quids 