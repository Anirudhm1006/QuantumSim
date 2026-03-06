#include "Laser.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

// =============================================================================
// Laser System Implementation
// =============================================================================

LaserSystem::LaserSystem()
    : scheme_(LaserLevelScheme::THREE_LEVEL)
    , photon_density_(0.0)
{
    // Initialize default HeNe transition
    transition_ = helium_neon_transition();
    photon_energy_J_ = transition_.energy * LaserParameters::EV_TO_J;

    // Initialize populations: 4 levels (N0, N1, N2, N3)
    populations_.resize(4, 0.0);
    populations_[0] = 1.0;  // Ground state

    // Set default parameters
    params_.length = 0.1;  // 10 cm cavity
    params_.refractive_index = 1.0;
    params_.cross_section = 1e-23;
    params_.upper_lifetime = 1e-3;
    params_.lower_lifetime = 1e-6;
    params_.reflectivity_out = 0.95;
    params_.loss_coefficient = 0.02;
}

LaserSystem::LaserSystem(LaserLevelScheme scheme)
    : scheme_(scheme)
    , photon_density_(0.0)
{
    // Initialize populations based on level scheme
    if (scheme == LaserLevelScheme::TWO_LEVEL) {
        populations_.resize(2, 0.0);
        transition_ = helium_neon_transition();
    } else if (scheme == LaserLevelScheme::THREE_LEVEL) {
        populations_.resize(3, 0.0);
        transition_ = ruby_transition();
    } else {
        populations_.resize(4, 0.0);
        transition_ = nd_yag_transition();
    }

    populations_[0] = 1.0;  // Ground state
    photon_energy_J_ = transition_.energy * LaserParameters::EV_TO_J;
}

LaserSystem::LaserSystem(const LaserTransition& transition)
    : scheme_(LaserLevelScheme::FOUR_LEVEL)
    , transition_(transition)
    , photon_density_(0.0)
{
    populations_.resize(4, 0.0);
    populations_[0] = 1.0;
    photon_energy_J_ = transition.energy * LaserParameters::EV_TO_J;
}

std::vector<std::complex<double>> LaserSystem::get_wavefunction() const {
    // Return populations as "amplitudes" (squared gives populations)
    std::vector<std::complex<double>> wf(populations_.size(), {0.0, 0.0});

    for (size_t i = 0; i < populations_.size() && i < wf.size(); ++i) {
        wf[i] = {std::sqrt(populations_[i]), 0.0};
    }

    return wf;
}

double LaserSystem::get_probability_density(const std::vector<double>& position) const {
    if (position.empty()) {
        // Return total population
        double total = 0.0;
        for (double p : populations_) {
            total += p;
        }
        return total;
    }

    // Position[0] specifies level
    size_t level = static_cast<size_t>(position[0]);
    if (level >= populations_.size()) {
        throw std::out_of_range("Level index out of range");
    }

    return populations_[level];
}

std::string LaserSystem::get_state_descriptor() const {
    std::ostringstream oss;
    oss << "Laser(";

    switch (scheme_) {
        case LaserLevelScheme::TWO_LEVEL:
            oss << "2-level";
            break;
        case LaserLevelScheme::THREE_LEVEL:
            oss << "3-level";
            break;
        case LaserLevelScheme::FOUR_LEVEL:
            oss << "4-level";
            break;
    }

    oss << ", λ=" << std::fixed << std::setprecision(1) << transition_.wavelength
        << " nm, ΔN=" << get_population_inversion()
        << ", I=" << std::scientific << get_intensity() << " W/m²)";

    return oss.str();
}

// =============================================================================
// Population Management
// =============================================================================

void LaserSystem::set_population(int level, double population) {
    if (level < 0 || static_cast<size_t>(level) >= populations_.size()) {
        throw std::out_of_range("Invalid level index");
    }
    populations_[level] = population;
}

double LaserSystem::get_population(int level) const {
    if (level < 0 || static_cast<size_t>(level) >= populations_.size()) {
        throw std::out_of_range("Invalid level index");
    }
    return populations_[level];
}

double LaserSystem::get_population_inversion() const {
    if (populations_.size() < 2) return 0.0;

    // N2 - N1 (upper - lower laser levels)
    int upper = transition_.upper_level;
    int lower = transition_.lower_level;

    if (upper < 0 || lower < 0 ||
        static_cast<size_t>(upper) >= populations_.size() ||
        static_cast<size_t>(lower) >= populations_.size()) {
        return 0.0;
    }

    return populations_[upper] - populations_[lower];
}

