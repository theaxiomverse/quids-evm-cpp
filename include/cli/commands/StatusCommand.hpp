#pragma once
#include "cli/QuidsCommand.hpp"
#include "control/QuidsControl.hpp"

namespace quids {
namespace cli {

class StatusCommand : public QuidsCommand {
public:
    std::string getName() const override { return "status"; }
    std::string getDescription() const override { 
        return "Get node status"; 
    }
    std::string getUsage() const override {
        return "quids status [--json]";
    }
    int execute(const std::vector<std::string>& args) override;

private:
    void printHumanStatus(const control::QuidsControl::NodeStatus& status);
    void printJsonStatus(const control::QuidsControl::NodeStatus& status);
};

} // namespace cli
} // namespace quids 