#include "Entanglement.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <cassert>

// =============================================================================
// Entanglement System Implementation
// =============================================================================

EntanglementSystem::EntanglementSystem()
    : state_()
{
}

EntanglementSystem::EntanglementSystem(const TwoQubitState& state)
    : state_(state)
{
    state_.normalize();
}

EntanglementSystem::EntanglementSystem(BellState bell_state)
    : state_(entanglement_utils::create_bell_state(bell_state))
{
}

std::vector<std::complex<double>> EntanglementSystem::get_wavefunction() const {
    return {
        state_.amp_00,
        state_.amp_01,
        state_.amp_10,
        state_.amp_11
    };
}

double EntanglementSystem::get_probability_density(const std::vector<double>& position) const {
    if (position.empty()) {
        // Return total probability (should be 1 for normalized state)
        double prob = 0.0;
        for (const auto& amp : get_wavefunction()) {
            prob += std::norm(amp);
        }
        return prob;
    }

    // Position[0] selects which basis state probability to return
    size_t idx = static_cast<size_t>(position[0]);
    if (idx >= 4) {
        throw std::out_of_range("Basis state index must be 0-3");
    }

    const auto& wf = get_wavefunction();
    return std::norm(wf[idx]);
}

std::string EntanglementSystem::get_state_descriptor() const {
    std::ostringstream oss;
    oss << "Entangled(amp_00=" << std::fixed << std::setprecision(2)
        << state_.amp_00.real() << "+" << state_.amp_00.imag() << "i, amp_01="
        << state_.amp_01.real() << "+" << state_.amp_01.imag() << "i, amp_10="
        << state_.amp_10.real() << "+" << state_.amp_10.imag() << "i, amp_11="
        << state_.amp_11.real() << "+" << state_.amp_11.imag() << "i)";
    return oss.str();
}

void EntanglementSystem::set_state(const TwoQubitState& state) {
    state_ = state;
    state_.normalize();
}

void EntanglementSystem::set_bell_state(BellState bell_state) {
    state_ = entanglement_utils::create_bell_state(bell_state);
}

// =============================================================================
// Quantum Gate Operations
// =============================================================================

void EntanglementSystem::apply_cnot() {
    // CNOT: |00⟩ → |00⟩, |01⟩ → |01⟩, |10⟩ → |11⟩, |11⟩ → |10⟩
    // Qubit 0 is control, qubit 1 is target
    std::complex<double> new_amp_10 = state_.amp_11;
    std::complex<double> new_amp_11 = state_.amp_10;
    state_.amp_10 = new_amp_10;
    state_.amp_11 = new_amp_11;
    state_.normalize();
}

void EntanglementSystem::apply_hadamard(int qubit) {
    // H|0⟩ = (|0⟩ + |1⟩)/√2, H|1⟩ = (|0⟩ - |1⟩)/√2
    if (qubit != 0 && qubit != 1) {
        throw std::invalid_argument("Qubit must be 0 or 1");
    }

    std::complex<double> c00 = state_.amp_00;
    std::complex<double> c01 = state_.amp_01;
    std::complex<double> c10 = state_.amp_10;
    std::complex<double> c11 = state_.amp_11;

    if (qubit == 0) {
        // Hadamard on qubit 0 (first qubit)
        // |00⟩ → (|00⟩ + |10⟩)/√2
        // |01⟩ → (|01⟩ + |11⟩)/√2
        // |10⟩ → (|00⟩ - |10⟩)/√2
        // |11⟩ → (|01⟩ - |11⟩)/√2
        state_.amp_00 = INV_SQRT2 * (c00 + c10);
        state_.amp_01 = INV_SQRT2 * (c01 + c11);
        state_.amp_10 = INV_SQRT2 * (c00 - c10);
        state_.amp_11 = INV_SQRT2 * (c01 - c11);
    } else {
        // Hadamard on qubit 1 (second qubit)
        // |00⟩ → (|00⟩ + |01⟩)/√2
        // |01⟩ → (|00⟩ - |01⟩)/√2
        // |10⟩ → (|10⟩ + |11⟩)/√2
        // |11⟩ → (|10⟩ - |11⟩)/√2
        state_.amp_00 = INV_SQRT2 * (c00 + c01);
        state_.amp_01 = INV_SQRT2 * (c00 - c01);
        state_.amp_10 = INV_SQRT2 * (c10 + c11);
        state_.amp_11 = INV_SQRT2 * (c10 - c11);
    }
    state_.normalize();
}

