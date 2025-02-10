#include <gtest/gtest.h>
#include "evm/uint256.hpp"
#include <unordered_map>

using namespace evm;

TEST(uint256Test, DefaultConstructor) {
    uint256_t value;
    EXPECT_EQ(value, 0);
}

TEST(uint256Test, IntegerConstructors) {
    uint256_t a(static_cast<uint64_t>(123));
    uint256_t b(static_cast<uint64_t>(456u));
    uint256_t c(static_cast<uint64_t>(789ll));
    
    EXPECT_EQ(a, uint256_t(123));
    EXPECT_EQ(b, uint256_t(456));
    EXPECT_EQ(c, uint256_t(789));
}

TEST(uint256Test, ArithmeticOperations) {
    uint256_t a(100);
    uint256_t b(50);
    
    EXPECT_EQ(a + b, 150);
    EXPECT_EQ(a - b, 50);
    EXPECT_EQ(a * b, 5000);
    EXPECT_EQ(a / b, 2);
    EXPECT_EQ(a % b, 0);
}

TEST(uint256Test, BitwiseOperations) {
    uint256_t a(0xFF);
    uint256_t b(0xF0);
    
    EXPECT_EQ(a & b, 0xF0);
    EXPECT_EQ(a | b, 0xFF);
    EXPECT_EQ(a ^ b, 0x0F);
    EXPECT_EQ(a << 4, 0xFF0);
    EXPECT_EQ(a >> 4, 0x0F);
}

TEST(uint256Test, ComparisonOperators) {
    uint256_t a(100);
    uint256_t b(200);
    
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a <= b);
    EXPECT_FALSE(a >= b);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(uint256Test, HashFunction) {
    std::unordered_map<uint256_t, int> map;
    uint256_t key(123);
    map[key] = 456;
    
    EXPECT_EQ(map[key], 456);
}

TEST(uint256Test, Overflow) {
    uint256_t max(std::numeric_limits<uint64_t>::max());
    uint256_t one(1);
    
    EXPECT_NO_THROW(max + one);
    auto result = max + one;
    EXPECT_GT(result, max);
}

TEST(uint256Test, DivisionByZero) {
    uint256_t a(100);
    uint256_t zero(0);
    
    EXPECT_THROW(a / zero, std::runtime_error);
    EXPECT_THROW(a % zero, std::runtime_error);
}

TEST(uint256Test, NegativeShift) {
    uint256_t a(0x1234);
    EXPECT_THROW(a << -1, std::runtime_error);
    EXPECT_THROW(a >> -1, std::runtime_error);
}

TEST(uint256Test, LargeNumbers) {
    uint256_t a = uint256_t(1) << 256 - 1;  // Max 256-bit value
    uint256_t b = a;
    
    EXPECT_NO_THROW(a * b);
    EXPECT_NO_THROW(a + b);
}