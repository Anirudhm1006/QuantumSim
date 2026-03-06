#pragma once

#include <string>
#include <vector>
#include <memory>

// Raylib types
#include <raylib.h>

// =============================================================================
// Inspector Panel
// =============================================================================
// 2D overlay panel showing physics data on the right side of the screen
// Displays quantum state information based on current visualization mode
// =============================================================================

namespace ui_handler {

struct InspectorData {
    std::string mode_name;
    std::string title;

    // Common data
    double time = 0.0;
    int current_mode = 0;

    // Spin/Bloch sphere data
    double theta = 0.0;
    double phi = 0.0;
    double spin_x = 0.0;
    double spin_y = 0.0;
    double spin_z = 0.0;

    // Wave packet data
    double x0_1 = 0.0;
    double sigma_1 = 0.0;
    double x0_2 = 0.0;
    double sigma_2 = 0.0;
    bool superposition = false;

    // Hydrogen atom data
    int n = 1;
    int l = 0;
    int m = 0;
    double energy_eV = 0.0;
    bool quantum_mode = true;

    // Laser data
    double wavelength = 0.0;
    double frequency = 0.0;
    double photon_energy = 0.0;
    int n_photons = 0;
};

class InspectorPanel {
public:
    // Constructor
    InspectorPanel();

    // Destructor
    ~InspectorPanel();

    // Update the data to display
    void update_data(const InspectorData& data);

    // Render the inspector panel
    void render();

    // Get panel width
    [[nodiscard]] int get_width() const { return panel_width_; }

    // Get panel height
    [[nodiscard]] int get_height() const { return panel_height_; }

    // Set position offset
    void set_position(int x, int y);

private:
    // Rendering helpers
    void draw_panel_background();
    void draw_header();
    void draw_data_section();
    void draw_mode_info();
    void draw_quantum_state();
    void draw_controls_hint();

    // Mode-specific data rendering
    void draw_hydrogen_data(int x, int& y);
    void draw_spin_data(int x, int& y);
    void draw_wavepacket_data(int x, int& y);
    void draw_laser_data(int x, int& y);

    // Format helpers
    [[nodiscard]] std::string format_double(double value, int precision = 4) const;
    [[nodiscard]] std::string format_scientific(double value, int precision = 2) const;

    // Panel properties
    int panel_width_;
    int panel_height_;
    int position_x_;
    int position_y_;

    // Current data
    InspectorData data_;

    // Colors
    int bg_alpha_;
};

} // namespace ui_handler