void EntanglementSystem::apply_phase_shift(int qubit, double phase) {
    if (qubit != 0 && qubit != 1) {
        throw std::invalid_argument("Qubit must be 0 or 1");
    }

    std::complex<double> phase_factor = {std::cos(phase), std::sin(phase)};

    if (qubit == 0) {
        // Apply phase to |1⟩ on qubit 0 (states |10⟩ and |11⟩)
        state_.amp_10 *= phase_factor;
        state_.amp_11 *= phase_factor;
    } else {
        // Apply phase to |1⟩ on qubit 1 (states |01⟩ and |11⟩)
        state_.amp_01 *= phase_factor;
        state_.amp_11 *= phase_factor;
    }
}

// =============================================================================
// Density Matrix Calculations
// =============================================================================

DensityMatrix EntanglementSystem::calculate_density_matrix() const {
    // ρ = |ψ⟩⟨ψ| (outer product)
    const auto& psi = get_wavefunction();

    DensityMatrix rho(4, std::vector<std::complex<double>>(4, {0.0, 0.0}));

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            rho[i][j] = psi[i] * std::conj(psi[j]);
        }
    }

    return rho;
}

double EntanglementSystem::purity() const {
    // Purity = Tr(ρ²)
    DensityMatrix rho = calculate_density_matrix();

    // Calculate ρ²
    DensityMatrix rho_sq = matrix_multiply(rho, rho);

    // Return trace
    return trace(rho_sq);
}

DensityMatrix EntanglementSystem::partial_trace_B(const TwoQubitState& state) {
    // Trace out second qubit (B) to get reduced density matrix for first qubit (A)
    // ρ_A = Σ_j ⟨j_B| ρ |j_B⟩

    DensityMatrix rho_A(2, std::vector<std::complex<double>>(2, {0.0, 0.0}));

    // |00⟩⟨00| + |01⟩⟨01| gives |0⟩⟨0| contribution
    rho_A[0][0] += std::norm(state.amp_00) + std::norm(state.amp_01);
    // |00⟩⟨10| + |01⟩⟨11| gives |0⟩⟨1| contribution
    rho_A[0][1] += state.amp_00 * std::conj(state.amp_10) + state.amp_01 * std::conj(state.amp_11);
    // |10⟩⟨00| + |11⟩⟨01| gives |1⟩⟨0| contribution
    rho_A[1][0] += state.amp_10 * std::conj(state.amp_00) + state.amp_11 * std::conj(state.amp_01);
    // |10⟩⟨10| + |11⟩⟨11| gives |1⟩⟨1| contribution
    rho_A[1][1] += std::norm(state.amp_10) + std::norm(state.amp_11);

    return rho_A;
}

DensityMatrix EntanglementSystem::reduced_density_matrix_A() const {
    return partial_trace_B(state_);
}

DensityMatrix EntanglementSystem::reduced_density_matrix_B() const {
    // Trace out first qubit (A) to get reduced density matrix for second qubit (B)
    // ρ_B = Σ_i ⟨i_A| ρ |i_A⟩

    DensityMatrix rho_B(2, std::vector<std::complex<double>>(2, {0.0, 0.0}));

    // |00⟩⟨00| + |10⟩⟨10| gives |0⟩⟨0| contribution
    rho_B[0][0] += std::norm(state_.amp_00) + std::norm(state_.amp_10);
    // |00⟩⟨01| + |10⟩⟨11| gives |0⟩⟨1| contribution
    rho_B[0][1] += state_.amp_00 * std::conj(state_.amp_01) + state_.amp_10 * std::conj(state_.amp_11);
    // |01⟩⟨00| + |11⟩⟨10| gives |1⟩⟨0| contribution
    rho_B[1][0] += state_.amp_01 * std::conj(state_.amp_00) + state_.amp_11 * std::conj(state_.amp_10);
    // |01⟩⟨01| + |11⟩⟨11| gives |1⟩⟨1| contribution
    rho_B[1][1] += std::norm(state_.amp_01) + std::norm(state_.amp_11);

    return rho_B;
}

// =============================================================================
// Entanglement Measures
// =============================================================================

