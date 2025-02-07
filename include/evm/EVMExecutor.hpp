#pragma once

#include <array>
#include <vector>
#include <memory>
#include <map>
#include <stack>
#include <functional>
#include <string>
#include "evm/Memory.hpp"
#include "evm/Storage.hpp"
#include "evm/Stack.hpp"
#include "evm/uint256.hpp"
#include "evm/Address.hpp"

namespace evm {

// Forward declarations
class Memory;
class Storage;
class Stack;

class EVMExecutor {
public:
    struct ExecutionResult {
        bool success;
        std::vector<uint8_t> return_data;
        uint64_t gas_used;
        std::string error_message;
    };

    // Constructor and destructor
    EVMExecutor(std::shared_ptr<Memory> memory,
                std::shared_ptr<Stack> stack,
                std::shared_ptr<Storage> storage);
    ~EVMExecutor();

    // Disable copy and move
    EVMExecutor(const EVMExecutor&) = delete;
    EVMExecutor& operator=(const EVMExecutor&) = delete;
    EVMExecutor(EVMExecutor&&) = delete;
    EVMExecutor& operator=(EVMExecutor&&) = delete;

    // Contract execution
    [[nodiscard]] ExecutionResult execute_contract(const Address& contract_address,
                                                 const std::vector<uint8_t>& code,
                                                 const std::vector<uint8_t>& input_data,
                                                 uint64_t gas_limit);

    // Gas management
    [[nodiscard]] uint64_t get_gas_used() const { return gas_used_; }
    [[nodiscard]] uint64_t get_gas_limit() const { return gas_limit_; }

private:
    std::shared_ptr<Memory> memory_;
    std::shared_ptr<Stack> stack_;
    std::shared_ptr<Storage> storage_;
    uint64_t gas_used_{0};
    uint64_t gas_limit_{0};

    void require_gas(uint64_t gas);

    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace evm 