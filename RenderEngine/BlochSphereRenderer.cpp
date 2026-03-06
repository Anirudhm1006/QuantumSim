#include "BlochSphereRenderer.hpp"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

namespace render_engine {

// =============================================================================
// Constructor / Destructor
// =============================================================================

BlochSphereRenderer::BlochSphereRenderer(float radius)
    : theta_(M_PI / 2.0)  // Start at equator (|+x> state)
    , phi_(0.0)
    , radius_(radius)
    , sphere_segments_(24)
    , rings_(16)
{
    // Initialize state vector to default position
    state_vector_ = {
        static_cast<float>(std::sin(theta_) * std::cos(phi_)),
        static_cast<float>(std::sin(theta_) * std::sin(phi_)),
        static_cast<float>(std::cos(theta_))
    };
}

BlochSphereRenderer::~BlochSphereRenderer() {
    // Cleanup if needed
}

// =============================================================================
// State Updates
// =============================================================================

void BlochSphereRenderer::update_state(double theta, double phi) {
    theta_ = theta;
    phi_ = phi;

    // Clamp theta to valid range
    if (theta_ < 0.0) theta_ = 0.0;
    if (theta_ > M_PI) theta_ = M_PI;

    // Normalize phi to [0, 2*pi]
    while (phi_ < 0.0) phi_ += 2.0 * M_PI;
    while (phi_ > 2.0 * M_PI) phi_ -= 2.0 * M_PI;

    // Calculate state vector from spherical coordinates
    state_vector_.x = static_cast<float>(std::sin(theta_) * std::cos(phi_));
    state_vector_.y = static_cast<float>(std::sin(theta_) * std::sin(phi_));
    state_vector_.z = static_cast<float>(std::cos(theta_));
}

void BlochSphereRenderer::set_from_spin_system(double theta, double phi) {
    update_state(theta, phi);
}

Vector3 BlochSphereRenderer::get_state_vector() const {
    return state_vector_;
}

// =============================================================================
// Rendering
// =============================================================================

void BlochSphereRenderer::render(const Camera3D& camera) {
    (void)camera; // Camera can be used for advanced effects

    draw_sphere_wireframe();
    draw_axes();
    draw_state_vector();
    draw_basis_labels();
}

void BlochSphereRenderer::draw_sphere_wireframe() const {
    // Draw wireframe sphere using latitude and longitude lines
    Color sphere_color = {80, 80, 100, 180};

    // Draw longitude lines
    for (int i = 0; i < sphere_segments_; ++i) {
        float phi = (2.0f * PI * i) / sphere_segments_;
        Vector3 start_point = {
            radius_ * std::sin(0.0f),
            radius_ * std::cos(phi),
            radius_ * std::sin(0.0f) * std::sin(phi)
        };

        for (int j = 0; j <= rings_; ++j) {
            float theta = (PI * j) / rings_;
            Vector3 end_point = {
                radius_ * std::sin(theta) * std::cos(phi),
                radius_ * std::cos(theta),
                radius_ * std::sin(theta) * std::sin(phi)
            };

            // Draw line segment
            DrawLine3D(
                {start_point.x * radius_, start_point.y, start_point.z * radius_},
                {end_point.x * radius_, end_point.y, end_point.z * radius_},
                sphere_color
            );
            start_point = end_point;
        }
    }

    // Draw latitude lines
    for (int j = 1; j < rings_; ++j) {
        float theta = (PI * j) / rings_;
        float r = radius_ * std::sin(theta);
        float y = radius_ * std::cos(theta);

        for (int i = 0; i < sphere_segments_; ++i) {
            float phi1 = (2.0f * PI * i) / sphere_segments_;
            float phi2 = (2.0f * PI * (i + 1)) / sphere_segments_;

            Vector3 p1 = {r * std::cos(phi1), y, r * std::sin(phi1)};
            Vector3 p2 = {r * std::cos(phi2), y, r * std::sin(phi2)};

            DrawLine3D(p1, p2, sphere_color);
        }
    }
}

void BlochSphereRenderer::draw_axes() const {
    float axis_length = radius_ * 1.3f;

    // X axis (red)
    DrawLine3D({-axis_length, 0, 0}, {axis_length, 0, 0}, RED);
    DrawSphere({axis_length, 0, 0}, 0.08f, RED);

    // Y axis (green) - note: Y is up in Raylib
    DrawLine3D({0, -axis_length, 0}, {0, axis_length, 0}, GREEN);
    DrawSphere({0, axis_length, 0}, 0.08f, GREEN);

    // Z axis (blue)
    DrawLine3D({0, 0, -axis_length}, {0, 0, axis_length}, BLUE);
    DrawSphere({0, 0, axis_length}, 0.08f, BLUE);

    // Axis labels
    DrawText("X", static_cast<int>(axis_length * 20) + 10, -10, 20, RED);
    DrawText("Y", 10, static_cast<int>(-axis_length * 20) - 20, 20, GREEN);
    DrawText("Z", -10, 10, 20, BLUE);
}

void BlochSphereRenderer::draw_state_vector() {
    Vector3 start = {0, 0, 0};
    Vector3 end = {
        radius_ * state_vector_.x,
        radius_ * state_vector_.y,
        radius_ * state_vector_.z
    };

    // Get color based on state
    Color state_color = get_state_color();

    // Draw the state vector line
    DrawLine3D(start, end, state_color);

    // Draw arrowhead (small sphere at the tip)
    DrawSphere(end, 0.12f, state_color);

    // Draw a faint projection on the XY plane
    Vector3 projection = {end.x, 0, end.z};
    DrawLine3D({0, 0, 0}, projection, {state_color.r, state_color.g, state_color.b, 100});
}

void BlochSphereRenderer::draw_basis_labels() const {
    float label_offset = radius_ * 1.4f;

    // |0> state (north pole, |+z>)
    DrawText("|0>", -10, static_cast<int>(-label_offset * 20) - 20, 20, GREEN);

    // |1> state (south pole, |-z>)
    DrawText("|1>", -10, static_cast<int>(label_offset * 20), 20, RED);

    // |+x> state (on equator, phi=0)
    DrawText("|+x>", static_cast<int>(label_offset * 20), 10, 20, YELLOW);

    // |-x> state (on equator, phi=PI)
    DrawText("|-x>", static_cast<int>(-label_offset * 20) - 30, 10, 20, {100, 100, 255, 255});

    // |+y> state (on equator, phi=PI/2)
    DrawText("|+y>", 10, static_cast<int>(-label_offset * 20) - 10, 20, MAGENTA);
}

Color BlochSphereRenderer::get_state_color() const {
    // Color based on theta (latitude) and phi (longitude)
    // North pole (theta=0): green |+z>
    // South pole (theta=PI): red |-z>
    // Equator: blends toward yellow/blue based on phi

    float theta_normalized = static_cast<float>(theta_ / M_PI);

    if (theta_normalized < 0.1f) {
        // Near |+z> - green
        return GREEN;
    } else if (theta_normalized > 0.9f) {
        // Near |-z> - red
        return RED;
    } else {
        // On equator - color based on phi
        float phi_normalized = static_cast<float>(phi_ / (2.0 * M_PI));

        if (phi_normalized < 0.25f || phi_normalized > 0.75f) {
            // Near +X - yellow
            return YELLOW;
        } else if (phi_normalized > 0.25f && phi_normalized < 0.75f) {
            // Near -X - blue
            return {100, 100, 255, 255};
        }
    }

    // Default white
    return WHITE;
}

} // namespace render_engine