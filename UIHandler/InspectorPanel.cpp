#include "InspectorPanel.hpp"

#include <raylib.h>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ui_handler {

// =============================================================================
// InspectorPanel Implementation
// =============================================================================

InspectorPanel::InspectorPanel()
    : panel_width_(220)
    , panel_height_(0)  // Will be set based on screen
    , position_x_(0)
    , position_y_(0)
    , bg_alpha_(180) {
    // Default initialization
}

InspectorPanel::~InspectorPanel() {
}

void InspectorPanel::update_data(const InspectorData& data) {
    data_ = data;
}

void InspectorPanel::render() {
    // Calculate panel position (right side of screen)
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();

    position_x_ = screen_width - panel_width_ - 10;
    position_y_ = 10;
    panel_height_ = screen_height - 20;

    // Draw panel background
    draw_panel_background();

    // Draw header
    draw_header();

    // Draw data section based on mode
    draw_data_section();

    // Draw controls hint at bottom
    draw_controls_hint();
}

void InspectorPanel::set_position(int x, int y) {
    position_x_ = x;
    position_y_ = y;
}

// =============================================================================
// Private rendering methods
// =============================================================================

void InspectorPanel::draw_panel_background() {
    Color bg_color = {20, 20, 30, bg_alpha_};
    DrawRectangle(position_x_, position_y_, panel_width_, panel_height_, bg_color);

    // Draw border
    Color border_color = {100, 100, 120, 150};
    DrawRectangleLinesEx(
        (Rectangle){(float)position_x_, (float)position_y_, (float)panel_width_, (float)panel_height_},
        1.0f, border_color);
}

void InspectorPanel::draw_header() {
    int x = position_x_ + 15;
    int y = position_y_ + 15;

    // Draw title
    DrawText("QUANTUM SIMULATOR", x, y, 18, GOLD);

    // Draw mode indicator
    y += 30;
    std::string mode_str = "Mode: ";

    switch (data_.current_mode) {
        case 0: mode_str += "Hydrogen"; break;
        case 1: mode_str += "Spin/Bloch"; break;
        case 2: mode_str += "Wave Packet"; break;
        case 3: mode_str += "Laser"; break;
        default: mode_str += "Unknown"; break;
    }

    DrawText(mode_str.c_str(), x, y, 16, WHITE);
}

void InspectorPanel::draw_data_section() {
    int x = position_x_ + 15;
    int y = position_y_ + 80;

    // Draw separator line
    DrawLine(x, y - 10, position_x_ + panel_width_ - 15, y - 10, {80, 80, 100, 150});

    // Draw mode-specific data
    switch (data_.current_mode) {
        case 0: // Hydrogen
            draw_hydrogen_data(x, y);
            break;
        case 1: // Spin/Bloch
            draw_spin_data(x, y);
            break;
        case 2: // Wave Packet
            draw_wavepacket_data(x, y);
            break;
        case 3: // Laser
            draw_laser_data(x, y);
            break;
    }
}

void InspectorPanel::draw_hydrogen_data(int x, int& y) {
    DrawText("QUANTUM NUMBERS", x, y, 14, YELLOW);
    y += 25;

    std::stringstream ss;
    ss << "n = " << data_.n;
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 22;

    ss.str("");
    ss << "l = " << data_.l;
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 22;

    ss.str("");
    ss << "m = " << data_.m;
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 30;

    DrawText("ENERGY", x, y, 14, YELLOW);
    y += 25;

    ss.str("");
    ss << std::fixed << std::setprecision(4) << data_.energy_eV << " eV";
    DrawText(ss.str().c_str(), x, y, 16, WHITE);
    y += 22;

    ss.str("");
    ss << std::fixed << std::setprecision(2) << -13.6 / (data_.n * data_.n) << " (theory)";
    DrawText(ss.str().c_str(), x, y, 14, GRAY);
}

