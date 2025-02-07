#pragma once

#include "blockchain/Transaction.hpp"
#include "quantum/QuantumState.hpp"
#include "zkp/QZKPGenerator.hpp"
#include <vector>
#include <array>
#include <memory>
#include <optional>

namespace quids {
namespace blockchain {

class AIBlock {
public:
    struct AIMetrics {
        double quantum_security_score{0.0};
        double transaction_efficiency{0.0};
        double network_health{0.0};
        double consensus_confidence{0.0};
        std::vector<double> ml_features;
    };

    struct BlockHeader {
        uint64_t number{0};
        std::array<uint8_t, 32> previous_hash;
        std::array<uint8_t, 32> transactions_root;
        std::array<uint8_t, 32> state_root;
        std::array<uint8_t, 32> receipts_root;
        uint64_t timestamp{0};
        AIMetrics metrics;
    };

    AIBlock() = default;
    explicit AIBlock(const BlockHeader& header);

    // Core functionality
    bool addTransaction(const Transaction& tx);
    bool verifyBlock() const;
    std::array<uint8_t, 32> computeHash() const;
    
    // AI-specific methods
    void updateMetrics();
    double predictOptimalGasPrice() const;
    std::optional<uint64_t> suggestBlockSize() const;
    double calculateSecurityScore() const;
    
    // Quantum-resistant features
    bool generateQuantumProof();
    bool verifyQuantumProof() const;
    
    // Getters
    const BlockHeader& getHeader() const { return header_; }
    const std::vector<Transaction>& getTransactions() const { return transactions_; }
    const AIMetrics& getMetrics() const { return header_.metrics; }
    
    // ML model interaction
    void updateMLFeatures();
    std::vector<double> extractFeatures() const;
    
private:
    BlockHeader header_;
    std::vector<Transaction> transactions_;
    std::vector<uint8_t> quantum_proof_;
    
    // Helper methods
    std::array<uint8_t, 32> computeMerkleRoot() const;
    bool validateTransactions() const;
    void updateQuantumSecurityScore();
    void updateTransactionEfficiency();
    void updateNetworkHealth();
    void updateConsensusConfidence();
};

} // namespace blockchain
} // namespace quids 