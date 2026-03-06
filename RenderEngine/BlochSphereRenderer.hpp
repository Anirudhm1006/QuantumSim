#pragma once

#include <memory>
#include <vector>
#include <cmath>

// Raylib includes
#include <raylib.h>

namespace render_engine {

// =============================================================================
// Bloch Sphere Renderer
// =============================================================================
// Visualizes a spin-1/2 system on a Bloch sphere with:
// - Wireframe sphere with X, Y, Z axes
// - State vector pointing in direction (theta, phi)
// - Color-coded basis states: |+z> = green, |-z> = red, |+x> = yellow, |-x> = blue
// - Labels for |0> and |1> states
// =============================================================================

class BlochSphereRenderer {
public:
    // Constructor with visualization radius
    explicit BlochSphereRenderer(float radius = 2.0f);

    // Destructor - cleanup resources
    ~BlochSphereRenderer();

    // Update the spin state to visualize (theta and phi in radians)
    void update_state(double theta, double phi);

    // Render the Bloch sphere visualization
    void render(const Camera3D& camera);

    // Get current Bloch vector direction
    [[nodiscard]] Vector3 get_state_vector() const;

    // Set state from SpinSystem
    void set_from_spin_system(double theta, double phi);

private:
    // Rendering helpers
    void draw_sphere_wireframe() const;
    void draw_axes() const;
    void draw_state_vector();
    void draw_basis_labels() const;

    // Color mapping based on state
    [[nodiscard]] Color get_state_color() const;

    // Physics state
    double theta_;  // Polar angle [0, pi]
    double phi_;    // Azimuthal angle [0, 2*pi]

    // Visualization parameters
    float radius_;
    int sphere_segments_;
    int rings_;

    // Cached state vector for efficiency
    Vector3 state_vector_;
};

} // namespace render_engine