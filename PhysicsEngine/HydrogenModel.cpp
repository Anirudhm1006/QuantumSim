#include "HydrogenModel.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <random>
#include <cmath>

// =============================================================================
// Quantum Numbers Implementation
// =============================================================================

std::string QuantumNumbers::to_string() const {
    std::ostringstream oss;
    oss << "n=" << n << ", l=" << l << ", m=" << m;
    return oss.str();
}

// =============================================================================
// Hydrogen Model Implementation
// =============================================================================

HydrogenModel::HydrogenModel()
    : quantum_numbers_(1, 0, 0)
    , Z_(1)
{
}

HydrogenModel::HydrogenModel(const QuantumNumbers& qn, int Z)
    : quantum_numbers_(qn)
    , Z_(Z)
{
    if (!validate_quantum_numbers(qn.n, qn.l, qn.m)) {
        throw std::invalid_argument("Invalid quantum numbers: " + qn.to_string());
    }
}

HydrogenModel::HydrogenModel(int n, int l, int m, int Z)
    : quantum_numbers_(n, l, m)
    , Z_(Z)
{
    if (!validate_quantum_numbers(n, l, m)) {
        throw std::invalid_argument("Invalid quantum numbers: n=" + std::to_string(n) +
            ", l=" + std::to_string(l) + ", m=" + std::to_string(m));
    }
}

std::vector<std::complex<double>> HydrogenModel::get_wavefunction() const {
    // Return a sample of the wavefunction (not normalized in real space)
    std::vector<std::complex<double>> wf(100);

    // Sample at r = 0.1 to 10 Bohr radii
    double r_min = 0.1;
    double r_max = 10.0;
    double dr = (r_max - r_min) / 99.0;

    for (int i = 0; i < 100; ++i) {
        double r = r_min + i * dr;
        wf[i] = wavefunction(r, 0.0, 0.0); // Use θ=0, φ=0 for s-orbital sampling
    }

    return wf;
}

double HydrogenModel::get_probability_density(const std::vector<double>& position) const {
    if (position.size() < 3) {
        throw std::invalid_argument("Position must have 3 coordinates (r, theta, phi)");
    }

    double r = position[0];
    double theta = position[1];
    double phi = position[2];

    std::complex<double> psi = wavefunction(r, theta, phi);
    return std::norm(psi);
}

std::string HydrogenModel::get_state_descriptor() const {
    std::ostringstream oss;
    oss << "Hydrogen(" << get_orbital_name() << ", Z=" << Z_ << ", E="
        << std::fixed << std::setprecision(2) << get_energy_eV() << " eV)";
    return oss.str();
}

double HydrogenModel::get_energy_eV() const {
    // E_n = -R_H * Z² / n²
    return -RYDBERG_H * Z_ * Z_ / (quantum_numbers_.n * quantum_numbers_.n);
}

double HydrogenModel::get_wavelength_nm(const HydrogenModel& final_state) const {
    double delta_E = get_energy_eV() - final_state.get_energy_eV();

    if (delta_E <= 0) {
        throw std::invalid_argument("Transition energy must be positive");
    }

    // λ = hc / ΔE
    return HC_EV_NM / delta_E;
}

double HydrogenModel::calculate_normalization() const {
    int n = quantum_numbers_.n;
    int l = quantum_numbers_.l;

    // N = sqrt((2Z/na₀)³ * (n-l-1)! / (2n[(n+l)!]))
    double term1 = std::pow(2.0 * Z_ / n, 1.5);
    double term2 = factorial(n - l - 1);
    double term3 = 2.0 * n * factorial(n + l);

    return std::sqrt(term1 * term2 / term3);
}

double HydrogenModel::radial_wavefunction(double r) const {
    int n = quantum_numbers_.n;
    int l = quantum_numbers_.l;

    // ρ = 2Zr / na₀
    double rho = 2.0 * Z_ * r / n;

    // R_nl(r) = N * e^(-ρ/2) * ρ^l * L^{2l+1}_{n-l-1}(rho)
    double N = calculate_normalization();
    double exp_factor = std::exp(-rho / 2.0);
    double rho_power = std::pow(rho, l);
    double laguerre = associated_laguerre(n, l, rho);

    return N * exp_factor * rho_power * laguerre;
}

double HydrogenModel::radial_probability(double r) const {
    double R = radial_wavefunction(r);
    return r * r * R * R;
}

std::complex<double> HydrogenModel::spherical_harmonic(double theta, double phi) const {
    int l = quantum_numbers_.l;
    int m = quantum_numbers_.m;

    // Y_lm(θ, φ) = N_lm * P_l^m(cosθ) * e^(imφ)
    double cos_theta = std::cos(theta);

    // Associated Legendre polynomial
    double P = legendre_poly(l, m, cos_theta);

    // Normalization factor
    double prefactor = std::sqrt(
        (2 * l + 1) * factorial(l - m) / (4.0 * M_PI * factorial(l + m))
    );

    // e^(imφ)
    std::complex<double> phase(0.0, m * phi);

    return prefactor * P * std::exp(phase);
}

std::complex<double> HydrogenModel::wavefunction(double r, double theta, double phi) const {
    double R = radial_wavefunction(r);
    std::complex<double> Y = spherical_harmonic(theta, phi);

    return R * Y;
}

std::string HydrogenModel::get_orbital_name() const {
    int l = quantum_numbers_.l;
    std::string orbital;

    switch (l) {
        case 0: orbital = "s"; break;
        case 1: orbital = "p"; break;
        case 2: orbital = "d"; break;
        case 3: orbital = "f"; break;
        default: orbital = "l=" + std::to_string(l); break;
    }

    return std::to_string(quantum_numbers_.n) + orbital;
}

