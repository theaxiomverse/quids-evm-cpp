#pragma once

#include <vector>
#include <memory>
#include <string>
#include "evm/Memory.hpp"
#include "evm/Address.hpp"

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
    explicit ProofVerifier(std::shared_ptr<StateManager> state_manager);
    
    // Verification methods
    bool verify_state_transition(const ProofData& proof);
    bool verify_transaction(const std::vector<uint8_t>& tx_hash, const ProofData& proof);
    bool verify_storage_proof(const Address& address, const std::vector<uint8_t>& key, const ProofData& proof);
    
    // Proof generation
    ProofData generate_state_proof(uint64_t block_number);
    ProofData generate_transaction_proof(const std::vector<uint8_t>& tx_hash);
    ProofData generate_storage_proof(const Address& address, const std::vector<uint8_t>& key);
    
    // ZK-proof specific methods
    bool verify_zk_proof(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs);
    std::vector<uint8_t> generate_zk_proof(const std::vector<uint8_t>& private_inputs, const std::vector<uint8_t>& public_inputs);

private:
    std::shared_ptr<StateManager> state_manager_;
    
    // Internal verification helpers
    bool verify_merkle_proof(const std::vector<uint8_t>& root, const std::vector<uint8_t>& leaf, const std::vector<std::vector<uint8_t>>& proof);
    bool verify_state_root(const std::vector<uint8_t>& root);
    
    // Compression methods for proof data
    std::vector<uint8_t> compress_proof(const std::vector<uint8_t>& proof_data);
    std::vector<uint8_t> decompress_proof(const std::vector<uint8_t>& compressed_data);
};

} // namespace evm 