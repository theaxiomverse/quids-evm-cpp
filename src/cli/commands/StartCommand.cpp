#include "cli/commands/StartCommand.hpp"
#include "node/QuidsNode.hpp"
#include <spdlog/spdlog.h>

namespace quids {
namespace cli {

int StartCommand::execute(const std::vector<std::string>& args) {
    QuidsConfig config;
    
    if (!parseArgs(args, config)) {
        printUsage();
        return 1;
    }
    
    if (!validateConfig(config)) {
        spdlog::error("Invalid configuration");
        return 1;
    }
    
    try {
        auto node = std::make_unique<QuidsNode>(config);
        return node->start() ? 0 : 1;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start node: {}", e.what());
        return 1;
    }
}

bool StartCommand::parseArgs(const std::vector<std::string>& args, QuidsConfig& config) {
    for (const auto& arg : args) {
        if (arg.find("--config=") == 0) {
            config.config_path = arg.substr(9);
        } else if (arg.find("--port=") == 0) {
            config.listen_port = std::stoi(arg.substr(7));
        } else if (arg.find("--rpc-port=") == 0) {
            config.rpc_port = std::stoi(arg.substr(11));
        } else if (arg.find("--data-dir=") == 0) {
            config.data_dir = arg.substr(11);
        } else if (arg.find("--network=") == 0) {
            config.network_type = arg.substr(10);
        }
    }
    return true;
}

bool StartCommand::validateConfig(const QuidsConfig& config) {
    if (config.listen_port == config.rpc_port) {
        spdlog::error("Listen port and RPC port must be different");
        return false;
    }
    
    if (config.data_dir.empty()) {
        spdlog::error("Data directory must be specified");
        return false;
    }
    
    return true;
}

} // namespace cli
} // namespace quids 