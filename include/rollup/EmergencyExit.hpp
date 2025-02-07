#pragma once

#include "rollup/StateManager.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "quantum/QuantumState.hpp"

namespace quids {
namespace rollup {

class EmergencyExit {
public:
    struct Proof {
        std::string account_address;
        uint64_t balance;
        quids::zkp::QZKPGenerator::Proof validity_proof;
    };

    EmergencyExit(
        std::unique_ptr<quids::rollup::StateManager>& state_manager,
        std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator
    );

    bool verify_proof(const Proof& proof);

    bool process_exit(
        const quids::rollup::StateManager& state,
        const Proof& proof
    );

    const quids::rollup::StateManager& get_state() const { return *state_manager_; }

private:
    std::unique_ptr<quids::rollup::StateManager> state_manager_;
    std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator_;

    quantum::QuantumState encode_account_state(const quids::rollup::StateManager::Account& account);
};

} // namespace rollup
} // namespace quids 