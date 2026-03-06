#include "HydrogenRenderer.hpp"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <cstdlib>

namespace render_engine {

// Constants
static constexpr double RYDBERG_EV = 13.605693009;  // Rydberg constant in eV
static constexpr double BOHR_ANGSTROM = 0.529177210903;  // Bohr radius in Angstroms

// =============================================================================
// Constructor / Destructor
// =============================================================================

HydrogenRenderer::HydrogenRenderer(float scale)
    : n_(1)
    , l_(0)
    , m_(0)
    , time_(0.0)
    , electron_angle_(0.0)
    , quantum_mode_(true)
    , scale_(scale)
    , nucleus_radius_(0.15f)
    , electron_radius_(0.08f)
{
    // Default to ground state
}

HydrogenRenderer::~HydrogenRenderer() {
    // Cleanup if needed
}

// =============================================================================
// State Updates
// =============================================================================

void HydrogenRenderer::set_quantum_numbers(int n, int l, int m) {
    n_ = n;
    l_ = l;
    m_ = m;

    // Validate quantum numbers
    if (n_ < 1) n_ = 1;
    if (l_ >= n_) l_ = n_ - 1;
    if (l_ < 0) l_ = 0;
    if (std::abs(m_) > l_) m_ = l_;
}

void HydrogenRenderer::set_quantum_mode(bool quantum) {
    quantum_mode_ = quantum;
}

void HydrogenRenderer::update_time(double time) {
    time_ = time;

    // Update electron orbital angle (faster for lower n)
    // Angular velocity inversely proportional to n^3 (from Bohr model)
    double angular_velocity = 1.0 / (n_ * n_ * n_);
    electron_angle_ += angular_velocity * time_ * 2.0;

    // Keep angle in [0, 2*PI]
    while (electron_angle_ > 2.0 * M_PI) electron_angle_ -= 2.0 * M_PI;
}

double HydrogenRenderer::get_energy_eV() const {
    // E_n = -R_H / n^2
    return -RYDBERG_EV / (n_ * n_);
}

const char* HydrogenRenderer::get_mode_name() const {
    return quantum_mode_ ? "Quantum (Wavefunction)" : "Classical (Bohr)";
}

// =============================================================================
// Rendering
// =============================================================================

void HydrogenRenderer::render(const Camera3D& camera) {
    (void)camera; // Camera can be used for advanced effects

    draw_nucleus();

    if (quantum_mode_) {
        draw_electron_cloud();
        draw_electron();
    } else {
        draw_bohr_orbits();
        draw_electron();
    }
}

void HydrogenRenderer::draw_nucleus() const {
    // Draw proton (positive charge) - red sphere in center
    Color nucleus_color = {255, 80, 80, 255};
    DrawSphere({0, 0, 0}, nucleus_radius_, nucleus_color);

    // Add a glow effect with a larger, more transparent sphere
    Color glow_color = {255, 100, 100, 80};
    DrawSphere({0, 0, 0}, nucleus_radius_ * 1.5f, glow_color);

    // Add "+" sign label near nucleus
    DrawText("+", -5, -5, 20, RED);
}

void HydrogenRenderer::draw_bohr_orbits() const {
    // Draw Bohr orbits for n = 1 to current n
    Color orbit_color = {100, 150, 255, 180};

    for (int i = 1; i <= n_; ++i) {
        float radius = get_bohr_radius(i) * scale_ * 0.5f;  // Scale for visualization

        // Draw circular orbit
        const int segments = 64;
        Vector3 previous = {radius, 0, 0};

        for (int j = 1; j <= segments; ++j) {
            float angle = (2.0f * PI * j) / segments;
            Vector3 current = {
                radius * std::cos(angle),
                0,
                radius * std::sin(angle)
            };

            DrawLine3D(previous, current, orbit_color);
            previous = current;
        }

        // Label the orbit
        if (i == n_) {
            // Highlight current orbit
            for (int j = 0; j <= segments; j += 8) {
                float angle = (2.0f * PI * j) / segments;
                Vector3 marker = {
                    radius * std::cos(angle),
                    0,
                    radius * std::sin(angle)
                };
                DrawSphere(marker, 0.05f, {150, 200, 255, 255});
            }
        }
    }
}

void HydrogenRenderer::draw_electron_cloud() const {
    // Draw probability cloud using multiple semi-transparent spheres
    // The density is highest near the Bohr radius

    float bohr_radius = get_bohr_radius(n_) * scale_ * 0.5f;

    // Draw multiple probability points
    const int num_points = 200;

    for (int i = 0; i < num_points; ++i) {
        // Random point within a sphere, weighted toward Bohr radius
        float r = bohr_radius * (0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f);
        float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;
        float phi = static_cast<float>(rand()) / RAND_MAX * PI;

        // Weight probability by radial distribution (higher at Bohr radius for n>1)
        float probability_weight = std::exp(-std::pow(r - bohr_radius, 2) / (bohr_radius * bohr_radius * 0.5f));

        if (static_cast<float>(rand()) / RAND_MAX > probability_weight) {
            continue;
        }

        Vector3 pos = {
            r * std::sin(phi) * std::cos(theta),
            r * std::cos(phi),
            r * std::sin(phi) * std::sin(theta)
        };

        // Color based on probability (brighter = more likely)
        unsigned char alpha = static_cast<unsigned char>(50 + 150 * probability_weight);
        Color cloud_color = {100, 180, 255, alpha};

        DrawSphere(pos, 0.03f + 0.05f * probability_weight, cloud_color);
    }

    // Draw orbital shape indicator (torus for p-orbitals, etc.)
    if (l_ > 0) {
        // Draw orbital path
        Color orbital_color = {150, 100, 255, 100};
        float orbital_radius = bohr_radius * 1.2f;

        const int segments = 48;
        Vector3 previous = {orbital_radius, 0, 0};

        for (int i = 1; i <= segments; ++i) {
            float angle = (2.0f * PI * i) / segments;
            Vector3 current = {
                orbital_radius * std::cos(angle),
                orbital_radius * 0.3f * std::sin(angle),  // Flattened for p-orbital
                orbital_radius * std::sin(angle)
            };

            DrawLine3D(previous, current, orbital_color);
            previous = current;
        }
    }
}

void HydrogenRenderer::draw_electron() const {
    float bohr_radius = get_bohr_radius(n_) * scale_ * 0.5f;

    // Position electron on orbit (classical) or at random position (quantum)
    float electron_x, electron_y, electron_z;

    if (quantum_mode_) {
        // Random position based on probability
        float r = bohr_radius * (0.3f + static_cast<float>(rand()) / RAND_MAX * 1.4f);
        float theta = static_cast<float>(electron_angle_);
        float phi = static_cast<float>(rand()) / RAND_MAX * PI;

        // Weight toward Bohr radius
        float prob = std::exp(-std::pow(r - bohr_radius, 2) / (bohr_radius * bohr_radius));
        if (static_cast<float>(rand()) / RAND_MAX > prob) {
            r = bohr_radius;
        }

        electron_x = r * std::sin(phi) * std::cos(theta);
        electron_y = r * std::cos(phi) * 0.5f;  // Flatten in Y for visualization
        electron_z = r * std::sin(phi) * std::sin(theta);
    } else {
        // Classical circular orbit
        electron_x = bohr_radius * std::cos(static_cast<float>(electron_angle_));
        electron_y = 0;
        electron_z = bohr_radius * std::sin(static_cast<float>(electron_angle_));
    }

    Vector3 electron_pos = {electron_x, electron_y, electron_z};

    // Draw electron (blue sphere)
    Color electron_color = {80, 150, 255, 255};
    DrawSphere(electron_pos, electron_radius_, electron_color);

    // Draw electron glow
    Color glow_color = {100, 180, 255, 100};
    DrawSphere(electron_pos, electron_radius_ * 2.0f, glow_color);

    // Draw "-" sign near electron
    int screen_x = static_cast<int>(electron_x * 40 + 400);
    int screen_y = static_cast<int>(-electron_y * 40 + 300);
    if (screen_x > 0 && screen_x < 800 && screen_y > 0 && screen_y < 600) {
        // Only draw if on screen
    }
}

void HydrogenRenderer::draw_transition_arrow(int n_from, int n_to) const {
    if (n_from <= n_to) return;  // Only show emission (going down)

    float r_from = get_bohr_radius(n_from) * scale_ * 0.5f;
    float r_to = get_bohr_radius(n_to) * scale_ * 0.5f;

    // Draw arrow from outer to inner orbit
    Vector3 start = {r_from, 0, 0};
    Vector3 end = {r_to, 0, 0};

    DrawLine3D(start, end, {255, 255, 100, 255});

    // Arrowhead
    Vector3 direction = Vector3Subtract(end, start);
    direction = Vector3Normalize(direction);
    Vector3 arrowhead1 = Vector3Add(end, Vector3Scale({-direction.y, direction.x, 0}, 0.2f));
    Vector3 arrowhead2 = Vector3Add(end, Vector3Scale({direction.y, -direction.x, 0}, 0.2f));

    DrawLine3D(end, arrowhead1, {255, 255, 100, 255});
    DrawLine3D(end, arrowhead2, {255, 255, 100, 255});
}

float HydrogenRenderer::get_bohr_radius(int n) const {
    // a_n = a_0 * n^2
    return static_cast<float>(BOHR_ANGSTROM * n * n);
}

} // namespace render_engine