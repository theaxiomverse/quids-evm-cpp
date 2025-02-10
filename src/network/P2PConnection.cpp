#include "network/P2PConnection.hpp"
#include "network/P2PNetwork.hpp"
#include "network/STUNClient.hpp"
#include "network/UPnPClient.hpp"
#include "network/NATPMP.hpp"

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
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <natpmp.h>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <spdlog/spdlog.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <optional>
#include <vector>
#include <cstdint>

using namespace std::chrono_literals; // For ms literal

namespace quids::network {

namespace {
    constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB
    constexpr uint32_t STUN_MAGIC_COOKIE = 0x2112A442;
}

struct P2PConnection::Impl {
    Config config_;
    boost::asio::io_context io_context_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::steady_timer timer_;
    std::atomic<bool> should_stop{false};
    std::thread receive_thread;
    std::unordered_map<std::string, Peer> peers_;
    mutable std::mutex peers_mutex_;
    MessageHandler message_handler_;
    PeerConnectedHandler peer_connected_handler_;
    PeerDisconnectedHandler peer_disconnected_handler_;
    std::string public_ip_;
    uint16_t public_port_{0};
    std::thread io_thread_;
    std::queue<Message> message_queue_;
    mutable std::mutex message_queue_mutex_;
    std::string local_address_;
    State state_{State::Disconnected};
    ConnectionStats stats_;
    bool running_{false};

    explicit Impl(const Config& config)
        : config_(config)
        , socket_(io_context_)
        , timer_(io_context_)
        , state_(State::Disconnected) {
        stats_ = ConnectionStats{};
    }
};

P2PConnection::P2PConnection(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    
    try {
        // Open socket
        impl_->socket_.open(boost::asio::ip::udp::v4());
        
        // Set socket options
        impl_->socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        impl_->socket_.set_option(boost::asio::socket_base::broadcast(true));
        
        // Bind to port
        impl_->socket_.bind(boost::asio::ip::udp::endpoint(
            boost::asio::ip::udp::v4(),
            impl_->config_.port
        ));

        spdlog::info("P2P connection initialized on port {}", impl_->config_.port);
        
        // Start background thread for io_context
        impl_->running_ = true;
        impl_->io_thread_ = std::thread([this]() {
            while (impl_->running_) {
                try {
                    impl_->io_context_.run();
                } catch (const std::exception& e) {
                    spdlog::error("IO context error: {}", e.what());
                }
                impl_->io_context_.restart();
            }
        });

        // Setup UPnP if enabled
        if (impl_->config_.enable_upnp) {
            if (setup_upnp()) {
                spdlog::info("UPnP port mapping successful");
            } else {
                spdlog::warn("UPnP port mapping failed");
            }
        }

        // Start receiving messages
        start_receive();

    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize P2P connection: {}", e.what());
        throw;
    }
}

P2PConnection::~P2PConnection() {
    if (impl_) {
        // Stop IO context
        impl_->running_ = false;
        impl_->io_context_.stop();

        // Close socket
        if (impl_->socket_.is_open()) {
            boost::system::error_code ec;
            impl_->socket_.close(ec);
            if (ec) {
                spdlog::error("Error closing socket: {}", ec.message());
            }
        }

        // Join IO thread
        if (impl_->io_thread_.joinable()) {
            impl_->io_thread_.join();
        }

        // Remove UPnP port mapping if it was set up
        if (impl_->config_.enable_upnp) {
            try {
                UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, nullptr);
                if (devlist) {
                    UPNPUrls urls;
                    IGDdatas data;
                    char lanaddr[64] = {0};
                    char wanaddr[64] = {0};

                    if (UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr),
                                       wanaddr, sizeof(wanaddr)) == 1) {
                        char port_str[16];
                        snprintf(port_str, sizeof(port_str), "%d", impl_->config_.port);
                        
                        UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype,
                                            port_str, "UDP", nullptr);
                        
                        FreeUPNPUrls(&urls);
                    }
                    freeUPNPDevlist(devlist);
                }
            } catch (const std::exception& e) {
                spdlog::error("Error removing UPnP port mapping: {}", e.what());
            }
        }
    }
}

