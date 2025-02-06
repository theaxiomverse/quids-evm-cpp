#include "rollup/FraudProof.h"
#include <blake3.h>
#include <iostream>

FraudProof::InvalidTransitionProof FraudProof::generate_fraud_proof(
    std::unique_ptr<StateManager> pre_state,
    std::unique_ptr<StateManager> post_state,
    const std::vector<Transaction>& transactions
) {
    std::cout << "generate_fraud_proof: Starting" << std::endl;
    
    // Get state roots before moving the states
    std::cout << "generate_fraud_proof: Getting state roots" << std::endl;
    auto pre_state_root = pre_state->get_state_root();
    auto post_state_root = post_state->get_state_root();
    std::cout << "generate_fraud_proof: Got state roots" << std::endl;
    
    // Generate quantum proof
    std::cout << "generate_fraud_proof: Encoding state diff" << std::endl;
    quantum::QuantumState quantum_state = encode_state_diff(*pre_state, *post_state);
    std::cout << "generate_fraud_proof: Generating ZKP proof" << std::endl;
    auto validity_proof = zkp_generator_->generate_proof(quantum_state);
    std::cout << "generate_fraud_proof: Generated ZKP proof" << std::endl;
    
    // Create proof
    InvalidTransitionProof proof;
    proof.pre_state_root = pre_state_root;
    proof.post_state_root = post_state_root;
    proof.transactions = transactions;
    
    // Create state proof by moving the states
    std::cout << "generate_fraud_proof: Creating state proof" << std::endl;
    StateProof state_proof;
    state_proof.pre_state = std::move(pre_state);
    state_proof.post_state = std::move(post_state);
    proof.state_proof = std::move(state_proof);
    std::cout << "generate_fraud_proof: Created state proof" << std::endl;
    
    proof.validity_proof = std::move(validity_proof);
    
    std::cout << "generate_fraud_proof: Completed" << std::endl;
    return proof;
}

FraudProof::FraudVerificationResult FraudProof::verify_fraud_proof(
    const InvalidTransitionProof& proof
) {
    std::cout << "verify_fraud_proof: Starting verification" << std::endl;
    
    // Verify state roots match
    std::cout << "verify_fraud_proof: Verifying state roots" << std::endl;
    if (!verify_state_roots(proof)) {
        std::cout << "verify_fraud_proof: State root verification failed" << std::endl;
        return {false, "State root verification failed"};
    }
    std::cout << "verify_fraud_proof: State roots verified" << std::endl;
    
    // For a fraud proof to be valid, the state transition must be invalid
    std::cout << "verify_fraud_proof: Verifying state transition" << std::endl;
    if (verify_state_transition(proof)) {
        std::cout << "verify_fraud_proof: No fraud detected" << std::endl;
        return {false, "No fraud detected - state transition is valid"};
    }
    std::cout << "verify_fraud_proof: Invalid state transition confirmed" << std::endl;
    
    // Verify ZKP proof
    std::cout << "verify_fraud_proof: Verifying ZKP proof" << std::endl;
    if (!verify_zkp_proof(proof)) {
        std::cout << "verify_fraud_proof: ZKP verification failed" << std::endl;
        return {false, "ZKP verification failed"};
    }
    std::cout << "verify_fraud_proof: ZKP verified" << std::endl;
    
    std::cout << "verify_fraud_proof: All verifications passed" << std::endl;
    return {true, "Fraud detected and proof verified successfully"};
}

bool FraudProof::verify_state_roots(const InvalidTransitionProof& proof) {
    std::cout << "verify_state_roots: Starting" << std::endl;
    
    // Check if state proof is valid
    if (!proof.state_proof.pre_state || !proof.state_proof.post_state) {
        std::cout << "verify_state_roots: Null state pointers detected" << std::endl;
        return false;
    }
    std::cout << "verify_state_roots: State pointers valid" << std::endl;
    
    // Compare pre-state root
    std::cout << "verify_state_roots: Getting pre-state root" << std::endl;
    auto pre_state_root = proof.state_proof.pre_state->get_state_root();
    if (pre_state_root != proof.pre_state_root) {
        std::cout << "verify_state_roots: Pre-state root mismatch" << std::endl;
        return false;
    }
    std::cout << "verify_state_roots: Pre-state root verified" << std::endl;
    
    // Compare post-state root
    std::cout << "verify_state_roots: Getting post-state root" << std::endl;
    auto post_state_root = proof.state_proof.post_state->get_state_root();
    if (post_state_root != proof.post_state_root) {
        std::cout << "verify_state_roots: Post-state root mismatch" << std::endl;
        return false;
    }
    std::cout << "verify_state_roots: Post-state root verified" << std::endl;
    
    std::cout << "verify_state_roots: All roots verified" << std::endl;
    return true;
}

