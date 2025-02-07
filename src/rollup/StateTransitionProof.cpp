#include "rollup/StateTransitionProof.hpp"
#include <blake3.h>

namespace quids {
namespace rollup {

StateTransitionProof::StateTransitionProof(const ProofData& data) : data_(data) {}

bool StateTransitionProof::verify() const {
    // TODO: Implement proper verification
    return !data_.zk_proof.empty() && !data_.quantum_signature.empty();
}

std::array<uint8_t, 32> StateTransitionProof::computeHash() const {
    std::array<uint8_t, 32> hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    // Add domain separation context
    const char* CONTEXT = "QUIDS_STATE_TRANSITION_PROOF_V1";
    blake3_hasher_update(&hasher, CONTEXT, strlen(CONTEXT));

    // Hash all proof components
    blake3_hasher_update(&hasher, data_.pre_state_root.data(), data_.pre_state_root.size());
    blake3_hasher_update(&hasher, data_.post_state_root.data(), data_.post_state_root.size());
    blake3_hasher_update(&hasher, data_.transactions_root.data(), data_.transactions_root.size());
    
    if (!data_.zk_proof.empty()) {
        blake3_hasher_update(&hasher, data_.zk_proof.data(), data_.zk_proof.size());
    }
    
    if (!data_.quantum_signature.empty()) {
        blake3_hasher_update(&hasher, data_.quantum_signature.data(), data_.quantum_signature.size());
    }

    blake3_hasher_finalize(&hasher, hash.data(), hash.size());
    return hash;
}

std::vector<uint8_t> StateTransitionProof::serialize() const {
    std::vector<uint8_t> result;
    
    // Reserve approximate size
    result.reserve(
        32 * 3 + // roots
        data_.zk_proof.size() +
        data_.quantum_signature.size() +
        16 // size markers
    );
    
    // Serialize roots
    result.insert(result.end(), data_.pre_state_root.begin(), data_.pre_state_root.end());
    result.insert(result.end(), data_.post_state_root.begin(), data_.post_state_root.end());
    result.insert(result.end(), data_.transactions_root.begin(), data_.transactions_root.end());
    
    // Serialize proofs
    auto addVector = [&result](const std::vector<uint8_t>& vec) {
        uint32_t size = vec.size();
        result.insert(result.end(),
            reinterpret_cast<const uint8_t*>(&size),
            reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        result.insert(result.end(), vec.begin(), vec.end());
    };
    
    addVector(data_.zk_proof);
    addVector(data_.quantum_signature);
    
    return result;
}

std::optional<StateTransitionProof> StateTransitionProof::deserialize(
    const std::vector<uint8_t>& data
) {
    if (data.size() < 32 * 3) {
        return std::nullopt;
    }

    ProofData proof_data;
    size_t pos = 0;

    // Read roots
    std::copy_n(data.begin() + pos, 32, proof_data.pre_state_root.begin());
    pos += 32;
    std::copy_n(data.begin() + pos, 32, proof_data.post_state_root.begin());
    pos += 32;
    std::copy_n(data.begin() + pos, 32, proof_data.transactions_root.begin());
    pos += 32;

    // Read vectors
    auto readVector = [&data, &pos]() -> std::optional<std::vector<uint8_t>> {
        if (pos + sizeof(uint32_t) > data.size()) {
            return std::nullopt;
        }
        uint32_t size;
        std::memcpy(&size, &data[pos], sizeof(size));
        pos += sizeof(size);
        
        if (pos + size > data.size()) {
            return std::nullopt;
        }
        std::vector<uint8_t> result(data.begin() + pos, data.begin() + pos + size);
        pos += size;
        return result;
    };

    auto zk_proof = readVector();
    auto quantum_sig = readVector();
    
    if (!zk_proof || !quantum_sig) {
        return std::nullopt;
    }
    
    proof_data.zk_proof = *zk_proof;
    proof_data.quantum_signature = *quantum_sig;

    return StateTransitionProof(proof_data);
}

} // namespace rollup
} // namespace quids 