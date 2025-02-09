#include "rollup/FraudProof.hpp"
#include "quantum/QuantumState.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "rollup/StateManager.hpp"
#include "blockchain/Transaction.hpp"
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <vector>
#include <complex>
#include <cmath>
#include <unordered_map>
#include <cassert>

namespace quids {
namespace rollup {

using quantum::QuantumState;
using zkp::QZKPGenerator;


// FraudProof::FraudProof(std::shared_ptr<QZKPGenerator> zkp_generator)
//     : zkp_generator_(std::move(zkp_generator)) {
//     if (!zkp_generator_) {
//         throw std::invalid_argument("ZKP generator cannot be null");
//     }
// }

FraudProof::InvalidTransitionProof FraudProof::generate_fraud_proof(
    std::unique_ptr<quids::rollup::StateManager> pre_state,
    std::unique_ptr<quids::rollup::StateManager> post_state,
    const std::vector<quids::blockchain::Transaction>& transactions
) {
    InvalidTransitionProof proof;
    
    // Generate state proof
    proof.state_proof = generate_state_proof(*pre_state, *post_state);
    
    // Encode state difference into quantum state
    QuantumState quantum_state = encode_state_diff(*pre_state, *post_state);
    
    // Generate ZKP proof
    proof.validity_proof = zkp_generator_->generate_proof(quantum_state);
    
    // Store state roots
    auto vec_root_pre = pre_state->get_state_root();
    if (vec_root_pre.size() < 32)
        throw std::runtime_error("Invalid pre-state root size");
    std::array<uint8_t, 32> pre_arr{};
    std::copy_n(vec_root_pre.begin(), 32, pre_arr.begin());
    proof.pre_state_root = pre_arr;

    auto vec_root_post = post_state->get_state_root();
    if (vec_root_post.size() < 32)
        throw std::runtime_error("Invalid post-state root size");
    std::array<uint8_t, 32> post_arr{};
    std::copy_n(vec_root_post.begin(), 32, post_arr.begin());
    proof.post_state_root = post_arr;
    
    // Store transactions
    proof.transactions = transactions;
    
    return proof;
}

FraudProof::FraudVerificationResult FraudProof::verify_fraud_proof(
    const InvalidTransitionProof& proof
) {
    FraudVerificationResult result;
    result.is_valid = true;
    
    // Verify state roots
    if (!verify_state_roots(proof)) {
        result.is_valid = false;
        result.message = "State root verification failed";
        return result;
    }
    
    // Verify state transition
    if (!verify_state_transition(proof)) {
        result.is_valid = false;
        result.message = "State transition verification failed";
        return result;
    }
    
    // Verify ZKP proof
    if (!verify_zkp_proof(proof)) {
        result.is_valid = false;
        result.message = "ZKP verification failed";
        return result;
    }
    
    result.message = "Fraud proof verified successfully";
    return result;
}

bool FraudProof::verify_state_roots(const InvalidTransitionProof& proof) const {
    // Verify pre-state root
    auto vec_root_pre = proof.state_proof.pre_state->get_state_root();
    if (vec_root_pre.size() < 32)
        throw std::runtime_error("Invalid pre-state root size");

    std::array<uint8_t, 32> pre_state_root_converted{};
    std::copy_n(vec_root_pre.begin(), 32, pre_state_root_converted.begin());
    if (pre_state_root_converted != proof.pre_state_root) {
        return false;
    }
    
    // Verify post-state root
    auto vec_root_post = proof.state_proof.post_state->get_state_root();
    if (vec_root_post.size() < 32)
        throw std::runtime_error("Invalid post-state root size");

    std::array<uint8_t, 32> post_state_root_converted{};
    std::copy_n(vec_root_post.begin(), 32, post_state_root_converted.begin());
    if (post_state_root_converted != proof.post_state_root) {
        return false;
    }
    
    return true;
}

bool FraudProof::verify_state_transition(const InvalidTransitionProof& proof) const {
    // Create a copy of pre-state
    auto temp_state = proof.state_proof.pre_state->clone();
    
    // Apply all transactions
    for (const auto& tx : proof.transactions) {
        if (!temp_state->apply_transaction(tx)) {
            return false;
        }
    }
    
    // Compare final state with post-state
    QuantumState quantum_state = encode_state_diff(
        *temp_state,
        *proof.state_proof.post_state
    );
    
    // Check if quantum state is different
    const double norm = quantum_state.normalized_vector().norm();
    return norm > 1e-10 && !std::isnan(norm) && std::isfinite(norm);
}

bool FraudProof::verify_zkp_proof(const InvalidTransitionProof& proof) const {
    // Encode state difference
    QuantumState quantum_state = encode_state_diff(
        *proof.state_proof.pre_state,
        *proof.state_proof.post_state
    );
    
    // Verify ZKP proof
    return zkp_generator_->verify_proof(proof.validity_proof, quantum_state);
}

FraudProof::StateProof FraudProof::generate_state_proof(
    const quids::rollup::StateManager& pre_state,
    const quids::rollup::StateManager& post_state
) {
    return StateProof(pre_state, post_state);
}

QuantumState FraudProof::encode_state_diff(
    const quids::rollup::StateManager& pre_state,
    const quids::rollup::StateManager& post_state
) const {
    // Get account states from both states
    auto pre_accounts = pre_state.get_accounts_snapshot();
    auto post_accounts = post_state.get_accounts_snapshot();
    
    // Create a state vector to store the differences
    std::vector<double> state_vector;
    state_vector.reserve(pre_accounts.size() + post_accounts.size());
    
    // Compare account states and encode differences
    for (const auto& [address, pre_account] : pre_accounts) {
        auto post_it = post_accounts.find(address);
        if (post_it != post_accounts.end()) {
            // Account exists in both states, encode differences
            const auto& post_account = post_it->second;
            state_vector.push_back(post_account.balance - pre_account.balance);
            state_vector.push_back(post_account.nonce - pre_account.nonce);
        } else {
            // Account deleted in post state
            state_vector.push_back(-pre_account.balance);
            state_vector.push_back(-pre_account.nonce);
        }
    }
    
    // Handle new accounts in post state
    for (const auto& [address, post_account] : post_accounts) {
        if (pre_accounts.find(address) == pre_accounts.end()) {
            // New account in post state
            state_vector.push_back(post_account.balance);
            state_vector.push_back(post_account.nonce);
        }
    }
    
    // Create quantum state from the difference vector
    Eigen::VectorXcd complex_vector(state_vector.size());
    for (size_t i = 0; i < state_vector.size(); ++i) {
        complex_vector(i) = std::complex<double>(state_vector[i], 0.0);
    }
    return QuantumState(complex_vector);
}

} // namespace rollup
} // namespace quids