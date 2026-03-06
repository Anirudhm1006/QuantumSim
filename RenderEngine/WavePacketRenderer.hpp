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

} // namespace render_engine