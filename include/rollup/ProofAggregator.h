#pragma once
#include "zkp/QZKPGenerator.h"
#include <vector>

class ProofAggregator {
public:
    ProofAggregator();
    
    std::vector<uint8_t> aggregate_proofs(
        const std::vector<QZKPGenerator::Proof>& proofs
    );
    
    bool verify_aggregated_proof(
        const std::vector<uint8_t>& aggregated_proof,
        const std::vector<QZKPGenerator::Proof>& original_proofs
    );
}; 