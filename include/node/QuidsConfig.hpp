#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace quids {

struct NetworkConfig {
    std::string listen_addr{"0.0.0.0"};  // Default bind address
    uint16_t port{8080};                 // Default P2P port
    std::vector<std::string> bootstrap_peers;
    size_t max_connections{100};         // Max number of P2P connections
    size_t buffer_size{1024 * 1024};    // Default 1MB buffer size
    size_t ping_interval_ms{30000};      // 30 seconds ping interval
    size_t connection_timeout_ms{60000}; // 60 seconds connection timeout
    bool enable_discovery{true};         // Enable peer discovery
};

struct EVMConfig {
    // Placeholder for EVM configuration
};

struct ChainConfig {
    // Placeholder for chain configuration
};

struct QuidsConfig {
    // Network settings
    std::string network_type{"mainnet"};
    uint16_t listen_port{8545};
    uint16_t rpc_port{8546};
    size_t max_peers{50};
    
    // System paths
    std::string data_dir;
    std::string config_path;
    std::string log_dir;
    
    // Resource limits
    size_t max_memory_mb{8192};
    size_t num_worker_threads{4};
    
    // Quantum settings
    size_t num_qubits{24};
    bool use_error_correction{true};
    
    // AI settings
    size_t ml_batch_size{32};
    double learning_rate{0.001};
    size_t hidden_size{128};
    
    // Chain settings
    bool enable_ai_optimization{true};
    bool enable_quantum_proofs{true};
    uint64_t target_block_time{15}; // seconds
    
    // Validation
    bool validate() const;

    NetworkConfig network;
    EVMConfig evm;
    ChainConfig chain;
};

} // namespace quids 