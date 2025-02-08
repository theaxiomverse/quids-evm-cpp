#pragma once

#include "network/P2PConnection.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

namespace quids {
namespace network {

// Forward declarations
class P2PConnection;
class MessageHandler;

class P2PNode {
public:
    struct Config {
        uint16_t port{8080};
        size_t max_connections{100};
        size_t buffer_size{1024 * 1024}; // 1MB
        std::string bind_address{"0.0.0.0"};
        size_t ping_interval_ms{30000}; // 30 seconds
        size_t connection_timeout_ms{60000}; // 60 seconds
    };

    struct PeerInfo {
        std::string address;
        uint16_t port;
        std::chrono::system_clock::time_point last_seen;
        size_t messages_received{0};
        size_t messages_sent{0};
        bool is_connected{false};
    };

    explicit P2PNode(const Config& config = Config{});
    ~P2PNode();

    // Node lifecycle
    bool start();
    void stop();
    bool is_running() const { return running_; }

    // Peer management
    bool connect_to_peer(const std::string& address, uint16_t port);
    void disconnect_from_peer(const std::string& address, uint16_t port);
    std::vector<PeerInfo> get_connected_peers() const;
    size_t get_connection_count() const;

    // Message handling
    using MessageHandler = std::function<void(const std::string& peer_address, 
                                            uint16_t peer_port,
                                            const std::vector<uint8_t>& message)>;
    
    void register_message_handler(MessageHandler handler);
    bool broadcast_message(const std::vector<uint8_t>& message);
    bool send_message_to_peer(const std::string& address, 
                            uint16_t port,
                            const std::vector<uint8_t>& message);

    // Node discovery
    void add_bootstrap_peer(const std::string& address, uint16_t port);
    void discover_peers();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    Config config_;
    std::atomic<bool> running_{false};
    std::vector<MessageHandler> message_handlers_;
    mutable std::mutex handlers_mutex_;

    // Internal methods
    void accept_connections();
    void manage_connections();
    void handle_incoming_messages();
    void cleanup_disconnected_peers();
    bool validate_peer(const std::string& address, uint16_t port);
    void update_peer_info(const std::string& address, uint16_t port, bool connected);
};

} // namespace network
} // namespace quids 