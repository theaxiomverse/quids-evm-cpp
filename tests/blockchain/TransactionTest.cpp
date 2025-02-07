#include <gtest/gtest.h>
#include "blockchain/Transaction.hpp"
#include <array>

using namespace quids::blockchain;

class TransactionTest : public ::testing::Test {
protected:
    std::array<uint8_t, 32> test_private_key{};
    
    void SetUp() override {
        // Initialize test private key
        for (size_t i = 0; i < test_private_key.size(); i++) {
            test_private_key[i] = static_cast<uint8_t>(i);
        }
    }
};

TEST_F(TransactionTest, DefaultConstructor) {
    Transaction tx;
    EXPECT_TRUE(tx.from.empty());
    EXPECT_TRUE(tx.to.empty());
    EXPECT_EQ(tx.value, 0);
    EXPECT_EQ(tx.gas_price, 0);
    EXPECT_EQ(tx.gas_limit, 21000);
    EXPECT_TRUE(tx.data.empty());
    EXPECT_TRUE(tx.signature.empty());
    EXPECT_EQ(tx.nonce, 0);
}

TEST_F(TransactionTest, ParameterizedConstructor) {
    Transaction tx("sender", "recipient", 1000);
    EXPECT_EQ(tx.from, "sender");
    EXPECT_EQ(tx.to, "recipient");
    EXPECT_EQ(tx.value, 1000);
}

TEST_F(TransactionTest, SignAndVerify) {
    Transaction tx("sender", "recipient", 1000);
    EXPECT_TRUE(tx.sign(test_private_key));
    EXPECT_TRUE(tx.verify());
}

TEST_F(TransactionTest, InvalidTransaction) {
    Transaction tx;
    EXPECT_FALSE(tx.is_valid());
}

TEST_F(TransactionTest, GasCalculations) {
    Transaction tx;
    tx.gas_limit = 21000;
    tx.gas_price = 100;
    tx.value = 1000;
    
    EXPECT_EQ(tx.calculate_gas_cost(), 2100000);
    EXPECT_EQ(tx.calculate_total_cost(), 2101000);
}

TEST_F(TransactionTest, MaxGasLimit) {
    Transaction tx;
    tx.gas_limit = Transaction::MAX_GAS_LIMIT + 1;
    EXPECT_FALSE(tx.is_valid());
}

TEST_F(TransactionTest, InvalidSignature) {
    Transaction tx("sender", "recipient", 1000);
    tx.signature = std::vector<uint8_t>(63, 0);  // Wrong size signature
    EXPECT_FALSE(tx.verify());
}

TEST_F(TransactionTest, LargeDataPayload) {
    Transaction tx;
    tx.data = std::vector<uint8_t>(Transaction::MAX_DATA_SIZE + 1, 0);
    EXPECT_FALSE(tx.is_valid());
}

TEST_F(TransactionTest, ZeroValue) {
    Transaction tx("sender", "recipient", 0);
    EXPECT_FALSE(tx.is_valid());
}

TEST_F(TransactionTest, EmptyAddresses) {
    Transaction tx("", "", 1000);
    EXPECT_FALSE(tx.is_valid());
} 