#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <queue>
#include <boost/asio.hpp>
#include <boost/system.hpp>

namespace quids::network {

// Forward declarations
class P2PConnection;

enum class State {
    Disconnected,
    Connecting,
    Connected
};

struct Message {
    std::string sender_address;
    uint16_t sender_port;
    std::vector<uint8_t> data;
};

struct ConnectionStats {
    size_t bytes_sent{0};
    size_t bytes_received{0};
    size_t messages_sent{0};
    size_t messages_received{0};
    std::chrono::system_clock::time_point connected_since{};
};

struct Peer {
    std::string id;
    std::string address;
    uint16_t port;
    std::chrono::system_clock::time_point last_seen;
    ConnectionStats stats;
};

using MessageHandler = std::function<void(const std::string&, uint16_t, const std::vector<uint8_t>&)>;
using PeerConnectedHandler = std::function<void(const std::string&)>;
using PeerDisconnectedHandler = std::function<void(const std::string&)>;

class P2PConnection {
public:
    struct Config {
        uint16_t port{0};
        std::string stun_server;
        uint16_t stun_port{3478};
        bool enable_upnp{false};
        bool enable_nat_pmp{false};
        size_t max_peers{10};
        std::chrono::milliseconds hole_punch_timeout{5000};
        std::chrono::milliseconds keep_alive_interval{30000};
    };

    explicit P2PConnection(const Config& config);
    ~P2PConnection();

    // Prevent copying
    P2PConnection(const P2PConnection&) = delete;
    P2PConnection& operator=(const P2PConnection&) = delete;

    // Allow moving
    P2PConnection(P2PConnection&&) noexcept = default;
    P2PConnection& operator=(P2PConnection&&) noexcept = default;

    bool start();
    void stop();
    bool connect();
    void disconnect();
    bool is_connected() const;
    bool has_message() const;
    std::optional<Message> receive_message();
    bool send_message(const std::string& peer_address, uint16_t peer_port, const std::vector<uint8_t>& message);
    bool broadcast(const std::vector<uint8_t>& message);
    bool perform_nat_traversal(const std::string& address, uint16_t port);
    
    std::string get_address() const;
    uint16_t get_port() const;
    std::chrono::system_clock::time_point get_last_seen() const;
    ConnectionStats get_stats() const;
    void ping();
    
    void set_message_handler(MessageHandler handler);
    void set_peer_connected_handler(PeerConnectedHandler handler);
    void set_peer_disconnected_handler(PeerDisconnectedHandler handler);
    
    std::vector<Peer> get_connected_peers() const;
    bool is_peer_connected(const std::string& peer_id) const;
    void disconnect_peer(const std::string& peer_id);
    bool send_to_peer(const std::string& peer_id, const std::vector<uint8_t>& message);
    std::pair<std::string, uint16_t> get_public_endpoint() const;

    bool perform_stun_request() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    bool setup_port_mapping();
    bool setup_upnp();
    bool setup_nat_pmp();
    void start_hole_punching(const std::string& peer_address, uint16_t peer_port);
    void process_incoming_connections();
    void handle_nat_traversal_response(const boost::system::error_code& error, std::size_t bytes_transferred);
    void maintain_connections();
    void start_receive();
    void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred,
                       std::shared_ptr<std::vector<uint8_t>> buffer,
                       std::shared_ptr<boost::asio::ip::udp::endpoint> sender);
    void handle_error(const std::string& error_msg);
    bool handle_handshake();
    void process_incoming_data();
    void update_stats(size_t bytes_sent, size_t bytes_received);
};

} // namespace quids::network 