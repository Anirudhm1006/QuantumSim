#include "SpinSystem.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

// =============================================================================
// Pauli Matrix Implementations
// =============================================================================

namespace pauli_matrices {
    ComplexMatrix2x2 sigma_x() {
        ComplexMatrix2x2 m;
        m[0][0] = std::complex<double>(0.0, 0.0);
        m[0][1] = std::complex<double>(1.0, 0.0);
        m[1][0] = std::complex<double>(1.0, 0.0);
        m[1][1] = std::complex<double>(0.0, 0.0);
        return m;
    }

    ComplexMatrix2x2 sigma_y() {
        ComplexMatrix2x2 m;
        m[0][0] = std::complex<double>(0.0, 0.0);
        m[0][1] = std::complex<double>(0.0, -1.0);
        m[1][0] = std::complex<double>(0.0, 1.0);
        m[1][1] = std::complex<double>(0.0, 0.0);
        return m;
    }

    ComplexMatrix2x2 sigma_z() {
        ComplexMatrix2x2 m;
        m[0][0] = std::complex<double>(1.0, 0.0);
        m[0][1] = std::complex<double>(0.0, 0.0);
        m[1][0] = std::complex<double>(0.0, 0.0);
        m[1][1] = std::complex<double>(-1.0, 0.0);
        return m;
    }

    ComplexMatrix2x2 identity() {
        ComplexMatrix2x2 m;
        m[0][0] = std::complex<double>(1.0, 0.0);
        m[0][1] = std::complex<double>(0.0, 0.0);
        m[1][0] = std::complex<double>(0.0, 0.0);
        m[1][1] = std::complex<double>(1.0, 0.0);
        return m;
    }

    std::vector<std::complex<double>> apply(
        const ComplexMatrix2x2& matrix,
        const std::vector<std::complex<double>>& state
    ) {
        if (state.size() != 2) {
            throw std::invalid_argument("State must be 2-dimensional");
        }

        std::vector<std::complex<double>> result(2);
        for (int i = 0; i < 2; ++i) {
            result[i] = matrix[i][0] * state[0] + matrix[i][1] * state[1];
        }
        return result;
    }
}

// =============================================================================
// Spin Operator Implementations
// =============================================================================

namespace spin_operators {
    // In atomic units, ħ/2 = 0.5
    ComplexMatrix2x2 spin_x() {
        auto sigma = pauli_matrices::sigma_x();
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                sigma[i][j] *= 0.5;
            }
        }
        return sigma;
    }

    ComplexMatrix2x2 spin_y() {
        auto sigma = pauli_matrices::sigma_y();
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                sigma[i][j] *= 0.5;
            }
        }
        return sigma;
    }

    ComplexMatrix2x2 spin_z() {
        auto sigma = pauli_matrices::sigma_z();
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                sigma[i][j] *= 0.5;
            }
        }
        return sigma;
    }

    ComplexMatrix2x2 spin_squared() {
        // S² = Sx² + Sy² + Sz² = (3/4)��² I for spin-1/2
        ComplexMatrix2x2 m;
        m[0][0] = std::complex<double>(0.75, 0.0);
        m[0][1] = std::complex<double>(0.0, 0.0);
        m[1][0] = std::complex<double>(0.0, 0.0);
        m[1][1] = std::complex<double>(0.75, 0.0);
        return m;
    }
}

// =============================================================================
// SpinSystem Implementation
// =============================================================================

SpinSystem::SpinSystem()
    : theta_(0.0)
    , phi_(0.0)
    , alpha_(1.0, 0.0)
    , beta_(0.0, 0.0)
{
}

SpinSystem::SpinSystem(double theta, double phi)
    : theta_(theta)
    , phi_(phi)
{
    update_state_from_angles();
}

SpinSystem::SpinSystem(double theta, double phi, bool initial_spin_up)
    : theta_(theta)
    , phi_(phi)
{
    if (initial_spin_up) {
        // |0⟩ state (spin up along z)
        alpha_ = std::complex<double>(1.0, 0.0);
        beta_ = std::complex<double>(0.0, 0.0);
    } else {
        // |1⟩ state (spin down along z)
        alpha_ = std::complex<double>(0.0, 0.0);
        beta_ = std::complex<double>(1.0, 0.0);
    }
}

void SpinSystem::update_state_from_angles() {
    // |ψ⟩ = cos(θ/2)|0⟩ + e^(iφ)sin(θ/2)|1⟩
    double half_theta = theta_ / 2.0;
    alpha_ = std::complex<double>(std::cos(half_theta), 0.0);
    beta_ = std::exp(std::complex<double>(0.0, phi_)) * std::sin(half_theta);
}

