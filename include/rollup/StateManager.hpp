#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <map>
#include <deque>
#include "blockchain/Transaction.hpp"
#include "evm/Address.hpp"
#include "evm/uint256.hpp"

// Custom hasher for std::vector<unsigned char>
#include <cstddef>

namespace std {
    template<>
    struct hash<std::vector<unsigned char>> {
        size_t operator()(const std::vector<unsigned char>& v) const {
            size_t hash_val = 0;
            for (auto byte : v) {
                hash_val = hash_val * 31u + byte;
            }
            return hash_val;
        }
    };
}

namespace quids {
namespace rollup {



class StateManager {
public:
    struct Account {
        std::string address;
        uint64_t balance;  // Changed from uint256_t to uint64_t for now
        uint64_t nonce;
        std::vector<uint8_t> code;
        std::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>> storage;
        
        std::vector<uint8_t> serialize() const;
        static std::optional<Account> deserialize(const std::vector<uint8_t>& data);
    };

    StateManager();
    ~StateManager();

    // State management
    bool apply_transaction(const blockchain::Transaction& tx);
    bool verify_transaction(const blockchain::Transaction& tx) const;
    bool revert_transaction(const blockchain::Transaction& tx);
    bool commit_state();
    bool rollback_state();

    // State queries
    uint64_t get_balance(const std::string& address) const;
    uint64_t get_nonce(const std::string& address) const;
    std::vector<uint8_t> get_storage(const ::evm::Address& address, const std::vector<uint8_t>& key) const;
    std::vector<uint8_t> get_code(const ::evm::Address& address) const;
    std::vector<uint8_t> get_state_root() const;
    std::vector<uint8_t> get_previous_root() const;
    std::optional<Account> get_account(const std::string& address) const;
    std::map<std::string, Account> get_accounts_snapshot() const;

    // State modifications
    bool set_balance(const std::string& address, uint64_t balance);
    bool set_nonce(const std::string& address, uint64_t nonce);
    bool set_storage(const ::evm::Address& address, const std::vector<uint8_t>& key, const std::vector<uint8_t>& value);
    bool set_code(const ::evm::Address& address, const std::vector<uint8_t>& code);
    void add_account(std::string address, Account account);

    // Transaction management
    bool apply_transactions(const std::vector<blockchain::Transaction>& txs);
    std::vector<blockchain::Transaction> get_account_history(const std::string& address) const;
    void record_transaction(const std::string& address, const blockchain::Transaction& tx);

    std::unique_ptr<StateManager> clone() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    mutable std::shared_mutex mutex_;
};

} // namespace rollup
} // namespace quids 