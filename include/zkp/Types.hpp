#pragma once
#include <vector>
#include "quantum/QuantumState.hpp"

namespace zkp {
    struct ZKProof {
        std::vector<bool> measurement_basis;
        std::vector<bool> measurement_outcomes;
        std::vector<double> phase_angles;
        std::vector<uint8_t> proof_data;
    };
    
    struct EntanglementProof {
        quids::quantum::QuantumState measured_state;
        std::vector<bool> parity_checks;
    };
} 