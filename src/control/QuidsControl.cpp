#include "control/QuidsControl.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace quids {
namespace control {

QuidsControl::QuidsControl() {
    try {
        logger_ = spdlog::get("quids");
        if (!logger_) {
            auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            logger_ = std::make_shared<spdlog::logger>("quids", sink);
            spdlog::register_logger(logger_);
        }
    } catch (const spdlog::spdlog_ex& ex) {
        throw std::runtime_error(std::string("Logger initialization failed: ") + ex.what());
    }
}

QuidsControl::~QuidsControl() {
    if (node_) {
        stopNode();
    }
}

QuidsControl::NodeStatus QuidsControl::getStatus() const {
    NodeStatus status;
    if (!node_) {
        return status;
    }

    status.is_running = true;
    status.block_height = node_->getCurrentHeight();
    status.peer_count = node_->getPeerCount();
    status.sync_status = node_->getSyncStatus();
    status.health = node_->getHealth();

    return status;
}

bool QuidsControl::startNode(const QuidsConfig& config) {
    try {
        if (node_) {
            logger_->warn("Node is already running");
            return false;
        }

        node_ = std::make_unique<QuidsNode>(config);
        return node_->start();
    } catch (const std::exception& e) {
        logger_->error("Failed to start node: {}", e.what());
        return false;
    }
}

bool QuidsControl::stopNode() {
    try {
        if (!node_) {
            logger_->warn("Node is not running");
            return false;
        }

        bool result = node_->stop();
        if (result) {
            node_.reset();
        }
        return result;
    } catch (const std::exception& e) {
        logger_->error("Failed to stop node: {}", e.what());
        return false;
    }
}

bool QuidsControl::restartNode() {
    if (!node_) {
        logger_->warn("Node is not running");
        return false;
    }

    auto config = node_->getConfig();
    if (!stopNode()) {
        return false;
    }

    return startNode(config);
}

bool QuidsControl::upgradeNode(const UpgradeConfig& config) {
    logger_->info("Starting node upgrade to version {}", config.version);
    // TODO: Implement upgrade logic
    return true;
}

} // namespace control
} // namespace quids 