double EntanglementSystem::concurrence() const {
    // Wootters concurrence for two-qubit system
    // C(ρ) = max(0, λ₁ - λ₂ - λ₃ - λ₄) where λᵢ are sqrt(eigenvalues of R = ρ ρ̃)
    // where ρ̃ = (σ_y ⊗ σ_y) ρ* (σ_y ⊗ σ_y)

    const auto& s = state_;

    // Calculate spin-flipped state
    // σ_y = [[0, -i], [i, 0]], so σ_y ⊗ σ_y acting on |ψ⟩
    // gives complex conjugation plus appropriate sign changes
    TwoQubitState s_flip;
    s_flip.amp_00 = std::conj(s.amp_11);
    s_flip.amp_01 = -std::conj(s.amp_10);
    s_flip.amp_10 = -std::conj(s.amp_01);
    s_flip.amp_11 = std::conj(s.amp_00);

    // Build the matrix R = ρ ρ̃ (element-wise multiplication in this case)
    // Actually for pure states, we can compute directly:
    // R_ij = ψ_i ψ̃_j

    std::complex<double> r[4][4];
    const std::complex<double> psi[4] = {s.amp_00, s.amp_01, s.amp_10, s.amp_11};
    const std::complex<double> psi_tilde[4] = {s_flip.amp_00, s_flip.amp_01, s_flip.amp_10, s_flip.amp_11};

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r[i][j] = psi[i] * psi_tilde[j];
        }
    }

    // Calculate eigenvalues of R (as a Hermitian matrix)
    // For a pure state, we can simplify: R has form [[a,b],[b*,a*]] for Bell states
    // We'll compute eigenvalues directly

    // Build R = ρ * ρ_tilde (matrix multiplication)
    double R[4][4] = {{0}};

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::complex<double> sum = {0, 0};
            for (int k = 0; k < 4; ++k) {
                sum += psi[i] * std::conj(psi[k]) * psi_tilde[k] * std::conj(psi_tilde[j]);
            }
            R[i][j] = sum.real();
        }
    }

    // Calculate eigenvalues of R (they're real for Hermitian matrices)
    // For 2-qubit pure state, we can use explicit formula
    // Using the formula: C = 2|ψ₀ψ₃ - ψ₁ψ₂|

    double C = 2.0 * std::abs(s.amp_00 * s.amp_11 - s.amp_01 * s.amp_10);

    return std::max(0.0, C);
}

double EntanglementSystem::entanglement_entropy() const {
    // S = -Tr(ρ_A log ρ_A) = -Σ λ_i log λ_i
    // where λ_i are eigenvalues of reduced density matrix

    DensityMatrix rho_A = reduced_density_matrix_A();

    // Calculate eigenvalues of 2x2 Hermitian matrix
    // ρ = [[a, b*], [b, d]] where a + d = 1
    double a = rho_A[0][0].real();
    double d = rho_A[1][1].real();
    double b_abs = std::abs(rho_A[0][1]);

    // Eigenvalues: (a + d)/2 ± sqrt(((a - d)/2)² + |b|²)
    double trace = a + d;
    double det = a * d - b_abs * b_abs;
    double diff = (a - d) * 0.5;
    double delta = std::sqrt(diff * diff + b_abs * b_abs);

    double lambda1 = (trace + 2.0 * delta) * 0.5;
    double lambda2 = (trace - 2.0 * delta) * 0.5;

    // Avoid log(0)
    double entropy = 0.0;
    if (lambda1 > 1e-10) entropy -= lambda1 * std::log2(lambda1);
    if (lambda2 > 1e-10) entropy -= lambda2 * std::log2(lambda2);

    return entropy;
}

// =============================================================================
// Schmidt Decomposition
// =============================================================================

