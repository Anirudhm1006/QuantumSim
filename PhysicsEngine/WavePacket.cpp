#include "WavePacket.hpp"
#include <sstream>
#include <iomanip>

// =============================================================================
// Gaussian Wave Packet Implementation
// =============================================================================

WavePacket::WavePacket(double x0, double v, double sigma, double k0, double omega)
    : x0_(x0)
    , v_(v)
    , sigma_(sigma)
    , k0_(k0)
    , omega_(omega)
    , t_(0.0)
{
}

std::complex<double> WavePacket::evaluate(double x, double t) const {
    // Calculate the center of the wave packet at time t
    double x_center = x0_ + v_ * t;

    // Gaussian envelope: exp(-(x - x_center)² / 4σ²)
    double exponent_envelope = -std::pow(x - x_center, 2) / (4.0 * sigma_ * sigma_);
    double envelope = std::exp(exponent_envelope);

    // Phase factor: exp(i(k0*x - ω*t))
    double phase = k0_ * x - omega_ * t;
    std::complex<double> phase_factor(0.0, phase);

    // Normalization factor
    double norm = calculate_normalization();

    // Full wavefunction: A * envelope * exp(i(kx - ωt))
    return norm * envelope * std::exp(phase_factor);
}

std::vector<std::complex<double>> WavePacket::get_wavefunction() const {
    // Return wavefunction sampled at discrete points
    std::vector<std::complex<double>> wf(100);

    // Sample over range [-20, 20] for demonstration
    double x_min = -20.0;
    double x_max = 20.0;
    double dx = (x_max - x_min) / 99.0;

    for (int i = 0; i < 100; ++i) {
        double x = x_min + i * dx;
        wf[i] = evaluate(x, t_);
    }

    return wf;
}

double WavePacket::get_probability_density(const std::vector<double>& position) const {
    if (position.empty()) return 0.0;

    double x = position[0];
    std::complex<double> psi = evaluate(x, t_);

    // |ψ|² = ψ*ψ = |ψ|²
    return std::norm(psi);
}

std::string WavePacket::get_state_descriptor() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "WavePacket(x0=" << x0_ << ", v=" << v_
        << ", σ=" << sigma_ << ", k0=" << k0_ << ", ω=" << omega_ << ")";
    return oss.str();
}

double WavePacket::get_group_velocity() const {
    // v_g = dω/dk = ħk₀/m (in atomic units, ħ=1, m=1)
    return HBAR * k0_ / MASS;
}

double WavePacket::get_phase_velocity() const {
    // v_p = ω/k₀
    if (std::abs(k0_) < 1e-10) return 0.0;
    return omega_ / k0_;
}

double WavePacket::calculate_normalization() const {
    // Normalization factor for Gaussian wave packet:
    // A = 1 / (2πσ²)^(1/4)
    return 1.0 / std::pow(2.0 * M_PI * sigma_ * sigma_, 0.25);
}

WavePacket WavePacket::create_superposition(
    const WavePacket& wp1,
    const WavePacket& wp2,
    double phase_shift
) {
    // For superposition, we'll use the average of parameters
    // This creates a new wave packet representing the combined state
    double x0 = (wp1.x0_ + wp2.x0_) / 2.0;
    double v = (wp1.v_ + wp2.v_) / 2.0;
    double sigma = std::min(wp1.sigma_, wp2.sigma_); // Use narrower width
    double k0 = (wp1.k0_ + wp2.k0_) / 2.0;
    double omega = (wp1.omega_ + wp2.omega_) / 2.0;

    WavePacket result(x0, v, sigma, k0, omega);
    return result;
}

double WavePacket::interference_at(
    const WavePacket& wp1,
    const WavePacket& wp2,
    double x,
    double t
) {
    std::complex<double> psi1 = wp1.evaluate(x, t);
    std::complex<double> psi2 = wp2.evaluate(x, t);

    // |ψ_total|² = |ψ1 + ψ2|² = |ψ1|² + |ψ2|² + 2*Re(ψ1*ψ2)
    double psi1_sq = std::norm(psi1);
    double psi2_sq = std::norm(psi2);
    double interference = 2.0 * std::real(std::conj(psi1) * psi2);

    return psi1_sq + psi2_sq + interference;
}

// =============================================================================
// Wave Packet Propagation Utilities Implementation
// =============================================================================

namespace wave_packet_propagation {
    double superposition_probability(
        const std::vector<WavePacket>& packets,
        double x,
        double t
    ) {
        std::complex<double> psi_total(0.0, 0.0);

        for (const auto& wp : packets) {
            psi_total += wp.evaluate(x, t);
        }

        return std::norm(psi_total);
    }

    bool is_constructive_interference(
        const WavePacket& wp1,
        const WavePacket& wp2,
        double x,
        double t
    ) {
        std::complex<double> psi1 = wp1.evaluate(x, t);
        std::complex<double> psi2 = wp2.evaluate(x, t);

        // Constructive: real part of ψ1*ψ2 > 0
        double interference = std::real(std::conj(psi1) * psi2);
        return interference > 0.0;
    }

    double interference_visibility(
        const WavePacket& wp1,
        const WavePacket& wp2,
        double x_min,
        double x_max,
        double t
    ) {
        double p_max = -1.0;
        double p_min = std::numeric_limits<double>::infinity();

        // Sample across the range
        const int num_samples = 1000;
        double dx = (x_max - x_min) / (num_samples - 1);

        for (int i = 0; i < num_samples; ++i) {
            double x = x_min + i * dx;
            double p = WavePacket::interference_at(wp1, wp2, x, t);

            p_max = std::max(p_max, p);
            p_min = std::min(p_min, p);
        }

        // Visibility V = (P_max - P_min) / (P_max + P_min)
        if (p_max + p_min > 0.0) {
            return (p_max - p_min) / (p_max + p_min);
        }
        return 0.0;
    }
} // namespace wave_packet_propagation

nlohmann::json WavePacket::to_json() const {
    return {
        {"type", "WavePacket"},
        {"x0", x0_},
        {"v", v_},
        {"sigma", sigma_},
        {"k0", k0_},
        {"omega", omega_}
    };
}