#include <gtest/gtest.h>
#include "evm/uint256.hpp"
#include <unordered_map>

using namespace evm;

TEST(uint256Test, DefaultConstructor) {
    uint256_t value;
    EXPECT_EQ(value, 0);
}

TEST(uint256Test, IntegerConstructors) {
    uint256_t a(123);
    uint256_t b(456u);
    uint256_t c(789ll);
    
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
    EXPECT_EQ(c, 789);
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
    uint256_t a("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    uint256_t b("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    
    EXPECT_NO_THROW(a * b);
    EXPECT_NO_THROW(a + b);
}

TEST(uint256Test, StringConversion) {
    uint256_t a("123456789");
    EXPECT_EQ(a.to_string(), "123456789");
    
    uint256_t b("0xff");
    EXPECT_EQ(b.to_string(16), "ff");
} 