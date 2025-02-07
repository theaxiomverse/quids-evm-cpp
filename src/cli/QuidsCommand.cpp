#include "cli/QuidsCommand.hpp"
#include <iostream>
#include <algorithm>

namespace quids {
namespace cli {

bool QuidsCommand::hasArg(const std::vector<std::string>& args, const std::string& arg) const {
    return std::find(args.begin(), args.end(), arg) != args.end();
}

std::string QuidsCommand::getArgValue(const std::vector<std::string>& args, const std::string& arg) const {
    auto it = std::find_if(args.begin(), args.end(),
        [&arg](const std::string& a) { return a.find(arg + "=") == 0; });
    
    if (it != args.end()) {
        return it->substr(arg.length() + 1);
    }
    return "";
}

void QuidsCommand::printUsage() const {
    std::cout << "Usage: " << getUsage() << "\n"
              << "Description: " << getDescription() << "\n";
}

} // namespace cli
} // namespace quids 