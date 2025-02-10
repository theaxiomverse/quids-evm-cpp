#pragma once

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <Eigen/Dense>
#include "rollup/RollupPerformanceMetrics.hpp"
#include "quantum/QuantumParameters.hpp"
#include "rollup/CrossChainState.hpp"
#include "rollup/RollupTypes.hpp"
#include "rollup/RollupMLModel.hpp"
#include "quantum/QuantumState.hpp"

namespace quids {
namespace rollup {

// Forward declarations
class RollupPerformanceMetrics;

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
    struct ModelParameters {
        size_t hidden_size;
        size_t num_layers;
        double learning_rate;
        double beta1;
        double beta2;
        double epsilon;
    };

    explicit EnhancedRollupMLModel(const ModelParameters& params);
    ~EnhancedRollupMLModel();

    void train(
        const std::vector<RollupPerformanceMetrics>& metrics_history,
        const std::vector<QuantumParameters>& param_history
    );

    [[nodiscard]] QuantumParameters predict_optimal_parameters(
        const RollupPerformanceMetrics& current_metrics
    ) const;

    [[nodiscard]] EnhancedQueryResult process_natural_language_query(
        const std::string& query
    ) const;

    [[nodiscard]] CrossChainState optimize_chain_distribution(
        const std::vector<RollupPerformanceMetrics>& chain_metrics,
        const std::vector<QuantumParameters>& chain_params
    ) const;

    [[nodiscard]] std::vector<QuantumParameters> optimize_chain_parameters(
        const CrossChainState& current_state,
        const std::vector<RollupPerformanceMetrics>& chain_metrics
    ) const;

    [[nodiscard]] ComplexQueryResult process_complex_query(
        const std::string& query,
        const RollupPerformanceMetrics& current_metrics,
        const CrossChainState& chain_state
    ) const;

    [[nodiscard]] OptimizationResult optimize_parameters(
        const RollupPerformanceMetrics& current_metrics,
        const std::vector<QuantumParameters>& chain_params
    ) const;

    // Model management
    void update_model(const std::vector<double>& new_data);
    void save_model(const std::string& path) const;
    bool load_model(const std::string& path);

    // Configuration
    void set_learning_rate(double rate);
    void set_batch_size(size_t size);
    void set_optimization_weights(const std::vector<double>& weights);

    // Training methods
    void train_with_attention(
        const std::vector<RollupPerformanceMetrics>& metrics_history,
        const std::vector<QuantumParameters>& param_history,
        const std::vector<CrossChainState>& chain_history
    );
    
    // Prediction and optimization
    [[nodiscard]] double predict_performance(
        const RollupPerformanceMetrics& current_metrics,
        const std::vector<double>& proposed_changes
    ) const;
    
    // Performance analysis
    [[nodiscard]] std::vector<std::string> analyze_performance_bottlenecks(
        const RollupPerformanceMetrics& metrics
    ) const;
    
    [[nodiscard]] std::vector<std::string> suggest_optimizations(
        const RollupPerformanceMetrics& metrics
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

    [[nodiscard]] Eigen::VectorXd forward_pass(const Eigen::VectorXd& features) const;

    void optimize_chain_parameters(const std::vector<QuantumParameters>& chain_params);

    void initialize_model();
    OptimizationResult train_model();

private:
    struct Impl; // Forward declaration
    std::unique_ptr<Impl> impl_;

    void initialize_transformer();
    void initialize_optimizer();
    
    [[nodiscard]] Eigen::VectorXd engineer_advanced_features(const RollupPerformanceMetrics& metrics) const;
    void update_model_with_adam(const std::vector<Eigen::VectorXd>& gradients);
    [[nodiscard]] double calculate_multi_objective_loss(
        const Eigen::VectorXd& prediction,
        const Eigen::VectorXd& target,
        const std::vector<double>& weights
    ) const;
};

} // namespace rollup
} // namespace quids 