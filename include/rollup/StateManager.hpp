#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <optional>
#include <chrono>
#include "blockchain/Transaction.hpp"

namespace quids {
namespace rollup {

class StateManager {
public:
    struct Account {
        std::string address;
        uint64_t balance;
        uint64_t nonce;
        std::chrono::system_clock::time_point last_update;
        
        // Default constructor
        Account()
            : balance(0)
            , nonce(0)
            , last_update(std::chrono::system_clock::now())
        {}
        
        // Constructor with values
        Account(
            std::string address_,
            uint64_t balance_,
            uint64_t nonce_
        ) : address(std::move(address_))
            , balance(balance_)
            , nonce(nonce_)
            , last_update(std::chrono::system_clock::now())
        {}
        
        // Rule of 5
        Account(const Account&) = default;
        Account& operator=(const Account&) = default;
        Account(Account&&) noexcept = default;
        Account& operator=(Account&&) noexcept = default;
        ~Account() = default;
        
        [[nodiscard]] std::vector<uint8_t> serialize() const;
        static std::optional<Account> deserialize(const std::vector<uint8_t>& data);
        
        [[nodiscard]] bool is_valid() const {
            return !address.empty();
        }
        
        bool operator==(const Account& other) const {
            return address == other.address &&
                   balance == other.balance &&
                   nonce == other.nonce;
        }
        
        bool operator!=(const Account& other) const {
            return !(*this == other);
        }
    };
    
    // Constructor and destructor
    StateManager();
    ~StateManager() = default;
    
    // Rule of 5
    StateManager(const StateManager& other);
    StateManager& operator=(const StateManager& other);
    StateManager(StateManager&&) noexcept = delete;
    StateManager& operator=(StateManager&&) noexcept = delete;
    
    // Core functionality
    [[nodiscard]] bool apply_transaction(const quids::blockchain::Transaction& tx);
    [[nodiscard]] std::optional<Account> get_account(const std::string& address) const;
    void add_account(std::string address, Account account);
    [[nodiscard]] std::vector<quids::blockchain::Transaction> get_account_history(const std::string& address) const;
    [[nodiscard]] std::array<uint8_t, 32> get_state_root() const;
    
    // Batch operations
    [[nodiscard]] bool apply_transactions(const std::vector<quids::blockchain::Transaction>& txs);
    void add_accounts(const std::vector<std::pair<std::string, Account>>& accounts);
    
    // State queries
    [[nodiscard]] size_t get_account_count() const;
    [[nodiscard]] std::vector<std::string> get_all_addresses() const;
    [[nodiscard]] std::map<std::string, Account> get_accounts_snapshot() const;
    
    // State validation
    [[nodiscard]] bool validate_state() const;
    [[nodiscard]] bool verify_transaction(const quids::blockchain::Transaction& tx) const;
    
    // State management
    void clear();
    void rollback_to_nonce(const std::string& address, uint64_t nonce);
    void create_checkpoint();
    void restore_checkpoint();
    
    // Performance metrics
    [[nodiscard]] size_t get_total_transactions() const { return total_transactions_; }
    [[nodiscard]] double get_average_tx_time() const { return avg_tx_time_; }
    [[nodiscard]] size_t get_failed_transactions() const { return failed_transactions_; }
    
private:
    // Thread-safe state access
    mutable std::shared_mutex mutex_;
    std::map<std::string, Account> accounts_;
    std::map<std::string, std::vector<quids::blockchain::Transaction>> history_;
    
    // Checkpointing
    struct Checkpoint {
        std::map<std::string, Account> accounts;
        std::map<std::string, std::vector<quids::blockchain::Transaction>> history;
        std::chrono::system_clock::time_point timestamp;
    };
    std::vector<Checkpoint> checkpoints_;
    
    // Performance tracking
    std::atomic<size_t> total_transactions_{0};
    std::atomic<size_t> failed_transactions_{0};
    std::atomic<double> avg_tx_time_{0.0};
    
    // Internal helper methods
    [[nodiscard]] bool has_sufficient_balance(const Account& account, uint64_t amount) const;
    void update_account_nonce(Account& account);
    void record_transaction(const std::string& address, const quids::blockchain::Transaction& tx);
    void update_metrics(std::chrono::microseconds tx_time, bool success);
    
    // Constants
    static constexpr size_t MAX_HISTORY_PER_ACCOUNT = 1000;
    static constexpr size_t MAX_CHECKPOINTS = 10;
    static constexpr uint64_t MAX_BALANCE = std::numeric_limits<uint64_t>::max();
    static constexpr size_t MAX_BATCH_SIZE = 1000;
};

} // namespace rollup
} // namespace quids 