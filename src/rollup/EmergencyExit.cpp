#include "rollup/EmergencyExit.hpp"
#include <stdexcept>
#include <cmath>

namespace quids {
namespace rollup {

typedef quantum::QuantumState QuantumState;

EmergencyExit::EmergencyExit(
    std::unique_ptr<quids::rollup::StateManager>& state_manager,
    std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator
) : state_manager_(std::move(state_manager)), zkp_generator_(std::move(zkp_generator)) {}

bool EmergencyExit::process_exit(
    const quids::rollup::StateManager& state,
    const Proof& proof
) {
    auto account = state.get_account(proof.account_address);
    if (!account) {
        throw std::runtime_error("Account not found");
    }

    QuantumState quantum_state = encode_account_state(*account);
    
    if (!zkp_generator_->verify_proof(proof.validity_proof, quantum_state)) {
        return false;
    }

    if (account->balance != proof.balance) {
        return false;
    }

    account->balance = 0;
    state_manager_->add_account(proof.account_address, *account);

    return true;
}

bool EmergencyExit::verify_proof(const Proof& proof) {
    quids::rollup::StateManager::Account account;
    account.balance = proof.balance;

    QuantumState quantum_state = encode_account_state(account);
    return zkp_generator_->verify_proof(proof.validity_proof, quantum_state);
}

QuantumState EmergencyExit::encode_account_state(const quids::rollup::StateManager::Account& account) {
    Eigen::VectorXcd state_vector = Eigen::VectorXcd::Zero(1 << 6);
    
    for (size_t i = 0; i < 64; i++) {
        if (account.balance & (1ULL << i)) {
            state_vector(i) = std::complex<double>(1.0, 0.0);
        }
    }
    
    state_vector.normalize();
    
    return QuantumState(state_vector);
}

} // namespace rollup
} // namespace quids 