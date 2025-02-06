#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace evm {

class Memory {
public:
    Memory() = default;
    ~Memory() = default;
    
    // Basic memory operations
    void store(size_t offset, uint8_t value);
    void store(size_t offset, const std::vector<uint8_t>& data);
    uint8_t load(size_t offset) const;
    std::vector<uint8_t> load(size_t offset, size_t size) const;
    
    // Memory expansion and gas calculation
    uint64_t expand(size_t new_size);
    uint64_t calculate_expansion_cost(size_t offset, size_t size) const;
    
    // Memory state
    size_t size() const { return data_.size(); }
    bool is_zero(size_t offset, size_t size) const;
    void clear();
    
    // Debug helpers
    std::string dump() const;
    std::string dump(size_t offset, size_t size) const;

private:
    std::vector<uint8_t> data_;
    
    void ensure_capacity(size_t offset, size_t size);
    uint64_t calculate_words(size_t size) const;
};

} // namespace evm 