#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "IQuantumObject.hpp"

// =============================================================================
// Entanglement Module - Quantum Entanglement for Multi-Qubit Systems
// =============================================================================
// Implements:
// - Bell states: maximally entangled two-qubit states
// - Density matrix representation for mixed states
// - Concurrence: measure of entanglement (0 = separable, 1 = maximally entangled)
// - Schmidt decomposition: bipartite entanglement characterization
// =============================================================================

// Bell state enumeration
enum class BellState {
    PHI_PLUS,   // |Φ⁺⟩ = (|00⟩ + |11⟩)/√2
    PHI_MINUS,  // |Φ⁻⟩ = (|00⟩ - |11⟩)/√2
    PSI_PLUS,   // |Ψ⁺⟩ = (|01⟩ + |10⟩)/√2
    PSI_MINUS   // |Ψ⁻⟩ = (|01⟩ - |10⟩)/√2
};

// Two-qubit state representation
struct TwoQubitState {
    std::complex<double> amp_00;  // |00⟩ amplitude
    std::complex<double> amp_01;  // |01⟩ amplitude
    std::complex<double> amp_10;  // |10⟩ amplitude
    std::complex<double> amp_11;  // |11⟩ amplitude

    TwoQubitState() : amp_00({1.0, 0.0}), amp_01({0.0, 0.0}),
                      amp_10({0.0, 0.0}), amp_11({0.0, 0.0}) {}

    // Normalize the state
    void normalize() {
        double norm_sq = std::norm(amp_00) + std::norm(amp_01) +
                        std::norm(amp_10) + std::norm(amp_11);
        if (norm_sq > 0.0) {
            double norm = std::sqrt(norm_sq);
            amp_00 /= norm;
            amp_01 /= norm;
            amp_10 /= norm;
            amp_11 /= norm;
        }
    }

    // Get probability for each basis state
    [[nodiscard]] std::array<double, 4> probabilities() const {
        return {{
            std::norm(amp_00),
            std::norm(amp_01),
            std::norm(amp_10),
            std::norm(amp_11)
        }};
    }
};

// Density matrix for 2-qubit system (4x4 complex matrix)
using DensityMatrix = std::vector<std::vector<std::complex<double>>>;

// Schmidt decomposition result
struct SchmidtDecomposition {
    std::vector<double> singular_values;  // sqrt(eigenvalues of ρ_A)
    std::vector<std::vector<std::complex<double>>> basis_A;  // Orthonormal basis for subsystem A
    std::vector<std::vector<std::complex<double>>> basis_B;  // Orthonormal basis for subsystem B
    int rank;  // Schmidt rank (number of non-zero singular values)
};

class EntanglementSystem : public IQuantumObject {
public:
    // Constructors
    EntanglementSystem();
    explicit EntanglementSystem(const TwoQubitState& state);
    explicit EntanglementSystem(BellState bell_state);

    // Destructor
    ~EntanglementSystem() override = default;

    // IQuantumObject interface
    [[nodiscard]] std::vector<std::complex<double>> get_wavefunction() const override;
    [[nodiscard]] double get_probability_density(const std::vector<double>& position) const override;
    [[nodiscard]] std::string get_state_descriptor() const override;
    [[nodiscard]] nlohmann::json to_json() const override;
    [[nodiscard]] std::string get_type_name() const override { return "EntanglementSystem"; }

    // State manipulation
    void set_state(const TwoQubitState& state);
    void set_bell_state(BellState bell_state);
    [[nodiscard]] const TwoQubitState& get_state() const { return state_; }

    // Apply quantum gates
    void apply_cnot();  // CNOT gate (control on qubit 0, target on qubit 1)
    void apply_hadamard(int qubit);  // Hadamard on specified qubit
    void apply_phase_shift(int qubit, double phase);  // Phase gate

    // Density matrix calculations
    [[nodiscard]] DensityMatrix calculate_density_matrix() const;
    [[nodiscard]] double purity() const;  // Tr(ρ²) = 1 for pure states

    // Entanglement measures
    [[nodiscard]] double concurrence() const;  // Wootters concurrence (0 to 1)
    [[nodiscard]] double entanglement_entropy() const;  // Von Neumann entropy of reduced density matrix

    // Schmidt decomposition
    [[nodiscard]] SchmidtDecomposition schmidt_decomposition() const;

    // Partial trace (trace out subsystem B to get reduced density matrix for A)
    [[nodiscard]] DensityMatrix reduced_density_matrix_A() const;
    [[nodiscard]] DensityMatrix reduced_density_matrix_B() const;

    // Fidelity with a Bell state
    [[nodiscard]] double fidelity(BellState bell_state) const;

    // Check if state is separable (not entangled)
    [[nodiscard]] bool is_entangled() const;

    // Public static helpers for utilities
    [[nodiscard]] static DensityMatrix matrix_multiply(const DensityMatrix& A, const DensityMatrix& B);
    [[nodiscard]] static double trace(const DensityMatrix& rho);

private:
    TwoQubitState state_;
    static constexpr double INV_SQRT2 = 0.70710678118654752440;  // 1/√2

    // Helper: partial trace over subsystem B
    [[nodiscard]] static DensityMatrix partial_trace_B(const TwoQubitState& state);
};

// =============================================================================
// Entanglement Utilities
// =============================================================================

namespace entanglement_utils {
    // Create a specific Bell state
    [[nodiscard]] TwoQubitState create_bell_state(BellState state);

    // Calculate tangle (concurrence squared)
    [[nodiscard]] double tangle(double concurrence);

    // Negativity (entanglement measure based on partial transpose)
    [[nodiscard]] double negativity(const DensityMatrix& rho);

    // Logarithmic negativity (entanglement measure)
    [[nodiscard]] double logarithmic_negativity(const DensityMatrix& rho);

    // Check PPT criterion (Peres-Horodecki criterion)
    [[nodiscard]] bool is_ppt(const DensityMatrix& rho);

    // Von Neumann entropy: S(ρ) = -Tr(ρ log ρ)
    [[nodiscard]] double von_neumann_entropy(const DensityMatrix& rho);

    // Relative entropy of entanglement
    [[nodiscard]] double relative_entropy_entanglement(const DensityMatrix& rho);
}
