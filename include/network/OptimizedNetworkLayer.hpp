
#include <memory>
#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include "network/QUICTransport.hpp"
#include "node/QuidsConfig.hpp"
#include "utils/LockFreeQueue.hpp"
#include "quantum/QuantumTypes.hpp"

namespace quids {
namespace network {

struct NetworkConfig {
    uint16_t port;
    size_t maxConnections;
    size_t bufferSize;
    bool useQuantumEncryption;
    std::string certificatePath;
    std::string privateKeyPath;
    size_t numWorkerThreads;
};

class OptimizedNetworkLayer {
public:
    explicit OptimizedNetworkLayer(const NetworkConfig& config);
    ~OptimizedNetworkLayer();

    // Disable copy
    OptimizedNetworkLayer(const OptimizedNetworkLayer&) = delete;
    OptimizedNetworkLayer& operator=(const OptimizedNetworkLayer&) = delete;

    // Network operations
    void start();
    void stop();
    void broadcastMessage(const Message& msg);
    void sendMessage(const NodeID& target, const Message& msg);

    // Connection management
    void addPeer(const NodeID& peer);
    void removePeer(const NodeID& peer);
    std::vector<NodeID> getActivePeers() const;

    // Message handling
    void registerMessageHandler(MessageType type, MessageHandler handler);
    void processIncomingMessages();

    // Metrics
    NetworkMetrics getMetrics() const;
    void resetMetrics();

private:
    // Core components
    std::unique_ptr<QUICTransport> transport_;
    std::vector<std::unique_ptr<std::thread>> workerThreads_;
    
    // Lock-free message queues
    utils::LockFreeQueue<Message> incomingQueue_;
    utils::LockFreeQueue<Message> outgoingQueue_;
    
    // Message handlers
    std::array<MessageHandler, MAX_MESSAGE_TYPES> messageHandlers_;
    
    // Network state
    std::atomic<bool> running_{false};
    NetworkConfig config_;
    
    // Metrics tracking
    struct alignas(64) NetworkMetrics {
        std::atomic<uint64_t> messagesProcessed{0};
        std::atomic<uint64_t> bytesTransferred{0};
        std::atomic<uint64_t> activeConnections{0};
        std::atomic<double> averageLatency{0.0};
        std::atomic<uint64_t> errorCount{0};
    } metrics_;

    // Internal helper functions
    void workerThread();
    void processMessage(const Message& msg);
    void handleError(const NetworkError& error);
    
    // SIMD-optimized message processing
    void processBatchSIMD(const std::vector<Message>& batch);
    void compressMessagesSIMD(std::vector<Message>& messages);
    
    // Quantum encryption
    void encryptQuantum(Message& msg);
    void decryptQuantum(Message& msg);
    
    // Constants
    static constexpr size_t BATCH_SIZE = 1024;
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t MAX_MESSAGE_TYPES = 256;
};

} // namespace network
} // namespace quids 