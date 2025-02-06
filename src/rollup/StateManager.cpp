#include "rollup/StateManager.h"
#include <openssl/sha.h>
#include <blake3.h>

StateManager::StateManager() {}

StateManager::StateManager(const StateManager& other) {
    std::lock_guard<std::mutex> lock(other.mutex_);
    
    // Copy accounts
    for (const auto& [address, account] : other.accounts_) {
        accounts_[address] = account;
        accounts_[address].address = address;  // Ensure address is set correctly
    }
    
    // Copy history
    for (const auto& [address, history] : other.history_) {
        history_[address] = history;
    }
}

void StateManager::add_account(const std::string& address, const Account& account) {
    std::lock_guard<std::mutex> lock(mutex_);
    accounts_[address] = account;
    accounts_[address].address = address;  // Ensure address is set correctly
}

bool StateManager::apply_transaction(const Transaction& tx) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify transaction signature
    if (!tx.verify()) {
        return false;
    }
    
    // Check if accounts exist
    if (accounts_.find(tx.sender) == accounts_.end() ||
        accounts_.find(tx.recipient) == accounts_.end()) {
        return false;
    }
    
    // Get sender and recipient accounts
    auto& sender = accounts_.at(tx.sender);
    auto& recipient = accounts_.at(tx.recipient);
    
    // Verify sender has sufficient balance
    if (sender.balance < tx.amount) {
        return false;
    }
    
    // Verify nonce
    if (tx.nonce != sender.nonce + 1) {
        return false;
    }
    
    // Update balances
    sender.balance -= tx.amount;
    recipient.balance += tx.amount;
    sender.nonce++;
    
    // Record transaction in history
    history_[tx.sender].push_back(tx);
    history_[tx.recipient].push_back(tx);
    
    return true;
}

StateManager::Account StateManager::get_account(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return accounts_.at(address);
}

std::vector<Transaction> StateManager::get_account_history(
    const std::string& address
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_.at(address);
}

std::array<uint8_t, 32> StateManager::get_state_root() const {
    std::array<uint8_t, 32> hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    
    // Create a sorted vector of accounts to ensure consistent ordering
    std::vector<std::pair<std::string, Account>> sorted_accounts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sorted_accounts = std::vector<std::pair<std::string, Account>>(
            accounts_.begin(), accounts_.end()
        );
    }
    
    // If no accounts, return a default hash
    if (sorted_accounts.empty()) {
        blake3_hasher_finalize(&hasher, hash.data(), BLAKE3_OUT_LEN);
        return hash;
    }
    
    std::sort(sorted_accounts.begin(), sorted_accounts.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    
    // Hash accounts in sorted order
    for (const auto& [address, account] : sorted_accounts) {
        const auto& account_data = account.serialize();
        if (!account_data.empty()) {
            blake3_hasher_update(&hasher, account_data.data(), account_data.size());
        }
    }
    
    blake3_hasher_finalize(&hasher, hash.data(), BLAKE3_OUT_LEN);
    return hash;
}

std::vector<uint8_t> StateManager::Account::serialize() const {
    std::vector<uint8_t> data;
    
    // Serialize address
    data.insert(data.end(),
               address.begin(),
               address.end());
    
    // Serialize balance
    data.insert(data.end(),
               reinterpret_cast<const uint8_t*>(&balance),
               reinterpret_cast<const uint8_t*>(&balance) + sizeof(balance));
               
    // Serialize nonce
    data.insert(data.end(),
               reinterpret_cast<const uint8_t*>(&nonce),
               reinterpret_cast<const uint8_t*>(&nonce) + sizeof(nonce));
               
    return data;
} 