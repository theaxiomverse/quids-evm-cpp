#include "consensus/POBPC.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>

namespace consensus {

class POBPC::Impl {
public:
    std::vector<std::vector<uint8_t>> pending_transactions;
    std::unordered_map<std::string, WitnessInfo> witnesses;
    std::vector<BatchProof> processed_batches;
    BatchProofGenerator proof_generator;
    
    // Metrics
    ConsensusMetrics metrics{0.0, 0.0, 0, 0, 0.0};
    std::chrono::system_clock::time_point last_batch_time;
    
    // Random number generation
    std::random_device rd;
    std::mt19937_64 gen{rd()};
};

POBPC::POBPC(const BatchConfig& config) 
    : impl_(std::make_unique<Impl>()),
      config_(config) {
    impl_->last_batch_time = std::chrono::system_clock::now();
}

POBPC::~POBPC() = default;

bool POBPC::addTransaction(const std::vector<uint8_t>& transaction) {
    if (impl_->pending_transactions.size() >= config_.max_transactions) {
        return false;
    }
    impl_->pending_transactions.push_back(transaction);
    return true;
}

BatchProof POBPC::generateBatchProof() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    BatchProof proof;
    proof.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    proof.transaction_count = impl_->pending_transactions.size();
    
    // Create batch hash
    proof.batch_hash = createBatchHash(impl_->pending_transactions);
    
    // Generate ZK proof for the batch
    proof.proof_data = impl_->proof_generator.generateProof(impl_->pending_transactions);
    
    // Clear pending transactions
    impl_->pending_transactions.clear();
    
    // Record metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    recordMetrics(proof, std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time));
    
    return proof;
}

bool POBPC::verifyBatchProof(const BatchProof& proof) {
    if (!validateBatchStructure(proof)) {
        return false;
    }
    
    return impl_->proof_generator.verifyProof(proof.proof_data, proof.batch_hash);
}

bool POBPC::registerWitness(const std::string& node_id, const std::vector<uint8_t>& public_key) {
    WitnessInfo info;
    info.node_id = node_id;
    info.public_key = public_key;
    info.reliability_score = 1.0;
    info.last_active = std::chrono::system_clock::now().time_since_epoch().count();
    
    impl_->witnesses[node_id] = info;
    return true;
}

std::vector<POBPC::WitnessInfo> POBPC::selectWitnesses() {
    // Filter active and reliable witnesses
    std::vector<WitnessInfo> active_witnesses;
    for (const auto& [id, info] : impl_->witnesses) {
        if (info.reliability_score >= 0.5) {  // Minimum reliability threshold
            active_witnesses.push_back(info);
        }
    }
    
    return selectWitnessesRandomly(std::min(config_.witness_count, active_witnesses.size()));
}

bool POBPC::submitWitnessVote(const std::string& witness_id,
                             const std::vector<uint8_t>& signature,
                             const BatchProof& proof) {
    auto it = impl_->witnesses.find(witness_id);
    if (it == impl_->witnesses.end()) {
        return false;
    }
    
    if (!verifyWitnessSignature(witness_id, signature, proof.batch_hash)) {
        updateWitnessReliability(witness_id, false);
        return false;
    }
    
    proof.witness_signatures.push_back(signature);
    updateWitnessReliability(witness_id, true);
    return true;
}

bool POBPC::hasReachedConsensus(const BatchProof& proof) {
    if (proof.witness_signatures.empty()) {
        return false;
    }
    
    double consensus_ratio = static_cast<double>(proof.witness_signatures.size()) /
                           static_cast<double>(config_.witness_count);
    return consensus_ratio >= config_.consensus_threshold;
}

double POBPC::calculateConsensusConfidence(const BatchProof& proof) {
    if (proof.witness_signatures.empty()) {
        return 0.0;
    }
    
    // Calculate weighted confidence based on witness reliability scores
    double total_weight = 0.0;
    double weighted_sum = 0.0;
    
    for (const auto& signature : proof.witness_signatures) {
        // Find witness info by matching signature
        for (const auto& [id, info] : impl_->witnesses) {
            if (verifyWitnessSignature(id, signature, proof.batch_hash)) {
                weighted_sum += info.reliability_score;
                total_weight += 1.0;
                break;
            }
        }
    }
    
    return total_weight > 0 ? weighted_sum / total_weight : 0.0;
}

