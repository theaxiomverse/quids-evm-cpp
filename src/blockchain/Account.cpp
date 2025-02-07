#include "blockchain/Account.hpp"
#include <spdlog/spdlog.h>

namespace quids {
namespace blockchain {

Account::Account(const std::string& addr) 
    : address(addr), balance(0), nonce(0), is_contract(false) {
}

} // namespace blockchain
} // namespace quids 