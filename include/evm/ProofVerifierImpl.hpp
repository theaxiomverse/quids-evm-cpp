#pragma once

namespace evm {

class ProofVerifierImpl {
public:
    virtual ~ProofVerifierImpl() = default;
    virtual bool verify_zk_proof(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs) = 0;
    virtual bool verify_state_transition(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs) = 0;
    virtual bool verify_transaction(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs) = 0;
    virtual bool verify_storage_proof(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs) = 0;
    virtual bool verify_state_root(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs) = 0;
    virtual bool verify_transaction_root(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs) = 0;
    virtual bool verify_receipt_root(const std::vector<uint8_t>& proof, const std::vector<uint8_t>& public_inputs) = 0;
 
    

    // Add any required definitions here.
};

} // namespace evm 