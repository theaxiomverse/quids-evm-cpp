#include <gtest/gtest.h>
#include "rollup/StateManager.h"
#include "rollup/FraudProof.h"
#include "rollup/CrossRollupBridge.h"
#include "rollup/MEVProtection.h"
#include "rollup/EmergencyExit.h"
#include "zkp/QZKPGenerator.h"

struct InvalidTransition {
    std::unique_ptr<StateManager> post_state;
    std::vector<Transaction> transactions;
    
    InvalidTransition(const StateManager& state) {
        std::cout << "InvalidTransition: Constructor starting" << std::endl;
        try {
            // Create a deep copy of the state first
            std::cout << "InvalidTransition: Creating initial state copy" << std::endl;
            post_state = std::make_unique<StateManager>(state);
            std::cout << "InvalidTransition: Created initial state copy" << std::endl;
            
            std::cout << "InvalidTransition: Getting Account0" << std::endl;
            auto account = state.get_account("Account0");
            std::cout << "InvalidTransition: Got Account0" << std::endl;
            
            std::cout << "InvalidTransition: Creating transaction" << std::endl;
            auto tx = Transaction("Account0", "Account1", account.balance + 1000, account.nonce + 1);
            std::cout << "InvalidTransition: Created transaction" << std::endl;
            
            // Create a test private key for signing
            std::cout << "InvalidTransition: Signing transaction" << std::endl;
            std::array<uint8_t, 32> private_key;  // Use array instead of vector
            std::fill(private_key.begin(), private_key.end(), 0x42);  // Fill with test value
            tx.sign(private_key);
            std::cout << "InvalidTransition: Signed transaction" << std::endl;
            
            transactions.push_back(tx);
            std::cout << "InvalidTransition: Added transaction" << std::endl;
            
            // Force the invalid state by directly modifying the account balance
            std::cout << "InvalidTransition: Modifying account balance" << std::endl;
            auto modified_account = account;
            modified_account.balance = 0;  // Set balance to 0 to make the state invalid
            post_state->add_account("Account0", modified_account);
            std::cout << "InvalidTransition: Modified Account0" << std::endl;
            
            // Also update the recipient's balance to maintain consistency
            std::cout << "InvalidTransition: Getting Account1" << std::endl;
            auto recipient = state.get_account("Account1");
            std::cout << "InvalidTransition: Got Account1" << std::endl;
            
            auto modified_recipient = recipient;
            modified_recipient.balance += account.balance + 1000;  // Add the transferred amount
            post_state->add_account("Account1", modified_recipient);
            std::cout << "InvalidTransition: Modified Account1" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "InvalidTransition: Exception caught: " << e.what() << std::endl;
            std::cout << "InvalidTransition: Creating default invalid state" << std::endl;
            
            // If any account operations fail, create a default invalid state
            if (!post_state) {
                std::cout << "InvalidTransition: Creating new state manager" << std::endl;
                post_state = std::make_unique<StateManager>();
            }
            
            auto default_account = StateManager::Account{};
            default_account.address = "Account0";
            default_account.balance = 0;
            default_account.nonce = 0;
            post_state->add_account("Account0", default_account);
            std::cout << "InvalidTransition: Created default Account0" << std::endl;
            
            auto recipient_account = StateManager::Account{};
            recipient_account.address = "Account1";
            recipient_account.balance = 1000;
            recipient_account.nonce = 0;
            post_state->add_account("Account1", recipient_account);
            std::cout << "InvalidTransition: Created default Account1" << std::endl;
            
            // Create and sign a default transaction
            std::cout << "InvalidTransition: Creating default transaction" << std::endl;
            auto tx = Transaction("Account0", "Account1", 1000, 1);
            std::array<uint8_t, 32> private_key;  // Use array instead of vector
            std::fill(private_key.begin(), private_key.end(), 0x42);  // Fill with test value
            tx.sign(private_key);
            transactions.push_back(tx);
            std::cout << "InvalidTransition: Added default transaction" << std::endl;
        }
        std::cout << "InvalidTransition: Constructor completed" << std::endl;
    }
    
