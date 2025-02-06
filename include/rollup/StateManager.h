#pragma once
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "blockchain/Transaction.h"

class StateManager {
public:
    struct Account {
        std::string address;
        uint64_t balance;
        uint64_t nonce;
        
        std::vector<uint8_t> serialize() const;
    };
    
    StateManager();
    StateManager(const StateManager& other);
    
    bool apply_transaction(const Transaction& tx);
    Account get_account(const std::string& address) const;
    void add_account(const std::string& address, const Account& account);
    std::vector<Transaction> get_account_history(const std::string& address) const;
    std::array<uint8_t, 32> get_state_root() const;
    const std::map<std::string, Account>& get_all_accounts() const { return accounts_; }

private:
    mutable std::mutex mutex_;
    std::map<std::string, Account> accounts_;
    std::map<std::string, std::vector<Transaction>> history_;
}; 