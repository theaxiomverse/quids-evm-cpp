#include <gtest/gtest.h>
#include "rollup/AIRollupAgent.hpp"
#include "rollup/RollupPerformanceMetrics.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "quantum/QuantumState.hpp"
#include <memory>

namespace quids {
namespace rollup {
namespace test {

class AIRollupTest : public ::testing::Test {
protected:
    void SetUp() override {
        zkp_generator_ = std::make_shared<quids::zkp::QZKPGenerator>();
        agent_ = std::make_unique<RLRollupAgent>(zkp_generator_);
    }

    void TearDown() override {
        agent_.reset();
        zkp_generator_.reset();
    }

    std::shared_ptr<quids::zkp::QZKPGenerator> zkp_generator_;
    std::unique_ptr<RLRollupAgent> agent_;
};

TEST_F(AIRollupTest, BasicConsensusSelection) {
    RollupPerformanceMetrics metrics;
    metrics.tx_throughput = 1000;
    metrics.block_interval = 2.0;
    metrics.quantum_energy_usage = 5000000;
    metrics.memory_usage = 1024 * 1024;
    metrics.network_bandwidth = 100;
    metrics.success_rate = 0.999;
    metrics.verification_time = 0.5;
    metrics.total_proofs_generated = 1024;
    metrics.active_validators = 10;

    agent_->analyzeRollupMetrics(metrics);
    auto consensus = agent_->selectConsensusAlgorithm();
    EXPECT_TRUE(consensus == RollupConsensusType::QUANTUM_ZKP ||
                consensus == RollupConsensusType::QUANTUM_DAG);
}

TEST_F(AIRollupTest, LoadBasedConsensusSelection) {
    // Test low load scenario
    RollupPerformanceMetrics low_load;
    low_load.tx_throughput = 500;
    low_load.network_bandwidth = 50;
    agent_->analyzeRollupMetrics(low_load);
    EXPECT_EQ(agent_->selectConsensusAlgorithm(), RollupConsensusType::QUANTUM_ZKP);

    // Test high load scenario
    RollupPerformanceMetrics high_load;
    high_load.tx_throughput = 5000;
    high_load.network_bandwidth = 200;
    agent_->analyzeRollupMetrics(high_load);
    EXPECT_EQ(agent_->selectConsensusAlgorithm(), RollupConsensusType::QUANTUM_DAG);

    // Test system overload
    RollupPerformanceMetrics overload;
    overload.tx_throughput = 10000;
    overload.network_bandwidth = 500;
    overload.success_rate = 0.9;
    agent_->analyzeRollupMetrics(overload);
    auto consensus = agent_->selectConsensusAlgorithm();
    EXPECT_TRUE(consensus == RollupConsensusType::QUANTUM_DAG);
}

TEST_F(AIRollupTest, SecurityBasedConsensusSelection) {
    // Test base security level
    RollupPerformanceMetrics base_metrics;
    base_metrics.success_rate = 0.95;
    base_metrics.failed_proofs = 10;
    agent_->analyzeRollupMetrics(base_metrics);
    auto base_consensus = agent_->selectConsensusAlgorithm();
    EXPECT_TRUE(base_consensus == RollupConsensusType::QUANTUM_ZKP ||
                base_consensus == RollupConsensusType::QUANTUM_DAG);

    // Test high security requirements
    RollupPerformanceMetrics high_security;
    high_security.success_rate = 0.99;
    high_security.failed_proofs = 1;
    agent_->analyzeRollupMetrics(high_security);
    EXPECT_EQ(agent_->selectConsensusAlgorithm(), RollupConsensusType::QUANTUM_ZKP);
}

TEST_F(AIRollupTest, TestPhaseAngleOptimization) {
    // Initial metrics
    RollupPerformanceMetrics metrics;
    metrics.tx_throughput = 500000;
    metrics.proof_generation_time = 1.5;
    metrics.verification_time = 0.8;
    metrics.quantum_energy_usage = 50.0;
    metrics.success_rate = 0.7;
    metrics.active_validators = 3;
    
    // First optimization cycle
    agent_->analyzeRollupMetrics(metrics);
    auto angles = agent_->optimizePhaseAngles();
    EXPECT_LE(angles.size(), 5);  // Should reduce angles for better performance
    
    // Improved metrics
    metrics.verification_time = 0.3;
    metrics.success_rate = 0.9;
    agent_->analyzeRollupMetrics(metrics);
    auto new_angles = agent_->optimizePhaseAngles();
    
    // Check that the new angles are valid
    EXPECT_LE(new_angles.size(), 5);
    for (const auto& angle : new_angles) {
        EXPECT_GE(angle, 0.0);
        EXPECT_LE(angle, 2.0 * M_PI);
    }
}

TEST_F(AIRollupTest, TestChildRollupCreation) {
    RollupPerformanceMetrics overload;
    overload.tx_throughput = 2000000;
    overload.verification_time = 1.5;
    agent_->analyzeRollupMetrics(overload);
    EXPECT_TRUE(agent_->shouldSpawnChildRollup());
    
    auto child = agent_->createChildAgent();
    EXPECT_NE(child, nullptr);
}

TEST_F(AIRollupTest, TestSecurityThresholdAdjustment) {
    RollupPerformanceMetrics base_metrics;
    base_metrics.tx_throughput = 100000;
    base_metrics.active_validators = 5;
    agent_->analyzeRollupMetrics(base_metrics);
    double base_threshold = agent_->calculateSecurityThreshold();
    EXPECT_DOUBLE_EQ(base_threshold, 0.75);
    
    RollupPerformanceMetrics high_security;
    high_security.tx_throughput = 2000000;
    high_security.active_validators = 15;
    agent_->analyzeRollupMetrics(high_security);
    double high_threshold = agent_->calculateSecurityThreshold();
    EXPECT_GT(high_threshold, base_threshold);
}

} // namespace test
} // namespace rollup
} // namespace quids 