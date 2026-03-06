#pragma once

#include <memory>
#include <vector>
#include <cmath>

// Raylib forward declarations
struct Camera3D;
struct Vector3;
struct Color;

namespace render_engine {

// =============================================================================
// Hydrogen Renderer
// =============================================================================
// Visualizes a hydrogen atom with:
// - Central nucleus (protons)
// - Electron probability cloud (quantum view)
// - Bohr orbit rings (classical view)
// - Orbital visualization for different n levels
// =============================================================================

class HydrogenRenderer {
public:
    // Constructor
    explicit HydrogenRenderer(float scale = 1.0f);

    // Destructor - cleanup resources
    ~HydrogenRenderer();

    // Set the principal quantum number (n)
    void set_quantum_numbers(int n, int l, int m);

    // Set visualization mode (true = quantum, false = classical Bohr)
    void set_quantum_mode(bool quantum);

    // Update animation time
    void update_time(double time);

    // Render the hydrogen atom visualization
    void render(const Camera3D& camera);

    // Get current energy level in eV
    [[nodiscard]] double get_energy_eV() const;

    // Get mode name for UI
    [[nodiscard]] const char* get_mode_name() const;

private:
    // Rendering helpers
    void draw_nucleus() const;
    void draw_bohr_orbits() const;
    void draw_electron_cloud() const;
    void draw_electron() const;
    void draw_transition_arrow(int n_from, int n_to) const;

    // Calculate Bohr radius for given n (in Angstroms)
    [[nodiscard]] float get_bohr_radius(int n) const;

    // Physics state
    int n_;  // Principal quantum number
    int l_;  // Angular momentum quantum number
    int m_;  // Magnetic quantum number

    // Animation
    double time_;
    double electron_angle_; // Current angle for electron orbiting

    // Visualization settings
    bool quantum_mode_;
    float scale_;
    float nucleus_radius_;
    float electron_radius_;
};

} // namespace render_engine