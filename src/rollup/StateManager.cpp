#include "rollup/StateManager.hpp"
#include <openssl/sha.h>
#include <blake3.h>
#include <stdexcept>
#include <omp.h>

namespace quids {
namespace rollup {

StateManager::StateManager() = default;

StateManager::StateManager(const StateManager& other) {
    std::shared_lock<std::shared_mutex> lock(other.mutex_);
    accounts_ = other.accounts_;
    history_ = other.history_;
}

StateManager& StateManager::operator=(const StateManager& other) {
    if (this != &other) {
        std::unique_lock<std::shared_mutex> lock1(mutex_, std::defer_lock);
        std::shared_lock<std::shared_mutex> lock2(other.mutex_, std::defer_lock);
        std::lock(lock1, lock2);
        
        accounts_ = other.accounts_;
        history_ = other.history_;
    }
    return *this;
}

void StateManager::add_account(std::string address, Account account) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    accounts_[std::move(address)] = std::move(account);
}

bool StateManager::apply_transactions(const std::vector<quids::blockchain::Transaction>& txs) {
    std::shared_lock read_lock(mutex_);  // Allow concurrent reads
    
    // Pre-validate all transactions
    std::vector<bool> valid_txs(txs.size());
    #pragma omp parallel for
    for (size_t i = 0; i < txs.size(); i++) {
        valid_txs[i] = verify_transaction(txs[i]);
    }
    
    read_lock.unlock();
    std::unique_lock write_lock(mutex_);
    
    // Apply all valid transactions
    bool all_success = true;
    for (size_t i = 0; i < txs.size(); i++) {
        if (valid_txs[i]) {
            if (!apply_transaction(txs[i])) {
                all_success = false;
            }
        } else {
            all_success = false;
        }
    }
    
    return all_success;
}

std::optional<StateManager::Account> StateManager::get_account(const std::string& address) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = accounts_.find(address);
    if (it != accounts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<quids::blockchain::Transaction> StateManager::get_account_history(const std::string& address) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = history_.find(address);
    if (it != history_.end()) {
        return it->second;
    }
    return {};
}

std::array<uint8_t, 32> StateManager::get_state_root() const {
    std::array<uint8_t, 32> root{};
    
    // Compute Merkle root of all account states
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        for (const auto& [address, account] : accounts_) {
            auto account_bytes = account.serialize();
            // TODO: Implement proper Merkle tree computation
            // For now, just XOR all account bytes
            for (size_t i = 0; i < std::min(account_bytes.size(), root.size()); i++) {
                root[i] ^= account_bytes[i];
            }
        }
    }
    
    return root;
}

void StateManager::record_transaction(const std::string& address, const quids::blockchain::Transaction& tx) {
    auto& history = history_[address];
    history.push_back(tx);
    
    // Keep history size bounded
    if (history.size() > MAX_HISTORY_PER_ACCOUNT) {
        history.erase(history.begin());
    }
}

std::vector<uint8_t> StateManager::Account::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(address.size() + sizeof(balance) + sizeof(nonce));
    
    // Serialize address
    result.insert(result.end(), address.begin(), address.end());
    
    // Serialize balance
    const uint8_t* balance_bytes = reinterpret_cast<const uint8_t*>(&balance);
    result.insert(result.end(), balance_bytes, balance_bytes + sizeof(balance));
    
    // Serialize nonce
    const uint8_t* nonce_bytes = reinterpret_cast<const uint8_t*>(&nonce);
    result.insert(result.end(), nonce_bytes, nonce_bytes + sizeof(nonce));
    
    return result;
}

std::optional<StateManager::Account> StateManager::Account::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint64_t) * 2) {
        return std::nullopt;
    }
    
    Account account;
    
    // Deserialize address (variable length)
    size_t address_size = data.size() - sizeof(uint64_t) * 2;
    account.address = std::string(data.begin(), data.begin() + address_size);
    
    // Deserialize balance
    std::memcpy(&account.balance, data.data() + address_size, sizeof(uint64_t));
    
    // Deserialize nonce
    std::memcpy(&account.nonce, data.data() + address_size + sizeof(uint64_t), sizeof(uint64_t));
    
    return account;
}

std::map<std::string, StateManager::Account> StateManager::get_accounts_snapshot() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return accounts_;
}

bool StateManager::verify_transaction(const blockchain::Transaction& tx) const {
    // TODO: Implement proper verification
    return true;
}

bool StateManager::apply_transaction(const blockchain::Transaction& tx) {
    if (!verify_transaction(tx)) {
        return false;
    }
    try {
        // TODO: Implement state changes
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace rollup
} // namespace quids 