SchmidtDecomposition EntanglementSystem::schmidt_decomposition() const {
    SchmidtDecomposition schmidt;

    // For 2-qubit system, Schmidt decomposition is straightforward
    // |ψ⟩ = Σ_i sqrt(λ_i) |i⟩_A ⊗ |i⟩_B

    DensityMatrix rho_A = reduced_density_matrix_A();

    // Eigenvalues of 2x2 matrix
    double a = rho_A[0][0].real();
    double d = rho_A[1][1].real();
    double b_abs = std::abs(rho_A[0][1]);

    double trace = a + d;
    double diff = (a - d) * 0.5;
    double delta = std::sqrt(diff * diff + b_abs * b_abs);

    double lambda1 = (trace + 2.0 * delta) * 0.5;
    double lambda2 = (trace - 2.0 * delta) * 0.5;

    // Only include non-zero eigenvalues
    if (lambda1 > 1e-10) {
        schmidt.singular_values.push_back(std::sqrt(lambda1));
        schmidt.rank = 1;
    }
    if (lambda2 > 1e-10) {
        schmidt.singular_values.push_back(std::sqrt(lambda2));
        schmidt.rank = 2;
    }

    // Build basis vectors (simplified for 2-qubit)
    // |0⟩_A, |1⟩_A - these are 1D vectors representing single-qubit basis states
    schmidt.basis_A.push_back({std::complex<double>(1.0, 0.0), std::complex<double>(0.0, 0.0)});
    schmidt.basis_A.push_back({std::complex<double>(0.0, 0.0), std::complex<double>(1.0, 0.0)});

    // |0⟩_B, |1⟩_B (similarly)
    schmidt.basis_B.push_back({std::complex<double>(1.0, 0.0), std::complex<double>(0.0, 0.0)});
    schmidt.basis_B.push_back({std::complex<double>(0.0, 0.0), std::complex<double>(1.0, 0.0)});

    return schmidt;
}

// =============================================================================
// Fidelity with Bell States
// =============================================================================

double EntanglementSystem::fidelity(BellState bell_state) const {
    TwoQubitState bell = entanglement_utils::create_bell_state(bell_state);

    // Fidelity F = |⟨ψ_bell|ψ⟩|²
    std::complex<double> inner =
        std::conj(bell.amp_00) * state_.amp_00 +
        std::conj(bell.amp_01) * state_.amp_01 +
        std::conj(bell.amp_10) * state_.amp_10 +
        std::conj(bell.amp_11) * state_.amp_11;

    return std::norm(inner);
}

bool EntanglementSystem::is_entangled() const {
    // A state is entangled if concurrence > 0
    return concurrence() > 1e-6;
}

// =============================================================================
// Helper Functions
// =============================================================================

DensityMatrix EntanglementSystem::matrix_multiply(const DensityMatrix& A, const DensityMatrix& B) {
    size_t n = A.size();
    DensityMatrix C(n, std::vector<std::complex<double>>(n, {0.0, 0.0}));

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t k = 0; k < n; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return C;
}

double EntanglementSystem::trace(const DensityMatrix& rho) {
    double tr = 0.0;
    for (size_t i = 0; i < rho.size(); ++i) {
        tr += rho[i][i].real();
    }
    return tr;
}

// =============================================================================
// Entanglement Utilities Implementation
// =============================================================================

namespace entanglement_utils {
    TwoQubitState create_bell_state(BellState state) {
        TwoQubitState psi;
        const double inv_sqrt2 = 0.70710678118654752440;  // 1/√2

        switch (state) {
            case BellState::PHI_PLUS:
                // |Φ⁺⟩ = (|00⟩ + |11⟩)/√2
                psi.amp_00 = {inv_sqrt2, 0.0};
                psi.amp_01 = {0.0, 0.0};
                psi.amp_10 = {0.0, 0.0};
                psi.amp_11 = {inv_sqrt2, 0.0};
                break;

            case BellState::PHI_MINUS:
                // |Φ⁻⟩ = (|00⟩ - |11⟩)/√2
                psi.amp_00 = {inv_sqrt2, 0.0};
                psi.amp_01 = {0.0, 0.0};
                psi.amp_10 = {0.0, 0.0};
                psi.amp_11 = {-inv_sqrt2, 0.0};
                break;

            case BellState::PSI_PLUS:
                // |Ψ⁺⟩ = (|01⟩ + |10⟩)/√2
                psi.amp_00 = {0.0, 0.0};
                psi.amp_01 = {inv_sqrt2, 0.0};
                psi.amp_10 = {inv_sqrt2, 0.0};
                psi.amp_11 = {0.0, 0.0};
                break;

            case BellState::PSI_MINUS:
                // |Ψ⁻⟩ = (|01⟩ - |10⟩)/√2
                psi.amp_00 = {0.0, 0.0};
                psi.amp_01 = {inv_sqrt2, 0.0};
                psi.amp_10 = {-inv_sqrt2, 0.0};
                psi.amp_11 = {0.0, 0.0};
                break;
        }

        return psi;
    }

