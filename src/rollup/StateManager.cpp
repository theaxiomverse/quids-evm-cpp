#include "rollup/StateManager.hpp"
#include <stdexcept>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <shared_mutex>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include <blake3.h>
#include <deque>

namespace quids {
namespace rollup {

    // Helper function to convert address to hex string
    std::string address_to_hex(const ::evm::Address& address) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : address.bytes) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    // Helper function to calculate state root
    std::vector<uint8_t> calculate_state_root(const std::unordered_map<std::string, StateManager::Account>& accounts) {
        blake3_hasher hasher;
        blake3_hasher_init(&hasher);

        // Sort accounts by address for deterministic root
        std::vector<std::pair<std::string, const StateManager::Account*>> sorted_accounts;
        sorted_accounts.reserve(accounts.size());
        for (const auto& [addr, account] : accounts) {
            sorted_accounts.emplace_back(addr, &account);
        }
        std::sort(sorted_accounts.begin(), sorted_accounts.end());

        // Hash each account's state
        for (const auto& [addr, account] : sorted_accounts) {
            std::vector<uint8_t> account_data = account->serialize();
            blake3_hasher_update(&hasher, account_data.data(), account_data.size());
        }

        std::vector<uint8_t> root(BLAKE3_OUT_LEN);
        blake3_hasher_finalize(&hasher, root.data(), root.size());
        return root;
    }


struct StateManager::Impl {
    std::unordered_map<std::string, Account> accounts;
    std::vector<uint8_t> current_state_root;
    std::vector<uint8_t> previous_state_root;
    std::unordered_map<std::string, std::deque<blockchain::Transaction>> history;
    static constexpr size_t MAX_HISTORY_PER_ACCOUNT = 1000;
};

StateManager::StateManager() : impl_(std::make_unique<Impl>()) {
    impl_->current_state_root = calculate_state_root(impl_->accounts);
    impl_->previous_state_root = impl_->current_state_root;
}

StateManager::~StateManager() = default;

void StateManager::add_account(std::string address, Account account) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    impl_->accounts[std::move(address)] = std::move(account);
}

bool StateManager::verify_transaction(const blockchain::Transaction& tx) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // Verify sender exists
    auto sender = get_account(tx.getSender());
    if (!sender) {
        return false;
    }
    
    // Verify nonce
    if (tx.getNonce() != sender->nonce + 1) {
        return false;
    }
    
    // Verify sufficient balance
    uint64_t total_cost = tx.getAmount() + tx.calculate_gas_cost();
    if (sender->balance < total_cost) {
        return false;
    }
    
    // Verify signature
    return tx.verify();
}

bool StateManager::apply_transactions(const std::vector<quids::blockchain::Transaction>& txs) {
    std::shared_lock<std::shared_mutex> read_lock(mutex_);
    
    // Verify all transactions first
    std::vector<bool> valid_txs(txs.size());
    for (size_t i = 0; i < txs.size(); i++) {
        valid_txs[i] = verify_transaction(txs[i]);
    }
    
    // Release read lock before acquiring write lock
    read_lock.unlock();
    
    std::unique_lock<std::shared_mutex> write_lock(mutex_);
    
    // Store current state root before modification
    impl_->previous_state_root = impl_->current_state_root;
    
    // Apply all valid transactions
    bool success = true;
    for (size_t i = 0; i < txs.size(); i++) {
        if (valid_txs[i]) {
            if (!apply_transaction(txs[i])) {
                success = false;
                break;
            }
        }
    }
    
    return success;
}

std::optional<StateManager::Account> StateManager::get_account(const std::string& address) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address);
    if (it != impl_->accounts.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<quids::blockchain::Transaction> StateManager::get_account_history(const std::string& address) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->history.find(address);
    if (it != impl_->history.end()) {
        return std::vector<quids::blockchain::Transaction>(it->second.begin(), it->second.end());
    }
    return {};
}

std::vector<uint8_t> StateManager::get_state_root() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return impl_->current_state_root;
}

std::vector<uint8_t> StateManager::get_previous_root() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return impl_->previous_state_root;
}

void StateManager::record_transaction(const std::string& address, const quids::blockchain::Transaction& tx) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto& history = impl_->history[address];
    history.push_back(tx);
    
    // Keep history size bounded
    if (history.size() > Impl::MAX_HISTORY_PER_ACCOUNT) {
        history.pop_front();
    }
}

