#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace evm {

class Memory {
public:
    // Constructors and destructor
    Memory();
    ~Memory();  // Non-inline destructor declaration

    // Disable copy and move
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;
    Memory(Memory&&) = delete;
    Memory& operator=(Memory&&) = delete;
    
    // Basic memory operations
    void store(size_t offset, uint8_t value);
    void store(size_t offset, const std::vector<uint8_t>& data);
    [[nodiscard]] uint8_t load(size_t offset) const;
    [[nodiscard]] std::vector<uint8_t> load(size_t offset, size_t size) const;
    
    // Memory expansion and gas calculation
    [[nodiscard]] uint64_t expand(size_t new_size);
    [[nodiscard]] uint64_t calculate_expansion_cost(size_t offset, size_t size) const;
    
    // Memory state
    [[nodiscard]] size_t size() const { return data_.size(); }
    [[nodiscard]] bool is_zero(size_t offset, size_t size) const;
    void clear();
    
    // Debug helpers
    [[nodiscard]] std::string dump() const;
    [[nodiscard]] std::string dump(size_t offset, size_t size) const;

private:
    std::vector<uint8_t> data_;
    
    void ensure_capacity(size_t offset, size_t size);
    [[nodiscard]] uint64_t calculate_words(size_t size) const;

    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace evm 