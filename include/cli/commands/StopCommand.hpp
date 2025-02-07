#pragma once
#include "cli/QuidsCommand.hpp"

namespace quids {
namespace cli {

class StopCommand : public QuidsCommand {
public:
    std::string getName() const override { return "stop"; }
    std::string getDescription() const override { 
        return "Stop the Quids node"; 
    }
    std::string getUsage() const override {
        return "quids stop";
    }
    int execute(const std::vector<std::string>& args) override;
};

} // namespace cli
} // namespace quids 