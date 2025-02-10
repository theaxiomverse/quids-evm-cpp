#pragma once

#include <Eigen/Dense>
#include <complex>
#include <vector>
#include <cmath>
#include "quantum/QuantumTypes.hpp"
#include "quantum/QuantumState.hpp"

namespace quids {
namespace quantum {
namespace detail {

// Common gate matrices
namespace gates {
    using Complex = ::std::complex<double>;
    using Matrix2cd = Eigen::Matrix2cd;

    const Matrix2cd H = (Matrix2cd() << 
        1/::std::sqrt(2), 1/::std::sqrt(2),
        1/::std::sqrt(2), -1/::std::sqrt(2)
    ).finished();

    const Matrix2cd X = (Matrix2cd() <<
        0, 1,
        1, 0
    ).finished();

    const Matrix2cd Y = (Matrix2cd() <<
        0, Complex(0, -1),
        Complex(0, 1), 0
    ).finished();

    const Matrix2cd Z = (Matrix2cd() <<
        1, 0,
        0, -1
    ).finished();

    const Matrix2cd S = (Matrix2cd() <<
        1, 0,
        0, Complex(0, 1)
    ).finished();

    const Matrix2cd T = (Matrix2cd() <<
        1, 0,
        0, ::std::exp(Complex(0, M_PI/4))
    ).finished();

    // Helper functions to create parameterized gates
    inline Matrix2cd Rx(double theta) {
        const Complex i(0, 1);
        const Complex c = ::std::cos(theta/2);
        const Complex s = ::std::sin(theta/2);
        return (Matrix2cd() <<
            c, -i*s,
            -i*s, c
        ).finished();
    }

    inline Matrix2cd Ry(double theta) {
        const Complex c = ::std::cos(theta/2);
        const Complex s = ::std::sin(theta/2);
        return (Matrix2cd() <<
            c, -s,
            s, c
        ).finished();
    }

    inline Matrix2cd Rz(double theta) {
        const Complex i(0, 1);
        const Complex phase = ::std::exp(-i * theta/2.0);
        return (Matrix2cd() <<
            phase, 0,
            0, ::std::conj(phase)
        ).finished();
    }

    inline Matrix2cd Phase(double phi) {
        return (Matrix2cd() <<
            1, 0,
            0, ::std::exp(Complex(0, phi))
        ).finished();
    }
}

// Standard quantum gates
Eigen::Matrix2cd hadamard();
Eigen::Matrix2cd phase(double angle);
Eigen::Matrix4cd cnot();

// Error detection and correction
bool detectErrors(const QuantumState& state);
double calculateFidelity(const QuantumState& state1, const QuantumState& state2);

} // namespace quantum
} // namespace quids 