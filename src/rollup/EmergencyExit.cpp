#include "rollup/EmergencyExit.h"
#include <blake3.h>

EmergencyExit::EmergencyExit(
    std::unique_ptr<StateManager>& state_manager,
    std::shared_ptr<QZKPGenerator> zkp_generator
) : state_manager_(std::move(state_manager)), zkp_generator_(zkp_generator) {}

EmergencyExit::ExitProof EmergencyExit::generate_exit_proof(
    const std::string& address,
    const StateManager& state
) {
    // Get account state
    auto account = state.get_account(address);
    
    // Create quantum state encoding account data
    quantum::QuantumState quantum_state = encode_account_state(account);
    
    // Generate ZKP proof
    auto zkp_proof = zkp_generator_->generate_proof(quantum_state);
    
    // Create exit proof
    return ExitProof{
        .account_address = address,
        .balance = account.balance,
        .state_root = state.get_state_root(),
        .validity_proof = zkp_proof
    };
}

bool EmergencyExit::verify_exit_proof(const ExitProof& proof) {
    try {
        // Get current account state
        auto account = state_manager_->get_account(proof.account_address);
        
        // Verify balance matches
        if (account.balance != proof.balance) {
            return false;
        }
        
        // Create quantum state for verification
        quantum::QuantumState quantum_state = encode_account_state(account);
        
        // Verify ZKP proof
        return zkp_generator_->verify_proof(proof.validity_proof, quantum_state);
    } catch (const std::out_of_range&) {
        return false;  // Account not found
    }
}

void EmergencyExit::process_emergency_exit(const ExitProof& proof) {
    // Verify proof first
    if (!verify_exit_proof(proof)) {
        throw std::runtime_error("Invalid exit proof");
    }
    
    try {
        // Get existing account
        auto account = state_manager_->get_account(proof.account_address);
        
        // Update balance to zero but preserve other fields
        account.balance = 0;
        
        // Update state
        state_manager_->add_account(proof.account_address, account);
    } catch (const std::out_of_range&) {
        // If account doesn't exist, create a new one
        StateManager::Account account;
        account.address = proof.account_address;
        account.balance = 0;
        account.nonce = 0;
        
        // Update state
        state_manager_->add_account(proof.account_address, account);
    }
}

quantum::QuantumState EmergencyExit::encode_account_state(const StateManager::Account& account) {
    // Create quantum state (512 qubits to ensure power of 2)
    Eigen::VectorXcd state_vector = Eigen::VectorXcd::Zero(512);
    
    // Encode account data into quantum state
    // First 64 bits for balance
    for (size_t i = 0; i < 64; i++) {
        bool bit = (account.balance >> i) & 1;
        state_vector(i) = std::complex<double>(bit ? 1.0 : 0.0, 0.0);
    }
    
    // Next 64 bits for nonce
    for (size_t i = 0; i < 64; i++) {
        bool bit = (account.nonce >> i) & 1;
        state_vector(i + 64) = std::complex<double>(bit ? 1.0 : 0.0, 0.0);
    }
    
    // Initialize remaining qubits to |0⟩
    for (size_t i = 128; i < 512; i++) {
        state_vector(i) = std::complex<double>(0.0, 0.0);
    }
    
    return quantum::QuantumState(state_vector);
} 