bool P2PConnection::connect() {
    if (impl_->state_ != State::Disconnected) {
        return false;
    }

    impl_->state_ = State::Connecting;
    
    try {
        // Start receiving messages if not already started
        if (!impl_->socket_.is_open()) {
            impl_->socket_.open(boost::asio::ip::udp::v4());
            start_receive();
        }

        // Start connection maintenance timer
        impl_->timer_.expires_after(impl_->config_.keep_alive_interval);
        impl_->timer_.async_wait([this](const boost::system::error_code& error) {
            if (!error) {
                maintain_connections();
            }
        });

        impl_->state_ = State::Connected;
        impl_->stats_.connected_since = std::chrono::system_clock::now();
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect: {}", e.what());
        impl_->state_ = State::Disconnected;
        return false;
    }
}

void P2PConnection::disconnect() {
    if (impl_->state_ == State::Disconnected) {
        return;
    }

    try {
        // Cancel timer
        impl_->timer_.cancel();

        // Close socket
        if (impl_->socket_.is_open()) {
            boost::system::error_code ec;
            impl_->socket_.close(ec);
            if (ec) {
                spdlog::error("Error closing socket: {}", ec.message());
            }
        }

        // Clear message queue
        {
            std::lock_guard<std::mutex> lock(impl_->message_queue_mutex_);
            std::queue<Message> empty;
            std::swap(impl_->message_queue_, empty);
        }

        impl_->state_ = State::Disconnected;
        
        // Notify peer disconnected handler if set
        if (impl_->peer_disconnected_handler_) {
            impl_->peer_disconnected_handler_(""); // Notify with empty peer ID for general disconnect
        }

    } catch (const std::exception& e) {
        spdlog::error("Error during disconnect: {}", e.what());
    }
}

bool P2PConnection::is_connected() const {
    return impl_->state_ == State::Connected && impl_->socket_.is_open();
}

bool P2PConnection::has_message() const {
    std::lock_guard<std::mutex> lock(impl_->message_queue_mutex_);
    return !impl_->message_queue_.empty();
}

std::optional<Message> P2PConnection::receive_message() {
    std::lock_guard<std::mutex> lock(impl_->message_queue_mutex_);
    if (impl_->message_queue_.empty()) {
        return std::nullopt;
    }
    
    Message msg = std::move(impl_->message_queue_.front());
    impl_->message_queue_.pop();
    return msg;
}

bool P2PConnection::send_message(const std::string& peer_address, uint16_t peer_port, const std::vector<uint8_t>& message) {
    try {
        boost::asio::ip::udp::endpoint peer_endpoint(
            boost::asio::ip::make_address_v4(peer_address),
            peer_port
        );

        // Create packet with size header
        std::vector<uint8_t> packet;
        packet.reserve(message.size() + 4);
        
        // Add size header (big endian)
        uint32_t size = message.size();
        packet.push_back((size >> 24) & 0xFF);
        packet.push_back((size >> 16) & 0xFF);
        packet.push_back((size >> 8) & 0xFF);
        packet.push_back(size & 0xFF);
        
        // Add message content
        packet.insert(packet.end(), message.begin(), message.end());
        
        impl_->socket_.send_to(boost::asio::buffer(packet), peer_endpoint);
        update_stats(packet.size(), 0);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to send message: {}", e.what());
        return false;
    }
}

void P2PConnection::handle_error(const std::string& error_msg) {
    spdlog::error("P2P connection error: {}", error_msg);
}

bool P2PConnection::handle_handshake() {
    // Send handshake message
    std::vector<uint8_t> handshake = {'H', 'E', 'L', 'O'};
    return send_message(impl_->public_ip_, impl_->public_port_, handshake);
}

void P2PConnection::process_incoming_data() {
    std::vector<uint8_t> buffer(MAX_MESSAGE_SIZE);
    
    while (!impl_->should_stop) {
        try {
            boost::system::error_code ec;
            boost::asio::ip::udp::endpoint sender_endpoint;
            
            size_t len = impl_->socket_.receive_from(
                boost::asio::buffer(buffer),
                sender_endpoint,
                0,
                ec
            );
            
            if (ec) {
                if (ec != boost::asio::error::operation_aborted) {
                    handle_error("Receive failed: " + ec.message());
                }
                break;
            }
            
            if (len > 0) {
                std::vector<uint8_t> message(buffer.begin(), buffer.begin() + len);
                
                {
                    std::lock_guard<std::mutex> lock(impl_->message_queue_mutex_);
                    impl_->message_queue_.push(Message{
                        sender_endpoint.address().to_string(),
                        sender_endpoint.port(),
                        std::move(message)
                    });
                }
                
                update_stats(0, len);
                
                if (impl_->message_handler_) {
                    impl_->message_handler_(
                        sender_endpoint.address().to_string(),
                        sender_endpoint.port(),
                        message
                    );
                }
            }
        } catch (const std::exception& e) {
            if (!impl_->should_stop) {
                handle_error(e.what());
            }
        }
    }
}

