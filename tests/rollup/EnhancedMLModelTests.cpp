#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include "rollup/EnhancedRollupMLModel.h"
#include "rollup/RollupPerformanceMetrics.h"
#include "rollup/QuantumParameters.h"
#include "rollup/CrossChainState.h"

class EnhancedMLModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnhancedMLParameters params;
        params.num_layers = 3;
        params.hidden_size = 128;
        params.learning_rate = 0.001;
        params.dropout_rate = 0.2;
        
        model = std::make_unique<EnhancedRollupMLModel>(params);
    }
    
    RollupPerformanceMetrics createTestMetrics() {
        RollupPerformanceMetrics metrics;
        metrics.tx_throughput = 1000;
        metrics.proof_generation_time = 2.5;
        metrics.verification_time = 1.2;
        metrics.memory_usage = 512;
        metrics.quantum_energy_usage = 75.0;
        metrics.avg_tx_latency = 0.05;
        metrics.block_height = 1000;
        metrics.block_interval = 5.0;
        return metrics;
    }
    
    QuantumParameters createTestParameters() {
        QuantumParameters params;
        params.qubits_per_transaction = 10;
        params.num_qubits = 100;
        params.error_correction_overhead = 0.001;
        params.quantum_memory_capacity = 100.0;
        params.entanglement_strength = 0.999;
        return params;
    }
    
    CrossChainState createTestState(size_t num_chains = 3) {
        CrossChainState state;
        state.active_chains = num_chains;
        state.chain_loads = std::vector<double>(num_chains, 1.0 / num_chains);
        state.cross_chain_latencies = std::vector<double>(num_chains * num_chains, 0.1);
        state.total_throughput = 1000.0;
        state.energy_distribution = std::vector<double>(num_chains, 1.0 / num_chains);
        return state;
    }
    
    std::unique_ptr<EnhancedRollupMLModel> model;
};

TEST_F(EnhancedMLModelTest, TestMetricsSequenceGeneration) {
    std::vector<RollupPerformanceMetrics> metrics_history;
    std::vector<QuantumParameters> param_history;
    
    for (int i = 0; i < 10; i++) {
        auto metrics = createTestMetrics();
        metrics.tx_throughput += i * 100;
        metrics_history.push_back(metrics);
        
        auto params = createTestParameters();
        params.qubits_per_transaction += i;
        param_history.push_back(params);
    }
    
    model->train(metrics_history, param_history);
    
    auto predicted_params = model->predictOptimalParameters(metrics_history.back());
    EXPECT_GT(predicted_params.qubits_per_transaction, 0);
    EXPECT_GT(predicted_params.num_qubits, 0);
}

TEST_F(EnhancedMLModelTest, TestPerformanceOptimization) {
    auto metrics = createTestMetrics();
    auto state = createTestState();
    
    std::vector<std::pair<std::string, double>> objectives = {
        {"throughput", 0.6},
        {"latency", 0.4}
    };
    
    auto result = model->optimizeParameters(metrics, state, objectives);
    EXPECT_GT(result.objective_score, 0.0);
    EXPECT_FALSE(result.tradeoff_explanations.empty());
}

TEST_F(EnhancedMLModelTest, TestCrossChainOptimization) {
    std::vector<RollupPerformanceMetrics> chain_metrics;
    std::vector<QuantumParameters> chain_params;
    
    for (int i = 0; i < 3; i++) {
        auto metrics = createTestMetrics();
        metrics.tx_throughput += i * 200;
        chain_metrics.push_back(metrics);
        
        auto params = createTestParameters();
        params.qubits_per_transaction += i * 2;
        chain_params.push_back(params);
    }
    
    auto optimized_state = model->optimizeChainDistribution(chain_metrics, chain_params);
    EXPECT_EQ(optimized_state.active_chains, 3);
    EXPECT_FALSE(optimized_state.chain_loads.empty());
}

TEST_F(EnhancedMLModelTest, TestComplexQueryProcessing) {
    auto metrics = createTestMetrics();
    auto state = createTestState();
    
    std::string query = "What are the main performance bottlenecks?";
    auto result = model->processComplexQuery(query, metrics, state);
    
    EXPECT_GT(result.confidence, 0.5);
    EXPECT_FALSE(result.explanation.empty());
    EXPECT_FALSE(result.suggested_actions.empty());
}

TEST_F(EnhancedMLModelTest, TestPerformanceAnalysis) {
    auto metrics = createTestMetrics();
    auto bottlenecks = model->analyzePerformanceBottlenecks(metrics);
    EXPECT_FALSE(bottlenecks.empty());
    
    auto optimizations = model->suggestOptimizations(metrics);
    EXPECT_FALSE(optimizations.empty());
}