#include "rollup/EmergencyExit.hpp"
#include <stdexcept>
#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace quids {
namespace rollup {

typedef quantum::QuantumState QuantumState;

EmergencyExit::EmergencyExit(std::shared_ptr<StateManager> state_manager)
    : state_manager_(std::move(state_manager)) {}

bool EmergencyExit::verify_proof(const EmergencyProof& proof) {
    // Verify timestamp is not in the future
    auto current_time = std::chrono::system_clock::now().time_since_epoch().count();
    if (proof.timestamp > static_cast<uint64_t>(current_time)) {
        return false;
    }
    
    // Get current account state
    auto account = state_manager_->get_account(proof.account_address);
    if (!account) {
        return false;
    }
    
    // Regenerate quantum state
    quantum::QuantumState quantum_state = encode_state(account->balance, account->nonce);
    
    // Verify state root matches
    std::vector<uint8_t> computed_state_root;
    const Eigen::VectorXcd& state_vector = quantum_state.get_state_vector();
    computed_state_root.reserve(state_vector.size() * 2 * sizeof(double));
    
    // Convert quantum state amplitudes to bytes safely
    for (Eigen::Index i = 0; i < state_vector.size(); i++) {
        double real_val = state_vector(i).real();
        double imag_val = state_vector(i).imag();
        
        union {
            double d;
            uint8_t bytes[sizeof(double)];
        } real_converter, imag_converter;
        
        real_converter.d = real_val;
        imag_converter.d = imag_val;
        
        computed_state_root.insert(computed_state_root.end(), 
                                 real_converter.bytes, 
                                 real_converter.bytes + sizeof(double));
        computed_state_root.insert(computed_state_root.end(), 
                                 imag_converter.bytes, 
                                 imag_converter.bytes + sizeof(double));
    }
    
    if (computed_state_root != proof.state_root) {
        return false;
    }
    
    // Verify signature
    std::vector<uint8_t> message;
    message.insert(message.end(), proof.account_address.begin(), proof.account_address.end());
    
    union {
        uint64_t ts;
        uint8_t bytes[sizeof(uint64_t)];
    } ts_converter;
    ts_converter.ts = proof.timestamp;
    
    message.insert(message.end(), 
                  ts_converter.bytes, 
                  ts_converter.bytes + sizeof(uint64_t));
    message.insert(message.end(), proof.state_root.begin(), proof.state_root.end());
    
    // TODO: Replace with actual cryptographic signature verification
    return message == proof.signature;
}

bool EmergencyExit::process_exit(const EmergencyProof& proof) {
    if (!verify_proof(proof)) {
        return false;
    }
    
    auto account = state_manager_->get_account(proof.account_address);
    if (!account) {
        return false;
    }
    
    // Process emergency exit by setting balance to 0
    if (!state_manager_->set_balance(proof.account_address, 0)) {
        return false;
    }
    
    // Increment nonce
    if (!state_manager_->set_nonce(proof.account_address, account->nonce + 1)) {
        return false;
    }
    
    return true;
}

EmergencyProof EmergencyExit::generate_proof(const std::string& account_address) {
    EmergencyProof proof;
    proof.account_address = account_address;
    
    // Get current account state
    auto account = state_manager_->get_account(account_address);
    if (!account) {
        throw std::runtime_error("Account not found");
    }
    
    // Generate quantum state encoding of account data
    quantum::QuantumState quantum_state = encode_state(account->balance, account->nonce);
    
    // Set current timestamp
    proof.timestamp = static_cast<uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    
    // Generate state root from quantum state
    std::vector<uint8_t> state_root;
    const Eigen::VectorXcd& state_vector = quantum_state.get_state_vector();
    state_root.reserve(state_vector.size() * 2 * sizeof(double));
    
    // Convert quantum state amplitudes to bytes safely
    for (Eigen::Index i = 0; i < state_vector.size(); i++) {
        double real_val = state_vector(i).real();
        double imag_val = state_vector(i).imag();
        
        union {
            double d;
            uint8_t bytes[sizeof(double)];
        } real_converter, imag_converter;
        
        real_converter.d = real_val;
        imag_converter.d = imag_val;
        
        state_root.insert(state_root.end(), 
                         real_converter.bytes, 
                         real_converter.bytes + sizeof(double));
        state_root.insert(state_root.end(), 
                         imag_converter.bytes, 
                         imag_converter.bytes + sizeof(double));
    }
    proof.state_root = state_root;
    
    // Generate signature (account_address + timestamp + state_root)
    std::vector<uint8_t> message;
    message.insert(message.end(), account_address.begin(), account_address.end());
    
    union {
        uint64_t ts;
        uint8_t bytes[sizeof(uint64_t)];
    } ts_converter;
    ts_converter.ts = proof.timestamp;
    
    message.insert(message.end(), 
                  ts_converter.bytes, 
                  ts_converter.bytes + sizeof(uint64_t));
    message.insert(message.end(), state_root.begin(), state_root.end());
    
    // TODO: Replace with actual cryptographic signing when crypto module is ready
    proof.signature = message;  // Placeholder - should use proper signing
    
    return proof;
}

quantum::QuantumState EmergencyExit::encode_state(uint64_t balance, uint64_t nonce) {
    // We'll use 64 qubits for balance and 64 qubits for nonce = 128 qubits total
    quantum::QuantumState state(128);
    
    // Prepare quantum state in computational basis
    state.prepare_state();
    
    // Encode balance bits
    for (size_t i = 0; i < 64; i++) {
        if (balance & (1ULL << i)) {
            // Apply X gate (NOT) to flip qubit from |0⟩ to |1⟩ for 1 bits
            Eigen::Matrix2cd x_gate;
            x_gate << 0, 1,
                     1, 0;
            state.apply_single_qubit_gate(i, x_gate);
        }
    }
    
    // Encode nonce bits
    for (size_t i = 0; i < 64; i++) {
        if (nonce & (1ULL << i)) {
            Eigen::Matrix2cd x_gate;
            x_gate << 0, 1,
                     1, 0;
            state.apply_single_qubit_gate(i + 64, x_gate);
        }
    }
    
    // Apply Hadamard gates to create superposition
    for (size_t i = 0; i < 128; i++) {
        state.apply_hadamard(i);
    }
    
    // Create entanglement between balance and nonce qubits
    for (size_t i = 0; i < 63; i++) {
        state.apply_cnot(i, i + 64);
    }
    
    // Normalize the final state
    state.normalize();
    
    return state;
}

} // namespace rollup
} // namespace quids 