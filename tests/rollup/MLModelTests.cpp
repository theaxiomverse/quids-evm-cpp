#include <gtest/gtest.h>
#include "rollup/RollupMLModel.hpp"
#include "quantum/QuantumParameters.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

namespace quids {
namespace rollup {
namespace test {

class MLModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        quids::rollup::MLModelParameters params;
        params.learning_rate = 0.001;
        params.batch_size = 32;
        params.hidden_size = 128;
        params.num_layers = 3;
        params.dropout_rate = 0.2;
        model_ = std::make_unique<quids::rollup::RollupMLModel>(params, 10, 5);
    }
    
    quids::rollup::RollupPerformanceMetrics createTestMetrics(
        double tx_throughput = 1000,
        double verification_time = 1.2,
        double quantum_energy_usage = 75.0
    ) {
        quids::rollup::RollupPerformanceMetrics metrics;
        metrics.tx_throughput = tx_throughput;
        metrics.verification_time = verification_time;
        metrics.quantum_energy_usage = quantum_energy_usage;
        return metrics;
    }
    
    quids::rollup::QuantumParameters createTestParameters(
        size_t num_qubits = 100,
        double entanglement_degree = 0.8
    ) {
        std::vector<double> angles = {0.1, 0.2, 0.3};
        return quids::rollup::QuantumParameters(
            angles,
            num_qubits,
            entanglement_degree,
            true
        );
    }
    
    std::unique_ptr<quids::rollup::RollupMLModel> model_;
};

TEST_F(MLModelTest, TestTraining) {
    std::vector<quids::rollup::RollupPerformanceMetrics> metrics_history;
    std::vector<quids::rollup::QuantumParameters> param_history;
    
    for (int i = 0; i < 10; i++) {
        metrics_history.push_back(createTestMetrics(1000 + i * 100));
        param_history.push_back(createTestParameters(100 + i * 10, 0.8 + i * 0.02));
    }
    
    EXPECT_NO_THROW(model_->train(metrics_history, param_history));
}

TEST_F(MLModelTest, TestPrediction) {
    auto initial_metrics = createTestMetrics();
    auto initial_params = createTestParameters();
    
    std::vector<quids::rollup::RollupPerformanceMetrics> metrics_history{initial_metrics};
    std::vector<quids::rollup::QuantumParameters> param_history{initial_params};
    
    model_->train(metrics_history, param_history);
    
    auto predicted_params = model_->predictOptimalParameters(initial_metrics);
    EXPECT_GT(predicted_params.num_qubits, 0);
    EXPECT_GT(predicted_params.qubits_per_transaction, 0);
}

TEST_F(MLModelTest, TestPerformanceAnalysis) {
    auto metrics = createTestMetrics();
    auto bottlenecks = model_->analyzePerformanceBottlenecks(metrics);
    EXPECT_FALSE(bottlenecks.empty());
    
    auto optimizations = model_->suggestOptimizations(metrics);
    EXPECT_FALSE(optimizations.empty());
}

TEST_F(MLModelTest, TestNaturalLanguageQueries) {
    auto result = model_->processNaturalLanguageQuery("How can we improve throughput?");
    EXPECT_GT(result.confidence, 0.5);
    EXPECT_FALSE(result.explanation.empty());
    EXPECT_FALSE(result.suggestions.empty());
    
    // Verify suggestions contain expected metrics
    std::vector<std::string> expected_metrics = {
        "throughput",
        "latency",
        "energy"
    };
    
    for (const auto& suggestion : result.suggestions) {
        bool found = false;
        for (const auto& expected : expected_metrics) {
            if (suggestion.find(expected) != std::string::npos) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Suggestion '" << suggestion << "' does not contain any expected metrics";
    }
}

TEST_F(MLModelTest, TestPerformanceTargets) {
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

TEST_F(MLModelTest, TestPerformanceMetrics) {
    // Get and verify performance metrics
    auto metrics = createTestMetrics(100, 2.0, 1.0);
    EXPECT_GT(metrics.tx_throughput, 0);
    EXPECT_GT(metrics.verification_time, 0.0);
}

} // namespace test
} // namespace rollup
} // namespace quids 