#include <gtest/gtest.h>
#include "rollup/RollupMLModel.h"
#include <chrono>
#include <thread>

class MLModelTestSuite : public ::testing::Test {
protected:
    void SetUp() override {
        MLModelParameters params;
        params.learning_rate = 0.001;
        params.batch_size = 32;
        params.hidden_size = 128;
        params.num_layers = 3;
        params.dropout_rate = 0.2;
        
        model_ = std::make_unique<RollupMLModel>(params, 10, 5);
    }
    
    RollupPerformanceMetrics createTestMetrics(
        double throughput,
        double proof_time,
        double verify_time
    ) {
        RollupPerformanceMetrics metrics;
        metrics.avg_tx_latency = 0.001;
        metrics.tx_throughput = throughput;
        metrics.proof_generation_time = proof_time;
        metrics.verification_time = verify_time;
        metrics.quantum_energy_usage = 100.0;
        metrics.total_transactions = 1000000;
        metrics.pending_transactions = 100;
        metrics.active_validators = 5;
        metrics.success_rate = 0.99;
        return metrics;
    }
    
    QuantumParameters createTestParameters(
        double phase_angle,
        size_t num_qubits
    ) {
        std::vector<double> angles{phase_angle};
        return QuantumParameters(angles, num_qubits, 0.8, true);
    }
    
    std::unique_ptr<RollupMLModel> model_;
};

TEST_F(MLModelTestSuite, TestModelTraining) {
    std::vector<RollupPerformanceMetrics> metrics_history;
    std::vector<QuantumParameters> param_history;
    
    for (int i = 0; i < 10; i++) {
        metrics_history.push_back(createTestMetrics(1000 + i * 100, 2.0, 1.0));
        param_history.push_back(createTestParameters(0.1 * i, 10 + i));
    }
    
    model_->train(metrics_history, param_history);
    
    auto predicted = model_->predictOptimalParameters(metrics_history.back());
    EXPECT_GT(predicted.num_qubits, 0);
    EXPECT_FALSE(predicted.phase_angles.empty());
}

TEST_F(MLModelTestSuite, TestNaturalLanguageQueries) {
    auto result = model_->processNaturalLanguageQuery("How can we improve throughput?");
    
    EXPECT_GT(result.confidence, 0.5);
    EXPECT_FALSE(result.explanation.empty());
    EXPECT_FALSE(result.relevant_metrics.empty());
}

TEST_F(MLModelTestSuite, TestPerformanceAnalysis) {
    auto metrics = createTestMetrics(1000, 2.0, 1.0);
    auto bottlenecks = model_->analyzePerformanceBottlenecks(metrics);
    
    EXPECT_FALSE(bottlenecks.empty());
    
    auto optimizations = model_->suggestOptimizations(metrics);
    EXPECT_FALSE(optimizations.empty());
}

TEST_F(MLModelTestSuite, TestOptimizationImpact) {
    auto initial_metrics = createTestMetrics(1000, 2.0, 1.0);
    auto initial_params = createTestParameters(0.1, 10);
    
    std::vector<RollupPerformanceMetrics> metrics_history{initial_metrics};
    std::vector<QuantumParameters> param_history{initial_params};
    
    model_->train(metrics_history, param_history);
    
    auto optimized_params = model_->predictOptimalParameters(initial_metrics);
    EXPECT_NE(optimized_params.num_qubits, initial_params.num_qubits);
    EXPECT_NE(optimized_params.phase_angles[0], initial_params.phase_angles[0]);
}

TEST_F(MLModelTestSuite, TestAdaptiveOptimization) {
    // Create sequence of metrics showing system evolution
    std::vector<RollupPerformanceMetrics> metrics_sequence;
    
    // Initial state (low performance)
    metrics_sequence.push_back(createTestMetrics(500000.0, 0.002, 0.001));
    
    // Mid optimization (improving)
    metrics_sequence.push_back(createTestMetrics(1000000.0, 0.0015, 0.0008));
    
    // Final state (optimized)
    metrics_sequence.push_back(createTestMetrics(2000000.0, 0.001, 0.0005));
    
    std::vector<QuantumParameters> params_sequence;
    for (const auto& metrics : metrics_sequence) {
        auto params = model_->predictOptimalParameters(metrics);
        params_sequence.push_back(params);
        
        // Add to training data
        model_->train(
            std::vector<RollupPerformanceMetrics>{metrics},
            std::vector<QuantumParameters>{params}
        );
    }
    
    // Verify adaptive optimization
    for (size_t i = 1; i < params_sequence.size(); ++i) {
        // Parameters should adapt to improving performance
        EXPECT_NE(
            params_sequence[i].phase_angles[0],
            params_sequence[i-1].phase_angles[0]
        );
        
        // Quantum resource allocation should increase with performance
        EXPECT_GE(
            params_sequence[i].num_qubits,
            params_sequence[i-1].num_qubits
        );
    }
}

TEST_F(MLModelTestSuite, TestQueryComplexity) {
    // Test compound queries
    auto result = model_->processNaturalLanguageQuery(
        "What's our quantum efficiency and throughput like for proof generation?"
    );
    
    EXPECT_GT(result.confidence, 0.7);
    EXPECT_GE(result.relevant_metrics.size(), 3);  // Should catch multiple metrics
    
    // Verify all relevant aspects are captured
    std::set<std::string> expected_metrics = {
        "quantum",
        "throughput",
        "proof"
    };
    
    for (const auto& [metric, _] : result.relevant_metrics) {
        bool found_metric = false;
        for (const auto& expected : expected_metrics) {
            if (metric.find(expected) != std::string::npos) {
                found_metric = true;
                break;
            }
        }
        EXPECT_TRUE(found_metric);
    }
}

TEST_F(MLModelTestSuite, TestPerformanceTargets) {
    // Test if model suggests appropriate optimizations for different targets
    
    // Test high throughput target
    auto high_throughput_metrics = createTestMetrics(2500000.0, 0.001, 0.0005);
    auto high_throughput_suggestions = 
        model_->suggestOptimizations(high_throughput_metrics);
    
    bool has_scaling_suggestion = false;
    for (const auto& suggestion : high_throughput_suggestions) {
        if (suggestion.find("parallel") != std::string::npos ||
            suggestion.find("batch") != std::string::npos) {
            has_scaling_suggestion = true;
            break;
        }
    }
    EXPECT_TRUE(has_scaling_suggestion);
    
    // Test energy efficiency target
    auto energy_efficient_metrics = createTestMetrics(1000000.0, 0.002, 0.001);
    energy_efficient_metrics.quantum_energy_usage = 2000.0;  // High energy usage
    
    auto energy_suggestions = 
        model_->suggestOptimizations(energy_efficient_metrics);
    
    bool has_energy_suggestion = false;
    for (const auto& suggestion : energy_suggestions) {
        if (suggestion.find("energy") != std::string::npos ||
            suggestion.find("quantum") != std::string::npos) {
            has_energy_suggestion = true;
            break;
        }
    }
    EXPECT_TRUE(has_energy_suggestion);
}

TEST_F(MLModelTestSuite, TestPerformanceMetrics) {
    // Get and verify performance metrics
    auto metrics = createTestMetrics(100, 2.0, 1.0);
    EXPECT_GT(metrics.tx_throughput, 0);
    EXPECT_GT(metrics.avg_tx_latency, 0.0);
} 