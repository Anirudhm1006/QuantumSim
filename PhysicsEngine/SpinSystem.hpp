#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "IQuantumObject.hpp"

// =============================================================================
// Spin System for Superposition and Entanglement
// =============================================================================
// Implements:
// - Pauli matrices (σx, σy, σz)
// - Bloch sphere representation: |ψ⟩ = cos(θ/2)|0⟩ + e^(iφ)sin(θ/2)|1⟩
// - Spin eigenstates |+⟩_z and |-⟩_z
// =============================================================================

// 2x2 Matrix of complex numbers for quantum operators
struct ComplexMatrix2x2 {
    std::array<std::complex<double>, 2> data[2];

    std::complex<double>* operator[](size_t i) { return data[i].data(); }
    const std::complex<double>* operator[](size_t i) const { return data[i].data(); }
};

// Pauli matrix generators
namespace pauli_matrices {
    // σx = [[0, 1], [1, 0]]
    [[nodiscard]] ComplexMatrix2x2 sigma_x();

    // σy = [[0, -i], [i, 0]]
    [[nodiscard]] ComplexMatrix2x2 sigma_y();

    // σz = [[1, 0], [0, -1]]
    [[nodiscard]] ComplexMatrix2x2 sigma_z();

    // Identity matrix
    [[nodiscard]] ComplexMatrix2x2 identity();

    // Apply Pauli matrix to a state vector
    [[nodiscard]] std::vector<std::complex<double>> apply(
        const ComplexMatrix2x2& matrix,
        const std::vector<std::complex<double>>& state
    );
}

class SpinSystem : public IQuantumObject {
public:
    // Constructors
    SpinSystem();
    explicit SpinSystem(double theta, double phi);
    SpinSystem(double theta, double phi, bool initial_spin_up);

    // Destructor
    ~SpinSystem() override = default;

    // Get the spin state as a 2-component vector
    [[nodiscard]] std::vector<std::complex<double>> get_wavefunction() const override;

    // Calculate probability density (for compatibility with interface)
    [[nodiscard]] double get_probability_density(const std::vector<double>& position) const override;

    [[nodiscard]] std::string get_state_descriptor() const override;
    [[nodiscard]] nlohmann::json to_json() const override;
    [[nodiscard]] std::string get_type_name() const override { return "SpinSystem"; }

    // Set spin state using Bloch sphere angles
    void set_bloch_angles(double theta, double phi);

    // Get Bloch sphere angles
    [[nodiscard]] double get_theta() const { return theta_; }
    [[nodiscard]] double get_phi() const { return phi_; }

    // Get spin projection along z-axis (expectation value of σz)
    [[nodiscard]] double get_spin_projection_z() const;

    // Get spin projection along x-axis
    [[nodiscard]] double get_spin_projection_x() const;

    // Get spin projection along y-axis
    [[nodiscard]] double get_spin_projection_y() const;

    // Get the Bloch vector (nx, ny, nz)
    [[nodiscard]] std::vector<double> get_bloch_vector() const;

    // Apply Pauli matrix
    [[nodiscard]] std::vector<std::complex<double>> apply_pauli(const std::string& which) const;

    // Spin eigenstates
    static std::vector<std::complex<double>> spin_up();
    static std::vector<std::complex<double>> spin_down();
    static std::vector<std::complex<double>> spin_plus_x();
    static std::vector<std::complex<double>> spin_minus_x();

    // Calculate expectation value of an operator
    [[nodiscard]] double expectation_value(const ComplexMatrix2x2& op) const;

    // Create superposition of spin states
    static SpinSystem create_superposition(
        const std::vector<std::complex<double>>& coeff_up,
        const std::vector<std::complex<double>>& coeff_down
    );

private:
    double theta_;  // Polar angle [0, π]
    double phi_;    // Azimuthal angle [0, 2π]

    // State components in the |0⟩, |1⟩ basis
    std::complex<double> alpha_;  // Coefficient of |0⟩
    std::complex<double> beta_;   // Coefficient of |1⟩

    // Update internal state from Bloch angles
    void update_state_from_angles();
};

namespace spin_operators {
    // Spin operators in units of ħ/2
    // Sx = (ħ/2)σx, etc.

    [[nodiscard]] ComplexMatrix2x2 spin_x();
    [[nodiscard]] ComplexMatrix2x2 spin_y();
    [[nodiscard]] ComplexMatrix2x2 spin_z();

    // Total spin operator squared
    [[nodiscard]] ComplexMatrix2x2 spin_squared();
}