void InspectorPanel::draw_spin_data(int x, int& y) {
    DrawText("BLOCH SPHERE", x, y, 14, YELLOW);
    y += 25;

    std::stringstream ss;
    ss << "theta = " << std::fixed << std::setprecision(3) << data_.theta << " rad";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 22;

    ss.str("");
    ss << "phi = " << std::fixed << std::setprecision(3) << data_.phi << " rad";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 30;

    DrawText("SPIN PROJECTIONS", x, y, 14, YELLOW);
    y += 25;

    ss.str("");
    ss << "<sx> = " << std::fixed << std::setprecision(4) << data_.spin_x;
    DrawText(ss.str().c_str(), x, y, 16, WHITE);
    y += 22;

    ss.str("");
    ss << "<sy> = " << std::fixed << std::setprecision(4) << data_.spin_y;
    DrawText(ss.str().c_str(), x, y, 16, WHITE);
    y += 22;

    ss.str("");
    ss << "<sz> = " << std::fixed << std::setprecision(4) << data_.spin_z;
    DrawText(ss.str().c_str(), x, y, 16, WHITE);
}

void InspectorPanel::draw_wavepacket_data(int x, int& y) {
    DrawText("WAVE PACKET 1", x, y, 14, YELLOW);
    y += 25;

    std::stringstream ss;
    ss << "x0 = " << std::fixed << std::setprecision(2) << data_.x0_1 << " fm";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 22;

    ss.str("");
    ss << "sigma = " << std::fixed << std::setprecision(2) << data_.sigma_1 << " fm";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 30;

    DrawText("WAVE PACKET 2", x, y, 14, YELLOW);
    y += 25;

    ss.str("");
    ss << "x0 = " << std::fixed << std::setprecision(2) << data_.x0_2 << " fm";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 22;

    ss.str("");
    ss << "sigma = " << std::fixed << std::setprecision(2) << data_.sigma_2 << " fm";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 30;

    DrawText("SUPERPOSITION", x, y, 14, YELLOW);
    y += 25;

    const char* status = data_.superposition ? "Active" : "Inactive";
    Color status_color = data_.superposition ? GREEN : GRAY;
    DrawText(status, x, y, 16, status_color);
}

void InspectorPanel::draw_laser_data(int x, int& y) {
    DrawText("LASER PARAMETERS", x, y, 14, YELLOW);
    y += 25;

    std::stringstream ss;
    ss << "lambda = " << std::fixed << std::setprecision(1) << data_.wavelength << " nm";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 22;

    ss.str("");
    ss << "f = " << std::scientific << std::setprecision(2) << data_.frequency << " Hz";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 22;

    ss.str("");
    ss << "E = " << std::fixed << std::setprecision(4) << data_.photon_energy << " eV";
    DrawText(ss.str().c_str(), x, y, 16, LIGHTGRAY);
    y += 30;

    DrawText("PHOTON COUNT", x, y, 14, YELLOW);
    y += 25;

    ss.str("");
    ss << "N = " << data_.n_photons << " photons";
    DrawText(ss.str().c_str(), x, y, 16, WHITE);
}

void InspectorPanel::draw_controls_hint() {
    int y = position_y_ + panel_height_ - 50;

    DrawText("CONTROLS", position_x_ + 15, y, 14, YELLOW);
    y += 20;

    DrawText("1-4: Switch Mode", position_x_ + 15, y, 12, GRAY);
    y += 16;

    DrawText("Arrows: Adjust", position_x_ + 15, y, 12, GRAY);
    y += 16;

    DrawText("R: Reset", position_x_ + 15, y, 12, GRAY);
    y += 16;

    DrawText("ESC: Exit", position_x_ + 15, y, 12, GRAY);
}

std::string InspectorPanel::format_double(double value, int precision) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

std::string InspectorPanel::format_scientific(double value, int precision) const {
    std::stringstream ss;
    ss << std::scientific << std::setprecision(precision) << value;
    return ss.str();
}

} // namespace ui_handler