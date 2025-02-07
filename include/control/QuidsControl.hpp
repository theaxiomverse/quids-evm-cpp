#pragma once
#include "node/QuidsNode.hpp"
#include "node/QuidsConfig.hpp"
#include <spdlog/spdlog.h>
#include <memory>

namespace quids {
namespace control {

struct UpgradeConfig {
    uint32_t version;
    std::string description;
    std::vector<uint8_t> upgrade_code;
};

class QuidsControl {
public:
    struct NodeStatus {
        bool is_running{false};
        uint64_t block_height{0};
        size_t peer_count{0};
        std::string sync_status;
        SystemHealth health;
    };

    QuidsControl();
    ~QuidsControl();

    NodeStatus getStatus() const;
    bool startNode(const QuidsConfig& config);
    bool stopNode();
    bool restartNode();
    bool upgradeNode(const UpgradeConfig& config);

private:
    std::unique_ptr<QuidsNode> node_;
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace control
} // namespace quids 