#pragma once

#include <complex>
#include <vector>

// Forward declarations
template<typename T>
class StateVector;

class IQuantumObject {
public:
    virtual ~IQuantumObject() = default;

    // Get the wavefunction as a vector of complex amplitudes
    [[nodiscard]] virtual std::vector<std::complex<double>> get_wavefunction() const = 0;

    // Calculate probability density at a given position
    [[nodiscard]] virtual double get_probability_density(const std::vector<double>& position) const = 0;

    // Get the quantum state descriptor (n, l, m for hydrogen, etc.)
    [[nodiscard]] virtual std::string get_state_descriptor() const = 0;
};

// Template for state vectors in Dirac notation
template<typename T>
class StateVector {
public:
    std::vector<std::complex<T>> components;

    StateVector() = default;

    explicit StateVector(size_t dimension) : components(dimension, {0.0, 0.0}) {}

    // Normalize the state vector so that <psi|psi> = 1
    void normalize() {
        T norm_squared = 0.0;
        for (const auto& c : components) {
            norm_squared += std::norm(c);
        }
        if (norm_squared > 0.0) {
            T norm = std::sqrt(norm_squared);
            for (auto& c : components) {
                c /= norm;
            }
        }
    }

    // Calculate the norm squared <psi|psi>
    [[nodiscard]] T norm_squared() const {
        T sum = 0.0;
        for (const auto& c : components) {
            sum += std::norm(c);
        }
        return sum;
    }

    // Calculate expectation value <psi|A|psi> for a Hermitian operator
    template<typename Operator>
    [[nodiscard]] T expectation_value(const Operator& op) const {
        // Simplified expectation value calculation
        // In practice, this would apply the operator and take inner product
        T result = 0.0;
        for (size_t i = 0; i < components.size(); ++i) {
            result += std::norm(components[i]) * op(i, i);
        }
        return result;
    }
};

// Dirac notation helper functions
namespace dirac {
    // Bra-ket inner product: <bra|ket>
    template<typename T>
    std::complex<T> inner_product(const StateVector<T>& bra, const StateVector<T>& ket) {
        std::complex<T> result = {0.0, 0.0};
        for (size_t i = 0; i < bra.components.size() && i < ket.components.size(); ++i) {
            result += std::conj(bra.components[i]) * ket.components[i];
        }
        return result;
    }

    // Outer product: |ket><bra|
    template<typename T>
    std::vector<std::vector<std::complex<T>>> outer_product(
        const StateVector<T>& ket,
        const StateVector<T>& bra
    ) {
        size_t n = ket.components.size();
        size_t m = bra.components.size();
        std::vector<std::vector<std::complex<T>>> result(n, std::vector<std::complex<T>>(m));

        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < m; ++j) {
                result[i][j] = ket.components[i] * std::conj(bra.components[j]);
            }
        }
        return result;
    }
} // namespace dirac