double LaserSystem::get_inversion_ratio() const {
    double lower = get_population(transition_.lower_level);
    if (lower < 1e-10) return 1e10;  // Infinite ratio
    return get_population(transition_.upper_level) / lower;
}

// =============================================================================
// Pumping
// =============================================================================

void LaserSystem::set_pump_rate(double rate) {
    params_.pump_rate = rate;
}

void LaserSystem::pump(double dt) {
    if (dt <= 0.0) return;

    // Pump atoms from ground state to upper levels
    double pump_delta = params_.pump_rate * dt * params_.pump_efficiency;

    // Distribute pumped atoms based on level scheme
    switch (scheme_) {
        case LaserLevelScheme::TWO_LEVEL:
            // Pump directly to upper laser level
            if (populations_.size() >= 2) {
                double from_ground = std::min(pump_delta, populations_[0]);
                populations_[0] -= from_ground;
                populations_[1] += from_ground;
            }
            break;

        case LaserLevelScheme::THREE_LEVEL:
            // Pump to highest level, which decays to upper laser level
            if (populations_.size() >= 3) {
                double from_ground = std::min(pump_delta, populations_[0]);
                populations_[0] -= from_ground;
                populations_[2] += from_ground;  // To pump level

                // Fast decay from pump level (2) to upper laser level (1)
                double decay = populations_[2] * dt / 1e-12;  // Very fast
                decay = std::min(decay, populations_[2]);
                populations_[2] -= decay;
                populations_[1] += decay;
            }
            break;

        case LaserLevelScheme::FOUR_LEVEL:
            // Pump to highest level, decays to upper laser level
            if (populations_.size() >= 4) {
                double from_ground = std::min(pump_delta, populations_[0]);
                populations_[0] -= from_ground;
                populations_[3] += from_ground;

                // Fast decay from pump level (3) to upper laser level (2)
                double decay = populations_[3] * dt / 1e-12;
                decay = std::min(decay, populations_[3]);
                populations_[3] -= decay;
                populations_[2] += decay;

                // Fast decay from lower laser level (1) to ground (0)
                double decay_lower = populations_[1] * dt / params_.lower_lifetime;
                decay_lower = std::min(decay_lower, populations_[1]);
                populations_[1] -= decay_lower;
                populations_[0] += decay_lower;
            }
            break;
    }
}

// =============================================================================
// Transition Properties
// =============================================================================

void LaserSystem::set_transition(const LaserTransition& transition) {
    transition_ = transition;
    photon_energy_J_ = transition.energy * LaserParameters::EV_TO_J;
}

double LaserSystem::get_population_difference() const {
    return get_population_inversion();
}

double LaserSystem::get_gain_coefficient(double photon_density) const {
    // g = σ × (N₂ - N₁) × ρ(ν) for small signal
    // Or more generally: g = σ × ΔN (gain coefficient)

    double sigma = params_.cross_section;
    double delta_N = get_population_inversion();

    // Include saturation effects
    double saturation_density = 1.0 / (sigma * params_.upper_lifetime * LaserParameters::C_LIGHT);

    // g = σ × ΔN / (1 + ρ/ρ_sat)
    double gain = sigma * delta_N;
    if (saturation_density > 0.0) {
        gain /= (1.0 + photon_density / saturation_density);
    }

    return gain;
}

double LaserSystem::get_gain_per_pass() const {
    // Gain per round trip = exp(2 × g × L)
    double g = get_gain_coefficient(photon_density_);
    double gain = std::exp(2.0 * g * params_.length);

    return gain;
}

// =============================================================================
// Threshold Conditions
// =============================================================================

bool LaserSystem::is_above_threshold() const {
    return get_gain_per_pass() >= 1.0 / (params_.reflectivity_out * (1.0 - params_.loss_coefficient));
}

double LaserSystem::get_threshold_inversion() const {
    // Threshold condition: R₁R₂exp(2gL) >= (1-L)²
    // Solving for ΔN: g_th = σΔN >= (1/2L) × ln(1/(R₁R₂(1-L)²))

    double R_product = params_.reflectivity_in * params_.reflectivity_out;
    double L = params_.loss_coefficient;

    double g_th = (1.0 / (2.0 * params_.length)) *
                 std::log(1.0 / (R_product * (1.0 - L) * (1.0 - L)));

    // ΔN_th = g_th / σ
    if (params_.cross_section > 0.0) {
        return g_th / params_.cross_section;
    }
    return 0.0;
}

