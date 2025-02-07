#include "network/QUICTransport.hpp"
#include <chrono>
#include <algorithm>
#include <quiche.h>
#include "utils/Timer.hpp"
#include "crypto/QuantumCrypto.hpp"

namespace quids {
namespace network {

QUICTransport::BufferPool::BufferPool() {
    buffers.resize(NUM_BUFFERS);
    freeBuffers.resize(NUM_BUFFERS);
    
    // Initialize free buffer indices
    std::iota(freeBuffers.begin(), freeBuffers.end(), 0);
}

uint8_t* QUICTransport::BufferPool::acquireBuffer() {
    if (freeBuffers.empty()) {
        return nullptr;
    }
    
    size_t index = freeBuffers.back();
    freeBuffers.pop_back();
    return buffers[index].data();
}

void QUICTransport::BufferPool::releaseBuffer(uint8_t* buffer) {
    auto it = std::find_if(
        buffers.begin(),
        buffers.end(),
        [buffer](const auto& b) { return b.data() == buffer; }
    );
    
    if (it != buffers.end()) {
        size_t index = std::distance(buffers.begin(), it);
        freeBuffers.push_back(index);
    }
}

QUICTransport::QUICTransport(const NetworkConfig& config)
    : config_(config) {
    initializeQuicConfig();
}

void QUICTransport::initializeQuicConfig() {
    quiche_config_t* config = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    if (!config) {
        throw std::runtime_error("Failed to create QUIC config");
    }
    
    // Set QUIC configuration
    quiche_config_set_max_idle_timeout(config, TIMEOUT_INTERVAL);
    quiche_config_set_max_packet_size(config, MAX_PACKET_SIZE);
    quiche_config_set_initial_max_data(config, 10 * 1024 * 1024); // 10MB
    quiche_config_set_initial_max_stream_data_bidi_local(config, 1 * 1024 * 1024); // 1MB
    quiche_config_set_initial_max_stream_data_bidi_remote(config, 1 * 1024 * 1024);
    quiche_config_set_initial_max_streams_bidi(config, 100);
    
    // Enable early data
    quiche_config_enable_early_data(config);
    
    // Load TLS certificates
    if (quiche_config_load_cert_chain_from_pem_file(config, config_.certificatePath.c_str()) < 0) {
        throw std::runtime_error("Failed to load certificate");
    }
    
    if (quiche_config_load_priv_key_from_pem_file(config, config_.privateKeyPath.c_str()) < 0) {
        throw std::runtime_error("Failed to load private key");
    }
    
    quicConfig_.reset(config);
}

void QUICTransport::start() {
    // Initialize QUIC listener
    // ... QUIC-specific initialization code ...
}

void QUICTransport::stop() {
    // Close all connections
    for (auto& [peer, conn] : connections_) {
        closeStream(peer);
    }
    connections_.clear();
}

void QUICTransport::sendMessage(Message&& msg) {
    auto* stream = getOrCreateStream(msg.target);
    if (!stream) {
        throw std::runtime_error("Failed to create stream");
    }
    
    // Get buffer from pool
    uint8_t* buffer = bufferPool_.acquireBuffer();
    if (!buffer) {
        throw std::runtime_error("No available buffers");
    }
    
    try {
        // Serialize message to buffer
        size_t len = serializeMessage(msg, buffer, BufferPool::BUFFER_SIZE);
        
        // Send data through QUIC stream
        ssize_t written = quiche_stream_send(
            stream,
            buffer,
            len,
            true // fin
        );
        
        if (written < 0) {
            throw std::runtime_error("Failed to send data");
        }
        
        // Update metrics
        auto& metrics = streamMetrics_[msg.target];
        metrics.bytesProcessed.fetch_add(written, std::memory_order_relaxed);
        metrics.packetsProcessed.fetch_add(1, std::memory_order_relaxed);
        
    } catch (...) {
        bufferPool_.releaseBuffer(buffer);
        throw;
    }
    
    bufferPool_.releaseBuffer(buffer);
}

std::vector<Message> QUICTransport::receiveMessages() {
    std::vector<Message> messages;
    
    // Process incoming packets
    uint8_t* buffer = bufferPool_.acquireBuffer();
    if (!buffer) {
        return messages;
    }
    
    try {
        for (auto& [peer, conn] : connections_) {
            for (auto& stream : conn.streams) {
                // Read data from stream
                ssize_t read = quiche_stream_recv(
                    stream.get(),
                    buffer,
                    BufferPool::BUFFER_SIZE,
                    false // fin
                );
                
                if (read > 0) {
                    // Process received data
                    handleStreamData(stream.get(), buffer, read);
                    
                    // Deserialize message
                    Message msg;
                    if (deserializeMessage(buffer, read, msg)) {
                        messages.push_back(std::move(msg));
                    }
                    
                    // Update metrics
                    auto& metrics = streamMetrics_[peer];
                    metrics.bytesProcessed.fetch_add(read, std::memory_order_relaxed);
                    metrics.packetsProcessed.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    } catch (...) {
        bufferPool_.releaseBuffer(buffer);
        throw;
    }
    
    bufferPool_.releaseBuffer(buffer);
    return messages;
}

quiche::Stream* QUICTransport::getOrCreateStream(const NodeID& peer) {
    auto it = connections_.find(peer);
    if (it == connections_.end()) {
        // Create new connection
        Connection conn;
        conn.quic = std::unique_ptr<quiche::Connection>(
            quiche_connect(
                peer.data(),
                peer.size(),
                nullptr,
                0,
                quicConfig_.get()
            )
        );
        
        if (!conn.quic) {
            return nullptr;
        }
        
        // Create stream
        auto stream = std::unique_ptr<quiche::Stream>(
            quiche_stream_new(conn.quic.get(), 0, nullptr)
        );
        
        if (!stream) {
            return nullptr;
        }
        
        auto* streamPtr = stream.get();
        conn.streams.push_back(std::move(stream));
        connections_[peer] = std::move(conn);
        return streamPtr;
    }
    
    return it->second.streams[0].get();
}

void QUICTransport::handleStreamData(quiche::Stream* stream, const uint8_t* data, size_t len) {
    // Process stream data
    // ... QUIC-specific stream handling ...
}

size_t QUICTransport::getActiveConnections() const {
    return connections_.size();
}

void QUICTransport::cleanupInactiveConnections() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = connections_.begin(); it != connections_.end();) {
        auto timeSinceLastActivity = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - std::chrono::milliseconds(it->second.lastActivity)
        ).count();
        
        if (timeSinceLastActivity > TIMEOUT_INTERVAL) {
            if (onDisconnection_) {
                onDisconnection_(it->first);
            }
            it = connections_.erase(it);
        } else {
            ++it;
        }
    }
}

void QUICTransport::setConnectionHandler(ConnectionCallback handler) {
    onConnection_ = std::move(handler);
}

void QUICTransport::setDisconnectionHandler(ConnectionCallback handler) {
    onDisconnection_ = std::move(handler);
}

} // namespace network
} // namespace quids 