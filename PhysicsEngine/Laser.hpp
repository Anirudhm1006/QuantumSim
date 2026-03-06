#pragma once

#include <complex>
#include <vector>
#include <string>
#include <array>
#include <cmath>
#include <optional>
#include "IQuantumObject.hpp"

// =============================================================================
// Laser Physics Module
// =============================================================================
// Implements:
// - Population inversion: N₂ > N₁ (upper state > lower state)
// - Stimulated emission coefficient: B₁₂
// - Gain coefficient: g = (N₂ - N₁) × σ × ρ(ν)
// - Three-level/four-level laser system simulation
// - Threshold condition: gain >= losses
// - Output power calculation: P = η × (N₂ - N₁) × V × hν
// =============================================================================

// Laser system type
enum class LaserLevelScheme {
    TWO_LEVEL,      // Simple two-level system (not sustainable)
    THREE_LEVEL,    // Three-level laser (Ruby laser: 690 nm)
    FOUR_LEVEL      // Four-level laser (Nd:YAG: 1064 nm, HeNe: 632.8 nm)
};

// Laser transition details
struct LaserTransition {
    int upper_level;    // Upper laser level (n₂)
    int lower_level;    // Lower laser level (n₁)
    double wavelength;  // Transition wavelength in nm
    double energy;      // Transition energy in eV
    double lifetime;    // Upper level lifetime in seconds
    double sigma;       // Emission cross-section in m²

    LaserTransition()
        : upper_level(2), lower_level(1), wavelength(632.8),
          energy(0.0), lifetime(1e-8), sigma(1e-23) {}
};

// Laser system parameters
struct LaserParameters {
    // Physical constants
    static constexpr double PLANCK = 6.62607015e-34;    // J·s
    static constexpr double HBAR = 1.054571817e-34;     // J·s
    static constexpr double C_LIGHT = 299792458.0;     // m/s
    static constexpr double EV_TO_J = 1.602176634e-19;  // J/eV

    // Material properties
    double refractive_index = 1.0;     // Medium refractive index
    double length = 0.1;                // Cavity length in m
    double cross_section = 1e-23;     // Emission cross-section in m²
    double upper_lifetime = 1e-3;     // Upper state lifetime in s
    double lower_lifetime = 1e-6;     // Lower state lifetime in s

    // Pumping parameters
    double pump_rate = 0.0;            // Pump rate (photons/s)
    double pump_efficiency = 0.5;      // Pumping quantum efficiency

    // Cavity parameters
    double reflectivity_in = 1.0;     // Input mirror reflectivity
    double reflectivity_out = 0.95;   // Output coupler reflectivity
    double loss_coefficient = 0.01;   // Internal loss per pass

    // Derived parameters
    double get_cavity_round_trip() const;
    double get_round_trip_time() const;
    double get_photon_lifetime() const;
};

class LaserSystem : public IQuantumObject {
public:
    // Constructors
    LaserSystem();
    explicit LaserSystem(LaserLevelScheme scheme);
    explicit LaserSystem(const LaserTransition& transition);

    // Destructor
    ~LaserSystem() override = default;

    // IQuantumObject interface
    [[nodiscard]] std::vector<std::complex<double>> get_wavefunction() const override;
    [[nodiscard]] double get_probability_density(const std::vector<double>& position) const override;
    [[nodiscard]] std::string get_state_descriptor() const override;

    // Population management
    void set_population(int level, double population);
    [[nodiscard]] double get_population(int level) const;
    [[nodiscard]] double get_population_inversion() const;  // N₂ - N₁
    [[nodiscard]] double get_inversion_ratio() const;       // N₂ / N₁

    // Pumping
    void set_pump_rate(double rate);
    [[nodiscard]] double get_pump_rate() const { return params_.pump_rate; }
    void pump(double dt);  // Time step pumping

    // Laser transition properties
    void set_transition(const LaserTransition& transition);
    [[nodiscard]] const LaserTransition& get_transition() const { return transition_; }

    // Gain calculations
    [[nodiscard]] double get_population_difference() const;
    [[nodiscard]] double get_gain_coefficient(double photon_density) const;
    [[nodiscard]] double get_gain_per_pass() const;

    // Threshold conditions
    [[nodiscard]] bool is_above_threshold() const;
    [[nodiscard]] double get_threshold_inversion() const;
    [[nodiscard]] double get_threshold_pump_rate() const;

    // Output power calculations
    [[nodiscard]] double get_output_power() const;
    [[nodiscard]] double get_slope_efficiency() const;
    [[nodiscard]] double get_electrical_input_power() const;

    // Cavity dynamics
    void simulate_step(double dt);
    [[nodiscard]] double get_photon_density() const { return photon_density_; }
    [[nodiscard]] double get_intensity() const;

    // Laser parameters
    void set_parameters(const LaserParameters& params);
    [[nodiscard]] const LaserParameters& get_parameters() const { return params_; }

    // Efficiency calculations
    [[nodiscard]] double get_quantum_efficiency() const;
    [[nodiscard]] double get_power_efficiency() const;

    // Common laser presets
    static LaserTransition ruby_transition();
    static LaserTransition nd_yag_transition();
    static LaserTransition helium_neon_transition();
    static LaserTransition semiconductor_laser_transition();

private:
    LaserLevelScheme scheme_;
    LaserParameters params_;
    LaserTransition transition_;

    // Population levels (N₀, N₁, N₂, N�� for up to 4-level system)
    std::vector<double> populations_;

    // Photon density in cavity
    double photon_density_;

    // Derived quantity: photon energy
    double photon_energy_J_;

    // Helper: calculate Einstein B coefficient from cross-section
    [[nodiscard]] double calculate_B12() const;

    // Helper: calculate transition energy from wavelength
    [[nodiscard]] double calculate_energy_eV(double wavelength_nm) const;

    // Time evolution for different level schemes
    void simulate_three_level(double dt);
    void simulate_four_level(double dt);
};

// =============================================================================
// Laser Utilities
// =============================================================================

namespace laser_utils {
    // Convert wavelength (nm) to frequency (Hz)
    [[nodiscard]] double wavelength_to_frequency(double wavelength_nm);

    // Convert wavelength (nm) to photon energy (eV)
    [[nodiscard]] double wavelength_to_energy_eV(double wavelength_nm);

    // Calculate stimulated emission cross-section from Einstein B
    [[nodiscard]] double cross_section_from_B(double B, double wavelength_nm, double n);

    // Calculate gain from inversion and cross-section
    [[nodiscard]] double calculate_gain(double N2, double N1, double sigma, double photon_density);

    // Calculate threshold pump power
    [[nodiscard]] double calculate_threshold_pump(double gain_coefficient, double loss_coefficient,
                                                   double cross_section, double lifetime);

    // Saturation intensity calculation
    [[nodiscard]] double saturation_intensity(double sigma, double lifetime);

    // Cavity decay rate
    [[nodiscard]] double cavity_decay_rate(double loss, double length);

    // Beat note for heterodyne detection
    [[nodiscard]] double heterodyne_beat_frequency(double freq1, double freq2);
}
