#include "cli/commands/StatusCommand.hpp"
#include "control/QuidsControl.hpp"
#include <spdlog/spdlog.h>
#include <iostream>

namespace quids {
namespace cli {

int StatusCommand::execute(const std::vector<std::string>& args) {
    try {
        control::QuidsControl controller;
        auto status = controller.getStatus();

        if (hasArg(args, "--json")) {
            printJsonStatus(status);
        } else {
            printHumanStatus(status);
        }
        
        return status.is_running ? 0 : 1;
    } catch (const std::exception& e) {
        spdlog::error("Error getting status: {}", e.what());
        return 1;
    }
}

void StatusCommand::printHumanStatus(const control::QuidsControl::NodeStatus& status) {
    std::cout << "Node Status:\n"
              << "  Running: " << (status.is_running ? "Yes" : "No") << "\n"
              << "  Block Height: " << status.block_height << "\n"
              << "  Peers: " << status.peer_count << "\n"
              << "  Sync Status: " << status.sync_status << "\n"
              << "  Health: " << (status.health.is_healthy ? "Healthy" : "Unhealthy") << "\n";
}

void StatusCommand::printJsonStatus(const control::QuidsControl::NodeStatus& status) {
    // TODO: Implement JSON output when we add JSON library
    printHumanStatus(status);
}

} // namespace cli
} // namespace quids 