std::vector<uint8_t> StateManager::Account::serialize() const {
    std::vector<uint8_t> result;
    
    // Calculate total size needed
    size_t total_size = address.size() + sizeof(balance) + sizeof(nonce) + sizeof(size_t);
    
    // Add size for code
    total_size += code.size() + sizeof(size_t);
    
    // Add size for storage
    for (const auto& [key, value] : storage) {
        total_size += key.size() + value.size() + 2 * sizeof(size_t);
    }
    
    result.reserve(total_size);
    
    // Serialize address
    size_t addr_size = address.size();
    result.insert(result.end(), 
                 reinterpret_cast<const uint8_t*>(&addr_size),
                 reinterpret_cast<const uint8_t*>(&addr_size) + sizeof(size_t));
    result.insert(result.end(), address.begin(), address.end());
    
    // Serialize balance and nonce
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&balance),
                 reinterpret_cast<const uint8_t*>(&balance) + sizeof(balance));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&nonce),
                 reinterpret_cast<const uint8_t*>(&nonce) + sizeof(nonce));
    
    // Serialize code
    size_t code_size = code.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&code_size),
                 reinterpret_cast<const uint8_t*>(&code_size) + sizeof(size_t));
    result.insert(result.end(), code.begin(), code.end());
    
    // Serialize storage
    size_t storage_size = storage.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&storage_size),
                 reinterpret_cast<const uint8_t*>(&storage_size) + sizeof(size_t));
    
    for (const auto& [key, value] : storage) {
        size_t key_size = key.size();
        size_t value_size = value.size();
        
        result.insert(result.end(),
                     reinterpret_cast<const uint8_t*>(&key_size),
                     reinterpret_cast<const uint8_t*>(&key_size) + sizeof(size_t));
        result.insert(result.end(), key.begin(), key.end());
        
        result.insert(result.end(),
                     reinterpret_cast<const uint8_t*>(&value_size),
                     reinterpret_cast<const uint8_t*>(&value_size) + sizeof(size_t));
        result.insert(result.end(), value.begin(), value.end());
    }
    
    return result;
}

std::optional<StateManager::Account> StateManager::Account::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(size_t)) {
        return std::nullopt;
    }
    
    Account account;
    size_t offset = 0;
    
    // Deserialize address
    size_t addr_size;
    std::memcpy(&addr_size, data.data() + offset, sizeof(size_t));
    offset += sizeof(size_t);
    
    if (offset + addr_size > data.size()) {
        return std::nullopt;
    }
    account.address = std::string(data.begin() + offset, data.begin() + offset + addr_size);
    offset += addr_size;
    
    // Deserialize balance and nonce
    if (offset + sizeof(uint64_t) * 2 > data.size()) {
        return std::nullopt;
    }
    std::memcpy(&account.balance, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    std::memcpy(&account.nonce, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    
    // Deserialize code
    if (offset + sizeof(size_t) > data.size()) {
        return std::nullopt;
    }
    size_t code_size;
    std::memcpy(&code_size, data.data() + offset, sizeof(size_t));
    offset += sizeof(size_t);
    
    if (offset + code_size > data.size()) {
        return std::nullopt;
    }
    account.code = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + code_size);
    offset += code_size;
    
    // Deserialize storage
    if (offset + sizeof(size_t) > data.size()) {
        return std::nullopt;
    }
    size_t storage_size;
    std::memcpy(&storage_size, data.data() + offset, sizeof(size_t));
    offset += sizeof(size_t);
    
    for (size_t i = 0; i < storage_size; i++) {
        if (offset + sizeof(size_t) > data.size()) {
            return std::nullopt;
        }
        size_t key_size;
        std::memcpy(&key_size, data.data() + offset, sizeof(size_t));
        offset += sizeof(size_t);
        
        if (offset + key_size > data.size()) {
            return std::nullopt;
        }
        std::vector<uint8_t> key(data.begin() + offset, data.begin() + offset + key_size);
        offset += key_size;
        
        if (offset + sizeof(size_t) > data.size()) {
            return std::nullopt;
        }
        size_t value_size;
        std::memcpy(&value_size, data.data() + offset, sizeof(size_t));
        offset += sizeof(size_t);
        
        if (offset + value_size > data.size()) {
            return std::nullopt;
        }
        std::vector<uint8_t> value(data.begin() + offset, data.begin() + offset + value_size);
        offset += value_size;
        
        account.storage[key] = value;
    }
    
    return account;
}

std::map<std::string, StateManager::Account> StateManager::get_accounts_snapshot() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return std::map<std::string, Account>(impl_->accounts.begin(), impl_->accounts.end());
}