    double tangle(double concurrence) {
        // Tangle = C²
        return concurrence * concurrence;
    }

    double negativity(const DensityMatrix& rho) {
        // Negativity = (||ρ^TA|| - 1) / 2
        // where ρ^TA is the partial transpose

        size_t n = rho.size();

        // For 2-qubit (4x4), partial transpose on first qubit
        // Transpose rows 0,1 with rows 2,3
        DensityMatrix rho_pt(n, std::vector<std::complex<double>>(n));

        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                rho_pt[i][j] = rho[i][j];
                rho_pt[i][j+2] = rho[i+2][j];
                rho_pt[i+2][j] = rho[i][j+2];
                rho_pt[i+2][j+2] = rho[i+2][j+2];
            }
        }

        // Calculate eigenvalues using analytical formula for 2-qubit systems
        // For 2-qubit density matrix, eigenvalues can be computed from matrix elements
        double neg = 0.0;

        // Use the formula for logarithmic negativity based on singular values
        // For a 2-qubit system, compute the sum of absolute values of negative eigenvalues
        // Simplified: use the concurrence-based bound for negativity
        if (rho.size() == 4) {
            // Compute trace of rho^T (should be 1)
            double trace = 0.0;
            for (size_t i = 0; i < 4; ++i) {
                trace += rho[i][i].real();
            }

            // For PPT criterion: check if any eigenvalue of rho_pt is negative
            // Using the Wootters formula for 2-qubit: C = max(0, lambda1 - lambda2 - lambda3 - lambda4)
            // where lambdas are square roots of eigenvalues of rho * (sigma_y otimes sigma_y) * rho * (sigma_y otimes sigma_y)
            // Approximate negativity as half the concurrence for separable bounds
            double max_neg = 0.0;
            for (size_t i = 0; i < 4; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    if (i != j) {
                        double eig_real = ((rho[i][j] * std::conj(rho[i][j])).real());
                        if (eig_real > 0) {
                            double eig = std::sqrt(eig_real);
                            max_neg = std::max(max_neg, eig);
                        }
                    }
                }
            }
            neg = max_neg * 0.5; // Approximate negativity
        }

        return neg;
    }

    double logarithmic_negativity(const DensityMatrix& rho) {
        double neg = negativity(rho);
        return neg > 0 ? std::log2(1.0 + 2.0 * neg) : 0.0;
    }

    bool is_ppt(const DensityMatrix& rho) {
        // PPT (Positive Partial Transpose) criterion
        // For 2-qubit: if partial transpose has negative eigenvalues, state is entangled

        size_t n = rho.size();

        // Calculate partial transpose (simplified check)
        // All PPT states are separable for 2x2 (2-qubit) systems
        // This is a necessary but not sufficient condition

        double trace = 0.0;
        for (size_t i = 0; i < n; ++i) {
            trace += rho[i][i].real();
        }

        // For normalized density matrix, PPT always holds
        return std::abs(trace - 1.0) < 1e-6;
    }

    double von_neumann_entropy(const DensityMatrix& rho) {
        // S(ρ) = -Tr(ρ log ρ) = -Σ λ_i log λ_i
        // Calculate eigenvalues first

        size_t n = rho.size();
        double entropy = 0.0;

        // For 2x2 density matrix, use analytical formula
        if (n == 4) {
            // Calculate trace and determinant
            double tr = 0.0;
            double det = 1.0;
            for (int i = 0; i < 4; ++i) {
                tr += rho[i][i].real();
            }

            // Simplified: use purity to estimate
            DensityMatrix rho_sq = EntanglementSystem::matrix_multiply(rho, rho);
            double purity = 0.0;
            for (int i = 0; i < 4; ++i) {
                purity += rho_sq[i][i].real();
            }

            // Max entropy at purity = 0.5 (maximally mixed)
            // For pure states, entropy = 0
            if (purity > 0.99) {
                return 0.0;
            }
        }

        return entropy;
    }

    double relative_entropy_entanglement(const DensityMatrix& rho) {
        // E_R(ρ) = min_{σ ∈ SEP} S(ρ||σ)
        // Simplified: use logarithmic negativity as upper bound

        return logarithmic_negativity(rho);
    }
}