double LaserSystem::get_threshold_pump_rate() const {
    // N_th / τ_upper (accounting for quantum efficiency)
    double N_th = get_threshold_inversion();
    return N_th / (params_.upper_lifetime * params_.pump_efficiency);
}

// =============================================================================
// Output Power Calculations
// =============================================================================

double LaserSystem::get_output_power() const {
    if (!is_above_threshold()) return 0.0;

    // P = η × (N₂ - N₁) × V × hν
    // where η is the output efficiency

    double delta_N = get_population_inversion();
    if (delta_N <= 0.0) return 0.0;

    // Cavity volume (simplified: assume uniform mode)
    double volume = params_.length * 1e-6;  // 1 mm² cross-section

    // Output power with efficiency factor
    double output_coupling = (1.0 - params_.reflectivity_out);

    double power = output_coupling * delta_N * volume * photon_energy_J_ *
                   LaserParameters::C_LIGHT * params_.cross_section;

    return std::max(0.0, power);
}

double LaserSystem::get_slope_efficiency() const {
    // Slope efficiency = (P_out - P_th) / (P_pump - P_pump_th)
    // Simplified: dP_out/dP_pump

    double P_out = get_output_power();
    if (P_out <= 0.0) return 0.0;

    double P_pump = get_electrical_input_power();
    double P_pump_th = get_threshold_pump_rate() * photon_energy_J_;

    if (P_pump <= P_pump_th) return 0.0;

    return P_out / (P_pump - P_pump_th);
}

double LaserSystem::get_electrical_input_power() const {
    // Electrical input = pump photons × photon energy
    return params_.pump_rate * photon_energy_J_;
}

// =============================================================================
// Cavity Dynamics
// =============================================================================

void LaserSystem::simulate_step(double dt) {
    // Apply pumping
    pump(dt);

    // Radiative decay
    double upper = transition_.upper_level;
    double lower = transition_.lower_level;

    if (upper >= 0 && static_cast<size_t>(upper) < populations_.size()) {
        // Spontaneous and stimulated emission from upper level
        double decay_rate = 1.0 / params_.upper_lifetime;
        double decay = populations_[upper] * decay_rate * dt;
        decay = std::min(decay, populations_[upper]);

        // Stimulated emission proportional to photon density
        double stim_emission = decay * photon_density_ * params_.cross_section * dt * params_.length;
        stim_emission = std::min(stim_emission, populations_[upper] - decay);

        populations_[upper] -= decay;
        if (lower >= 0 && static_cast<size_t>(lower) < populations_.size()) {
            populations_[lower] += decay - stim_emission;
        }

        // Update photon density from stimulated emission
        photon_density_ += stim_emission / (params_.length * 1e-6);  // Normalize

        // Spontaneous emission contributes to photon density
        photon_density_ += (decay - stim_emission) * 0.01;  // Very small fraction
    }

    // Cavity losses
    double cavity_decay = params_.get_photon_lifetime();
    if (cavity_decay > 0.0) {
        photon_density_ *= std::exp(-dt / cavity_decay);
    }

    // Ensure populations are non-negative and normalized
    double total_pop = 0.0;
    for (double& p : populations_) {
        p = std::max(0.0, p);
        total_pop += p;
    }
    if (total_pop > 0.0) {
        for (double& p : populations_) {
            p /= total_pop;
        }
    }

    // Clamp photon density
    photon_density_ = std::max(0.0, photon_density_);
}

double LaserSystem::get_intensity() const {
    // Intensity = photon density × photon energy × c
    return photon_density_ * photon_energy_J_ * LaserParameters::C_LIGHT;
}

// =============================================================================
// Laser Parameters
// =============================================================================

void LaserSystem::set_parameters(const LaserParameters& params) {
    params_ = params;
}

double LaserSystem::calculate_B12() const {
    // B₁₂ = c² × σ / (8π × hν × ν²) - derived from cross-section
    double nu = LaserParameters::C_LIGHT / (transition_.wavelength * 1e-9);
    double hnu = photon_energy_J_;

    double B = LaserParameters::C_LIGHT * LaserParameters::C_LIGHT * params_.cross_section /
               (8.0 * M_PI * hnu * nu * nu);

    return B;
}

