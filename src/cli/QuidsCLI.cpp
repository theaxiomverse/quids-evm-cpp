#include "cli/QuidsCLI.hpp"
#include <spdlog/spdlog.h>
#include <iostream>

namespace quids {
namespace cli {

QuidsCLI::QuidsCLI() = default;
QuidsCLI::~QuidsCLI() = default;

int QuidsCLI::run(int argc, char** argv) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string command = argv[1];
    
    if (command == "--help" || command == "-h") {
        printHelp();
        return 0;
    }
    
    if (command == "--version" || command == "-v") {
        printVersion();
        return 0;
    }

    auto it = commands_.find(command);
    if (it == commands_.end()) {
        spdlog::error("Unknown command: {}", command);
        printHelp();
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    return it->second->execute(args);
}

void QuidsCLI::registerCommand(std::unique_ptr<QuidsCommand> command) {
    commands_[command->getName()] = std::move(command);
}

void QuidsCLI::printHelp() const {
    std::cout << "Quids Blockchain Node v" << VERSION << "\n\n"
              << "Usage: quids <command> [options]\n\n"
              << "Commands:\n";
              
    for (const auto& [name, cmd] : commands_) {
        std::cout << "  " << name << "\t" << cmd->getDescription() << "\n";
    }
    
    std::cout << "\nUse 'quids <command> --help' for more information about a command.\n";
}

void QuidsCLI::printVersion() const {
    std::cout << "Quids Blockchain Node v" << VERSION << "\n";
}

} // namespace cli
} // namespace quids 