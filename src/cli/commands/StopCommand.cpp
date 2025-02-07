#include "cli/commands/StopCommand.hpp"
#include "control/QuidsControl.hpp"
#include <spdlog/spdlog.h>

namespace quids {
namespace cli {

int StopCommand::execute(const std::vector<std::string>& args) {
    try {
        control::QuidsControl controller;
        if (!controller.stopNode()) {
            spdlog::error("Failed to stop node");
            return 1;
        }
        spdlog::info("Node stopped successfully");
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("Error stopping node: {}", e.what());
        return 1;
    }
}

} // namespace cli
} // namespace quids 