bool FraudProof::verify_state_transition(const InvalidTransitionProof& proof) {
    std::cout << "verify_state_transition: Starting" << std::endl;
    
    // Check if state proof is valid
    if (!proof.state_proof.pre_state || !proof.state_proof.post_state) {
        std::cout << "verify_state_transition: Null state pointers detected" << std::endl;
        return false;
    }
    std::cout << "verify_state_transition: State pointers valid" << std::endl;
    
    // Create a copy of the pre-state
    std::cout << "verify_state_transition: Creating temp state" << std::endl;
    StateManager temp_state(*proof.state_proof.pre_state);
    std::cout << "verify_state_transition: Created temp state" << std::endl;
    
    // Try to apply all transactions
    std::cout << "verify_state_transition: Applying transactions" << std::endl;
    bool all_transactions_succeeded = true;
    for (const auto& tx : proof.transactions) {
        if (!temp_state.apply_transaction(tx)) {
            std::cout << "verify_state_transition: Transaction application failed" << std::endl;
            all_transactions_succeeded = false;
            break;
        }
    }
    
    if (all_transactions_succeeded) {
        std::cout << "verify_state_transition: All transactions succeeded" << std::endl;
    }
    
    // Compare final states
    std::cout << "verify_state_transition: Comparing final states" << std::endl;
    bool states_match = temp_state.get_state_root() == proof.state_proof.post_state->get_state_root();
    
    if (states_match) {
        std::cout << "verify_state_transition: Final states match" << std::endl;
    } else {
        std::cout << "verify_state_transition: Final states differ" << std::endl;
    }
    
    return all_transactions_succeeded && states_match;
}

bool FraudProof::verify_zkp_proof(const InvalidTransitionProof& proof) {
    std::cout << "verify_zkp_proof: Starting" << std::endl;
    
    // Check if state proof is valid
    if (!proof.state_proof.pre_state || !proof.state_proof.post_state) {
        std::cout << "verify_zkp_proof: Null state pointers detected" << std::endl;
        return false;
    }
    std::cout << "verify_zkp_proof: State pointers valid" << std::endl;
    
    std::cout << "verify_zkp_proof: Encoding state diff" << std::endl;
    quantum::QuantumState quantum_state = encode_state_diff(
        *proof.state_proof.pre_state,
        *proof.state_proof.post_state
    );
    std::cout << "verify_zkp_proof: State diff encoded" << std::endl;
    
    std::cout << "verify_zkp_proof: Verifying proof" << std::endl;
    bool result = zkp_generator_->verify_proof(proof.validity_proof, quantum_state);
    
    if (result) {
        std::cout << "verify_zkp_proof: Proof verified successfully" << std::endl;
    } else {
        std::cout << "verify_zkp_proof: Proof verification failed" << std::endl;
    }
    
    return result;
}

FraudProof::StateProof FraudProof::generate_state_proof(
    const StateManager& pre_state,
    const StateManager& post_state
) {
    return StateProof(pre_state, post_state);
}

quantum::QuantumState FraudProof::encode_state_diff(
    const StateManager& pre_state,
    const StateManager& post_state
) {
    std::cout << "encode_state_diff: Starting" << std::endl;
    
    // Create state vector (512 qubits to ensure power of 2)
    Eigen::VectorXcd state_vector = Eigen::VectorXcd::Zero(512);
    
    // Get state roots
    std::cout << "encode_state_diff: Getting state roots" << std::endl;
    auto pre_root = pre_state.get_state_root();
    auto post_root = post_state.get_state_root();
    std::cout << "encode_state_diff: Got state roots" << std::endl;
    
    // Encode state difference into quantum state
    std::cout << "encode_state_diff: Encoding state difference" << std::endl;
    double normalization_factor = 0.0;
    for (size_t i = 0; i < 32; i++) {
        uint8_t diff = pre_root[i] ^ post_root[i];
        for (size_t j = 0; j < 8; j++) {
            bool bit = (diff >> j) & 1;
            // Use only the first 256 qubits for state difference
            if (bit) {
                state_vector(i * 8 + j) = std::complex<double>(1.0, 0.0);
                normalization_factor += 1.0;
            }
        }
    }
    
    // Normalize the state vector
    std::cout << "encode_state_diff: Normalizing state vector" << std::endl;
    if (normalization_factor > 0.0) {
        double scale = 1.0 / std::sqrt(normalization_factor);
        for (size_t i = 0; i < 512; i++) {
            state_vector(i) *= scale;
        }
    } else {
        // If no differences, put the system in an equal superposition of first two states
        state_vector(0) = std::complex<double>(1.0 / std::sqrt(2.0), 0.0);
        state_vector(1) = std::complex<double>(1.0 / std::sqrt(2.0), 0.0);
    }
    
    std::cout << "encode_state_diff: Creating QuantumState" << std::endl;
    auto result = quantum::QuantumState(state_vector);
    std::cout << "encode_state_diff: Completed" << std::endl;
    return result;
} 