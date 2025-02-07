#include "evm/Storage.hpp"
#include <stdexcept>

namespace evm {

void Storage::store(const Address& address, const uint256_t& key, const uint256_t& value) {
    storage_[address][key] = value;
}

uint256_t Storage::load(const Address& address, const uint256_t& key) const {
    auto it = storage_.find(address);
    if (it == storage_.end()) {
        return uint256_t(0);
    }
    
    auto value_it = it->second.find(key);
    if (value_it == it->second.end()) {
        return uint256_t(0);
    }
    
    return value_it->second;
}

void Storage::clear(const Address& address) {
    storage_.erase(address);
}

bool Storage::contains(const Address& address, const uint256_t& key) const {
    auto it = storage_.find(address);
    if (it == storage_.end()) {
        return false;
    }
    return it->second.find(key) != it->second.end();
}

size_t Storage::size(const Address& address) const {
    auto it = storage_.find(address);
    if (it == storage_.end()) {
        return 0;
    }
    return it->second.size();
}

void Storage::clear() {
    storage_.clear();
}

} // namespace evm 