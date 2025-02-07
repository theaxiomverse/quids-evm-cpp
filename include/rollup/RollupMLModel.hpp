#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <Eigen/Dense>
#include "rollup/RollupTransactionAPI.hpp"
#include "quantum/QuantumParameters.hpp"
#include "rollup/RollupPerformanceMetrics.hpp"

namespace quids {
namespace rollup {

// ML model parameters
struct MLModelParameters {
    size_t num_layers{3};
    size_t hidden_size{128};
    double learning_rate{0.001};
    size_t batch_size{32};
    double dropout_rate{0.2};
};

// Natural language query result
struct QueryResult {
    std::string explanation;
    double confidence;
    std::vector<std::pair<std::string, double>> relevant_metrics;
    std::vector<std::string> suggestions;
};

class RollupMLModel {
public:
    RollupMLModel(
        const MLModelParameters& params,
        size_t input_size,
        size_t output_size
    );
    
    // Training and prediction
    void train(
        const std::vector<RollupPerformanceMetrics>& metrics_history,
        const std::vector<QuantumParameters>& param_history
    );
    
    QuantumParameters predictOptimalParameters(
        const RollupPerformanceMetrics& current_metrics
    );
    
    // Natural language interface
    QueryResult processNaturalLanguageQuery(const std::string& query);
    
    // Performance analysis
    std::vector<std::string> analyzePerformanceBottlenecks(
        const RollupPerformanceMetrics& metrics
    );
    
    std::vector<std::string> suggestOptimizations(
        const RollupPerformanceMetrics& metrics
    );
    
    // Add this method
    void updateMetrics(const RollupPerformanceMetrics& metrics);
    
protected:
    void initializeWeights();
    Eigen::VectorXd forwardPass(const Eigen::VectorXd& input);
    Eigen::VectorXd extractFeatures(const RollupPerformanceMetrics& metrics);
    Eigen::VectorXd extractTargets(const QuantumParameters& params);
    double calculateCosineSimilarity(
        const Eigen::VectorXd& query,
        const Eigen::VectorXd& metric_embedding
    );
    
private:
    // Model architecture
    MLModelParameters params_;
    size_t input_size_;
    size_t output_size_;
    std::vector<Eigen::MatrixXd> weights_;
    std::vector<Eigen::VectorXd> biases_;
    
    // Add current metrics member
    RollupPerformanceMetrics current_metrics_;
    
    // Natural language processing
    void initializeNLPModel();
    std::vector<double> embedQuery(const std::string& query);
    double calculateQuerySimilarity(
        const std::vector<double>& query_embedding,
        const std::vector<double>& metric_embedding
    );
    
    // Model operations
    Eigen::VectorXd forward(const Eigen::VectorXd& input);
    void backward(
        const Eigen::VectorXd& input,
        const Eigen::VectorXd& target,
        double learning_rate
    );
    
    // Optimization strategies
    void updateModelParameters(
        const std::vector<Eigen::VectorXd>& gradients,
        double learning_rate
    );
    
    double calculateLoss(
        const Eigen::VectorXd& prediction,
        const Eigen::VectorXd& target
    );
};

} // namespace rollup
} // namespace quids 