void P2PConnection::update_stats(size_t bytes_sent, size_t bytes_received) {
    impl_->stats_.bytes_sent += bytes_sent;
    impl_->stats_.bytes_received += bytes_received;
    if (bytes_sent > 0) impl_->stats_.messages_sent++;
    if (bytes_received > 0) impl_->stats_.messages_received++;
}

std::string P2PConnection::get_address() const {
    return impl_->local_address_;
}

uint16_t P2PConnection::get_port() const {
    return impl_->config_.port;
}

std::chrono::system_clock::time_point P2PConnection::get_last_seen() const {
    return impl_->stats_.connected_since;
}

ConnectionStats P2PConnection::get_stats() const {
    return impl_->stats_;
}

void P2PConnection::ping() {
    std::vector<uint8_t> ping_message = {'P', 'I', 'N', 'G'};
    send_message(impl_->public_ip_, impl_->public_port_, ping_message);
}

bool P2PConnection::start() {
    try {
        // Setup socket
        impl_->socket_.open(boost::asio::ip::udp::v4());
        impl_->socket_.bind(boost::asio::ip::udp::endpoint(
            boost::asio::ip::udp::v4(), impl_->config_.port));

        // Setup NAT traversal
        if (!setup_port_mapping()) {
            spdlog::warn("Failed to setup port mapping");
        }

        // Get public endpoint
        auto [public_ip, public_port] = get_public_endpoint();
        impl_->public_ip_ = public_ip;
        impl_->public_port_ = public_port;

        // Start IO thread
        impl_->io_thread_ = std::thread([this]() {
            process_incoming_connections();
        });

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start P2P connection: {}", e.what());
        return false;
    }
}

void P2PConnection::stop() {
    impl_->should_stop = true;
    impl_->socket_.close();
    if (impl_->io_thread_.joinable()) {
        impl_->io_thread_.join();
    }
}

bool P2PConnection::setup_port_mapping() {
    bool success = false;
    
    if (impl_->config_.enable_upnp) {
        success = setup_upnp();
    }
    
    if (!success && impl_->config_.enable_nat_pmp) {
        success = setup_nat_pmp();
    }
    
    return success;
}

bool P2PConnection::setup_upnp() {
    UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, nullptr);
    if (!devlist) {
        spdlog::error("No UPnP devices found");
        return false;
    }

    UPNPUrls urls;
    IGDdatas data;
    char lanaddr[64] = {0};
    char wanaddr[64] = {0};

    int status = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr), 
                                 wanaddr, sizeof(wanaddr));
    
    freeUPNPDevlist(devlist);

    if (status != 1) {
        spdlog::error("No valid IGD found");
        return false;
    }

    // Add port mapping
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", impl_->config_.port);

    int error = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                  port_str, port_str, lanaddr,
                                  "QUIDS P2P", "UDP", nullptr, "0");

    FreeUPNPUrls(&urls);
    
    return error == 0;
}

bool P2PConnection::setup_nat_pmp() {
    natpmp_t natpmp;
    int ret = initnatpmp(&natpmp, 0, 0);
    if (ret != 0) {
        return false;
    }

    int r = sendnewportmappingrequest(
        &natpmp, NATPMP_PROTOCOL_UDP,
        impl_->config_.port, impl_->config_.port, 3600);
    if (r < 0) {
        closenatpmp(&natpmp);
        return false;
    }

    natpmpresp_t response;
    do {
        r = readnatpmpresponseorretry(&natpmp, &response);
    } while (r == NATPMP_TRYAGAIN);

    closenatpmp(&natpmp);
    return r == 0;
}

std::pair<std::string, uint16_t> P2PConnection::get_public_endpoint() const {
    if (!perform_stun_request()) {
        return {"", 0};
    }
    return {impl_->public_ip_, impl_->public_port_};
}