bool HydrogenModel::validate_quantum_numbers(int n, int l, int m) {
    // n must be positive integer
    if (n < 1) return false;

    // l must satisfy 0 ≤ l < n
    if (l < 0 || l >= n) return false;

    // m must satisfy -l ≤ m ≤ l
    if (m < -l || m > l) return false;

    return true;
}

double HydrogenModel::associated_laguerre(int n, int l, double x) const {
    // For hydrogen: L^{2l+1}_{n-l-1}(rho)
    // n = principal quantum number, l = angular momentum
    // The polynomial degree is n_r = n - l - 1 (radial quantum number)
    int n_r = n - l - 1;  // Radial quantum number

    // Recursive implementation to avoid overflow
    if (n_r < 0) return 0.0;  // Invalid state
    if (n_r == 0) return 1.0;
    if (n_r == 1) return 1.0 + (2 * l + 1) - x;

    // L^{k}_{m}(x) = ((2m-1+k-x)L^{k}_{m-1}(x) - (m-1+k)L^{k}_{m-2}(x)) / m
    int k = 2 * l + 1;

    double L_n_minus_1 = 1.0 + k - x;  // L^{k}_{1}
    double L_n_minus_2 = 1.0;          // L^{k}_{0}

    double L_n = 0.0;

    for (int i = 2; i <= n_r; ++i) {
        L_n = ((2 * i - 1 + k - x) * L_n_minus_1 - (i - 1 + k) * L_n_minus_2) / i;
        L_n_minus_2 = L_n_minus_1;
        L_n_minus_1 = L_n;
    }

    return L_n;
}

double HydrogenModel::factorial(int n) const {
    if (n < 0) return 0.0;
    if (n == 0 || n == 1) return 1.0;
    if (n > 20) return std::numeric_limits<double>::infinity(); // Prevent overflow

    double result = 1.0;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

double HydrogenModel::legendre_poly(int l, int m, double x) const {
    // Associated Legendre polynomial P_l^m(x)
    if (l == 0) return 1.0;
    if (l == 1) {
        if (m == 0) return x;
        if (m == 1) return -std::sqrt(1.0 - x * x);
    }

    // Use recurrence relation
    // P_l^m(x) = (2l-1)xP_{l-1}^m(x) - (l+m-1)P_{l-2}^m(x) / (l-m)
    double P_0 = 1.0;
    double P_1 = (m == 0) ? x : -std::sqrt(1.0 - x * x);

    double P = 0.0;

    for (int i = 2; i <= l; ++i) {
        if (i - m > 0) {
            P = ((2 * i - 1) * x * P_1 - (i + m - 1) * P_0) / (i - m);
        }
        P_0 = P_1;
        P_1 = P;
    }

    return P;
}

std::vector<double> HydrogenModel::sample_position() const {
    // Simple Metropolis-Hastings sampling
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist_r(0.0, 5.0 * quantum_numbers_.n);
    std::uniform_real_distribution<double> dist_theta(0.0, M_PI);
    std::uniform_real_distribution<double> dist_phi(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> dist_ratio(0.0, 1.0);

    // Initial guess
    double r = dist_r(gen);
    double theta = dist_theta(gen);
    double phi = dist_phi(gen);

    double current_prob = radial_probability(r);

    // 1000 iterations
    for (int i = 0; i < 1000; ++i) {
        double r_new = r + dist_ratio(gen) * 2.0 - 1.0;
        if (r_new < 0) r_new = 0.1;

        double new_prob = radial_probability(r_new);

        // Accept with probability min(1, new_prob/current_prob)
        if (dist_ratio(gen) < new_prob / current_prob) {
            r = r_new;
            current_prob = new_prob;
        }
    }

    // Convert to Cartesian and scale by Bohr radius
    double scale = BOHR_RADIUS;

    return {
        r * scale * std::sin(theta) * std::cos(phi),
        r * scale * std::sin(theta) * std::sin(phi),
        r * scale * std::cos(theta)
    };
}

// =============================================================================
// Hydrogen Utilities Implementation
// =============================================================================

namespace hydrogen_utils {
    // Local constant for utilities (same as HydrogenModel::RYDBERG_H)
    constexpr double RYDBERG_H_UTILS = 13.605693009;

    double rydberg_wavelength(int n_initial, int n_final, int Z) {
        if (n_initial <= n_final) {
            throw std::invalid_argument("n_initial must be > n_final");
        }

        // 1/λ = R_H * Z² * (1/n_final² - 1/n_initial²)
        double term = 1.0 / (n_final * n_final) - 1.0 / (n_initial * n_initial);
        double lambda_inv = RYDBERG_H_UTILS * Z * Z * term;

        return 1.0 / lambda_inv * 1e9 / RYDBERG_H_UTILS; // Convert to nm
    }

    double energy_difference(int n_initial, int n_final, int Z) {
        double E_initial = -RYDBERG_H_UTILS * Z * Z / (n_initial * n_initial);
        double E_final = -RYDBERG_H_UTILS * Z * Z / (n_final * n_final);
        return E_final - E_initial;
    }

    double balmer_alpha() {
        return 656.28; // nm
    }

    double balmer_beta() {
        return 486.13;
    }

    double balmer_gamma() {
        return 434.05;
    }

    double balmer_delta() {
        return 410.17;
    }
}

nlohmann::json HydrogenModel::to_json() const {
    return {
        {"type", "HydrogenModel"},
        {"n", quantum_numbers_.n},
        {"l", quantum_numbers_.l},
        {"m", quantum_numbers_.m},
        {"Z", Z_}
    };
}