double LaserSystem::calculate_energy_eV(double wavelength_nm) const {
    // E = hc/λ in eV
    return 1239.84193 / wavelength_nm;
}

// =============================================================================
// Common Laser Transitions
// =============================================================================

LaserTransition LaserSystem::ruby_transition() {
    LaserTransition t;
    t.upper_level = 2;
    t.lower_level = 1;
    t.wavelength = 694.3;  // nm (Ruby laser)
    t.energy = laser_utils::wavelength_to_energy_eV(694.3);
    t.lifetime = 3e-3;  // 3 ms
    t.sigma = 2e-24;  // m²
    return t;
}

LaserTransition LaserSystem::nd_yag_transition() {
    LaserTransition t;
    t.upper_level = 2;
    t.lower_level = 1;
    t.wavelength = 1064.0;  // nm
    t.energy = laser_utils::wavelength_to_energy_eV(1064.0);
    t.lifetime = 1e-3;  // 1 ms
    t.sigma = 4e-23;  // m²
    return t;
}

LaserTransition LaserSystem::helium_neon_transition() {
    LaserTransition t;
    t.upper_level = 2;
    t.lower_level = 1;
    t.wavelength = 632.8;  // nm (red)
    t.energy = laser_utils::wavelength_to_energy_eV(632.8);
    t.lifetime = 1e-7;  // 100 ns
    t.sigma = 1e-23;  // m²
    return t;
}

LaserTransition LaserSystem::semiconductor_laser_transition() {
    LaserTransition t;
    t.upper_level = 2;
    t.lower_level = 1;
    t.wavelength = 808.0;  // nm (typical diode)
    t.energy = laser_utils::wavelength_to_energy_eV(808.0);
    t.lifetime = 1e-9;  // 1 ns
    t.sigma = 1e-20;  // m² (larger cross-section)
    return t;
}

// =============================================================================
// Helper Functions for Parameters
// =============================================================================

double LaserParameters::get_cavity_round_trip() const {
    return 2.0 * length * refractive_index / C_LIGHT;
}

double LaserParameters::get_round_trip_time() const {
    return get_cavity_round_trip();
}

double LaserParameters::get_photon_lifetime() const {
    // τ_photon = -L / (c × ln(R₁R₂(1-L)²))
    double R_product = reflectivity_in * reflectivity_out * (1.0 - loss_coefficient);
    double decay_rate = -C_LIGHT / (length * refractive_index) * std::log(R_product);

    return 1.0 / decay_rate;
}

// =============================================================================
// Laser Utilities Implementation
// =============================================================================

namespace laser_utils {
    double wavelength_to_frequency(double wavelength_nm) {
        return LaserParameters::C_LIGHT / (wavelength_nm * 1e-9);
    }

    double wavelength_to_energy_eV(double wavelength_nm) {
        return 1239.84193 / wavelength_nm;
    }

    double cross_section_from_B(double B, double wavelength_nm, double n) {
        double nu = wavelength_to_frequency(wavelength_nm);
        double h = LaserParameters::PLANCK;
        double hnu = h * nu;

        // σ = hν × B / (n² × c)
        return hnu * B / (n * n * LaserParameters::C_LIGHT);
    }

    double calculate_gain(double N2, double N1, double sigma, double photon_density) {
        return sigma * (N2 - N1);
    }

    double calculate_threshold_pump(double gain_coefficient, double loss_coefficient,
                                      double cross_section, double lifetime) {
        // P_th = N_th / τ
        // N_th = (1/σL) × ln(1/(R(1-L)²))
        double N_th = gain_coefficient / (cross_section * lifetime);
        return N_th / lifetime;
    }

    double saturation_intensity(double sigma, double lifetime) {
        // I_sat = hν / (σ × τ)
        double h = LaserParameters::PLANCK;
        double wavelength = 1e-6;  // Assume 1000 nm
        double hnu = h * LaserParameters::C_LIGHT / wavelength;

        return hnu / (sigma * lifetime);
    }

    double cavity_decay_rate(double loss, double length) {
        // γ_c = c × loss / (2 × L)
        return LaserParameters::C_LIGHT * loss / (2.0 * length);
    }

    double heterodyne_beat_frequency(double freq1, double freq2) {
        return std::abs(freq1 - freq2);
    }

    double calculate_energy_eV(double wavelength_nm) {
        return wavelength_to_energy_eV(wavelength_nm);
    }
}