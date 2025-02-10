#pragma once
#include "quantum/QuantumState.hpp"
#include <vector>
#include <memory>

namespace quids::zkp {

class ProofSystem {
public:
    ProofSystem();
    ~ProofSystem();
    
    // Prevent copying
    ProofSystem(const ProofSystem&) = delete;
    ProofSystem& operator=(const ProofSystem&) = delete;
    
    // Allow moving
    ProofSystem(ProofSystem&&) noexcept = default;
    ProofSystem& operator=(ProofSystem&&) noexcept = default;
    
    // Core functionality
    std::vector<uint8_t> generate_challenge(const quantum::QuantumState& state);
    bool verify_response(const quantum::QuantumState& state,
                        const std::vector<uint8_t>& challenge,
                        const std::vector<uint8_t>& response);
                        
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace quids::zkp 