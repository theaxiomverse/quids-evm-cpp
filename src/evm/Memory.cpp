#include "evm/Memory.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

namespace evm {

class Memory::Impl {
public:
    Impl() = default;
    ~Impl() = default;
    
    void store(uint64_t offset, const std::vector<uint8_t>& data) {
        ensureSize(offset + data.size());
        std::copy(data.begin(), data.end(), memory_.begin() + offset);
    }
    
    std::vector<uint8_t> load(uint64_t offset, size_t size) const {
        if (offset + size > memory_.size()) {
            throw std::out_of_range("Memory access out of bounds");
        }
        return std::vector<uint8_t>(memory_.begin() + offset, 
                                  memory_.begin() + offset + size);
    }
    
private:
    std::vector<uint8_t> memory_;
    
    void ensureSize(size_t required_size) {
        if (required_size > memory_.size()) {
            memory_.resize(required_size);
        }
    }
};

// Memory implementation
void Memory::store(size_t offset, uint8_t value) {
    ensure_capacity(offset, 1);
    data_[offset] = value;
}

void Memory::store(size_t offset, const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    ensure_capacity(offset, data.size());
    std::copy(data.begin(), data.end(), data_.begin() + offset);
}

uint8_t Memory::load(size_t offset) const {
    if (offset >= data_.size()) {
        throw std::out_of_range("Memory access out of bounds");
    }
    return data_[offset];
}

std::vector<uint8_t> Memory::load(size_t offset, size_t size) const {
    std::vector<uint8_t> result(size, 0);
    if (offset >= data_.size()) return result;
    
    size_t copy_size = std::min(size, data_.size() - offset);
    std::copy(data_.begin() + offset, data_.begin() + offset + copy_size, result.begin());
    return result;
}

uint64_t Memory::expand(size_t new_size) {
    if (new_size <= data_.size()) {
        return 0;
    }
    
    uint64_t cost = calculate_expansion_cost(0, new_size);
    data_.resize(new_size, 0);
    return cost;
}

uint64_t Memory::calculate_expansion_cost(size_t offset, size_t size) const {
    if (size == 0) return 0;
    
    size_t required_size = offset + size;
    if (required_size <= data_.size()) {
        return 0;
    }
    
    // Calculate words needed
    uint64_t words = calculate_words(required_size);
    uint64_t current_words = calculate_words(data_.size());
    
    // Gas formula from Yellow Paper
    uint64_t linear_cost = words * 3;
    uint64_t quadratic_cost = (words * words) / 512 - (current_words * current_words) / 512;
    
    return linear_cost + quadratic_cost;
}

bool Memory::is_zero(size_t offset, size_t size) const {
    if (offset + size > data_.size()) {
        throw std::out_of_range("Memory range check out of bounds");
    }
    
    for (size_t i = 0; i < size; i++) {
        if (data_[offset + i] != 0) {
            return false;
        }
    }
    return true;
}

std::string Memory::dump() const {
    return dump(0, data_.size());
}

std::string Memory::dump(size_t offset, size_t size) const {
    if (offset + size > data_.size()) {
        throw std::out_of_range("Memory dump range out of bounds");
    }
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (size_t i = 0; i < size; i++) {
        if (i > 0 && i % 32 == 0) {
            ss << "\n";
        }
        ss << std::setw(2) << static_cast<int>(data_[offset + i]) << " ";
    }
    
    return ss.str();
}

void Memory::ensure_capacity(size_t offset, size_t size) {
    size_t required_size = offset + size;
    if (required_size > data_.size()) {
        [[maybe_unused]] uint64_t gas_cost = expand(required_size);
        // Gas cost can be used for gas metering if needed
    }
}

uint64_t Memory::calculate_words(size_t size) const {
    return (size + 31) / 32;
}

void Memory::clear() {
    data_.clear();
}

Memory::Memory() = default;
Memory::~Memory() = default;

} // namespace evm 