#include "evm/Address.hpp"
#include <algorithm>

namespace evm {

bool Address::operator==(const Address& other) const {
    return bytes == other.bytes;
}

bool Address::operator<(const Address& other) const {
    return std::lexicographical_compare(
        bytes.begin(), bytes.end(),
        other.bytes.begin(), other.bytes.end()
    );
}

} // namespace evm 