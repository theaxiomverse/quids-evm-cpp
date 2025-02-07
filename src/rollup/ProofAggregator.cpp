#include "rollup/ProofAggregator.hpp"
#include <openssl/evp.h>
#include <stdexcept>
#include <algorithm>

namespace {
std::vector<uint8_t> compute_proof_merkle_root(
    const std::vector<QZKPGenerator::Proof>& proofs
) {
    std::vector<std::array<uint8_t, 32>> hashes;
    hashes.reserve(proofs.size());

    // First level: hash individual proofs
    for (const auto& proof : proofs) {
        std::array<uint8_t, 32> hash;
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create EVP context");
        }

        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize SHA256");
        }

        // Hash the proof data
        if (EVP_DigestUpdate(ctx, proof.proof_data.data(), proof.proof_data.size()) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to update digest");
        }

        unsigned int hash_len;
        if (EVP_DigestFinal_ex(ctx, hash.data(), &hash_len) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to finalize digest");
        }

        EVP_MD_CTX_free(ctx);
        hashes.push_back(hash);
    }

    // Build Merkle tree
    while (hashes.size() > 1) {
        size_t pairs = hashes.size() / 2;
        for (size_t i = 0; i < pairs; i++) {
            std::array<uint8_t, 32> combined_hash;
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            if (!ctx) {
                throw std::runtime_error("Failed to create EVP context");
            }

            if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("Failed to initialize SHA256");
            }

            if (EVP_DigestUpdate(ctx, hashes[2*i].data(), 32) != 1 ||
                EVP_DigestUpdate(ctx, hashes[2*i + 1].data(), 32) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("Failed to update digest");
            }

            unsigned int hash_len;
            if (EVP_DigestFinal_ex(ctx, combined_hash.data(), &hash_len) != 1) {
                EVP_MD_CTX_free(ctx);
                throw std::runtime_error("Failed to finalize digest");
            }

            EVP_MD_CTX_free(ctx);
            hashes[i] = combined_hash;
        }

        // If odd number of hashes, keep the last one
        if (hashes.size() % 2 == 1) {
            hashes[pairs] = hashes.back();
            hashes.resize(pairs + 1);
        } else {
            hashes.resize(pairs);
        }
    }

    // Convert final hash to vector
    return std::vector<uint8_t>(hashes[0].begin(), hashes[0].end());
}
} // namespace

ProofAggregator::ProofAggregator() = default;

std::vector<uint8_t> ProofAggregator::aggregate_proofs(
    const std::vector<QZKPGenerator::Proof>& proofs
) {
    if (proofs.empty()) {
        throw std::invalid_argument("No proofs to aggregate");
    }

    // Compute Merkle root of all proofs
    auto merkle_root = compute_proof_merkle_root(proofs);

    // Combine all proof data
    std::vector<uint8_t> aggregated_data;
    for (const auto& proof : proofs) {
        aggregated_data.insert(
            aggregated_data.end(),
            proof.proof_data.begin(),
            proof.proof_data.end()
        );
    }

    // Add Merkle root to the end
    aggregated_data.insert(
        aggregated_data.end(),
        merkle_root.begin(),
        merkle_root.end()
    );

    return aggregated_data;
}

bool ProofAggregator::verify_aggregated_proof(
    const std::vector<uint8_t>& aggregated_proof,
    const std::vector<QZKPGenerator::Proof>& original_proofs
) {
    if (original_proofs.empty() || aggregated_proof.size() < 32) {
        return false;
    }

    // Extract Merkle root from aggregated proof
    std::vector<uint8_t> provided_root(
        aggregated_proof.end() - 32,
        aggregated_proof.end()
    );

    // Compute expected Merkle root
    auto expected_root = compute_proof_merkle_root(original_proofs);

    // Verify Merkle root matches
    return provided_root == expected_root;
} 