    // Ensure proper cleanup
    ~InvalidTransition() {
        std::cout << "InvalidTransition: Destructor called" << std::endl;
        transactions.clear();
        if (post_state) {
            post_state.reset();
        }
        std::cout << "InvalidTransition: Destructor completed" << std::endl;
    }
    
    // Prevent copying
    InvalidTransition(const InvalidTransition&) = delete;
    InvalidTransition& operator=(const InvalidTransition&) = delete;
    
    // Allow moving
    InvalidTransition(InvalidTransition&&) = default;
    InvalidTransition& operator=(InvalidTransition&&) = default;
};

class RollupTestSuite : public ::testing::Test {
protected:
    std::unique_ptr<StateManager> state_manager_;
    std::shared_ptr<QZKPGenerator> zkp_generator_;
    std::vector<StateManager::Account> test_accounts_;
    
    void SetUp() override {
        try {
            // Initialize components in a specific order
            state_manager_ = std::make_unique<StateManager>();
            if (!state_manager_) {
                throw std::runtime_error("Failed to create StateManager");
            }
            
            zkp_generator_ = std::make_shared<QZKPGenerator>();
            if (!zkp_generator_) {
                throw std::runtime_error("Failed to create QZKPGenerator");
            }
            
            // Setup test accounts
            setup_test_accounts();
            
            // Verify setup
            if (test_accounts_.empty()) {
                throw std::runtime_error("No test accounts created");
            }
            
            // Verify state manager has accounts
            try {
                state_manager_->get_account("Account0");
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to verify account setup");
            }
        } catch (const std::exception& e) {
            std::cerr << "Test setup failed: " << e.what() << std::endl;
            throw;
        }
    }
    
    void TearDown() override {
        // Clean up in reverse order
        test_accounts_.clear();
        zkp_generator_.reset();
        state_manager_.reset();
    }
    
    void setup_test_accounts() {
        test_accounts_.clear();  // Clear any existing accounts
        
        for (int i = 0; i < 5; i++) {
            auto account = create_test_account(1000);
            account.address = "Account" + std::to_string(i);
            test_accounts_.push_back(account);
            
            // Create a copy of the account for the state manager
            auto state_account = account;
            state_manager_->add_account(account.address, state_account);
        }
    }
    
    Transaction create_test_transaction(
        const std::string& from,
        const std::string& to,
        uint64_t amount
    ) {
        return Transaction(from, to, amount, get_nonce(from));
    }
    
    uint64_t get_nonce(const std::string& address) {
        return state_manager_->get_account(address).nonce;
    }
    
    StateManager::Account create_test_account(uint64_t balance) {
        StateManager::Account account;
        account.balance = balance;
        account.nonce = 0;
        return account;
    }
    
    InvalidTransition create_invalid_transition() {
        // Ensure we have at least two accounts
        if (test_accounts_.size() < 2) {
            throw std::runtime_error("Not enough test accounts");
        }
        return InvalidTransition(*state_manager_);
    }
    
    std::vector<uint8_t> create_test_payload() {
        return std::vector<uint8_t>{1, 2, 3, 4, 5};
    }
    
    QZKPGenerator::Proof create_test_proof() {
        QZKPGenerator::Proof proof;
        proof.proof_data.resize(32);
        proof.measurement_outcomes = {true, false, true};
        proof.phase_angles = {0.1, 0.2, 0.3};
        return proof;
    }
    
    std::vector<Transaction> create_test_batch() {
        std::vector<Transaction> batch;
        batch.push_back(Transaction(test_accounts_[0].address, test_accounts_[1].address, 100, 1));
        batch.push_back(Transaction(test_accounts_[1].address, test_accounts_[2].address, 50, 1));
        batch.push_back(Transaction(test_accounts_[2].address, test_accounts_[0].address, 75, 1));
        return batch;
    }
};

