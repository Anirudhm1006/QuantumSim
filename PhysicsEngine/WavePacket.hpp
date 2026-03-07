#pragma once

#include <cmath>
#include <complex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "IQuantumObject.hpp"

// =============================================================================
// Gaussian Wave Packet for Wave-Particle Duality
// =============================================================================
// Implements the wave packet formula:
// ψ(x,t) = A · exp(-(x - x0 - vt)² / 4σ²) · exp(i(k0x - ωt))
// =============================================================================

class WavePacket : public IQuantumObject {
public:
    // Constructor with wave packet parameters
    WavePacket(double x0, double v, double sigma, double k0, double omega);

    // Destructor
    ~WavePacket() override = default;

    // Evaluate wavefunction at position x and time t
    [[nodiscard]] std::complex<double> evaluate(double x, double t) const;

    // Get wavefunction as vector (for interface compatibility)
    [[nodiscard]] std::vector<std::complex<double>> get_wavefunction() const override;

    // Calculate probability density |ψ(x,t)|² at position x and time t
    [[nodiscard]] double get_probability_density(const std::vector<double>& position) const override;

    [[nodiscard]] std::string get_state_descriptor() const override;
    [[nodiscard]] nlohmann::json to_json() const override;
    [[nodiscard]] std::string get_type_name() const override { return "WavePacket"; }

    // Get group velocity (v_g = dω/dk = ħk₀/m)
    [[nodiscard]] double get_group_velocity() const;

    // Get phase velocity (v_p = ω/k₀)
    [[nodiscard]] double get_phase_velocity() const;

    // Create superposition of two wave packets
    static WavePacket create_superposition(
        const WavePacket& wp1,
        const WavePacket& wp2,
        double phase_shift = 0.0
    );

    // Calculate interference between two wave packets at a point
    [[nodiscard]] static double interference_at(
        const WavePacket& wp1,
        const WavePacket& wp2,
        double x,
        double t
    );

    // Get packet parameters
    [[nodiscard]] double get_x0() const { return x0_; }
    [[nodiscard]] double get_v() const { return v_; }
    [[nodiscard]] double get_sigma() const { return sigma_; }
    [[nodiscard]] double get_k0() const { return k0_; }
    [[nodiscard]] double get_omega() const { return omega_; }

    // Set time for propagation
    void set_time(double t) { t_ = t; }
    [[nodiscard]] double get_time() const { return t_; }

private:
    double x0_;      // Initial center position
    double v_;      // Group velocity
    double sigma_;  // Spatial width (standard deviation)
    double k0_;     // Wave number
    double omega_;  // Angular frequency
    double t_;      // Current time

    // Physical constants (in appropriate units)
    static constexpr double HBAR = 1.0;    // Reduced Planck constant (atomic units)
    static constexpr double MASS = 1.0;    // Particle mass (atomic units)

    // Calculate normalization factor
    [[nodiscard]] double calculate_normalization() const;
};

// =============================================================================
// Wave Packet Propagation Utilities
// =============================================================================

namespace wave_packet_propagation {
    // Calculate probability density for superposition of multiple wave packets
    [[nodiscard]] double superposition_probability(
        const std::vector<WavePacket>& packets,
        double x,
        double t
    );

    // Check if interference is constructive or destructive at a point
    [[nodiscard]] bool is_constructive_interference(
        const WavePacket& wp1,
        const WavePacket& wp2,
        double x,
        double t
    );

    // Calculate visibility of interference pattern (0 = no interference, 1 = max)
    [[nodiscard]] double interference_visibility(
        const WavePacket& wp1,
        const WavePacket& wp2,
        double x_min,
        double x_max,
        double t
    );
} // namespace wave_packet_propagation