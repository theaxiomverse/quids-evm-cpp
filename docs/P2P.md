# QUIDS P2P Networking System

## Overview
The QUIDS P2P networking system is a custom-built, high-performance peer-to-peer networking solution designed for decentralized blockchain applications. It provides robust, secure, and efficient communication between nodes in the network.

## Core Components

### 1. P2PConnection
A low-level class that manages individual peer connections using TCP sockets. Key features:
- Non-blocking I/O for better performance
- Automatic connection health monitoring
- Built-in ping/pong mechanism
- Message framing with size prefixing
- Connection statistics tracking
- Thread-safe message queuing
- Error handling and recovery

### 2. P2PNode
A high-level class that manages multiple peer connections and provides the core P2P networking functionality. Key features:
- Automatic peer discovery
- Connection management
- Message broadcasting
- Custom message handlers
- Bootstrap node support
- Connection pooling
- Configurable parameters

## Configuration Options

### P2PNode Configuration
```cpp
struct Config {
    uint16_t port{8080};                    // Listen port
    size_t max_connections{100};            // Maximum peer connections
    size_t buffer_size{1024 * 1024};        // 1MB buffer size
    std::string bind_address{"0.0.0.0"};    // Bind address
    size_t ping_interval_ms{30000};         // 30 seconds ping interval
    size_t connection_timeout_ms{60000};    // 60 seconds timeout
};
```

## Usage Examples

### 1. Starting a Node
```cpp
using namespace quids::network;

// Create and configure node
P2PNode::Config config;
config.port = 8080;
config.max_connections = 50;

P2PNode node(config);

// Start the node
if (node.start()) {
    std::cout << "Node started successfully" << std::endl;
}
```

### 2. Connecting to Peers
```cpp
// Connect to a peer
node.connect_to_peer("192.168.1.100", 8080);

// Add bootstrap peers for discovery
node.add_bootstrap_peer("seed1.example.com", 8080);
node.add_bootstrap_peer("seed2.example.com", 8080);

// Start peer discovery
node.discover_peers();
```

### 3. Message Handling
```cpp
// Register message handler
node.register_message_handler([](const std::string& peer_address,
                               uint16_t peer_port,
                               const std::vector<uint8_t>& message) {
    std::cout << "Received message from " << peer_address 
              << ":" << peer_port << std::endl;
});

// Send message to specific peer
std::vector<uint8_t> message = {1, 2, 3, 4};
node.send_message_to_peer("192.168.1.100", 8080, message);

// Broadcast message to all peers
node.broadcast_message(message);
```

## Connection States
The P2P system defines several connection states:
- `DISCONNECTED`: Initial state or after disconnection
- `CONNECTING`: During connection establishment
- `HANDSHAKING`: During protocol handshake
- `CONNECTED`: Active connection
- `ERROR`: Error state

## Statistics and Monitoring
Each connection maintains detailed statistics:
- Bytes sent/received
- Messages sent/received
- Error count
- Last message timestamp
- Connection duration
- Connection state

## Security Features
1. Connection validation
2. Message size limits
3. Rate limiting (configurable)
4. Peer verification
5. Automatic disconnection of misbehaving peers

## Error Handling
The system provides comprehensive error handling:
- Socket errors
- Connection timeouts
- Protocol violations
- Resource exhaustion
- Network interruptions

## Best Practices
1. Configure appropriate buffer sizes based on expected message sizes
2. Set reasonable connection limits based on available resources
3. Implement appropriate message handlers for your use case
4. Monitor connection statistics for network health
5. Use bootstrap nodes for reliable peer discovery
6. Handle connection errors gracefully
7. Implement proper shutdown procedures

## Thread Safety
The P2P system is designed to be thread-safe:
- All public methods are thread-safe
- Internal state is protected by mutexes
- Message queues are thread-safe
- Connection management is synchronized
- Statistics updates are atomic

## Performance Considerations
1. Non-blocking I/O for better scalability
2. Message batching for efficiency
3. Connection pooling
4. Efficient memory management
5. Configurable buffer sizes
6. Parallel message processing

## Limitations
1. Maximum message size is limited by buffer size
2. Maximum connections limited by system resources
3. IPv4 support only (IPv6 planned for future)
4. TCP-only (UDP support planned for future)

## Future Enhancements
1. IPv6 support
2. UDP support for specific message types
3. NAT traversal
4. DHT-based peer discovery
5. Encrypted connections
6. Peer reputation system
7. Advanced routing capabilities 