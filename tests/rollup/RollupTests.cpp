#include <gtest/gtest.h>
#include "rollup/RollupStateTransition.hpp"
#include "rollup/StateManager.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "blockchain/Transaction.hpp"
#include "quantum/QuantumState.hpp"
#include <memory>
#include <vector>
#include <array>

namespace quids {
namespace rollup {
namespace test {

class InvalidTransition : public std::exception {
private:
    std::unique_ptr<quids::rollup::StateManager> post_state;
    std::string message;

public:
    InvalidTransition(const quids::rollup::StateManager& state) {
        try {
            post_state = std::make_unique<quids::rollup::StateManager>(state);
            
            // Get account state
            auto account = post_state->get_account("Account0");
            if (!account) {
                throw std::runtime_error("Account0 not found");
            }
            
            // Create an invalid transaction (spending more than balance)
            auto tx = quids::blockchain::Transaction(
                "Account0",
                "Account1",
                account->balance + 1000,
                account->nonce + 1
            );
            
            // Sign the transaction (this would normally be done with a proper key)
            std::array<uint8_t, 32> private_key;  // Dummy key for testing
            std::fill(private_key.begin(), private_key.end(), 0xFF);
            if (!tx.sign(private_key)) {
                throw std::runtime_error("Failed to sign transaction");
            }
            
            // Apply the transaction to create an invalid state
            if (!post_state->apply_transaction(tx)) {
                throw std::runtime_error("Failed to apply transaction");
            }
            
            // Modify account state to make it invalid
            auto modified_account = post_state->get_account("Account0");
            if (!modified_account) {
                throw std::runtime_error("Failed to get modified account");
            }
            
            quids::rollup::StateManager::Account new_account = *modified_account;
            new_account.balance = 0;  // Set balance to 0 to make the state invalid
            post_state->add_account("Account0", new_account);
            
            // Also modify recipient's state
            auto modified_recipient = post_state->get_account("Account1");
            if (!modified_recipient) {
                throw std::runtime_error("Failed to get recipient account");
            }
            
            quids::rollup::StateManager::Account new_recipient = *modified_recipient;
            new_recipient.balance += account->balance + 1000;  // Add the transferred amount
            post_state->add_account("Account1", new_recipient);
            
        } catch (const std::exception& e) {
            message = std::string("Failed to create invalid state: ") + e.what();
            
            // Create a default invalid state
            try {
                post_state = std::make_unique<quids::rollup::StateManager>();
                
                // Add accounts with invalid balances
                quids::rollup::StateManager::Account default_account;
                default_account.address = "Account0";
                default_account.balance = 1000000;
                default_account.nonce = 0;
                post_state->add_account("Account0", default_account);
                
                // Add recipient with impossibly high balance
                quids::rollup::StateManager::Account recipient_account;
                recipient_account.address = "Account1";
                recipient_account.balance = std::numeric_limits<uint64_t>::max();
                recipient_account.nonce = 0;
                post_state->add_account("Account1", recipient_account);
                
                // Create and apply an invalid transaction
                auto tx = quids::blockchain::Transaction(
                    "Account0",
                    "Account1",
                    2000000,  // More than balance
                    0
                );
                
                std::array<uint8_t, 32> private_key;  // Dummy key for testing
                std::fill(private_key.begin(), private_key.end(), 0xFF);
                if (!tx.sign(private_key)) {
                    throw std::runtime_error("Failed to sign transaction");
                }
                
                if (!post_state->apply_transaction(tx)) {
                    throw std::runtime_error("Failed to apply transaction");
                }
                
            } catch (const std::exception& inner_e) {
                message += std::string(" (Fallback also failed: ") + inner_e.what() + ")";
            }
        }
    }
    
    [[nodiscard]] const char* what() const noexcept override {
        return message.c_str();
    }
    