bool P2PConnection::perform_stun_request() const {
    try {
        // Create STUN message
        std::vector<uint8_t> request(20);
        request[0] = 0x00; // Request type
        request[1] = 0x01;
        request[2] = 0x00; // Message length
        request[3] = 0x00;
        
        // Magic cookie
        uint32_t cookie = htonl(STUN_MAGIC_COOKIE);
        memcpy(&request[4], &cookie, 4);
        
        // Transaction ID
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (int i = 8; i < 20; ++i) {
            request[i] = static_cast<uint8_t>(dis(gen));
        }

        // Send request to STUN server
        boost::asio::ip::udp::resolver resolver(impl_->io_context_);
        auto endpoints = resolver.resolve(
            impl_->config_.stun_server,
            std::to_string(impl_->config_.stun_port)
        );

        impl_->socket_.send_to(
            boost::asio::buffer(request),
            *endpoints.begin()
        );

        // Receive response
        std::vector<uint8_t> response(512);
        boost::asio::ip::udp::endpoint sender_endpoint;
        size_t len = impl_->socket_.receive_from(
            boost::asio::buffer(response),
            sender_endpoint
        );

        // Parse XOR-MAPPED-ADDRESS
        if (len >= 20) {
            size_t pos = 20;
            while (pos + 4 <= len) {
                uint16_t type = (response[pos] << 8) | response[pos + 1];
                uint16_t length = (response[pos + 2] << 8) | response[pos + 3];
                
                if (type == 0x0020) { // XOR-MAPPED-ADDRESS
                    if (length >= 8) {
                        uint16_t port = ((response[pos + 6] << 8) | response[pos + 7]) ^ (STUN_MAGIC_COOKIE >> 16);
                        uint32_t ip;
                        memcpy(&ip, &response[pos + 8], 4);
                        ip ^= STUN_MAGIC_COOKIE;
                        
                        char ip_str[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &ip, ip_str, INET_ADDRSTRLEN);
                        
                        impl_->public_ip_ = ip_str;
                        impl_->public_port_ = port;
                        return true;
                    }
                }
                pos += 4 + length;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("STUN request failed: {}", e.what());
    }
    return false;
}

bool P2PConnection::perform_nat_traversal(const std::string& address, uint16_t port) {
    // Access config through impl_
    std::string public_ip;
    uint16_t public_port;
    if(!STUNClient::get_mapped_address(impl_->config_.stun_server, 
                                      impl_->config_.stun_port,
                                      public_ip, public_port)) {
        SPDLOG_ERROR("STUN request failed");
        return false;
    }

    bool mapping_success = false;
    if(impl_->config_.enable_upnp) {
        mapping_success = UPnPClient::add_port_mapping(
            public_port, impl_->config_.port, "TCP", "Quids P2P");
    } else if(impl_->config_.enable_nat_pmp) {
        mapping_success = NATPMP::map_port(impl_->config_.port, public_port, 
                                         NATPMP::Protocol::TCP, 3600);
    }

    // Use impl_->socket_ instead of socket_
    boost::asio::ip::udp::endpoint peer_endpoint(
        boost::asio::ip::make_address_v4(address),
        port
    );

    for(int i = 0; i < 3; ++i) {
        impl_->socket_.send_to(
            boost::asio::buffer("PUNCH", 5), 
            peer_endpoint
        );
        std::this_thread::sleep_for(100ms);
        
        if(has_message()) {
            return true;
        }
    }

    return mapping_success;
}

void P2PConnection::start_hole_punching(
    const std::string& peer_address,
    uint16_t peer_port
) {
    try {
        boost::asio::ip::udp::endpoint peer_endpoint(
            boost::asio::ip::make_address_v4(peer_address),
            peer_port
        );

        // Send hole punching packets
        std::vector<uint8_t> packet{'H', 'O', 'L', 'E'};
        
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < impl_->config_.hole_punch_timeout) {
            impl_->socket_.send_to(boost::asio::buffer(packet), peer_endpoint);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        spdlog::error("Hole punching failed: {}", e.what());
    }
}

void P2PConnection::process_incoming_connections() {
    std::vector<uint8_t> buffer(MAX_MESSAGE_SIZE);
    boost::asio::ip::udp::endpoint sender_endpoint;

    while (!impl_->should_stop) {
        try {
            size_t len = impl_->socket_.receive_from(
                boost::asio::buffer(buffer),
                sender_endpoint
            );

            if (len >= 4) {
                std::string peer_id = sender_endpoint.address().to_string() + ":" + 
                                    std::to_string(sender_endpoint.port());
                
                if (std::string(buffer.begin(), buffer.begin() + 4) == "HOLE") {
                    // Handle hole punching response
                    handle_nat_traversal_response(boost::system::error_code(), len);
                } else if (impl_->message_handler_) {
                    // Handle regular message
                    impl_->message_handler_(
                        sender_endpoint.address().to_string(),
                        sender_endpoint.port(),
                        std::vector<uint8_t>(buffer.begin(), buffer.begin() + len)
                    );
                }
            }
        } catch (const std::exception& e) {
            if (!impl_->should_stop) {
                spdlog::error("Error processing incoming connection: {}", e.what());
            }
        }
    }
}

void P2PConnection::handle_nat_traversal_response(
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
    if (error) {
        spdlog::error("NAT traversal response error: {}", error.message());
        return;
    }

    if (bytes_transferred < 4) {
        spdlog::error("Invalid NAT traversal response size");
        return;
    }

    impl_->state_ = State::Connected;
    spdlog::info("NAT traversal successful");
}

void P2PConnection::maintain_connections() {
    if (impl_->state_ != State::Connected) {
        return;
    }

    try {
        // Send keep-alive messages to all connected peers
        std::vector<uint8_t> keep_alive = {'P', 'I', 'N', 'G'};
        for (const auto& [peer_id, peer] : impl_->peers_) {
            send_message(peer.address, peer.port, keep_alive);
        }

        // Schedule next maintenance
        impl_->timer_.expires_after(impl_->config_.keep_alive_interval);
        impl_->timer_.async_wait([this](const boost::system::error_code& error) {
            if (!error) {
                maintain_connections();
            }
        });
    } catch (const std::exception& e) {
        spdlog::error("Error in connection maintenance: {}", e.what());
    }
}

bool P2PConnection::broadcast(const std::vector<uint8_t>& message) {
    if (impl_->state_ != State::Connected) {
        return false;
    }

    try {
        // Create broadcast endpoint
        boost::asio::ip::udp::endpoint broadcast_endpoint(
            boost::asio::ip::address_v4::broadcast(),
            impl_->config_.port
        );

        // Add message header with size
        std::vector<uint8_t> packet;
        packet.reserve(4 + message.size());
        
        // Add size header (big endian)
        uint32_t size = message.size();
        packet.push_back((size >> 24) & 0xFF);
        packet.push_back((size >> 16) & 0xFF);
        packet.push_back((size >> 8) & 0xFF);
        packet.push_back(size & 0xFF);
        
        // Add message content
        packet.insert(packet.end(), message.begin(), message.end());

        impl_->socket_.send_to(boost::asio::buffer(packet), broadcast_endpoint);
        
        // Update stats
        impl_->stats_.messages_sent++;
        impl_->stats_.bytes_sent += packet.size();
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to broadcast message: {}", e.what());
        return false;
    }
}

void P2PConnection::start_receive() {
    auto buffer = std::make_shared<std::vector<uint8_t>>(MAX_MESSAGE_SIZE);
    auto sender = std::make_shared<boost::asio::ip::udp::endpoint>();

    impl_->socket_.async_receive_from(
        boost::asio::buffer(*buffer), *sender,
        [this, buffer, sender](const boost::system::error_code& error, std::size_t bytes_transferred) {
            handle_receive(error, bytes_transferred, buffer, sender);
            start_receive(); // Continue receiving
        }
    );
}

void P2PConnection::handle_receive(
    const boost::system::error_code& error,
    std::size_t bytes_transferred,
    std::shared_ptr<std::vector<uint8_t>> buffer,
    std::shared_ptr<boost::asio::ip::udp::endpoint> sender
) {
    if (error) {
        if (!impl_->should_stop) {
            spdlog::error("Receive error: {}", error.message());
            start_receive();
        }
        return;
    }

    // Extract message size from header
    if (bytes_transferred < 4) {
        spdlog::error("Incomplete message received");
        return;
    }

    uint32_t message_size = ((*buffer)[0] << 24) | ((*buffer)[1] << 16) | 
                           ((*buffer)[2] << 8) | (*buffer)[3];

    if (bytes_transferred < message_size + 4) {
        spdlog::error("Incomplete message received");
        return;
    }

    // Extract message content
    std::vector<uint8_t> message_data(buffer->begin() + 4, buffer->begin() + 4 + message_size);

    // Create message and add to queue
    Message msg{
        sender->address().to_string(),
        sender->port(),
        std::move(message_data)
    };

    {
        std::lock_guard<std::mutex> lock(impl_->message_queue_mutex_);
        impl_->message_queue_.push(std::move(msg));
    }

    // Notify message handler if set
    if (impl_->message_handler_) {
        impl_->message_handler_(
            msg.sender_address,
            msg.sender_port,
            msg.data
        );
    }

    update_stats(0, bytes_transferred);
    start_receive();
}

} // namespace quids::network 