#include "cli/QuidsCLI.hpp"
#include "cli/commands/StartCommand.hpp"
#include "cli/commands/StopCommand.hpp"
#include "cli/commands/StatusCommand.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

using namespace quids;

void setupLogging() {
    auto console = spdlog::stdout_color_mt("quids");
    console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    spdlog::set_default_logger(console);
}

int main(int argc, char** argv) {
    try {
        // Setup logging
        setupLogging();
        
        // Create CLI
        cli::QuidsCLI cli;
        
        // Register commands
        cli.registerCommand(std::make_unique<cli::StartCommand>());
        cli.registerCommand(std::make_unique<cli::StopCommand>());
        cli.registerCommand(std::make_unique<cli::StatusCommand>());
        
        // Run CLI
        return cli.run(argc, argv);
        
    } catch (const std::exception& e) {
        SPDLOG_CRITICAL("Standard exception: {}", e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        SPDLOG_CRITICAL("Unknown exception type");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
} 