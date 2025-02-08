#pragma once

#include <array>
#include <vector>
#include <memory>
#include <map>
#include <stack>
#include <functional>
#include <string>
#include <unordered_map>

#include "evm/Memory.hpp"
#include "evm/Storage.hpp"
#include "evm/Stack.hpp"
#include "evm/uint256.hpp"
#include "evm/Address.hpp"
#include "blockchain/Transaction.hpp"
#include "node/QuidsConfig.hpp"

namespace evm {
class Memory;
class Storage;
class Stack;
}

namespace quids {
namespace evm {

class EVMExecutor {
public:
    // Implementation details
    struct Impl {
        // EVM state
        std::unordered_map<std::string, uint64_t> balances;
        std::unordered_map<std::string, std::vector<uint8_t>> code;
        std::unordered_map<std::string, std::unordered_map<::evm::uint256_t, std::vector<uint8_t>>> storage;
    };

    struct ExecutionResult {
        bool success;
        std::vector<uint8_t> return_data;
        uint64_t gas_used;
        std::string error_message;
    };

    // Constructor and destructor
    explicit EVMExecutor(const EVMConfig& config);
    ~EVMExecutor();

    // Disable copy and move
    EVMExecutor(const EVMExecutor&) = delete;
    EVMExecutor& operator=(const EVMExecutor&) = delete;
    EVMExecutor(EVMExecutor&&) = delete;
    EVMExecutor& operator=(EVMExecutor&&) = delete;

    // Core EVM execution
    [[nodiscard]] ExecutionResult execute_contract(
        const ::evm::Address& contract_address,
        const std::vector<uint8_t>& code,
        const std::vector<uint8_t>& input_data,
        uint64_t gas_limit
    );

    // Transaction execution
    bool execute(const blockchain::Transaction& tx);
    bool deploy(const std::vector<uint8_t>& code);

    // State access
    uint64_t getBalance(const std::string& address) const;
    std::vector<uint8_t> getCode(const std::string& address) const;
    std::vector<uint8_t> getStorage(const std::string& address, ::evm::uint256_t key) const;
    
    // Gas management
    [[nodiscard]] uint64_t get_gas_used() const { return gas_used_; }
    [[nodiscard]] uint64_t get_gas_limit() const { return gas_limit_; }

private:
    void require_gas(uint64_t gas);

    // Core components
    std::shared_ptr<::evm::Memory> memory_;
    std::shared_ptr<::evm::Stack> stack_;
    std::shared_ptr<::evm::Storage> storage_;
    
    // Gas tracking
    uint64_t gas_used_{0};
    uint64_t gas_limit_{0};

    // Configuration
    EVMConfig config_;

    // Implementation details
    std::unique_ptr<Impl> impl_;
};

} // namespace evm
} // namespace quids 