bool StateManager::apply_transaction(const blockchain::Transaction& tx) {
    std::unique_lock<std::shared_mutex> write_lock(mutex_);
    
    auto sender = get_account(tx.getSender());
    auto recipient = get_account(tx.getRecipient());
    
    if (!sender || !recipient) {
        return false;
    }

    // Verify nonce
    if (tx.getNonce() != sender->nonce + 1) {
        return false;
    }

    // Calculate total cost including gas
    uint64_t total_cost = tx.getAmount() + tx.calculate_gas_cost();
    
    // Verify sufficient balance
    if (sender->balance < total_cost) {
        return false;
    }

    // Verify signature
    if (!tx.verify()) {
        return false;
    }

    // Apply transaction
    sender->balance -= total_cost;
    recipient->balance += tx.getAmount();
    sender->nonce++;

    // Update accounts
    impl_->accounts[sender->address] = *sender;
    impl_->accounts[recipient->address] = *recipient;

    // Record transaction
    record_transaction(sender->address, tx);
    record_transaction(recipient->address, tx);

    return true;
}

bool StateManager::revert_transaction(const blockchain::Transaction& tx) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto sender = get_account(tx.getSender());
    auto recipient = get_account(tx.getRecipient());
    
    if (!sender || !recipient) {
        return false;
    }
    
    // Revert balances
    sender->balance += tx.getAmount() + tx.calculate_gas_cost();
    recipient->balance -= tx.getAmount();
    sender->nonce--;
    
    // Update accounts
    impl_->accounts[sender->address] = *sender;
    impl_->accounts[recipient->address] = *recipient;
    
    return true;
}

bool StateManager::commit_state() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    impl_->previous_state_root = impl_->current_state_root;
    impl_->current_state_root = calculate_state_root(impl_->accounts);
    return true;
}

bool StateManager::rollback_state() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    impl_->current_state_root = impl_->previous_state_root;
    return true;
}

uint64_t StateManager::get_balance(const std::string& address) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address);
    if (it != impl_->accounts.end()) {
        return it->second.balance;
    }
    return 0;
}

uint64_t StateManager::get_nonce(const std::string& address) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address);
    if (it != impl_->accounts.end()) {
        return it->second.nonce;
    }
    return 0;
}

std::vector<uint8_t> StateManager::get_storage(const ::evm::Address& address, const std::vector<uint8_t>& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address_to_hex(address));
    if (it != impl_->accounts.end()) {
        auto storage_it = it->second.storage.find(key);
        if (storage_it != it->second.storage.end()) {
            return storage_it->second;
        }
    }
    return std::vector<uint8_t>();
}

std::vector<uint8_t> StateManager::get_code(const ::evm::Address& address) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address_to_hex(address));
    if (it != impl_->accounts.end()) {
        return it->second.code;
    }
    return std::vector<uint8_t>();
}

bool StateManager::set_balance(const std::string& address, uint64_t balance) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address);
    if (it != impl_->accounts.end()) {
        it->second.balance = balance;
        return true;
    }
    return false;
}

bool StateManager::set_nonce(const std::string& address, uint64_t nonce) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address);
    if (it != impl_->accounts.end()) {
        it->second.nonce = nonce;
        return true;
    }
    return false;
}

bool StateManager::set_storage(const ::evm::Address& address, const std::vector<uint8_t>& key, const std::vector<uint8_t>& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address_to_hex(address));
    if (it != impl_->accounts.end()) {
        it->second.storage[key] = value;
        return true;
    }
    return false;
}

bool StateManager::set_code(const ::evm::Address& address, const std::vector<uint8_t>& code) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = impl_->accounts.find(address_to_hex(address));
    if (it != impl_->accounts.end()) {
        it->second.code = code;
        return true;
    }
    return false;
}

std::unique_ptr<StateManager> StateManager::clone() const {
    // Create a new StateManager and copy internal data.
    auto new_state = std::make_unique<StateManager>();
    {
        // Acquire lock if needed (assuming 'mutex_' exists).
        std::shared_lock<std::shared_mutex> lock(mutex_);
        new_state->impl_->accounts = impl_->accounts;
        new_state->impl_->current_state_root = impl_->current_state_root;
        new_state->impl_->previous_state_root = impl_->previous_state_root;
        new_state->impl_->history = impl_->history;
    }
    return new_state;
}

} // namespace rollup
} // namespace quids 