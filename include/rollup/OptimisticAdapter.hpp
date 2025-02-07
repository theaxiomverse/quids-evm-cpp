#pragma once
#include "RollupStateTransition.hpp"

using quids::rollup::StateManager;
using quids::rollup::StateTransitionProof;

class OptimisticAdapter {
public:
    struct OptimisticProof {
        StateTransitionProof zk_proof;
        std::chrono::system_clock::time_point challenge_period_end;
        bool has_fraud_proof;
    };

    OptimisticProof convert_to_optimistic(
        const StateTransitionProof& zk_proof
    );

    bool verify_optimistic_proof(
        const OptimisticProof& proof,
        const StateManager& state_manager
    );

private:
    bool verify_state_transition(
        const StateTransitionProof& proof,
        const StateManager& state_manager
    );
}; 