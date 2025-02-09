#pragma once

#include <vector>
#include <memory>
#include <string>
#include "evm/Memory.hpp"
#include "evm/Address.hpp"
#include "rollup/StateManager.hpp"
#include "ProofVerifierImpl.hpp"


namespace evm {

// Forward declarations
class StateManager;

struct ProofData {
    std::vector<uint8_t> pre_state_root;
    std::vector<uint8_t> post_state_root;
    std::vector<uint8_t> transactions_root;
    std::vector<uint8_t> receipts_root;
    uint64_t block_number;
    std::vector<uint8_t> proof_data;
};

class ProofVerifier {
public:
    // Constructor and destructor
    explicit ProofVerifier(std::shared_ptr<StateManager> state_manager);
    ~ProofVerifier() = default;

    // Disable copy and move
    ProofVerifier(const ProofVerifier&) = delete;
    ProofVerifier& operator=(const ProofVerifier&) = delete;
    ProofVerifier(ProofVerifier&&) = delete;
    ProofVerifier& operator=(ProofVerifier&&) = delete;
    
    // Verification methods
    [[nodiscard]] bool verify_state_transition(const ProofData& proof);
    [[nodiscard]] bool verify_transaction(const std::vector<uint8_t>& tx_hash, const ProofData& proof);
    [[nodiscard]] bool verify_storage_proof(const Address& address, const std::vector<uint8_t>& key, const ProofData& proof);
    
    // Proof generation
    [[nodiscard]] ProofData generate_state_proof(uint64_t block_number);
    [[nodiscard]] ProofData generate_transaction_proof(const std::vector<uint8_t>& tx_hash);
    [[nodiscard]] ProofData generate_storage_proof(const Address& address, const std::vector<uint8_t>& key);
    
    // ZK-proof specific methods
    [[nodiscard]] bool verify_zk_proof(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs);
    [[nodiscard]] std::vector<uint8_t> generate_zk_proof(const std::vector<uint8_t>& private_inputs, const std::vector<uint8_t>& public_inputs);

private:
    std::shared_ptr<StateManager> state_manager_;
    
    // Internal verification helpers
    [[nodiscard]] bool verify_merkle_proof(const std::vector<uint8_t>& root, const std::vector<uint8_t>& leaf, const std::vector<std::vector<uint8_t>>& proof);
    [[nodiscard]] bool verify_state_root(const std::vector<uint8_t>& root);
    
    // Compression methods for proof data
    [[nodiscard]] std::vector<uint8_t> compress_proof(const std::vector<uint8_t>& proof_data);
    [[nodiscard]] std::vector<uint8_t> decompress_proof(const std::vector<uint8_t>& compressed_data);

    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace evm 