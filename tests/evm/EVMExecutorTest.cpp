#include <gtest/gtest.h>
#include "evm/EVMExecutor.hpp"
#include "node/QuidsConfig.hpp"
#include "evm/Address.hpp"

using namespace quids::evm;

class EVMExecutorTest : public ::testing::Test {
protected:
   quids::EVMConfig config;
    std::unique_ptr<quids::evm::EVMExecutor> executor;
    
    void SetUp() override {
        executor = std::make_unique<EVMExecutor>(config);
    }
};

TEST_F(EVMExecutorTest, BasicTransfer) {
    quids::blockchain::Transaction tx;
    tx.from = "0x1234";
    tx.to = "0x5678";
    tx.value = 1000;
    
    EXPECT_TRUE(executor->execute(tx));
    EXPECT_EQ(executor->getBalance("0x5678"), 1000);
    EXPECT_EQ(executor->getBalance("0x1234"), 0);
}

TEST_F(EVMExecutorTest, ContractDeployment) {
    std::vector<uint8_t> code = {
        0x60, 0x00,  // PUSH1 0
        0x60, 0x00,  // PUSH1 0
        0x52,        // MSTORE
        0x60, 0x20,  // PUSH1 32
        0x60, 0x00,  // PUSH1 0
        0xf3         // RETURN
    };
    
    EXPECT_TRUE(executor->deploy(code));
}

TEST_F(EVMExecutorTest, StorageOperations) {
    std::string address = "0x1234";
    ::evm::uint256_t key(123);
    std::vector<uint8_t> value = {1, 2, 3, 4};
    
    // Test storage operations
    auto stored = executor->getStorage(address, key);
    EXPECT_TRUE(stored.empty());
}

TEST_F(EVMExecutorTest, InsufficientBalance) {
    quids::blockchain::Transaction tx;
    tx.from = "0x1234";
    tx.to = "0x5678";
    tx.value = 1000000;  // More than available
    
    EXPECT_FALSE(executor->execute(tx));
}

TEST_F(EVMExecutorTest, InvalidContractCode) {
    std::vector<uint8_t> invalid_code = {
        0xFF,  // Invalid opcode
        0xFF
    };
    
    EXPECT_FALSE(executor->deploy(invalid_code));
}

TEST_F(EVMExecutorTest, ContractExecution) {
    // Deploy a simple contract that adds two numbers
    std::vector<uint8_t> code = {
        0x60, 0x02,  // PUSH1 2
        0x60, 0x03,  // PUSH1 3
        0x01,        // ADD
        0x60, 0x00,  // PUSH1 0
        0x52,        // MSTORE
        0x60, 0x20,  // PUSH1 32
        0x60, 0x00,  // PUSH1 0
        0xf3         // RETURN
    };
    
    EXPECT_TRUE(executor->deploy(code));
    
    // Execute the contract
    ::evm::Address contract_addr;
    std::fill(contract_addr.bytes.begin(), contract_addr.bytes.end(), 0x12); // Use begin() and end()
    auto result = executor->execute_contract(
        contract_addr,
        code,
        std::vector<uint8_t>{},
        100000
    );
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.gas_used, 21);  // Basic operations
}

TEST_F(EVMExecutorTest, OutOfGas) {
    ::evm::Address contract_addr;
    auto result = executor->execute_contract(
        contract_addr,
        std::vector<uint8_t>{0x5b},  // JUMPDEST in infinite loop
        std::vector<uint8_t>{},
        100  // Very low gas limit
    );
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.gas_used, 100);
}

TEST(EVMExecutorTest, BasicExecution) {
    quids::EVMConfig config; // Use fully qualified name
    ::evm::Address contract_addr; // Use default constructor
    // ... rest of the test ...
}
