#include "blockchain/AIBlock.hpp"
#include <blake3.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>

namespace quids {
namespace blockchain {

AIBlock::AIBlock(const BlockHeader& header) : header_(header) {
    transactions_.reserve(1000); // Pre-allocate for efficiency
}

bool AIBlock::addTransaction(const Transaction& tx) {
    if (!tx.is_valid()) {
        return false;
    }
    
    // Check if transaction already exists
    auto it = std::find_if(transactions_.begin(), transactions_.end(),
        [&tx](const Transaction& existing) {
            return existing.compute_hash() == tx.compute_hash();
        });
        
    if (it != transactions_.end()) {
        return false;
    }
    
    transactions_.push_back(tx);
    updateMetrics();
    return true;
}

bool AIBlock::verifyBlock() const {
    // Verify basic structure
    if (transactions_.empty()) {
        return false;
    }
    
    // Verify all transactions
    if (!validateTransactions()) {
        return false;
    }
    
    // Verify Merkle root
    if (computeMerkleRoot() != header_.transactions_root) {
        return false;
    }
    
    // Verify quantum proof if present
    if (!quantum_proof_.empty() && !verifyQuantumProof()) {
        return false;
    }
    
    return true;
}

std::array<uint8_t, 32> AIBlock::computeHash() const {
    std::array<uint8_t, 32> hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    
    // Add block header fields
    blake3_hasher_update(&hasher, &header_.number, sizeof(header_.number));
    blake3_hasher_update(&hasher, header_.previous_hash.data(), header_.previous_hash.size());
    blake3_hasher_update(&hasher, header_.transactions_root.data(), header_.transactions_root.size());
    blake3_hasher_update(&hasher, header_.state_root.data(), header_.state_root.size());
    blake3_hasher_update(&hasher, header_.receipts_root.data(), header_.receipts_root.size());
    blake3_hasher_update(&hasher, &header_.timestamp, sizeof(header_.timestamp));
    
    // Add AI metrics
    blake3_hasher_update(&hasher, &header_.metrics.quantum_security_score, sizeof(double));
    blake3_hasher_update(&hasher, &header_.metrics.transaction_efficiency, sizeof(double));
    blake3_hasher_update(&hasher, &header_.metrics.network_health, sizeof(double));
    blake3_hasher_update(&hasher, &header_.metrics.consensus_confidence, sizeof(double));
    
    blake3_hasher_finalize(&hasher, hash.data(), hash.size());
    return hash;
}

void AIBlock::updateMetrics() {
    updateQuantumSecurityScore();
    updateTransactionEfficiency();
    updateNetworkHealth();
    updateConsensusConfidence();
    updateMLFeatures();
}

double AIBlock::predictOptimalGasPrice() const {
    // Use ML features to predict optimal gas price
    const auto& features = header_.metrics.ml_features;
    if (features.empty()) {
        return 1.0; // Default gas price
    }
    
    // Simple weighted average for demonstration
    // In production, use proper ML model
    double weighted_sum = 0.0;
    std::vector<double> weights = {0.3, 0.2, 0.3, 0.2}; // Example weights
    
    for (size_t i = 0; i < std::min(features.size(), weights.size()); i++) {
        weighted_sum += features[i] * weights[i];
    }
    
    return std::max(1.0, weighted_sum);
}

void AIBlock::updateMLFeatures() {
    std::vector<double> features;
    features.reserve(4);
    
    // Feature 1: Transaction density
    features.push_back(static_cast<double>(transactions_.size()) / 1000.0);
    
    // Feature 2: Average gas price
    double avg_gas_price = 0.0;
    if (!transactions_.empty()) {
        avg_gas_price = std::accumulate(transactions_.begin(), transactions_.end(), 0.0,
            [](double sum, const Transaction& tx) {
                return sum + tx.gas_price;
            }) / transactions_.size();
    }
    features.push_back(avg_gas_price / 100.0); // Normalize
    
    // Feature 3: Security score
    features.push_back(header_.metrics.quantum_security_score);
    
    // Feature 4: Network health
    features.push_back(header_.metrics.network_health);
    
    header_.metrics.ml_features = std::move(features);
}

void AIBlock::updateQuantumSecurityScore() {
    // Calculate quantum security score based on:
    // 1. Quantum proof strength
    // 2. Number of quantum-resistant signatures
    // 3. Entanglement metrics
    
    double proof_strength = !quantum_proof_.empty() ? 1.0 : 0.0;
    
    double signature_score = 0.0;
    for (const auto& tx : transactions_) {
        if (tx.verify()) {  // Assuming verify checks quantum resistance
            signature_score += 1.0;
        }
    }
    signature_score /= std::max(1UL, transactions_.size());

    // Combine metrics with weights
    header_.metrics.quantum_security_score = 
        0.4 * proof_strength +
        0.4 * signature_score +
        0.2 * calculateSecurityScore();  // Additional security metrics
}

void AIBlock::updateTransactionEfficiency() {
    if (transactions_.empty()) {
        header_.metrics.transaction_efficiency = 0.0;
        return;
    }

    // Calculate average gas usage efficiency
    double avg_gas_efficiency = 0.0;
    for (const auto& tx : transactions_) {
        double gas_used = tx.calculate_gas_cost();
        double gas_limit = tx.gas_limit;
        avg_gas_efficiency += gas_used / gas_limit;
    }
    avg_gas_efficiency /= transactions_.size();

    // Calculate transaction density
    double density = static_cast<double>(transactions_.size()) / 1000.0; // Normalize to target size

    header_.metrics.transaction_efficiency = 
        0.6 * avg_gas_efficiency +
        0.4 * density;
}

void AIBlock::updateNetworkHealth() {
    // Network health metrics:
    // 1. Transaction success rate
    // 2. Average response time
    // 3. Network participation

    size_t successful_txs = 0;
    for (const auto& tx : transactions_) {
        if (tx.is_valid()) {
            successful_txs++;
        }
    }

    double success_rate = transactions_.empty() ? 0.0 : 
        static_cast<double>(successful_txs) / transactions_.size();

    // Simplified network health score
    header_.metrics.network_health = success_rate;
}

void AIBlock::updateConsensusConfidence() {
    // Consensus confidence based on:
    // 1. Validator participation
    // 2. Block finality
    // 3. Network agreement

    // Simplified consensus score for now
    double base_confidence = 0.8; // Base confidence level
    
    // Adjust based on quantum proof
    if (!quantum_proof_.empty()) {
        base_confidence += 0.2;
    }

    // Adjust based on transaction validity
    size_t valid_txs = std::count_if(transactions_.begin(), transactions_.end(),
        [](const Transaction& tx) { return tx.is_valid(); });
    
    double tx_confidence = transactions_.empty() ? 1.0 :
        static_cast<double>(valid_txs) / transactions_.size();

    header_.metrics.consensus_confidence = 
        0.7 * base_confidence +
        0.3 * tx_confidence;
}

std::array<uint8_t, 32> AIBlock::computeMerkleRoot() const {
    if (transactions_.empty()) {
        return std::array<uint8_t, 32>{};
    }

    std::vector<std::array<uint8_t, 32>> hashes;
    hashes.reserve(transactions_.size());

    // Get transaction hashes
    for (const auto& tx : transactions_) {
        hashes.push_back(tx.compute_hash());
    }

    // Build Merkle tree
    while (hashes.size() > 1) {
        std::vector<std::array<uint8_t, 32>> new_hashes;
        new_hashes.reserve((hashes.size() + 1) / 2);

        for (size_t i = 0; i < hashes.size(); i += 2) {
            blake3_hasher hasher;
            blake3_hasher_init(&hasher);

            // Hash pair of nodes
            blake3_hasher_update(&hasher, hashes[i].data(), hashes[i].size());
            if (i + 1 < hashes.size()) {
                blake3_hasher_update(&hasher, hashes[i + 1].data(), hashes[i + 1].size());
            } else {
                blake3_hasher_update(&hasher, hashes[i].data(), hashes[i].size());
            }

            std::array<uint8_t, 32> new_hash;
            blake3_hasher_finalize(&hasher, new_hash.data(), new_hash.size());
            new_hashes.push_back(new_hash);
        }

        hashes = std::move(new_hashes);
    }

    return hashes[0];
}

bool AIBlock::validateTransactions() const {
    return std::all_of(transactions_.begin(), transactions_.end(),
        [](const Transaction& tx) { return tx.is_valid(); });
}

bool AIBlock::verifyQuantumProof() const {
    // TODO: Implement proper quantum proof verification
    return !quantum_proof_.empty();
}

double AIBlock::calculateSecurityScore() const {
    if (transactions_.empty()) {
        return 0.0;
    }

    // Calculate security metrics:
    // 1. Signature strength
    // 2. Transaction entropy
    // 3. Block complexity
    // 4. Quantum resistance level

    // Signature strength (0-1)
    double signature_strength = std::accumulate(transactions_.begin(), transactions_.end(), 0.0,
        [](double sum, const Transaction& tx) {
            return sum + (tx.getSignature().size() >= Transaction::SIGNATURE_SIZE ? 1.0 : 0.0);
        }) / transactions_.size();

    // Transaction entropy (0-1)
    double entropy = 0.0;
    std::map<std::string, size_t> address_freq;
    for (const auto& tx : transactions_) {
        address_freq[tx.getSender()]++;
        address_freq[tx.getRecipient()]++;
    }
    
    double total_addresses = address_freq.size();
    for (const auto& [_, freq] : address_freq) {
        double p = freq / (2.0 * transactions_.size()); // Normalize by total addresses
        entropy -= p * std::log2(p);
    }
    entropy = entropy / std::log2(std::max(2.0, total_addresses)); // Normalize to [0,1]

    // Block complexity (0-1)
    double complexity = std::min(1.0, transactions_.size() / 1000.0);

    // Quantum resistance level (0-1)
    double quantum_resistance = !quantum_proof_.empty() ? 1.0 : 0.0;

    // Weighted combination of security metrics
    return 0.3 * signature_strength +
           0.2 * entropy +
           0.2 * complexity +
           0.3 * quantum_resistance;
}

} // namespace blockchain
} // namespace quids 