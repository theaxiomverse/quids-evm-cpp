#pragma once

#include "blockchain/Transaction.hpp"
#include "quantum/QuantumState.hpp"
#include <vector>
#include <array>
#include <memory>

namespace quids {
namespace rollup {

class StateTransitionProof {
public:
    struct ProofData {
        std::array<uint8_t, 32> pre_state_root;
        std::array<uint8_t, 32> post_state_root;
        std::array<uint8_t, 32> transactions_root;
        std::vector<uint8_t> zk_proof;
        std::vector<uint8_t> quantum_signature;
    };

    StateTransitionProof() = default;
    explicit StateTransitionProof(const ProofData& data);

    // Core functionality
    bool verify() const;
    std::array<uint8_t, 32> computeHash() const;
    
    // Getters
    const ProofData& getData() const { return data_; }
    const std::array<uint8_t, 32>& getPreStateRoot() const { return data_.pre_state_root; }
    const std::array<uint8_t, 32>& getPostStateRoot() const { return data_.post_state_root; }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<StateTransitionProof> deserialize(const std::vector<uint8_t>& data);

private:
    ProofData data_;
};

} // namespace rollup
} // namespace quids 