TEST_F(RollupTestSuite, TestFraudProofGeneration) {
    std::cout << "Starting TestFraudProofGeneration" << std::endl;
    
    FraudProof fraud_proof(zkp_generator_);
    std::cout << "Created FraudProof instance" << std::endl;
    
    // Keep a copy of the original state for later verification
    std::cout << "Creating base state copy..." << std::endl;
    auto base_state = std::make_unique<StateManager>(*state_manager_);
    std::cout << "Created base state copy" << std::endl;
    
    // Create an invalid state transition
    std::cout << "Creating invalid state transition..." << std::endl;
    auto invalid_transition = create_invalid_transition();
    std::cout << "Created invalid state transition" << std::endl;
    
    // Create unique pointers for the states, using the base copy for pre_state
    std::cout << "Creating pre_state..." << std::endl;
    auto pre_state = std::make_unique<StateManager>(*base_state);
    std::cout << "Created pre_state" << std::endl;
    
    std::cout << "Creating post_state..." << std::endl;
    auto post_state = std::make_unique<StateManager>(*invalid_transition.post_state);
    std::cout << "Created post_state" << std::endl;
    
    // Generate fraud proof
    std::cout << "Generating fraud proof..." << std::endl;
    auto proof = fraud_proof.generate_fraud_proof(
        std::move(pre_state),
        std::move(post_state),
        invalid_transition.transactions
    );
    std::cout << "Generated fraud proof" << std::endl;
    
    // Keep the invalid transition alive until after verification
    std::cout << "Verifying fraud proof..." << std::endl;
    auto result = fraud_proof.verify_fraud_proof(proof);
    std::cout << "Verified fraud proof" << std::endl;
    
    EXPECT_TRUE(result.is_valid);
    
    // Clean up in reverse order
    std::cout << "Cleaning up..." << std::endl;
    base_state.reset();
    std::cout << "Cleanup complete" << std::endl;
}

TEST_F(RollupTestSuite, TestCrossRollupBridge) {
    CrossRollupBridge bridge;
    
    // Create test message
    CrossRollupBridge::CrossRollupMessage message{
        .source_chain_id = 1,
        .destination_chain_id = 2,
        .payload = create_test_payload(),
        .validity_proof = create_test_proof()
    };
    
    // Send message
    bridge.send_message(message);
    
    // Verify message
    EXPECT_TRUE(bridge.verify_incoming_message(message));
}

TEST_F(RollupTestSuite, TestMEVProtection) {
    MEVProtection mev_protection;
    
    // Create test batch
    auto transactions = create_test_batch();
    
    // Create commitment
    auto commitment = mev_protection.create_ordering_commitment(transactions);
    
    // Verify commitment matches transactions
    auto hash = mev_protection.compute_fairness_hash(transactions);
    EXPECT_EQ(commitment.batch_hash, hash);
    
    // Modify batch and verify hash differs
    std::swap(transactions[0], transactions[1]);
    auto modified_hash = mev_protection.compute_fairness_hash(transactions);
    EXPECT_NE(commitment.batch_hash, modified_hash);
}

TEST_F(RollupTestSuite, TestEmergencyExit) {
    // Create a new state manager for the test
    auto test_state = std::make_unique<StateManager>();
    
    // Add test accounts to the new state manager
    for (const auto& account : test_accounts_) {
        test_state->add_account(account.address, account);
    }
    
    // Store initial balance for verification
    uint64_t initial_balance = test_accounts_[0].balance;
    
    // Create a copy of the state for the EmergencyExit instance
    auto emergency_state = std::make_unique<StateManager>(*test_state);
    EmergencyExit emergency_exit(emergency_state, zkp_generator_);
    
    // Generate exit proof using the original test state
    auto proof = emergency_exit.generate_exit_proof(
        test_accounts_[0].address,
        *test_state
    );
    
    // Verify proof
    EXPECT_TRUE(emergency_exit.verify_exit_proof(proof));
    
    // Process exit
    emergency_exit.process_emergency_exit(proof);
    
    // Generate a new proof after exit using the EmergencyExit's state
    auto post_exit_proof = emergency_exit.generate_exit_proof(
        test_accounts_[0].address,
        emergency_exit.get_state()  // Use EmergencyExit's state
    );
    
    // Verify the balance in the proof is 0
    EXPECT_EQ(post_exit_proof.balance, 0);
    EXPECT_NE(initial_balance, 0);  // Sanity check that we actually had a balance
} 