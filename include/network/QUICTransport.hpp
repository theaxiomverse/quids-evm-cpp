#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>




// Forward declarations for QUIC library types
namespace quiche {
    class Connection;
    class Config;
    class Stream;
}

namespace quids {
namespace network {

class QUICTransport {
public:
    explicit QUICTransport(const NetworkConfig& config);
    ~QUICTransport();

    // Connection management
    void start();
    void stop();
    size_t getActiveConnections() const;

    struct Message {
        NodeID sender;
        NodeID recipient;
        std::vector<uint8_t> data;
    };

    struct MessageHandler {
        std::function<void(Message&&)> handler;
        std::function<void(const NodeID&)> onDisconnection;
    };

    struct MessageHandlerRegistry {
        std::unordered_map<NodeID, MessageHandler> handlers;
        std::mutex mutex;
    };

    // Message handling
    void sendMessage(Message&& msg);
    std::vector<Message> receiveMessages();

    // Connection events
    using ConnectionCallback = std::function<void(const NodeID&)>;
    void setConnectionHandler(ConnectionCallback handler);
    void setDisconnectionHandler(ConnectionCallback handler);

    // Stream management
    void createStream(const NodeID& peer);
    void closeStream(const NodeID& peer);

    // Configuration
    void updateConfig(const NetworkConfig& config);
    void setMaxStreamBitrate(size_t bitrate);

private:
    // QUIC configuration
    struct QUICConfig {
        uint32_t maxStreamData;
        uint32_t maxData;
        uint32_t maxStreamsBidi;
        uint32_t maxStreamsUni;
        uint32_t idleTimeout;
        bool enableEarlyData;
    };

    // Connection state
    struct Connection {
        std::unique_ptr<quiche::Connection> quic;
        std::vector<std::unique_ptr<quiche::Stream>> streams;
        uint64_t lastActivity;
        size_t bytesReceived;
        size_t bytesSent;
    };

    // Core components
    std::unique_ptr<quiche::Config> quicConfig_;
    std::unordered_map<NodeID, Connection> connections_;
    NetworkConfig config_;

    // Event handlers
    ConnectionCallback onConnection_;
    ConnectionCallback onDisconnection_;

    // Internal helper functions
    void initializeQuicConfig();
    void handleIncomingPacket(const uint8_t* data, size_t len);
    void processTimeouts();
    void cleanupInactiveConnections();
    
    // Stream management
    quiche::Stream* getOrCreateStream(const NodeID& peer);
    void handleStreamData(quiche::Stream* stream, const uint8_t* data, size_t len);
    
    // Zero-copy buffer management
    struct BufferPool {
        static constexpr size_t BUFFER_SIZE = 65536;
        static constexpr size_t NUM_BUFFERS = 1024;
        
        std::vector<std::array<uint8_t, BUFFER_SIZE>> buffers;
        std::vector<size_t> freeBuffers;
        
        BufferPool();
        uint8_t* acquireBuffer();
        void releaseBuffer(uint8_t* buffer);
    } bufferPool_;

    // Performance optimization
    struct alignas(64) StreamMetrics {
        std::atomic<uint64_t> bytesProcessed{0};
        std::atomic<uint64_t> packetsProcessed{0};
        std::atomic<double> averageLatency{0.0};
    };
    std::unordered_map<NodeID, StreamMetrics> streamMetrics_;

    // Constants
    static constexpr size_t MAX_PACKET_SIZE = 1350;
    static constexpr size_t MAX_DATAGRAM_SIZE = 1200;
    static constexpr uint64_t TIMEOUT_INTERVAL = 1000; // milliseconds
};

} // namespace network
} // namespace quids 