POBPC::ConsensusMetrics POBPC::getMetrics() const {
    return impl_->metrics;
}

std::vector<uint8_t> POBPC::createBatchHash(
    const std::vector<std::vector<uint8_t>>& transactions) {
    // Combine all transaction data
    std::vector<uint8_t> combined;
    for (const auto& tx : transactions) {
        combined.insert(combined.end(), tx.begin(), tx.end());
    }
    
    // Use quantum-safe hashing
    return quantum::QuantumCrypto::QHash(combined);
}

bool POBPC::verifyWitnessSignature(const std::string& witness_id,
                                  const std::vector<uint8_t>& signature,
                                  const std::vector<uint8_t>& message) {
    auto it = impl_->witnesses.find(witness_id);
    if (it == impl_->witnesses.end()) {
        return false;
    }
    
    return quantum::QuantumCrypto::QVerify(message, signature, it->second.public_key);
}

void POBPC::updateWitnessReliability(const std::string& witness_id, bool successful_validation) {
    auto it = impl_->witnesses.find(witness_id);
    if (it != impl_->witnesses.end()) {
        constexpr double ALPHA = 0.1;  // Learning rate
        it->second.reliability_score = (1.0 - ALPHA) * it->second.reliability_score +
                                     ALPHA * (successful_validation ? 1.0 : 0.0);
        it->second.last_active = std::chrono::system_clock::now().time_since_epoch().count();
    }
}

std::vector<POBPC::WitnessInfo> POBPC::selectWitnessesRandomly(size_t count) {
    std::vector<WitnessInfo> selected;
    std::vector<WitnessInfo> all_witnesses;
    
    for (const auto& [id, info] : impl_->witnesses) {
        all_witnesses.push_back(info);
    }
    
    if (all_witnesses.empty()) {
        return selected;
    }
    
    // Shuffle witnesses
    std::shuffle(all_witnesses.begin(), all_witnesses.end(), impl_->gen);
    
    // Select the first 'count' witnesses
    selected.insert(selected.end(),
                   all_witnesses.begin(),
                   all_witnesses.begin() + std::min(count, all_witnesses.size()));
    
    return selected;
}

bool POBPC::validateBatchStructure(const BatchProof& proof) {
    return !proof.proof_data.empty() &&
           !proof.batch_hash.empty() &&
           proof.timestamp > 0 &&
           proof.transaction_count > 0;
}

void POBPC::recordMetrics(const BatchProof& proof, std::chrono::microseconds processing_time) {
    auto& m = impl_->metrics;
    
    // Update averages
    m.avg_batch_time = (m.avg_batch_time * m.total_batches_processed + 
                       processing_time.count()) / (m.total_batches_processed + 1);
    
    // Update counters
    m.total_batches_processed++;
    m.total_transactions_processed += proof.transaction_count;
    
    // Update witness participation rate
    m.witness_participation_rate = static_cast<double>(proof.witness_signatures.size()) /
                                 static_cast<double>(config_.witness_count);
}

// BatchProofGenerator implementation
BatchProofGenerator::BatchProofGenerator() = default;
BatchProofGenerator::~BatchProofGenerator() = default;

std::vector<uint8_t> BatchProofGenerator::generateProof(
    const std::vector<std::vector<uint8_t>>& transactions) {
    auto commitment = createCommitment(transactions);
    return generateZKProof(commitment);
}

bool BatchProofGenerator::verifyProof(const std::vector<uint8_t>& proof,
                                    const std::vector<uint8_t>& batch_hash) {
    // Verify the ZK proof using quantum-safe verification
    return qcrypto_.QVerify(batch_hash, proof, qcrypto_.getPublicKey());
}

std::vector<uint8_t> BatchProofGenerator::createCommitment(
    const std::vector<std::vector<uint8_t>>& transactions) {
    std::vector<uint8_t> combined;
    for (const auto& tx : transactions) {
        combined.insert(combined.end(), tx.begin(), tx.end());
    }
    return qcrypto_.QHash(combined);
}

std::vector<uint8_t> BatchProofGenerator::generateZKProof(
    const std::vector<uint8_t>& commitment) {
    // Generate a quantum-safe ZK proof
    return qcrypto_.QSign(commitment);
}

} // namespace consensus 