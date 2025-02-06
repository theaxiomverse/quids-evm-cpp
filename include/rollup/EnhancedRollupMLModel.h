#pragma once

#include <vector>
#include <string>
#include <memory>
#include <Eigen/Dense>
#include "rollup/RollupPerformanceMetrics.h"
#include "rollup/QuantumParameters.h"
#include "rollup/CrossChainState.h"

// Forward declarations
class RollupPerformanceMetrics;

// Structures for ML model parameters and results
struct EnhancedMLParameters {
    size_t num_layers{3};
    size_t hidden_size{128};
    double learning_rate{0.001};
    double dropout_rate{0.2};
};

struct OptimizationResult {
    QuantumParameters parameters;
    double objective_score;
    std::vector<std::pair<std::string, double>> objective_breakdown;
    std::vector<std::string> tradeoff_explanations;
};

struct EnhancedQueryResult {
    std::string explanation;
    double confidence;
    std::vector<std::pair<std::string, double>> relevant_metrics;
    std::vector<std::pair<std::string, std::string>> causal_relationships;
    std::vector<std::string> suggested_actions;
};

// Attention layer structure
struct AttentionLayer {
    Eigen::MatrixXd query_weights;
    Eigen::MatrixXd key_weights;
    Eigen::MatrixXd value_weights;
    Eigen::VectorXd attention_bias;
};

class EnhancedRollupMLModel {
public:
    explicit EnhancedRollupMLModel(const EnhancedMLParameters& params);
    
    // Training methods
    void train(
        const std::vector<RollupPerformanceMetrics>& metrics_history,
        const std::vector<QuantumParameters>& param_history
    );
    
    // Prediction and optimization
    QuantumParameters predictOptimalParameters(
        const RollupPerformanceMetrics& current_metrics
    );
    
    // Natural language interface
    EnhancedQueryResult processNaturalLanguageQuery(
        const std::string& query
    );
    
    // Performance analysis
    std::vector<std::string> analyzePerformanceBottlenecks(
        const RollupPerformanceMetrics& metrics
    );
    
    std::vector<std::string> suggestOptimizations(
        const RollupPerformanceMetrics& metrics
    );
    
    // Cross-chain optimization
    CrossChainState optimizeChainDistribution(
        const std::vector<RollupPerformanceMetrics>& chain_metrics,
        const std::vector<QuantumParameters>& chain_params
    );
    
    std::vector<QuantumParameters> optimizeChainParameters(
        const CrossChainState& current_state,
        const std::vector<RollupPerformanceMetrics>& chain_metrics
    );
    
    // Complex query processing
    EnhancedQueryResult processComplexQuery(
        const std::string& query,
        const RollupPerformanceMetrics& current_metrics,
        const CrossChainState& chain_state
    );
    
    // Parameter optimization
    OptimizationResult optimizeParameters(
        const RollupPerformanceMetrics& current_metrics,
        const CrossChainState& chain_state,
        const std::vector<std::pair<std::string, double>>& objective_weights
    );
    
    // Single-argument version for RollupTransactionAPI
    void optimizeParameters(const RollupPerformanceMetrics& metrics);
    
    // Core ML operations
    Eigen::VectorXd forwardWithAttention(const Eigen::VectorXd& features);
    
    void backwardWithAttention(
        const Eigen::VectorXd& features,
        const Eigen::VectorXd& prediction,
        const Eigen::MatrixXd& attention_weights
    );
    
    void updateParameters(
        const Eigen::VectorXd& prediction,
        const Eigen::VectorXd& features,
        const std::vector<std::pair<std::string, double>>& objectives
    );
    
    // Optimization methods
    void optimizeQuantumCircuit(const CrossChainState& chain_state);
    void optimizeQuantumParameters(const Eigen::MatrixXd& interactions);
    
    // Training methods
    void trainWithAttention(
        const std::vector<RollupPerformanceMetrics>& metrics_history,
        const std::vector<QuantumParameters>& param_history,
        const std::vector<CrossChainState>& chain_history
    );
    
    // Gradient accumulation
    void accumulateGradients(
        const std::vector<Eigen::VectorXd>& gradients,
        const std::vector<Eigen::MatrixXd>& attention_gradients
    );

    // Feature interaction analysis
    Eigen::MatrixXd analyzeFeatureInteractions();

    // Adaptive batch size optimization
    size_t optimizeBatchSize(
        double current_throughput,
        double target_throughput,
        double current_latency,
        double max_latency
    );

    // Dynamic resource allocation
    std::vector<double> allocateResources(
        const std::vector<double>& demands,
        double total_capacity
    );

    // Quantum parameter tuning
    std::vector<QuantumParameters> tuneQuantumParameters(
        const std::vector<QuantumParameters>& chain_params
    );

    // Performance prediction
    double predictPerformance(
        const RollupPerformanceMetrics& current_metrics,
        const std::vector<double>& proposed_changes
    );

    // Cross-chain optimization
    void optimizeCrossChainPerformance(
        const std::vector<RollupPerformanceMetrics>& chain_metrics
    );

    // Add missing declarations
    bool validateQuantumParameters(const QuantumParameters& params) const;
    CrossChainState predictChainState(const RollupPerformanceMetrics& metrics) const;
    QuantumParameters optimizeQuantumParameters(const Eigen::VectorXd& prediction);

private:
    EnhancedMLParameters params_;
    std::vector<AttentionLayer> attention_layers_;
    std::vector<Eigen::MatrixXd> weights_;
    std::vector<Eigen::VectorXd> biases_;
    
    // Internal methods
    void initializeTransformer();
    void initializeOptimizer();
    
    Eigen::VectorXd engineerAdvancedFeatures(
        const RollupPerformanceMetrics& metrics,
        const CrossChainState& chain_state
    );
    
    double calculateMultiObjectiveLoss(
        const Eigen::VectorXd& prediction,
        const Eigen::VectorXd& features,
        const std::vector<std::pair<std::string, double>>& objectives
    );
    
    void updateModelWithAdam(
        const std::vector<Eigen::VectorXd>& gradients,
        const std::vector<Eigen::MatrixXd>& attention_gradients
    );
    
    Eigen::VectorXd forwardPass(const Eigen::VectorXd& features);
}; 