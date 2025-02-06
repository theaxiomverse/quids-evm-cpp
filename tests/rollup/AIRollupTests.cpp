#include <gtest/gtest.h>
#include "rollup/AIRollupAgent.h"
#include "rollup/RollupPerformanceMetrics.h"
#include "zkp/QZKPGenerator.h"
#include "quantum/QuantumState.h"

class AIRollupTestSuite : public ::testing::Test {
protected:
    std::shared_ptr<QZKPGenerator> zkp_generator_;
    std::unique_ptr<RLRollupAgent> agent_;
    
    void SetUp() override {
        zkp_generator_ = std::make_shared<QZKPGenerator>();
        agent_ = std::make_unique<RLRollupAgent>(zkp_generator_);
    }
};

TEST_F(AIRollupTestSuite, TestPhaseAngleOptimization) {
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

TEST_F(AIRollupTestSuite, TestConsensusSelection) {
    RollupPerformanceMetrics low_load;
    low_load.tx_throughput = 100000;
    low_load.active_validators = 3;
    EXPECT_EQ(agent_->selectConsensusAlgorithm(), RollupConsensusType::QUANTUM_ZKP);
    
    RollupPerformanceMetrics high_load;
    high_load.tx_throughput = 2000000;
    high_load.active_validators = 10;
    agent_->analyzeRollupMetrics(high_load);
    EXPECT_EQ(agent_->selectConsensusAlgorithm(), RollupConsensusType::QUANTUM_DAG);
}

TEST_F(AIRollupTestSuite, TestChildRollupCreation) {
    RollupPerformanceMetrics overload;
    overload.tx_throughput = 2000000;
    overload.verification_time = 1.5;
    agent_->analyzeRollupMetrics(overload);
    EXPECT_TRUE(agent_->shouldSpawnChildRollup());
    
    auto child = agent_->createChildAgent();
    EXPECT_NE(child, nullptr);
}

TEST_F(AIRollupTestSuite, TestSecurityThresholdAdjustment) {
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