#pragma once

#include <memory>
#include <string>
#include <vector>
#include "quantum/QuantumState.hpp"
#include "rollup/StateManager.hpp"

namespace quids {
namespace rollup {

struct EmergencyProof {
    std::string account_address;
    std::vector<uint8_t> signature;
    uint64_t timestamp;
    std::vector<uint8_t> state_root;
};

class EmergencyExit {
public:
    explicit EmergencyExit(std::shared_ptr<StateManager> state_manager);
    ~EmergencyExit() = default;

    bool verify_proof(const EmergencyProof& proof);
    bool process_exit(const EmergencyProof& proof);
    EmergencyProof generate_proof(const std::string& account_address);

private:
    quantum::QuantumState encode_state(uint64_t balance, uint64_t nonce);
    std::shared_ptr<StateManager> state_manager_;
};

} // namespace rollup
} // namespace quids 