#include "network/P2PConnection.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <chrono>
#include <thread>

namespace quids {
namespace network {

struct P2PConnection::Impl {
    int socket_fd{-1};
    std::atomic<bool> should_stop{false};
    std::thread receive_thread;
};

P2PConnection::P2PConnection(const std::string& address, uint16_t port, size_t buffer_size)
    : impl_(std::make_unique<Impl>())
    , address_(address)
    , port_(port)
    , buffer_size_(buffer_size) {
    stats_.last_message = std::chrono::system_clock::now();
}

P2PConnection::~P2PConnection() {
    disconnect();
}

bool P2PConnection::connect() {
    if (state_ != State::DISCONNECTED) {
        return false;
    }

    state_ = State::CONNECTING;
    
    impl_->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->socket_fd < 0) {
        handle_error("Failed to create socket");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, address_.c_str(), &server_addr.sin_addr) <= 0) {
        handle_error("Invalid address");
        return false;
    }

    if (::connect(impl_->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        handle_error("Connection failed");
        return false;
    }

    // Set non-blocking
    int flags = fcntl(impl_->socket_fd, F_GETFL, 0);
    fcntl(impl_->socket_fd, F_SETFL, flags | O_NONBLOCK);

    if (!handle_handshake()) {
        disconnect();
        return false;
    }

    state_ = State::CONNECTED;
    stats_.connected_since = std::chrono::system_clock::now();

    // Start receive thread
    impl_->receive_thread = std::thread([this]() { process_incoming_data(); });

    return true;
}

void P2PConnection::disconnect() {
    if (impl_->socket_fd >= 0) {
        impl_->should_stop = true;
        close(impl_->socket_fd);
        impl_->socket_fd = -1;
        
        if (impl_->receive_thread.joinable()) {
            impl_->receive_thread.join();
        }
    }
    
    state_ = State::DISCONNECTED;
}

bool P2PConnection::send_message(const std::vector<uint8_t>& message) {
    if (!is_connected() || message.empty()) {
        return false;
    }

    // Prepend message size
    uint32_t size = message.size();
    std::vector<uint8_t> buffer(sizeof(size) + size);
    memcpy(buffer.data(), &size, sizeof(size));
    memcpy(buffer.data() + sizeof(size), message.data(), size);

    ssize_t total_sent = 0;
    while (total_sent < buffer.size()) {
        ssize_t sent = send(impl_->socket_fd, 
                          buffer.data() + total_sent,
                          buffer.size() - total_sent, 
                          0);
        
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            handle_error("Send failed");
            return false;
        }
        
        total_sent += sent;
    }

    update_stats(total_sent, 0);
    stats_.messages_sent++;
    return true;
}

std::vector<uint8_t> P2PConnection::receive_message() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (message_queue_.empty()) {
        return {};
    }
    
    auto message = std::move(message_queue_.front());
    message_queue_.pop();
    return message;
}

bool P2PConnection::has_message() const {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return !message_queue_.empty();
}

std::chrono::system_clock::time_point P2PConnection::get_last_seen() const {
    return stats_.last_message;
}

bool P2PConnection::verify_connection() {
    return ping() && is_connected();
}

bool P2PConnection::ping() {
    std::vector<uint8_t> ping_message = {'P', 'I', 'N', 'G'};
    return send_message(ping_message);
}

bool P2PConnection::pong() {
    std::vector<uint8_t> pong_message = {'P', 'O', 'N', 'G'};
    return send_message(pong_message);
}

bool P2PConnection::handle_handshake() {
    state_ = State::HANDSHAKING;
    
    // Simple handshake - send version and wait for acknowledgment
    std::vector<uint8_t> version = {0x01, 0x00}; // Version 1.0
    if (!send_message(version)) {
        return false;
    }

    // Wait for response with timeout
    auto start = std::chrono::system_clock::now();
    while (std::chrono::system_clock::now() - start < std::chrono::seconds(5)) {
        std::vector<uint8_t> buffer(2);
        ssize_t received = recv(impl_->socket_fd, buffer.data(), buffer.size(), 0);
        
        if (received == buffer.size() && buffer[0] == 0x01 && buffer[1] == 0x00) {
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return false;
}

void P2PConnection::update_stats(size_t bytes_sent, size_t bytes_received) {
    stats_.bytes_sent += bytes_sent;
    stats_.bytes_received += bytes_received;
    stats_.last_message = std::chrono::system_clock::now();
}

void P2PConnection::handle_error(const std::string& error) {
    stats_.errors++;
    state_ = State::ERROR;
    std::cerr << "P2PConnection error: " << error << " (" << strerror(errno) << ")" << std::endl;
}

bool P2PConnection::validate_message(const std::vector<uint8_t>& message) {
    return !message.empty() && message.size() <= buffer_size_;
}

void P2PConnection::process_incoming_data() {
    std::vector<uint8_t> size_buffer(sizeof(uint32_t));
    std::vector<uint8_t> message_buffer;

    while (!impl_->should_stop) {
        // Read message size
        ssize_t received = recv(impl_->socket_fd, 
                              size_buffer.data(), 
                              size_buffer.size(),
                              0);
        
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            handle_error("Receive failed");
            break;
        }
        
        if (received == 0) {
            // Connection closed by peer
            state_ = State::DISCONNECTED;
            break;
        }

        uint32_t message_size;
        memcpy(&message_size, size_buffer.data(), sizeof(message_size));

        if (message_size > buffer_size_) {
            handle_error("Message too large");
            continue;
        }

        // Read message content
        message_buffer.resize(message_size);
        size_t total_received = 0;

        while (total_received < message_size) {
            received = recv(impl_->socket_fd,
                          message_buffer.data() + total_received,
                          message_size - total_received,
                          0);
            
            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                handle_error("Receive failed");
                break;
            }
            
            if (received == 0) {
                state_ = State::DISCONNECTED;
                return;
            }

            total_received += received;
        }

        if (validate_message(message_buffer)) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            message_queue_.push(message_buffer);
            queue_cv_.notify_one();
            
            update_stats(0, total_received);
            stats_.messages_received++;

            // Handle ping/pong
            if (message_buffer == std::vector<uint8_t>{'P', 'I', 'N', 'G'}) {
                pong();
            }
        }
    }
}

} // namespace network
} // namespace quids 