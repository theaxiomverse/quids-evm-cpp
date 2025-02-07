#pragma once

#include <map>
#include <vector>
#include "evm/uint256.hpp"
#include "evm/Address.hpp"

namespace evm {

class Storage {
public:
    Storage() = default;
    ~Storage() = default;

    // Contract storage operations
    void store(const Address& address, const uint256_t& key, const uint256_t& value);
    uint256_t load(const Address& address, const uint256_t& key) const;
    bool contains(const Address& address, const uint256_t& key) const;
    void clear(const Address& address);
    size_t size(const Address& address) const;
    void clear();

private:
    std::map<Address, std::map<uint256_t, uint256_t>> storage_;
};

} // namespace evm 