#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace quids {
namespace network {

class P2PConnection {
public:
    enum class State {
        DISCONNECTED,
        CONNECTING,
        HANDSHAKING,
        CONNECTED,
        ERROR
    };

    struct Stats {
        size_t bytes_sent{0};
        size_t bytes_received{0};
        size_t messages_sent{0};
        size_t messages_received{0};
        size_t errors{0};
        std::chrono::system_clock::time_point last_message;
        std::chrono::system_clock::time_point connected_since;
    };

    P2PConnection(const std::string& address, uint16_t port, size_t buffer_size);
    ~P2PConnection();

    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const { return state_ == State::CONNECTED; }
    State get_state() const { return state_; }

    // Message handling
    bool send_message(const std::vector<uint8_t>& message);
    std::vector<uint8_t> receive_message();
    bool has_message() const;

    // Connection info
    const std::string& get_address() const { return address_; }
    uint16_t get_port() const { return port_; }
    const Stats& get_stats() const { return stats_; }
    std::chrono::system_clock::time_point get_last_seen() const;

    // Connection validation
    bool verify_connection();
    bool ping();
    bool pong();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string address_;
    uint16_t port_;
    size_t buffer_size_;
    std::atomic<State> state_{State::DISCONNECTED};
    Stats stats_;

    // Message queue
    std::queue<std::vector<uint8_t>> message_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Internal methods
    bool handle_handshake();
    void update_stats(size_t bytes_sent, size_t bytes_received);
    void handle_error(const std::string& error);
    bool validate_message(const std::vector<uint8_t>& message);
    void process_incoming_data();
};

} // namespace network
} // namespace quids 