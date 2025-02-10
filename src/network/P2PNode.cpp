#include "network/P2PNode.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <mutex>
#include <atomic>

using namespace std::chrono_literals;

namespace quids {
namespace network {

P2PNode::P2PNode(const Config& config)
    : config_(config)
    , impl_(std::make_unique<Impl>()) {
    
    int retries = 3;
    int current_port = config.port;
    
    while (retries-- > 0) {
        try {
            // Socket creation
            impl_->server_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (impl_->server_socket < 0) {
                throw std::system_error(errno, std::generic_category(), 
                    "Socket creation failed");
            }

            // Set socket options
            int opt = 1;
            if (setsockopt(impl_->server_socket, SOL_SOCKET, 
                SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
                throw std::system_error(errno, std::generic_category(),
                    "Setsockopt failed");
            }

            // Bind
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(current_port);
            
            if (bind(impl_->server_socket, 
                (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                throw std::system_error(errno, std::generic_category(),
                    "Bind failed on port " + std::to_string(current_port));
            }

            // Listen
            if (listen(impl_->server_socket, 10) < 0) {
                throw std::system_error(errno, std::generic_category(),
                    "Listen failed");
            }

            SPDLOG_INFO("P2P node listening on port {}", current_port);
            return; // Success
            
        } catch (const std::system_error& e) {
            cleanup();
            current_port++;
            SPDLOG_WARN("Retrying... ({}/3 remaining)", retries);
            std::this_thread::sleep_for(1s);
        }
    }
    
    throw std::runtime_error("Failed to initialize P2P node after 3 attempts");
}

void P2PNode::cleanup() noexcept {
    if (impl_->server_socket >= 0) {
        close(impl_->server_socket);
        impl_->server_socket = -1;
    }
}

P2PNode::~P2PNode() {
    // Signal stop
    impl_->should_stop = true;
    
    // Close sockets
    if (impl_->server_socket >= 0) {
        close(impl_->server_socket);
        impl_->server_socket = -1;
    }
    
    // Join threads
    if (impl_->accept_thread.joinable()) impl_->accept_thread.join();
    if (impl_->management_thread.joinable()) impl_->management_thread.join();
    if (impl_->message_thread.joinable()) impl_->message_thread.join();
}

bool P2PNode::start() {
    if (running_) {
        return false;
    }

    impl_->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->server_socket < 0) {
        std::cerr << "Failed to create server socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(impl_->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        close(impl_->server_socket);
        return false;
    }

    // Bind socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config_.port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(impl_->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        close(impl_->server_socket);
        return false;
    }

    // Listen for connections
    if (listen(impl_->server_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
        close(impl_->server_socket);
        return false;
    }

    running_ = true;
    impl_->should_stop = false;

    // Start worker threads
    impl_->accept_thread = std::thread(&P2PNode::accept_connections, this);
    impl_->management_thread = std::thread(&P2PNode::manage_connections, this);
    impl_->message_thread = std::thread(&P2PNode::handle_incoming_messages, this);

    std::cout << "P2P node started on port " << config_.port << std::endl;
    return true;
}

void P2PNode::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    impl_->should_stop = true;

    // Close server socket
    if (impl_->server_socket >= 0) {
        close(impl_->server_socket);
        impl_->server_socket = -1;
    }

    // Wait for threads to finish
    if (impl_->accept_thread.joinable()) {
        impl_->accept_thread.join();
    }
    if (impl_->management_thread.joinable()) {
        impl_->management_thread.join();
    }
    if (impl_->message_thread.joinable()) {
        impl_->message_thread.join();
    }

    // Close all connections
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    impl_->connections.clear();

    std::cout << "P2P node stopped" << std::endl;
}

bool P2PNode::connect_to_peer(const std::string& address, uint16_t port) {
    if (!running_ || !validate_peer(address, port)) {
        return false;
    }

    std::string peer_key = address + ":" + std::to_string(port);
    
    // Check if already connected
    {
        std::lock_guard<std::mutex> lock(impl_->connections_mutex);
        if (impl_->connections.find(peer_key) != impl_->connections.end()) {
            return true;
        }
    }

    // Create connection config
    P2PConnection::Config conn_config;
    conn_config.port = config_.port;
    conn_config.stun_server = config_.stun_server;
    conn_config.stun_port = config_.stun_port;
    conn_config.enable_upnp = config_.enable_upnp;
    conn_config.enable_nat_pmp = config_.enable_nat_pmp;
    conn_config.max_peers = config_.max_peers;
    conn_config.hole_punch_timeout = std::chrono::milliseconds(config_.connection_timeout_ms);
    conn_config.keep_alive_interval = std::chrono::milliseconds(config_.ping_interval_ms);

    // Create and start connection
    auto connection = std::make_shared<P2PConnection>(conn_config);
    if (!connection->start()) {
        return false;
    }

    // Perform NAT traversal
    if (!connection->perform_nat_traversal(address, port)) {
        connection->stop();
        return false;
    }

    // Store connection
    {
        std::lock_guard<std::mutex> lock(impl_->connections_mutex);
        impl_->connections[peer_key] = connection;
    }

    update_peer_info(address, port, true);
    return true;
}

void P2PNode::disconnect_from_peer(const std::string& address, uint16_t port) {
    std::string peer_key = address + ":" + std::to_string(port);
    
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(peer_key);
    if (it != impl_->connections.end()) {
        it->second->disconnect();
        impl_->connections.erase(it);
        update_peer_info(address, port, false);
    }
}

std::vector<P2PNode::PeerInfo> P2PNode::get_connected_peers() const {
    std::vector<PeerInfo> peers;
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    
    for (const auto& [key, connection] : impl_->connections) {
        if (connection->is_connected()) {
            PeerInfo info;
            info.address = connection->get_address();
            info.port = connection->get_port();
            info.last_seen = connection->get_last_seen();
            const auto& stats = connection->get_stats();
            info.messages_received = stats.messages_received;
            info.messages_sent = stats.messages_sent;
            info.is_connected = true;
            peers.push_back(info);
        }
    }
    
    return peers;
}

size_t P2PNode::get_connection_count() const {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    return impl_->connections.size();
}

void P2PNode::register_message_handler(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    message_handlers_.push_back(std::move(handler));
}

bool P2PNode::broadcast_message(const std::vector<uint8_t>& message) {
    if (!running_ || message.empty()) {
        return false;
    }

    bool success = false;
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    
    for (const auto& [key, connection] : impl_->connections) {
        if (connection->is_connected()) {
            std::string address = connection->get_address();
            uint16_t port = connection->get_port();
            if (connection->send_message(address, port, message)) {
                success = true;
            }
        }
    }
    
    return success;
}

bool P2PNode::send_message_to_peer(const std::string& address, 
                                 uint16_t port,
                                 const std::vector<uint8_t>& message) {
    if (!running_ || message.empty()) {
        return false;
    }

    std::string peer_key = address + ":" + std::to_string(port);
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    
    auto it = impl_->connections.find(peer_key);
    if (it != impl_->connections.end() && it->second->is_connected()) {
        return it->second->send_message(address, port, message);
    }
    
    return false;
}

void P2PNode::add_bootstrap_peer(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(impl_->bootstrap_mutex);
    impl_->bootstrap_peers.emplace_back(address, port);
}

std::vector<std::pair<std::string, uint16_t>> P2PNode::get_bootstrap_peers() const {
    std::vector<std::pair<std::string, uint16_t>> peers;
    {
        std::lock_guard<std::mutex> lock(impl_->bootstrap_mutex);
        peers = impl_->bootstrap_peers;
    }
    return peers;
}

void P2PNode::discover_peers() {
    std::vector<std::pair<std::string, uint16_t>> peers;
    {
        std::lock_guard<std::mutex> lock(impl_->bootstrap_mutex);
        peers = impl_->bootstrap_peers;
    }

    for (const auto& [address, port] : peers) {
        connect_to_peer(address, port);
    }
}

void P2PNode::accept_connections() {
    while (!impl_->should_stop) {
        try {
            // Accept new connection
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_socket = accept(impl_->server_socket,
                                     (struct sockaddr*)&client_addr,
                                     &addr_len);

            if (client_socket < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "Accept failed: " << strerror(errno) << std::endl;
                }
                continue;
            }

            // Get client info
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            uint16_t client_port = ntohs(client_addr.sin_port);

            // Create connection config
            P2PConnection::Config conn_config;
            conn_config.port = config_.port;
            conn_config.stun_server = config_.stun_server;
            conn_config.stun_port = config_.stun_port;
            conn_config.enable_upnp = config_.enable_upnp;
            conn_config.enable_nat_pmp = config_.enable_nat_pmp;
            conn_config.max_peers = config_.max_peers;
            conn_config.hole_punch_timeout = std::chrono::milliseconds(config_.connection_timeout_ms);
            conn_config.keep_alive_interval = std::chrono::milliseconds(config_.ping_interval_ms);

            // Create and start connection
            auto connection = std::make_shared<P2PConnection>(conn_config);
            if (connection->start()) {
                std::string peer_key = std::string(client_ip) + ":" + std::to_string(client_port);
                {
                    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
                    impl_->connections[peer_key] = connection;
                }
                
                update_peer_info(client_ip, client_port, true);
            } else {
                close(client_socket);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error accepting connection: " << e.what() << std::endl;
        }
    }
}

void P2PNode::manage_connections() {
    while (running_ && !impl_->should_stop) {
        cleanup_disconnected_peers();
        
        // Check connection health
        std::lock_guard<std::mutex> lock(impl_->connections_mutex);
        for (auto& [key, connection] : impl_->connections) {
            if (connection->is_connected()) {
                auto last_seen = connection->get_last_seen();
                auto now = std::chrono::system_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_seen).count();
                
                if (duration > static_cast<int64_t>(config_.connection_timeout_ms)) {
                    connection->disconnect();
                } else if (duration > static_cast<int64_t>(config_.ping_interval_ms)) {
                    connection->ping();
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void P2PNode::handle_incoming_messages() {
    while (!impl_->should_stop) {
        std::lock_guard<std::mutex> lock(impl_->connections_mutex);
        for (auto& [peer_key, connection] : impl_->connections) {
            while (connection->has_message()) {
                auto message = connection->receive_message();
                if (!message) continue;

                // Update peer info
                update_peer_info(message->sender_address, message->sender_port, true);

                // Notify handlers
                std::lock_guard<std::mutex> lock(handlers_mutex_);
                for (const auto& handler : message_handlers_) {
                    handler(message->sender_address, message->sender_port, message->data);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void P2PNode::cleanup_disconnected_peers() {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    for (auto it = impl_->connections.begin(); it != impl_->connections.end();) {
        if (!it->second->is_connected()) {
            update_peer_info(it->second->get_address(), it->second->get_port(), false);
            it = impl_->connections.erase(it);
        } else {
            ++it;
        }
    }
}

bool P2PNode::validate_peer(const std::string& address, uint16_t port) {
    if (address.empty() || port == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    return impl_->connections.size() < config_.max_connections;
}

void P2PNode::update_peer_info(const std::string& address, uint16_t port, bool connected) {
    // This method can be extended to maintain persistent peer information
    // For now, it just logs connection status changes
    std::cout << "Peer " << address << ":" << port 
              << (connected ? " connected" : " disconnected") << std::endl;
}

void P2PNode::process_incoming_connections() {
    auto buffer = std::make_shared<std::vector<uint8_t>>(impl_->MAX_MESSAGE_SIZE);
    auto sender = std::make_shared<boost::asio::ip::udp::endpoint>();

    impl_->socket_.async_receive_from(
        boost::asio::buffer(*buffer), *sender,
        [this, buffer, sender](const boost::system::error_code& error, size_t len) {
            if (!error) {
                // Handle NAT traversal first
                if (len >= 4 && std::memcmp(buffer->data(), "PUNCH", 4) == 0) {
                    handle_nat_traversal_response(error, len);
                } else if (impl_->message_handler_) {
                    impl_->message_handler_(
                        sender->address().to_string(),
                        sender->port(),
                        std::vector<uint8_t>(buffer->begin(), buffer->begin() + len)
                    );
                }
            }
            process_incoming_connections(); // Continue listening
        }
    );
}

void P2PNode::handle_nat_traversal_response(
    const boost::system::error_code& error, 
    size_t bytes_transferred
) {
    (void)error; // Explicitly mark as unused
    (void)bytes_transferred;
    
    // Implementation using the parameters
    if (!error && bytes_transferred > 0) {
        // Actual handling code
    }
}

} // namespace network
} // namespace quids 