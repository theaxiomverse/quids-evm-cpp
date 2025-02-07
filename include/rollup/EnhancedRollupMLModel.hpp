#pragma once

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <Eigen/Dense>
#include "rollup/RollupPerformanceMetrics.hpp"
#include "quantum/QuantumParameters.hpp"
#include "rollup/CrossChainState.hpp"

namespace quids {
namespace rollup {

// Forward declarations
class RollupPerformanceMetrics;

// Structures for ML model parameters and results
struct EnhancedMLParameters {
    size_t num_layers{3};
    size_t hidden_size{128};
    double learning_rate{0.001};
    double dropout_rate{0.2};
    size_t attention_heads{8};
    size_t max_sequence_length{1024};
    bool use_layer_norm{true};
    double gradient_clip_norm{5.0};
};

struct OptimizationResult {
    QuantumParameters parameters;
    double objective_score;
    std::vector<std::pair<std::string, double>> objective_breakdown;
    std::vector<std::string> tradeoff_explanations;
    std::chrono::system_clock::time_point timestamp;
};

struct EnhancedQueryResult {
    std::string explanation;
    double confidence;
    std::vector<std::pair<std::string, double>> relevant_metrics;
    std::vector<std::pair<std::string, std::string>> causal_relationships;
    std::vector<std::string> suggested_actions;
    std::chrono::system_clock::time_point timestamp;
};

// Attention layer structure
struct AttentionLayer {
    Eigen::MatrixXd query_weights;
    Eigen::MatrixXd key_weights;
    Eigen::MatrixXd value_weights;
    Eigen::VectorXd attention_bias;
    Eigen::MatrixXd output_weights;
    Eigen::VectorXd output_bias;
    double attention_dropout{0.1};
};

class EnhancedRollupMLModel {
public:
    // Constructor and destructor
    explicit EnhancedRollupMLModel(const EnhancedMLParameters& params);
    ~EnhancedRollupMLModel() = default;

    // Disable copy
    EnhancedRollupMLModel(const EnhancedRollupMLModel&) = delete;
    EnhancedRollupMLModel& operator=(const EnhancedRollupMLModel&) = delete;

    // Enable move
    EnhancedRollupMLModel(EnhancedRollupMLModel&&) noexcept = default;
    EnhancedRollupMLModel& operator=(EnhancedRollupMLModel&&) noexcept = default;
    
    // Training methods
    void train(
        const std::vector<RollupPerformanceMetrics>& metrics_history,
        const std::vector<QuantumParameters>& param_history
    );
    
    void train_with_attention(
        const std::vector<RollupPerformanceMetrics>& metrics_history,
        const std::vector<QuantumParameters>& param_history,
        const std::vector<CrossChainState>& chain_history
    );
    
    // Prediction and optimization
    [[nodiscard]] QuantumParameters predict_optimal_parameters(
        const RollupPerformanceMetrics& current_metrics
    ) const;
    
    // Natural language interface
    [[nodiscard]] EnhancedQueryResult process_natural_language_query(
        const std::string& query
    ) const;
    
    // Performance analysis
    [[nodiscard]] std::vector<std::string> analyze_performance_bottlenecks(
        const RollupPerformanceMetrics& metrics
    ) const;
    
    [[nodiscard]] std::vector<std::string> suggest_optimizations(
        const RollupPerformanceMetrics& metrics
    ) const;
    
    // Cross-chain optimization
    [[nodiscard]] CrossChainState optimize_chain_distribution(
        const std::vector<RollupPerformanceMetrics>& chain_metrics,
        const std::vector<QuantumParameters>& chain_params
    ) const;
    
    [[nodiscard]] std::vector<QuantumParameters> optimize_chain_parameters(
        const CrossChainState& current_state,
        const std::vector<RollupPerformanceMetrics>& chain_metrics
    ) const;
    
    // Complex query processing
    [[nodiscard]] EnhancedQueryResult process_complex_query(
        const std::string& query,
        const RollupPerformanceMetrics& current_metrics,
        const CrossChainState& chain_state
    ) const;
    
