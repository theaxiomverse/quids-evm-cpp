#pragma once

#include <vector>
#include "evm/uint256.hpp"

namespace evm {

class Stack {
public:
    Stack() = default;
    ~Stack() = default;

    void push(const uint256_t& value);
    uint256_t pop();
    uint256_t peek(size_t depth = 0) const;
    void swap(size_t n);
    void dup(size_t n);
    size_t size() const;
    void clear();
    bool empty() const;

private:
    std::vector<uint256_t> items_;
};

} // namespace evm 