    [[nodiscard]] const quids::rollup::StateManager* get_state() const {
        return post_state.get();
    }
};

class RollupTest : public ::testing::Test {
protected:
    void SetUp() override {
        state_manager_ = std::make_unique<quids::rollup::StateManager>();
        zkp_generator_ = std::make_shared<quids::zkp::QZKPGenerator>();
        
        // Initialize test accounts
        for (int i = 0; i < 5; i++) {
            test_accounts_.push_back(create_test_account(1000000));
            state_manager_->add_account(
                test_accounts_[i].address,
                test_accounts_[i]
            );
        }
    }
    
    void TearDown() override {
        state_manager_.reset();
        zkp_generator_.reset();
        test_accounts_.clear();
    }
    
    [[nodiscard]] quids::blockchain::Transaction create_test_transaction(
        const std::string& from,
        const std::string& to,
        uint64_t value,
        uint64_t nonce
    ) {
        quids::blockchain::Transaction tx(from, to, value, nonce);
        std::array<uint8_t, 32> private_key;  // Dummy key for testing
        std::fill(private_key.begin(), private_key.end(), 0xFF);
        if (!tx.sign(private_key)) {
            throw std::runtime_error("Failed to sign transaction");
        }
        return tx;
    }
    
    [[nodiscard]] quids::rollup::StateManager::Account create_test_account(uint64_t balance) {
        static int account_counter = 0;
        
        quids::rollup::StateManager::Account account;
        account.address = "Account" + std::to_string(account_counter++);
        account.balance = balance;
        account.nonce = 0;
        
        return account;
    }
    
    [[nodiscard]] quids::zkp::QZKPGenerator::Proof create_test_proof() {
        quantum::QuantumState state(32);  // Create a 32-qubit state
        return zkp_generator_->generate_proof(state);
    }
    
    std::unique_ptr<quids::rollup::StateManager> state_manager_;
    std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator_;
    std::vector<quids::rollup::StateManager::Account> test_accounts_;
};

TEST_F(RollupTest, TestBasicTransaction) {
    auto tx = create_test_transaction(
        test_accounts_[0].address,
        test_accounts_[1].address,
        1000,
        0
    );
    
    EXPECT_TRUE(state_manager_->apply_transaction(tx));
    
    auto sender = state_manager_->get_account(test_accounts_[0].address);
    auto recipient = state_manager_->get_account(test_accounts_[1].address);
    
    ASSERT_TRUE(sender);
    ASSERT_TRUE(recipient);
    EXPECT_EQ(sender->balance, 999000);
    EXPECT_EQ(recipient->balance, 1001000);
}

TEST_F(RollupTest, TestInvalidTransaction) {
    // Try to send more than balance
    auto tx = create_test_transaction(
        test_accounts_[0].address,
        test_accounts_[1].address,
        2000000,  // More than initial balance
        0
    );
    
    EXPECT_FALSE(state_manager_->apply_transaction(tx));
    
    auto sender = state_manager_->get_account(test_accounts_[0].address);
    auto recipient = state_manager_->get_account(test_accounts_[1].address);
    
    ASSERT_TRUE(sender);
    ASSERT_TRUE(recipient);
    EXPECT_EQ(sender->balance, 1000000);  // Balance should be unchanged
    EXPECT_EQ(recipient->balance, 1000000);
}

TEST_F(RollupTest, TestNonceHandling) {
    // First transaction should succeed
    auto tx1 = create_test_transaction(
        test_accounts_[0].address,
        test_accounts_[1].address,
        1000,
        0
    );
    EXPECT_TRUE(state_manager_->apply_transaction(tx1));
    
    // Same nonce should fail
    auto tx2 = create_test_transaction(
        test_accounts_[0].address,
        test_accounts_[1].address,
        1000,
        0
    );
    EXPECT_FALSE(state_manager_->apply_transaction(tx2));
    
    // Next nonce should succeed
    auto tx3 = create_test_transaction(
        test_accounts_[0].address,
        test_accounts_[1].address,
        1000,
        1
    );
    EXPECT_TRUE(state_manager_->apply_transaction(tx3));
}

} // namespace test
} // namespace rollup
} // namespace quids 