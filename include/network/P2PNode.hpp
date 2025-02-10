#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <chrono>
#include "network/P2PConnection.hpp"
#include <boost/asio.hpp>

namespace quids {
namespace network {

class P2PNode {
public:
    struct Config {
        uint16_t port{0};
        std::string stun_server;
        uint16_t stun_port{3478};
        bool enable_upnp{true};
        bool enable_nat_pmp{true};
        size_t max_peers{100};
        size_t max_connections{1000};
        size_t buffer_size{64 * 1024}; // 64KB
        uint32_t connection_timeout_ms{30000}; // 30 seconds
        uint32_t ping_interval_ms{10000}; // 10 seconds
        std::vector<std::pair<std::string, uint16_t>> bootstrap_peers;
        std::vector<std::pair<std::string, uint16_t>> known_peers;
        std::vector<std::pair<std::string, uint16_t>> pending_peers;
        std::vector<std::pair<std::string, uint16_t>> connected_peers;
        std::vector<std::pair<std::string, uint16_t>> disconnected_peers;
        std::vector<std::pair<std::string, uint16_t>> failed_peers;
        std::vector<std::pair<std::string, uint16_t>> banned_peers;
        std::vector<std::pair<std::string, uint16_t>> whitelisted_peers;
        std::vector<std::pair<std::string, uint16_t>> blacklisted_peers;
        std::vector<std::pair<std::string, uint16_t>> pending_connections;
        std::vector<std::pair<std::string, uint16_t>> connected_connections;
        std::string bind_address;
        uint16_t bind_port{0};
        std::string node_id;
        std::string node_name;
        std::string node_version;
        std::string node_listen_address;
        
    };

    struct PeerInfo {
        std::string address;
        uint16_t port{0};
        std::chrono::system_clock::time_point last_seen;
        size_t messages_sent{0};
        size_t messages_received{0};
        bool is_connected{false};
    };

    using MessageHandler = std::function<void(const std::string&, uint16_t, const std::vector<uint8_t>&)>;

    explicit P2PNode(const Config& config);
    ~P2PNode();

    // Prevent copying and moving
    P2PNode(const P2PNode&) = delete;
    P2PNode& operator=(const P2PNode&) = delete;
    P2PNode(P2PNode&&) = delete;
    P2PNode& operator=(P2PNode&&) = delete;

    // Node management
    bool start();
    void stop();
    bool connect_to_peer(const std::string& address, uint16_t port);
    void disconnect_from_peer(const std::string& address, uint16_t port);
    std::vector<PeerInfo> get_connected_peers() const;
    size_t get_connection_count() const;

    // Message handling
    void register_message_handler(MessageHandler handler);
    bool broadcast_message(const std::vector<uint8_t>& message);
    bool send_message_to_peer(const std::string& address, uint16_t port, const std::vector<uint8_t>& message);

    // Peer discovery
    void add_bootstrap_peer(const std::string& address, uint16_t port);
    void discover_peers();
    std::vector<std::pair<std::string, uint16_t>> get_bootstrap_peers() const;

    // Add missing method declaration
    void process_incoming_connections();

    void cleanup() noexcept;

private:
    void accept_connections();
    void manage_connections();
    void handle_incoming_messages();
    void cleanup_disconnected_peers();
    bool validate_peer(const std::string& address, uint16_t port);
    void update_peer_info(const std::string& address, uint16_t port, bool connected);

    // Add missing method declaration
    void handle_nat_traversal_response(const boost::system::error_code& error, size_t bytes_transferred);

    Config config_;
    bool running_{false};
    std::vector<MessageHandler> message_handlers_;
    std::mutex handlers_mutex_;

    struct Impl {
        // Socket and network members
        int server_socket = -1;
        boost::asio::io_context io_context_;
        boost::asio::ip::udp::socket socket_;
        
        // Thread control
        std::atomic<bool> should_stop{false};
        std::thread accept_thread;
        std::thread management_thread;
        std::thread message_thread;
        
        // Connection tracking
        std::unordered_map<std::string, std::shared_ptr<P2PConnection>> connections;
        mutable std::mutex connections_mutex;
        
        // Configuration
        MessageHandler message_handler_;
        const size_t MAX_MESSAGE_SIZE = 65507;

        // Add bootstrap tracking
        std::vector<std::pair<std::string, uint16_t>> bootstrap_peers;
        mutable std::mutex bootstrap_mutex;

        Impl() 
            : socket_(io_context_) {}
    };

    std::unique_ptr<Impl> impl_;
};

} // namespace network
} // namespace quids 