#pragma once

#include <complex>
#include <functional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "IQuantumObject.hpp"

// =============================================================================
// Hydrogen Atom Model
// =============================================================================
// Implements:
// - Radial probability: P_nl(r) = r² |R_nl(r)|²
// - Energy levels: E_n = -13.6 eV / n²
// - Hydrogenic wavefunctions: ψ_nlm(r, θ, φ) = R_nl(r)Y_lm(θ, φ)
// =============================================================================

// Quantum numbers for hydrogen state
struct QuantumNumbers {
    int n;  // Principal quantum number (n = 1, 2, 3, ...)
    int l;  // Orbital quantum number (0 ≤ l < n)
    int m;  // Magnetic quantum number (-l ≤ m ≤ l)

    QuantumNumbers() : n(1), l(0), m(0) {}
    QuantumNumbers(int n_, int l_, int m_) : n(n_), l(l_), m(m_) {}

    [[nodiscard]] std::string to_string() const;
};

class HydrogenModel : public IQuantumObject {
public:
    // Constructors
    HydrogenModel();
    explicit HydrogenModel(const QuantumNumbers& quantum_numbers, int Z = 1);
    HydrogenModel(int n, int l, int m, int Z = 1);

    // Destructor
    ~HydrogenModel() override = default;

    // Get the wavefunction as vector (for interface compatibility)
    [[nodiscard]] std::vector<std::complex<double>> get_wavefunction() const override;

    // Calculate probability density at a given position (r, theta, phi)
    [[nodiscard]] double get_probability_density(const std::vector<double>& position) const override;

    [[nodiscard]] std::string get_state_descriptor() const override;
    [[nodiscard]] nlohmann::json to_json() const override;
    [[nodiscard]] std::string get_type_name() const override { return "HydrogenModel"; }

    // Get quantum numbers
    [[nodiscard]] const QuantumNumbers& get_quantum_numbers() const { return quantum_numbers_; }

    // Get atomic number
    [[nodiscard]] int get_Z() const { return Z_; }

    // Get energy level in eV
    [[nodiscard]] double get_energy_eV() const;

    // Get wavelength for transition to another level (in nm)
    [[nodiscard]] double get_wavelength_nm(const HydrogenModel& final_state) const;

    // Calculate radial wavefunction R_nl(r) at radius r (in Bohr radii)
    [[nodiscard]] double radial_wavefunction(double r) const;

    // Calculate radial probability density P(r) = r²|R_nl(r)|²
    [[nodiscard]] double radial_probability(double r) const;

    // Calculate spherical harmonic Y_lm(theta, phi)
    [[nodiscard]] std::complex<double> spherical_harmonic(double theta, double phi) const;

    // Full wavefunction ψ_nlm(r, θ, φ)
    [[nodiscard]] std::complex<double> wavefunction(double r, double theta, double phi) const;

    // Sample a random position from the probability distribution (Metropolis-Hastings)
    [[nodiscard]] std::vector<double> sample_position() const;

    // Get orbital name (1s, 2p, 3d, etc.)
    [[nodiscard]] std::string get_orbital_name() const;

    // Validate quantum numbers
    static bool validate_quantum_numbers(int n, int l, int m);

private:
    QuantumNumbers quantum_numbers_;
    int Z_;  // Atomic number

    // Physical constants
    static constexpr double RYDBERG_H = 13.605693009;  // Rydberg constant in eV
    static constexpr double BOHR_RADIUS = 0.052917721090;  // a₀ in nm
    static constexpr double HC_EV_NM = 1239.84193;  // hc in eV·nm

    // Calculate normalization factor
    [[nodiscard]] double calculate_normalization() const;

    // Associated Laguerre polynomial L^{2l+1}_{n-l-1}(ρ)
    [[nodiscard]] double associated_laguerre(int n, int l, double x) const;

    // Factorial with overflow protection
    [[nodiscard]] double factorial(int n) const;

    // Legendre polynomial P_l^m(x)
    [[nodiscard]] double legendre_poly(int l, int m, double x) const;
};

// =============================================================================
// Hydrogen Model Utilities
// =============================================================================

namespace hydrogen_utils {
    // Rydberg formula for wavelength
    [[nodiscard]] double rydberg_wavelength(int n_initial, int n_final, int Z = 1);

    // Energy difference between levels
    [[nodiscard]] double energy_difference(int n_initial, int n_final, int Z = 1);

    // Balmer series wavelengths (n_final = 2)
    [[nodiscard]] double balmer_alpha();   // n=3 → n=2 (656.3 nm)
    [[nodiscard]] double balmer_beta();    // n=4 → n=2 (486.1 nm)
    [[nodiscard]] double balmer_gamma();   // n=5 → n=2 (434.0 nm)
    [[nodiscard]] double balmer_delta();   // n=6 → n=2 (410.2 nm)
}