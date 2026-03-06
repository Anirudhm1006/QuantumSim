#pragma once

#include <memory>
#include <vector>
#include <cmath>
#include <string>
#include <functional>

// Raylib forward declarations
struct Camera3D;
struct Vector3;
struct Color;

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

// =============================================================================
// Wave Packet Renderer
// =============================================================================
// Visualizes quantum wave packets as 3D gaussian envelopes:
// - Series of points/lines representing probability density
// - Multiple small spheres showing |psi|^2 at different positions
// - Two wave packets in superposition showing interference
// - Color intensity shows probability (brighter = higher |psi|^2)
// =============================================================================

class WavePacketRenderer {
public:
    // Constructor
    WavePacketRenderer();

    // Destructor - cleanup resources
    ~WavePacketRenderer();

    // Update wave packet parameters
    void update_wave_packet_1(double x0, double sigma, double time);
    void update_wave_packet_2(double x0, double sigma, double time);

    // Set superposition mode (shows interference)
    void set_superposition_mode(bool enabled);

    // Render the wave packet visualization
    void render(const Camera3D& camera);

    // Set rendering bounds
    void set_bounds(double x_min, double x_max, int num_points);

    // Get current probability at a position (for external use)
    [[nodiscard]] double get_probability_at(double x, int packet_id) const;

private:
    // Rendering helpers
    void draw_gaussian_envelope(double x0, double sigma, double time, Color color);
    void draw_interference_pattern();
    void draw_probability_points();

    // Calculate gaussian probability density
    [[nodiscard]] double gaussian_density(double x, double x0, double sigma) const;

    // Wave packet 1 parameters
    double x0_1_;
    double sigma_1_;
    double time_1_;

    // Wave packet 2 parameters
    double x0_2_;
    double sigma_2_;
    double time_2_;

    // Visualization settings
    bool superposition_enabled_;
    double x_min_;
    double x_max_;
    int num_points_;
    int vertical_resolution_;
};

// =============================================================================
// Visualization Manager
// =============================================================================
// Handles toggling between different visualization modes

enum class VisualizationMode {
    BlochSphere,
    WavePacket,
    Count
};

class VisualizerManager {
public:
    VisualizerManager();
    ~VisualizerManager();

    // Set the current visualization mode
    void set_mode(VisualizationMode mode);

    // Get current mode
    [[nodiscard]] VisualizationMode get_mode() const { return current_mode_; }

    // Cycle to next visualization
    void next_mode();

    // Update and render based on current mode
    void update_and_render(const Camera3D& camera);

    // Update spin state (for Bloch sphere)
    void update_spin_state(double theta, double phi);

    // Update wave packet (for wave packet visualization)
    void update_wave_packets(double x0_1, double sigma_1, double time1,
                             double x0_2, double sigma_2, double time2);

    // Toggle superposition
    void toggle_superposition();

    // Get mode name for UI display
    [[nodiscard]] std::string get_mode_name() const;

private:
    VisualizationMode current_mode_;

    std::unique_ptr<BlochSphereRenderer> bloch_renderer_;
    std::unique_ptr<WavePacketRenderer> wave_packet_renderer_;
};

} // namespace render_engine
