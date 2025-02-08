#include "evm/EVMExecutor.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace {
    // EVM opcodes
    enum Opcodes {
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

    // Gas costs
    constexpr uint64_t ZERO = 0;
    constexpr uint64_t BASE = 2;
    constexpr uint64_t VERYLOW = 3;
    constexpr uint64_t LOW = 5;
    constexpr uint64_t MID = 8;
    constexpr uint64_t HIGH = 10;
    constexpr uint64_t SHA3_GAS = 30;
    constexpr uint64_t CREATE_GAS = 32000;
    constexpr uint64_t SELFDESTRUCT_GAS = 5000;
    constexpr uint64_t WARM_STORAGE_READ_COST = 100;
    constexpr uint64_t SSTORE_SET_GAS = 20000;
}

namespace quids {
namespace evm {

EVMExecutor::EVMExecutor(const EVMConfig& config)
    : memory_(std::make_shared<::evm::Memory>())
    , stack_(std::make_shared<::evm::Stack>())
    , storage_(std::make_shared<::evm::Storage>())
    , config_(config)
    , impl_(std::make_unique<Impl>()) {
}

EVMExecutor::~EVMExecutor() = default;

EVMExecutor::ExecutionResult EVMExecutor::execute_contract(
    const ::evm::Address& contract_address,
    const std::vector<uint8_t>& code,
    const std::vector<uint8_t>& /*input_data*/,
    uint64_t gas_limit
) {
    gas_used_ = 0;
    gas_limit_ = gas_limit;

    ExecutionResult result{};
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
                    stack_->push(::evm::uint256_t(0));
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
                        ::evm::uint256_t value(0);
                        for (uint8_t i = 0; i < push_bytes; i++) {
                            value = (value << 8) | ::evm::uint256_t(code[pc + 1 + i]);
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

        result.success = true;
        result.gas_used = gas_used_;
        result.return_data = {};
        result.error_message = "";
    } catch (const std::exception& e) {
        result.success = false;
        result.gas_used = gas_used_;
        result.return_data = {};
        result.error_message = e.what();
    }
    return result;
}

void EVMExecutor::require_gas(uint64_t gas) {
    if (gas_used_ + gas > gas_limit_) {
        throw std::runtime_error("Out of gas");
    }
    gas_used_ += gas;
}

bool EVMExecutor::execute(const blockchain::Transaction& tx) {
    try {
        // Basic transaction execution
        auto& sender_balance = impl_->balances[tx.from];
        if (sender_balance < tx.value) {
            return false;
        }

        sender_balance -= tx.value;
        impl_->balances[tx.to] += tx.value;

        // Execute contract code if present
        if (!tx.data.empty() && !impl_->code[tx.to].empty()) {
            // TODO: Implement actual EVM execution
        }

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Transaction execution failed: {}", e.what());
        return false;
    }
}

bool EVMExecutor::deploy(const std::vector<uint8_t>& code) {
    try {
        // Validate code
        if (code.empty()) {
            return false;
        }

        // Basic code validation
        for (size_t i = 0; i < code.size(); i++) {
            if (code[i] == INVALID || code[i] == SELFDESTRUCT) {
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Contract deployment failed: {}", e.what());
        return false;
    }
}

uint64_t EVMExecutor::getBalance(const std::string& address) const {
    auto it = impl_->balances.find(address);
    return it != impl_->balances.end() ? it->second : 0;
}

std::vector<uint8_t> EVMExecutor::getCode(const std::string& address) const {
    auto it = impl_->code.find(address);
    return it != impl_->code.end() ? it->second : std::vector<uint8_t>{};
}

std::vector<uint8_t> EVMExecutor::getStorage(
    const std::string& address, 
    ::evm::uint256_t key
) const {
    auto account_it = impl_->storage.find(address);
    if (account_it == impl_->storage.end()) {
        return {};
    }
    auto storage_it = account_it->second.find(key);
    return storage_it != account_it->second.end() ? storage_it->second : std::vector<uint8_t>{};
}

} // namespace evm
} // namespace quids 