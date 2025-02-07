#pragma once
#include "cli/QuidsCommand.hpp"
#include <map>
#include <memory>

namespace quids {
namespace cli {

class QuidsCLI {
public:
    QuidsCLI();
    ~QuidsCLI();

    int run(int argc, char** argv);
    void registerCommand(std::unique_ptr<QuidsCommand> command);

private:
    void printHelp() const;
    void printVersion() const;
    std::vector<std::string> parseArgs(int argc, char** argv) const;
    
    std::map<std::string, std::unique_ptr<QuidsCommand>> commands_;
    static constexpr const char* VERSION = "1.0.0";
};

} // namespace cli
} // namespace quids 