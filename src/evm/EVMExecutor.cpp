#include "evm/EVMExecutor.h"
#include <stdexcept>

namespace evm {

namespace {
    // EVM opcodes
    enum Opcode : uint8_t {
        STOP = 0x00,
        ADD = 0x01,
        MUL = 0x02,
        SUB = 0x03,
        DIV = 0x04,
        SDIV = 0x05,
        MOD = 0x06,
        SMOD = 0x07,
        ADDMOD = 0x08,
        MULMOD = 0x09,
        EXP = 0x0a,
        SIGNEXTEND = 0x0b,
        
        LT = 0x10,
        GT = 0x11,
        SLT = 0x12,
        SGT = 0x13,
        EQ = 0x14,
        ISZERO = 0x15,
        AND = 0x16,
        OR = 0x17,
        XOR = 0x18,
        NOT = 0x19,
        BYTE = 0x1a,
        
        SHA3 = 0x20,
        
        ADDRESS = 0x30,
        BALANCE = 0x31,
        ORIGIN = 0x32,
        CALLER = 0x33,
        CALLVALUE = 0x34,
        CALLDATALOAD = 0x35,
        CALLDATASIZE = 0x36,
        CALLDATACOPY = 0x37,
        CODESIZE = 0x38,
        CODECOPY = 0x39,
        GASPRICE = 0x3a,
        EXTCODESIZE = 0x3b,
        EXTCODECOPY = 0x3c,
        RETURNDATASIZE = 0x3d,
        RETURNDATACOPY = 0x3e,
        
        BLOCKHASH = 0x40,
        COINBASE = 0x41,
        TIMESTAMP = 0x42,
        NUMBER = 0x43,
        DIFFICULTY = 0x44,
        GASLIMIT = 0x45,
        
        POP = 0x50,
        MLOAD = 0x51,
        MSTORE = 0x52,
        MSTORE8 = 0x53,
        SLOAD = 0x54,
        SSTORE = 0x55,
        JUMP = 0x56,
        JUMPI = 0x57,
        PC = 0x58,
        MSIZE = 0x59,
        GAS = 0x5a,
        JUMPDEST = 0x5b,
        
        PUSH1 = 0x60,
        PUSH32 = 0x7f,
        
        DUP1 = 0x80,
        DUP16 = 0x8f,
        
        SWAP1 = 0x90,
        SWAP16 = 0x9f,
        
        LOG0 = 0xa0,
        LOG4 = 0xa4,
        
        CREATE = 0xf0,
        CALL = 0xf1,
        CALLCODE = 0xf2,
        RETURN = 0xf3,
        DELEGATECALL = 0xf4,
        CREATE2 = 0xf5,
        STATICCALL = 0xfa,
        REVERT = 0xfd,
        INVALID = 0xfe,
        SELFDESTRUCT = 0xff
    };
}

EVMExecutor::EVMExecutor(std::shared_ptr<Memory> memory,
                        std::shared_ptr<Stack> stack,
                        std::shared_ptr<Storage> storage)
    : memory_(std::move(memory))
    , stack_(std::move(stack))
    , storage_(std::move(storage)) {
    
    if (!memory_ || !stack_ || !storage_) {
        throw std::invalid_argument("EVMExecutor components cannot be null");
    }
}

EVMExecutor::ExecutionResult EVMExecutor::execute_contract(
    const Address& contract_address,
    const std::vector<uint8_t>& code,
    const std::vector<uint8_t>& input_data,
    uint64_t gas_limit) {
    
    gas_used_ = 0;
    gas_limit_ = gas_limit;

    try {
        size_t pc = 0;
        while (pc < code.size()) {
            uint8_t opcode = code[pc];
            
            switch (opcode) {
                case 0x00:  // STOP
                    require_gas(2);
                    return ExecutionResult{
                        .success = true,
                        .gas_used = gas_used_,
                        .return_data = {}
                    };

                case 0x01:  // ADD
                    {
                        require_gas(3);
                        auto a = stack_->pop();
                        auto b = stack_->pop();
                        stack_->push(a + b);
                    }
                    break;

                case 0x02:  // MUL
                    {
                        require_gas(5);
                        auto a = stack_->pop();
                        auto b = stack_->pop();
                        stack_->push(a * b);
                    }
                    break;

                case 0x03:  // SUB
                    {
                        require_gas(3);
                        auto a = stack_->pop();
                        auto b = stack_->pop();
                        stack_->push(a - b);
                    }
                    break;

                case 0x04:  // DIV
                    {
                        require_gas(5);
                        auto a = stack_->pop();
                        auto b = stack_->pop();
                        if (b == uint256_t(0)) {
                            stack_->push(uint256_t(0));
                        } else {
                            stack_->push(a / b);
                        }
                    }
                    break;

                case 0x10:  // LT
                    {
                        require_gas(3);
                        auto a = stack_->pop();
                        auto b = stack_->pop();
                        stack_->push(a < b ? uint256_t(1) : uint256_t(0));
                    }
                    break;

                case 0x11:  // GT
                    {
                        require_gas(3);
                        auto a = stack_->pop();
                        auto b = stack_->pop();
                        stack_->push(a > b ? uint256_t(1) : uint256_t(0));
                    }
                    break;

                case 0x14:  // EQ
                    {
                        require_gas(3);
                        auto a = stack_->pop();
                        auto b = stack_->pop();
                        stack_->push(a == b ? uint256_t(1) : uint256_t(0));
                    }
                    break;

                case 0x54:  // SLOAD
                    require_gas(200);
                    {
                        auto key = stack_->pop();
                        stack_->push(storage_->load(contract_address, key));
                    }
                    break;

                case 0x55:  // SSTORE
                    require_gas(5000);
                    {
                        auto value = stack_->pop();
                        auto key = stack_->pop();
                        storage_->store(contract_address, key, value);
                    }
                    break;

                case 0x60:  // PUSH1
                case 0x61:  // PUSH2
                case 0x62:  // PUSH3
                    {
                        uint8_t push_bytes = (opcode - 0x60) + 1;
                        require_gas(3);
                        
                        if (pc + push_bytes >= code.size()) {
                            throw std::runtime_error("Push exceeds code size");
                        }

                        uint256_t value(0);
                        for (uint8_t i = 0; i < push_bytes; i++) {
                            value = (value << 8) | uint256_t(code[pc + 1 + i]);
                        }
                        stack_->push(value);
                        pc += push_bytes;
                    }
                    break;

                default:
                    throw std::runtime_error("Unknown opcode");
            }

            pc++;
        }

        if (gas_used_ >= gas_limit) {
            throw std::runtime_error("Out of gas");
        }

        return ExecutionResult{
            .success = true,
            .gas_used = gas_used_,
            .return_data = {}  // TODO: Implement return data handling
        };

    } catch (const std::exception& e) {
        return ExecutionResult{
            .success = false,
            .gas_used = gas_used_,
            .return_data = {},
            .error_message = e.what()
        };
    }
}

void EVMExecutor::require_gas(uint64_t gas) {
    if (gas_used_ + gas > gas_limit_) {
        throw std::runtime_error("Out of gas");
    }
    gas_used_ += gas;
}

} // namespace evm 