    // Parameter optimization
    [[nodiscard]] OptimizationResult optimize_parameters(
        const RollupPerformanceMetrics& current_metrics,
        const CrossChainState& chain_state,
        const std::vector<std::pair<std::string, double>>& objective_weights
    ) const;
    
    // Single-argument version for RollupTransactionAPI
    void optimize_parameters(const RollupPerformanceMetrics& metrics);
    
    // Core ML operations
    [[nodiscard]] Eigen::VectorXd forward_with_attention(
        const Eigen::VectorXd& features
    ) const;
    
    void backward_with_attention(
        const Eigen::VectorXd& features,
        const Eigen::VectorXd& prediction,
        const Eigen::MatrixXd& attention_weights
    );
    
    void update_parameters(
        const Eigen::VectorXd& prediction,
        const Eigen::VectorXd& features,
        const std::vector<std::pair<std::string, double>>& objectives
    );
    
    // Optimization methods
    void optimize_quantum_circuit(const CrossChainState& chain_state);
    void optimize_quantum_parameters(const Eigen::MatrixXd& interactions);
    
    // Gradient accumulation
    void accumulate_gradients(
        const std::vector<Eigen::VectorXd>& gradients,
        const std::vector<Eigen::MatrixXd>& attention_gradients
    );

    // Feature interaction analysis
    [[nodiscard]] Eigen::MatrixXd analyze_feature_interactions() const;

    // Adaptive batch size optimization
    [[nodiscard]] size_t optimize_batch_size(
        double current_throughput,
        double target_throughput,
        double current_latency,
        double max_latency
    ) const;

    // Dynamic resource allocation
    [[nodiscard]] std::vector<double> allocate_resources(
        const std::vector<double>& demands,
        double total_capacity
    ) const;

    // Quantum parameter tuning
    [[nodiscard]] std::vector<QuantumParameters> tune_quantum_parameters(
        const std::vector<QuantumParameters>& chain_params
    ) const;

    // Performance prediction
    [[nodiscard]] double predict_performance(
        const RollupPerformanceMetrics& current_metrics,
        const std::vector<double>& proposed_changes
    ) const;

    // Cross-chain optimization
    void optimize_cross_chain_performance(
        const std::vector<RollupPerformanceMetrics>& chain_metrics
    );

    // Validation and prediction
    [[nodiscard]] bool validate_quantum_parameters(
        const QuantumParameters& params
    ) const;
    
    [[nodiscard]] CrossChainState predict_chain_state(
        const RollupPerformanceMetrics& metrics
    ) const;
    
    [[nodiscard]] QuantumParameters optimize_quantum_parameters(
        const Eigen::VectorXd& prediction
    ) const;

private:
    EnhancedMLParameters params_;
    std::vector<AttentionLayer> attention_layers_;
    std::vector<Eigen::MatrixXd> weights_;
    std::vector<Eigen::VectorXd> biases_;
    
    // Internal methods
    void initialize_transformer();
    void initialize_optimizer();
    
    [[nodiscard]] Eigen::VectorXd engineer_advanced_features(
        const RollupPerformanceMetrics& metrics,
        const CrossChainState& chain_state
    ) const;
    
    [[nodiscard]] double calculate_multi_objective_loss(
        const Eigen::VectorXd& prediction,
        const Eigen::VectorXd& features,
        const std::vector<std::pair<std::string, double>>& objectives
    ) const;
    
    void update_model_with_adam(
        const std::vector<Eigen::VectorXd>& gradients,
        const std::vector<Eigen::MatrixXd>& attention_gradients
    );
    
    [[nodiscard]] Eigen::VectorXd forward_pass(
        const Eigen::VectorXd& features
    ) const;
    
    // Constants
    static constexpr size_t MAX_SEQUENCE_LENGTH = 1024;
    static constexpr size_t MIN_BATCH_SIZE = 1;
    static constexpr size_t MAX_BATCH_SIZE = 1024;
    static constexpr double LEARNING_RATE_MIN = 1e-6;
    static constexpr double LEARNING_RATE_MAX = 1e-2;
    static constexpr double GRADIENT_CLIP_THRESHOLD = 5.0;
};

} // namespace rollup
} // namespace quids 