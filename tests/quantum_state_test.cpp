#include <gtest/gtest.h>
#include "quantum/QuantumState.hpp"

using namespace quids::quantum;

class QuantumStateTest : public ::testing::Test {
protected:
    Eigen::VectorXcd create_test_vector() {
        Eigen::VectorXcd vec(4);
        vec << std::complex<double>(1,0),
               std::complex<double>(0,1),
               std::complex<double>(-1,0),
               std::complex<double>(0,-1);
        return vec / 2.0;  // Normalized
    }
};

TEST_F(QuantumStateTest, Normalization) {
    auto vec = create_test_vector();
    QuantumState state(vec);
    EXPECT_NEAR(state.normalized_vector().norm(), 1.0, 1e-10);
}

TEST_F(QuantumStateTest, EntanglementGeneration) {
    auto vec = create_test_vector();
    QuantumState state(vec);
    auto entanglement = state.generate_entanglement();
    EXPECT_TRUE(entanglement.rows() == 4);
    EXPECT_TRUE(entanglement.cols() == 4);
} 