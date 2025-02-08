#include "network/P2PNode.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

using namespace quids::network;

// Global flag for graceful shutdown
volatile sig_atomic_t running = 1;

void signal_handler(int) {
    running = 0;
}

int main() {
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Configure the node
        P2PNode::Config config;
        config.port = 8080;
        config.max_connections = 50;
        config.buffer_size = 1024 * 1024;  // 1MB
        config.ping_interval_ms = 30000;    // 30 seconds
        config.connection_timeout_ms = 60000; // 60 seconds

        // Create and start the node
        P2PNode node(config);
        if (!node.start()) {
            std::cerr << "Failed to start node" << std::endl;
            return 1;
        }

        std::cout << "P2P node started on port " << config.port << std::endl;

        // Add some bootstrap peers
        node.add_bootstrap_peer("localhost", 8081);
        node.add_bootstrap_peer("localhost", 8082);

        // Register message handler
        node.register_message_handler([](const std::string& peer_address,
                                      uint16_t peer_port,
                                      const std::vector<uint8_t>& message) {
            std::cout << "Received message from " << peer_address 
                      << ":" << peer_port 
                      << " (size: " << message.size() << " bytes)" << std::endl;
        });

        // Start peer discovery
        node.discover_peers();

        // Main loop
        while (running) {
            // Print connection status every 5 seconds
            auto peers = node.get_connected_peers();
            std::cout << "\nConnected peers: " << peers.size() << std::endl;
            
            for (const auto& peer : peers) {
                std::cout << "- " << peer.address << ":" << peer.port 
                          << " (msgs sent: " << peer.messages_sent
                          << ", received: " << peer.messages_received << ")" 
                          << std::endl;
            }

            // Broadcast a test message
            std::string msg = "Hello from node at " + 
                             std::to_string(std::chrono::system_clock::now()
                                 .time_since_epoch().count());
            std::vector<uint8_t> message(msg.begin(), msg.end());
            
            if (node.broadcast_message(message)) {
                std::cout << "Broadcast message sent" << std::endl;
            }

            // Wait for 5 seconds
            for (int i = 0; i < 50 && running; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // Graceful shutdown
        std::cout << "\nShutting down..." << std::endl;
        node.stop();
        std::cout << "Node stopped" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 