std::vector<std::complex<double>> SpinSystem::get_wavefunction() const {
    return {alpha_, beta_};
}

double SpinSystem::get_probability_density(const std::vector<double>&) const {
    // For spin-1/2, probability is always 1 (normalized state)
    // The position parameter is ignored for spin systems
    double prob_up = std::norm(alpha_);
    double prob_down = std::norm(beta_);
    return prob_up + prob_down;
}

std::string SpinSystem::get_state_descriptor() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "|ψ⟩ = " << std::abs(alpha_) << "|0⟩ + "
        << std::abs(beta_) << "e^(" << std::arg(beta_) << "i)|1⟩";
    oss << " [θ=" << theta_ << ", φ=" << phi_ << "]";
    return oss.str();
}

void SpinSystem::set_bloch_angles(double theta, double phi) {
    theta_ = theta;
    phi_ = phi;
    update_state_from_angles();
}

double SpinSystem::get_spin_projection_z() const {
    // ⟨σz⟩ = |α|² - |β|² = cos²(θ/2) - sin²(θ/2) = cos(θ)
    return std::cos(theta_);
}

double SpinSystem::get_spin_projection_x() const {
    // ⟨σx⟩ = sin(θ)cos(φ)
    return std::sin(theta_) * std::cos(phi_);
}

double SpinSystem::get_spin_projection_y() const {
    // ⟨σy⟩ = sin(θ)sin(φ)
    return std::sin(theta_) * std::sin(phi_);
}

std::vector<double> SpinSystem::get_bloch_vector() const {
    return {
        get_spin_projection_x(),
        get_spin_projection_y(),
        get_spin_projection_z()
    };
}

std::vector<std::complex<double>> SpinSystem::apply_pauli(const std::string& which) const {
    ComplexMatrix2x2 matrix;

    if (which == "x" || which == "sigma_x") {
        matrix = pauli_matrices::sigma_x();
    } else if (which == "y" || which == "sigma_y") {
        matrix = pauli_matrices::sigma_y();
    } else if (which == "z" || which == "sigma_z") {
        matrix = pauli_matrices::sigma_z();
    } else {
        throw std::invalid_argument("Unknown Pauli matrix: " + which);
    }

    return pauli_matrices::apply(matrix, {alpha_, beta_});
}

std::vector<std::complex<double>> SpinSystem::spin_up() {
    return {std::complex<double>(1.0, 0.0), std::complex<double>(0.0, 0.0)};
}

std::vector<std::complex<double>> SpinSystem::spin_down() {
    return {std::complex<double>(0.0, 0.0), std::complex<double>(1.0, 0.0)};
}

std::vector<std::complex<double>> SpinSystem::spin_plus_x() {
    // |+⟩_x = (|0⟩ + |1⟩)/√2
    double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    return {std::complex<double>(inv_sqrt2, 0.0), std::complex<double>(inv_sqrt2, 0.0)};
}

std::vector<std::complex<double>> SpinSystem::spin_minus_x() {
    // |-⟩_x = (|0⟩ - |1⟩)/√2
    double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    return {std::complex<double>(inv_sqrt2, 0.0), std::complex<double>(-inv_sqrt2, 0.0)};
}

double SpinSystem::expectation_value(const ComplexMatrix2x2& op) const {
    // ⟨ψ|A|ψ⟩ = ψ† A ψ
    std::vector<std::complex<double>> state = {alpha_, beta_};
    std::vector<std::complex<double>> op_state = pauli_matrices::apply(op, state);

    // Calculate ⟨ψ|A|ψ⟩ = Σ_i ψ_i* (Aψ)_i
    std::complex<double> result = {0.0, 0.0};
    for (int i = 0; i < 2; ++i) {
        result += std::conj(state[i]) * op_state[i];
    }

    return std::real(result);
}

SpinSystem SpinSystem::create_superposition(
    const std::vector<std::complex<double>>& coeff_up,
    const std::vector<std::complex<double>>& coeff_down
) {
    // Create a normalized superposition state
    double norm = std::sqrt(std::norm(coeff_up[0]) + std::norm(coeff_down[0]));

    double theta = 2.0 * std::asin(norm);
    double phi = std::arg(coeff_down[0]) - std::arg(coeff_up[0]);

    return SpinSystem(theta, phi);
}

nlohmann::json SpinSystem::to_json() const {
    return {
        {"type", "SpinSystem"},
        {"theta", theta_},
        {"phi", phi_}
    };
}
