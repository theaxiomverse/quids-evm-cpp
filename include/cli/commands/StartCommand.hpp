#pragma once
#include "cli/QuidsCommand.hpp"
#include "node/QuidsConfig.hpp"

namespace quids {
namespace cli {

class StartCommand : public QuidsCommand {
public:
    std::string getName() const override { return "start"; }
    
    std::string getDescription() const override { 
        return "Start the Quids node"; 
    }
    
    std::string getUsage() const override {
        return "quids start [--config=<path>] [--port=<port>] [--rpc-port=<port>] "
               "[--data-dir=<path>] [--network=<mainnet|testnet>]";
    }
    
    int execute(const std::vector<std::string>& args) override;

private:
    bool parseArgs(const std::vector<std::string>& args, QuidsConfig& config);
    bool validateConfig(const QuidsConfig& config);
};

} // namespace cli
} // namespace quids 