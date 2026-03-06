#include "WavePacketRenderer.hpp"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>

namespace render_engine {

// =============================================================================
// Constructor / Destructor
// =============================================================================

WavePacketRenderer::WavePacketRenderer()
    : x0_1_(-3.0)
    , sigma_1_(1.0)
    , time_1_(0.0)
    , x0_2_(3.0)
    , sigma_2_(1.0)
    , time_2_(0.0)
    , superposition_enabled_(false)
    , x_min_(-8.0)
    , x_max_(8.0)
    , num_points_(100)
    , vertical_resolution_(20)
{
    // Default bounds
}

WavePacketRenderer::~WavePacketRenderer() {
    // Cleanup if needed
}

// =============================================================================
// Parameter Updates
// =============================================================================

void WavePacketRenderer::update_wave_packet_1(double x0, double sigma, double time) {
    x0_1_ = x0;
    sigma_1_ = sigma;
    time_1_ = time;
}

void WavePacketRenderer::update_wave_packet_2(double x0, double sigma, double time) {
    x0_2_ = x0;
    sigma_2_ = sigma;
    time_2_ = time;
}

void WavePacketRenderer::set_superposition_mode(bool enabled) {
    superposition_enabled_ = enabled;
}

void WavePacketRenderer::set_bounds(double x_min, double x_max, int num_points) {
    x_min_ = x_min;
    x_max_ = x_max;
    num_points_ = num_points;
}

double WavePacketRenderer::get_probability_at(double x, int packet_id) const {
    if (packet_id == 1) {
        return gaussian_density(x, x0_1_, sigma_1_);
    } else if (packet_id == 2) {
        return gaussian_density(x, x0_2_, sigma_2_);
    }
    return 0.0;
}

// =============================================================================
// Gaussian Helper
// =============================================================================

double WavePacketRenderer::gaussian_density(double x, double x0, double sigma) const {
    double diff = x - x0;
    return std::exp(-(diff * diff) / (2.0 * sigma * sigma));
}

// =============================================================================
// Rendering
// =============================================================================

void WavePacketRenderer::render(const Camera3D& camera) {
    (void)camera; // Camera can be used for advanced effects

    if (superposition_enabled_) {
        // Draw both packets and interference
        draw_gaussian_envelope(x0_1_, sigma_1_, time_1_, {100, 200, 255, 200});
        draw_gaussian_envelope(x0_2_, sigma_2_, time_2_, {255, 200, 100, 200});
        draw_interference_pattern();
    } else {
        // Draw individual packets
        draw_gaussian_envelope(x0_1_, sigma_1_, time_1_, {100, 200, 255, 255});
        draw_gaussian_envelope(x0_2_, sigma_2_, time_2_, {255, 200, 100, 255});
    }

    draw_probability_points();
}

void WavePacketRenderer::draw_gaussian_envelope(double x0, double sigma, double time, Color color) {
    (void)time; // Could be used for animation

    float step = static_cast<float>((x_max_ - x_min_) / num_points_);
    float scale = 3.0f; // Vertical scale factor

    // Draw the gaussian curve as a series of line segments
    Vector3 previous_point = {
        static_cast<float>(x_min_),
        scale * static_cast<float>(gaussian_density(x_min_, x0, sigma)),
        0.0f
    };

    for (int i = 1; i <= num_points_; ++i) {
        float x = static_cast<float>(x_min_) + i * step;
        float y = scale * static_cast<float>(gaussian_density(x, x0, sigma));

        Vector3 current_point = {x, y, 0.0f};
        DrawLine3D(previous_point, current_point, color);

        previous_point = current_point;
    }

    // Draw baseline
    DrawLine3D(
        {static_cast<float>(x_min_), 0.0f, 0.0f},
        {static_cast<float>(x_max_), 0.0f, 0.0f},
        {80, 80, 80, 150}
    );

    // Draw center marker
    Vector3 center_marker = {
        static_cast<float>(x0),
        0.0f,
        0.0f
    };
    DrawSphere(center_marker, 0.15f, color);
}

void WavePacketRenderer::draw_interference_pattern() {
    float step = static_cast<float>((x_max_ - x_min_) / num_points_);
    float scale = 3.0f; // Vertical scale factor

    // Calculate interference pattern: |psi1 + psi2|^2 = |psi1|^2 + |psi2|^2 + 2*Re(psi1*conj(psi2))
    // For real wavefunctions, this simplifies

    Vector3 previous_point = {
        static_cast<float>(x_min_),
        0.0f,
        0.2f // Slightly offset in Z for visibility
    };

    for (int i = 1; i <= num_points_; ++i) {
        float x = static_cast<float>(x_min_) + i * step;

        // Calculate probability densities
        double psi1 = gaussian_density(x, x0_1_, sigma_1_);
        double psi2 = gaussian_density(x, x0_2_, sigma_2_);

        // Total probability with interference
        // |psi1 + psi2|^2 = psi1^2 + psi2^2 + 2*psi1*psi2 (for real wavefunctions)
        double total_prob = psi1 * psi1 + psi2 * psi2 + 2.0 * psi1 * psi2;

        float y = scale * static_cast<float>(total_prob);

        Vector3 current_point = {x, y, 0.2f};
        DrawLine3D(previous_point, current_point, {255, 100, 255, 255});

        previous_point = current_point;
    }
}

void WavePacketRenderer::draw_probability_points() {
    float step = static_cast<float>((x_max_ - x_min_) / num_points_);
    float sphere_scale = 0.08f;

    // Draw probability points for packet 1
    for (int i = 0; i < num_points_; i += 5) {
        float x = static_cast<float>(x_min_) + i * step;
        float prob1 = static_cast<float>(gaussian_density(x, x0_1_, sigma_1_));
        float y = 3.0f * prob1;

        // Color based on probability intensity
        unsigned char alpha = static_cast<unsigned char>(100 + 155 * prob1);
        Color color = {100, 200, 255, alpha};

        Vector3 pos = {x, y, -0.5f};
        DrawSphere(pos, sphere_scale * prob1, color);
    }

    // Draw probability points for packet 2
    for (int i = 0; i < num_points_; i += 5) {
        float x = static_cast<float>(x_min_) + i * step;
        float prob2 = static_cast<float>(gaussian_density(x, x0_2_, sigma_2_));
        float y = 3.0f * prob2;

        unsigned char alpha = static_cast<unsigned char>(100 + 155 * prob2);
        Color color = {255, 200, 100, alpha};

        Vector3 pos = {x, y, 0.5f};
        DrawSphere(pos, sphere_scale * prob2, color);
    }

    // Draw interference points if superposition is enabled
    if (superposition_enabled_) {
        for (int i = 0; i < num_points_; i += 5) {
            float x = static_cast<float>(x_min_) + i * step;

            double psi1 = gaussian_density(x, x0_1_, sigma_1_);
            double psi2 = gaussian_density(x, x0_2_, sigma_2_);
            double total = psi1 * psi1 + psi2 * psi2 + 2.0 * psi1 * psi2;

            float prob = static_cast<float>(total);
            float y = 3.0f * prob;

            unsigned char alpha = static_cast<unsigned char>(100 + 155 * prob);
            Color color = {255, 100, 255, alpha};

            Vector3 pos = {x, y, 0.0f};
            DrawSphere(pos, sphere_scale * prob * 1.2f, color);
        }
    }
}

} // namespace render_engine