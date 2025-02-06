#pragma once

#include <array>
#include <vector>
#include <memory>
#include <map>
#include <stack>
#include <functional>
#include "evm/Memory.h"
#include "evm/Storage.h"
#include "evm/Stack.h"
#include "evm/uint256.h"
#include "evm/Address.h"

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

    // Constructor
    EVMExecutor(std::shared_ptr<Memory> memory,
                std::shared_ptr<Stack> stack,
                std::shared_ptr<Storage> storage);

    // Contract execution
    ExecutionResult execute_contract(const Address& contract_address,
                                   const std::vector<uint8_t>& code,
                                   const std::vector<uint8_t>& input_data,
                                   uint64_t gas_limit);

private:
    std::shared_ptr<Memory> memory_;
    std::shared_ptr<Stack> stack_;
    std::shared_ptr<Storage> storage_;
    uint64_t gas_used_{0};
    uint64_t gas_limit_{0};

    void require_gas(uint64_t gas);
};

} // namespace evm 