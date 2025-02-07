#include "evm/EVMExecutor.hpp"
#include <stdexcept>

namespace evm {

namespace {
    // Gas cost tiers from EVMC
    enum GasCosts {
        ZERO = 0,
        BASE = 2,
        VERYLOW = 3,
        LOW = 5,
        MID = 8,
        HIGH = 10,
        
        // Special costs
        WARM_STORAGE_READ_COST = 100,
        COLD_SLOAD_COST = 2100,
        SSTORE_SET_GAS = 20000,
        SSTORE_RESET_GAS = 5000,
        SSTORE_CLEARS_SCHEDULE = 15000,
        CREATE_GAS = 32000,
        SHA3_GAS = 30,
        BLOCKHASH_GAS = 20,
        SELFDESTRUCT_GAS = 5000
    };

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
        EXTCODEHASH = 0x3f,
        
        BLOCKHASH = 0x40,
        COINBASE = 0x41,
        TIMESTAMP = 0x42,
        NUMBER = 0x43,
        DIFFICULTY = 0x44,
        GASLIMIT = 0x45,
        CHAINID = 0x46,
        BASEFEE = 0x48,
        
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
        
        PUSH0 = 0x5f,
        PUSH1 = 0x60,
        PUSH2 = 0x61,
        PUSH3 = 0x62,
        PUSH4 = 0x63,
        PUSH5 = 0x64,
        PUSH6 = 0x65,
        PUSH7 = 0x66,
        PUSH8 = 0x67,
        PUSH9 = 0x68,
        PUSH10 = 0x69,
        PUSH11 = 0x6a,
        PUSH12 = 0x6b,
        PUSH13 = 0x6c,
        PUSH14 = 0x6d,
        PUSH15 = 0x6e,
        PUSH16 = 0x6f,
        PUSH17 = 0x70,
        PUSH18 = 0x71,
        PUSH19 = 0x72,
        PUSH20 = 0x73,
        PUSH21 = 0x74,
        PUSH22 = 0x75,
        PUSH23 = 0x76,
        PUSH24 = 0x77,
        PUSH25 = 0x78,
        PUSH26 = 0x79,
        PUSH27 = 0x7a,
        PUSH28 = 0x7b,
        PUSH29 = 0x7c,
        PUSH30 = 0x7d,
        PUSH31 = 0x7e,
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

class EVMExecutor::Impl {
public:
    Impl(std::shared_ptr<Memory> memory,
         std::shared_ptr<Stack> stack,
         std::shared_ptr<Storage> storage)
        : memory_(memory), stack_(stack), storage_(storage) {}
    
    ~Impl() = default;
    
private:
    std::shared_ptr<Memory> memory_;
    std::shared_ptr<Stack> stack_;
    std::shared_ptr<Storage> storage_;
};

EVMExecutor::EVMExecutor(std::shared_ptr<Memory> memory,
                        std::shared_ptr<Stack> stack,
                        std::shared_ptr<Storage> storage)
    : memory_(memory), stack_(stack), storage_(storage),
      impl_(std::make_unique<Impl>(memory, stack, storage)) {}

EVMExecutor::~EVMExecutor() = default;

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
                case STOP:
                    require_gas(ZERO);
                    return ExecutionResult{true, {}, gas_used_, ""};

                case ADD:
                case SUB:
                case OR:
                case XOR:
                case NOT:
                case BYTE:
                    require_gas(VERYLOW);
                    // ... arithmetic operations ...
                    break;

                case MUL:
                case DIV:
                case SDIV:
                case MOD:
                case SMOD:
                    require_gas(LOW);
                    // ... arithmetic operations ...
                    break;

                case ADDMOD:
                case MULMOD:
                    require_gas(MID);
                    // ... arithmetic operations ...
                    break;

                case SHA3:
                    require_gas(SHA3_GAS);
                    // ... SHA3 operation ...
                    break;

                case ADDRESS:
                case ORIGIN:
                case CALLER:
                case CALLVALUE:
                case CALLDATASIZE:
                case CODESIZE:
                case GASPRICE:
                case COINBASE:
                case TIMESTAMP:
                case NUMBER:
                case GASLIMIT:
                case CHAINID:
                case BASEFEE:
                    require_gas(BASE);
                    // ... context operations ...
                    break;

                case BALANCE:
                case EXTCODESIZE:
                case EXTCODEHASH:
                    require_gas(WARM_STORAGE_READ_COST);
                    // ... external state operations ...
                    break;

                case CALLDATALOAD:
                case MLOAD:
                case MSTORE:
                case MSTORE8:
                    require_gas(VERYLOW);
                    // ... memory operations ...
                    break;

                case SLOAD:
                    require_gas(WARM_STORAGE_READ_COST);
                    {
                        auto key = stack_->pop();
                        stack_->push(storage_->load(contract_address, key));
                    }
                    break;

                case SSTORE:
                    require_gas(SSTORE_SET_GAS);
                    {
                        auto value = stack_->pop();
                        auto key = stack_->pop();
                        storage_->store(contract_address, key, value);
                    }
                    break;

                case JUMP:
                    require_gas(MID);
                    // ... jump operation ...
                    break;

                case JUMPI:
                    require_gas(HIGH);
                    // ... conditional jump operation ...
                    break;

                case JUMPDEST:
                    require_gas(1);  // Special case: JUMPDEST costs exactly 1
                    break;

                case CREATE:
                    require_gas(CREATE_GAS);
                    // ... create operation ...
                    break;

                case CALL:
                case CALLCODE:
                    require_gas(WARM_STORAGE_READ_COST);
                    // ... call operations ...
                    break;

                case RETURN:
                case REVERT:
                    require_gas(ZERO);
                    // ... return/revert operations ...
                    break;

                case SELFDESTRUCT:
                    require_gas(SELFDESTRUCT_GAS);
                    // ... selfdestruct operation ...
                    break;

                case PUSH0:
                    require_gas(BASE);
                    stack_->push(uint256_t(0));
                    break;

                case PUSH1:
                case PUSH2:
                case PUSH3:
                case PUSH4:
                case PUSH5:
                case PUSH6:
                case PUSH7:
                case PUSH8:
                case PUSH9:
                case PUSH10:
                case PUSH11:
                case PUSH12:
                case PUSH13:
                case PUSH14:
                case PUSH15:
                case PUSH16:
                case PUSH17:
                case PUSH18:
                case PUSH19:
                case PUSH20:
                case PUSH21:
                case PUSH22:
                case PUSH23:
                case PUSH24:
                case PUSH25:
                case PUSH26:
                case PUSH27:
                case PUSH28:
                case PUSH29:
                case PUSH30:
                case PUSH31:
                case PUSH32:
                    {
                        require_gas(VERYLOW);
                        uint8_t push_bytes = (opcode - PUSH1) + 1;
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
                    throw std::runtime_error("Unknown or unimplemented opcode");
            }

            pc++;
        }

        return ExecutionResult{
            .success = true,
            .gas_used = gas_used_,
            .return_data = {},
            .error_message = ""
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