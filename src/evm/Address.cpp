#include "evm/Address.h"
#include <algorithm>

namespace evm {

bool Address::operator==(const Address& other) const {
    return bytes == other.bytes;
}

bool Address::operator<(const Address& other) const {
    return bytes < other.bytes;
}

} // namespace evm 