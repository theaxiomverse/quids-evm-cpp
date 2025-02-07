#include "evm/Stack.hpp"
#include <stdexcept>

namespace evm {

void Stack::push(const uint256_t& value) {
    if (items_.size() >= 1024) {
        throw std::runtime_error("Stack overflow");
    }
    items_.push_back(value);
}

uint256_t Stack::pop() {
    if (items_.empty()) {
        throw std::runtime_error("Stack underflow");
    }
    uint256_t value = items_.back();
    items_.pop_back();
    return value;
}

uint256_t Stack::peek(size_t depth) const {
    if (items_.size() <= depth) {
        throw std::runtime_error("Stack underflow");
    }
    return items_[items_.size() - 1 - depth];
}

void Stack::swap(size_t n) {
    if (n == 0 || n > 16) {
        throw std::invalid_argument("Invalid swap depth");
    }
    if (items_.size() <= n) {
        throw std::runtime_error("Stack underflow");
    }
    
    std::swap(items_[items_.size() - 1], items_[items_.size() - 1 - n]);
}

void Stack::dup(size_t n) {
    if (n == 0 || n > 16) {
        throw std::invalid_argument("Invalid dup depth");
    }
    if (items_.size() < n) {
        throw std::runtime_error("Stack underflow");
    }
    
    push(items_[items_.size() - n]);
}

size_t Stack::size() const {
    return items_.size();
}

void Stack::clear() {
    items_.clear();
}

bool Stack::empty() const {
    return items_.empty();
}

} // namespace evm 