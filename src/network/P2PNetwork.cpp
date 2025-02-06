#include "network/P2PNetwork.h"
#include <libp2p/host/basic_host.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/transport/tcp.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <spdlog/spdlog.h>

using libp2p::Host;
using libp2p::peer::PeerId;
using libp2p::protocol::kademlia::Kademlia;

class P2PNetwork::Impl {
public:
    std::shared_ptr<Host> host;
    std::shared_ptr<Kademlia> kad;
    std::unordered_map<std::string, MessageHandler> message_handlers;
    std::vector<PeerId> connected_peers;
    bool running{false};
};

P2PNetwork::P2PNetwork(const NetworkConfig& config)
    : config_(config), impl_(std::make_unique<Impl>()) {
    node_id_ = generate_node_id();
    is_validator_ = false;
}

void P2PNetwork::start() {
    if (impl_->running) return;
    
    // Create libp2p host
    auto host_config = libp2p::host::BasicHost::Config()
        .withPort(config_.listen_port)
        .withTransport<libp2p::transport::TcpTransport>()
        .withSecurity<libp2p::security::Noise>();
    
    impl_->host = libp2p::host::BasicHost::create(host_config);
    
    // Initialize Kademlia DHT
    impl_->kad = std::make_shared<Kademlia>(impl_->host);
    
    // Start listening
    auto listen_addr = "/ip4/0.0.0.0/tcp/" + std::to_string(config_.listen_port);
    impl_->host->listen(libp2p::multi::Multiaddress::create(listen_addr).value());
    
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
    
    impl_->host->stop();
    impl_->running = false;
    spdlog::info("P2P network stopped");
}

bool P2PNetwork::connect_to_peer(const std::string& peer_address) {
    try {
        auto ma = libp2p::multi::Multiaddress::create(peer_address).value();
        auto peer_id = impl_->host->connectToMultiaddr(ma);
        if (peer_id) {
            handle_peer_connection(peer_id->toHex());
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to peer {}: {}", peer_address, e.what());
    }
    return false;
}

void P2PNetwork::broadcast_transaction(const Transaction& tx) {
    if (!impl_->running) return;
    
    // Serialize transaction
    std::vector<uint8_t> data = tx.serialize();
    
    // Broadcast to all peers
    impl_->kad->publish("transactions", data);
    spdlog::debug("Broadcasted transaction to network");
}

void P2PNetwork::broadcast_state_update(const StateTransitionProof& proof) {
    if (!impl_->running) return;
    
    // Serialize proof
    std::vector<uint8_t> data;  // TODO: Implement proof serialization
    
    // Broadcast to all peers
    impl_->kad->publish("state_updates", data);
    spdlog::debug("Broadcasted state update to network");
}

std::vector<P2PNetwork::NodeInfo> P2PNetwork::get_connected_peers() const {
    std::vector<NodeInfo> peers;
    for (const auto& peer_id : impl_->connected_peers) {
        NodeInfo info;
        info.id = peer_id.toHex();
        // TODO: Get IP and port from peer info
        info.is_validator = false;  // TODO: Implement validator detection
        peers.push_back(info);
    }
    return peers;
}

void P2PNetwork::register_message_handler(const std::string& topic, MessageHandler handler) {
    impl_->message_handlers[topic] = std::move(handler);
    
    // Subscribe to topic
    impl_->kad->subscribe(topic, [this, topic](const std::vector<uint8_t>& data) {
        if (auto it = impl_->message_handlers.find(topic); it != impl_->message_handlers.end()) {
            it->second(data, topic);
        }
    });
}

bool P2PNetwork::register_as_validator(const std::string& validator_key) {
    // TODO: Implement validator registration with key verification
    is_validator_ = true;
    return true;
}

void P2PNetwork::discover_peers() {
    // Start periodic peer discovery
    std::thread([this]() {
        while (impl_->running) {
            impl_->kad->findPeers();
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }).detach();
}

void P2PNetwork::handle_peer_connection(const std::string& peer_id) {
    spdlog::info("New peer connected: {}", peer_id);
    impl_->connected_peers.push_back(PeerId::fromHex(peer_id).value());
}

void P2PNetwork::handle_peer_disconnection(const std::string& peer_id) {
    spdlog::info("Peer disconnected: {}", peer_id);
    auto it = std::find_if(impl_->connected_peers.begin(), impl_->connected_peers.end(),
        [&](const PeerId& p) { return p.toHex() == peer_id; });
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
    ss << std::hex << dis(gen);
    return ss.str();
} 