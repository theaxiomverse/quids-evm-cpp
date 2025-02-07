#pragma once
#include "zkp/QZKPGenerator.hpp"
#include <vector>

using quids::zkp::QZKPGenerator;

class ProofAggregator {
public:
    ProofAggregator();
    
    std::vector<uint8_t> aggregate_proofs(
        const std::vector<quids::zkp::QZKPGenerator::Proof>& proofs
    );
    
    bool verify_aggregated_proof(
        const std::vector<uint8_t>& aggregated_proof,
        const std::vector<quids::zkp::QZKPGenerator::Proof>& original_proofs
    );
}; 