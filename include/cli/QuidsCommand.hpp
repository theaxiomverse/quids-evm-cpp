#pragma once
#include <string>
#include <vector>

namespace quids {
namespace cli {

class QuidsCommand {
public:
    virtual ~QuidsCommand() = default;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getUsage() const = 0;
    virtual int execute(const std::vector<std::string>& args) = 0;

protected:
    // Helper methods for argument parsing
    bool hasArg(const std::vector<std::string>& args, const std::string& arg) const;
    std::string getArgValue(const std::vector<std::string>& args, const std::string& arg) const;
    void printUsage() const;
};

} // namespace cli
} // namespace quids 