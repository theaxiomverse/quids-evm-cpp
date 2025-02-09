#pragma once
#include "node/QuidsConfig.hpp"
#include "node/SystemHealth.hpp"
#include "node/ForwardDeclarations.hpp"
#include "network/P2PNode.hpp"
#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace quids {

class QuidsNode {
public:
    explicit QuidsNode(const QuidsConfig& config);
    ~QuidsNode();

    bool start();
    bool stop();
    
    // Status methods
    uint64_t getCurrentHeight() const;
    size_t getPeerCount() const;
    std::string getSyncStatus() const;
    SystemHealth getHealth() const;
    const QuidsConfig& getConfig() const { return config_; }

private:
    // Initialization methods
    bool loadConfiguration();
    bool initializeCore();
    bool initializeQuantumSystem();
    bool initializeAISystem();
    bool initializeNetwork();
    bool initializeEVM();
    bool initializeChain();
    bool completeBoot();

    // Member variables
    QuidsConfig config_;
    std::shared_ptr<spdlog::logger> logger_;
    bool running_{false};
    
    // Network components
    std::shared_ptr<network::P2PNode> p2p_node_;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace quids 