#include <cmath>
#include <string>

#include <raylib.h>

#include "PropertiesPanel.hpp"
#include "IQuantumObject.hpp"
#include "GridSolver.hpp"
#include "HydrogenModel.hpp"
#include "SpinSystem.hpp"
#include "WavePacket.hpp"
#include "Laser.hpp"
#include "Entanglement.hpp"

namespace {
    const Color PANEL_BG       = {35, 35, 42, 255};
    const Color PANEL_BORDER   = {55, 55, 65, 255};
    const Color TEXT_PRIMARY   = {220, 220, 225, 255};
    const Color TEXT_SECONDARY = {140, 140, 150, 255};
    const Color TEXT_DISABLED  = {80, 80, 90, 255};
    const Color ACCENT         = {70, 130, 180, 255};
    const Color SUCCESS        = {60, 160, 80, 255};
    const Color DANGER         = {180, 60, 60, 255};
}

PropertiesPanel::PropertiesPanel()
    : font_{}
    , has_font_(false)
    , width_(250)
{
}

void PropertiesPanel::set_font(Font font) {
    font_ = font;
    has_font_ = true;
}

void PropertiesPanel::draw_text(const char* text, float x, float y, float size, Color color) {
    if (has_font_) {
        DrawTextEx(font_, text, {x, y}, size, 1, color);
    } else {
        DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), color);
    }
}

float PropertiesPanel::measure_text(const char* text, float size) const {
    if (has_font_) {
        return MeasureTextEx(font_, text, size, 1).x;
    }
    return static_cast<float>(MeasureText(text, static_cast<int>(size)));
}

void PropertiesPanel::render(int menu_bar_height, const IQuantumObject* selected, const GridSolver* solver) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    int panel_x = screen_w - width_;
    int panel_h = screen_h - menu_bar_height;

    DrawRectangle(panel_x, menu_bar_height, width_, panel_h, PANEL_BG);
    DrawLine(panel_x, menu_bar_height, panel_x, screen_h, PANEL_BORDER);

    int x = panel_x + 12;
    int y = menu_bar_height + 8;

    draw_text("PROPERTIES", static_cast<float>(x), static_cast<float>(y), 12, TEXT_SECONDARY);
    y += 22;

    if (selected) {
        std::string type_name = selected->get_type_name();
        draw_text(type_name.c_str(), static_cast<float>(x), static_cast<float>(y), 16, ACCENT);
        y += 24;

        draw_separator(x, y, width_ - 24);

        if (type_name == "HydrogenModel") {
            render_hydrogen(x, y, selected);
        } else if (type_name == "SpinSystem") {
            render_spin(x, y, selected);
        } else if (type_name == "WavePacket") {
            render_wavepacket(x, y, selected);
        } else if (type_name == "LaserSystem") {
            render_laser(x, y, selected);
        } else if (type_name == "EntanglementSystem") {
            render_entanglement(x, y, selected);
        } else {
            draw_text(selected->get_state_descriptor().c_str(), static_cast<float>(x), static_cast<float>(y), 13, TEXT_PRIMARY);
            y += 20;
        }
    } else {
        draw_text("No Selection", static_cast<float>(x), static_cast<float>(y), 14, TEXT_DISABLED);
        y += 24;
    }

    if (solver) {
        y += 10;
        draw_separator(x, y, width_ - 24);
        render_solver_info(x, y, solver);
    }
}

void PropertiesPanel::draw_section_header(int x, int& y, const char* title) {
    draw_text(title, static_cast<float>(x), static_cast<float>(y), 14, ACCENT);
    y += 20;
}

void PropertiesPanel::draw_property_row(int x, int& y, const char* label, const char* value, const char* unit) {
    draw_text(label, static_cast<float>(x), static_cast<float>(y), 13, TEXT_SECONDARY);
    draw_text(value, static_cast<float>(x + 100), static_cast<float>(y), 13, TEXT_PRIMARY);
    if (unit && unit[0] != '\0') {
        draw_text(unit, static_cast<float>(x + 190), static_cast<float>(y), 13, TEXT_SECONDARY);
    }
    y += 18;
}

void PropertiesPanel::draw_separator(int x, int& y, int w) {
    DrawLine(x, y, x + w, y, PANEL_BORDER);
    y += 8;
}

void PropertiesPanel::render_hydrogen(int x, int& y, const IQuantumObject* obj) {
    auto* h = dynamic_cast<const HydrogenModel*>(obj);
    if (!h) return;

    draw_section_header(x, y, "Quantum Numbers");

    auto qn = h->get_quantum_numbers();
    draw_property_row(x, y, "n", TextFormat("%d", qn.n), "");
    draw_property_row(x, y, "l", TextFormat("%d", qn.l), "");
    draw_property_row(x, y, "m", TextFormat("%d", qn.m), "");

    y += 4;
    draw_section_header(x, y, "Energy");

    draw_property_row(x, y, "E", TextFormat("%.4f", h->get_energy_eV()), "eV");
    draw_property_row(x, y, "Orbital", h->get_orbital_name().c_str(), "");
    draw_property_row(x, y, "Z", TextFormat("%d", h->get_Z()), "");
}

void PropertiesPanel::render_spin(int x, int& y, const IQuantumObject* obj) {
    auto* s = dynamic_cast<const SpinSystem*>(obj);
    if (!s) return;

    draw_section_header(x, y, "Bloch Sphere");
    draw_property_row(x, y, "theta", TextFormat("%.4f", s->get_theta()), "rad");
    draw_property_row(x, y, "phi", TextFormat("%.4f", s->get_phi()), "rad");

    y += 4;
    draw_section_header(x, y, "Spin Projections");
    draw_property_row(x, y, "<Sx>", TextFormat("%.4f", s->get_spin_projection_x()), "");
    draw_property_row(x, y, "<Sy>", TextFormat("%.4f", s->get_spin_projection_y()), "");
    draw_property_row(x, y, "<Sz>", TextFormat("%.4f", s->get_spin_projection_z()), "");
}

void PropertiesPanel::render_wavepacket(int x, int& y, const IQuantumObject* obj) {
    auto* wp = dynamic_cast<const WavePacket*>(obj);
    if (!wp) return;

    draw_section_header(x, y, "Wave Packet");
    draw_property_row(x, y, "x0", TextFormat("%.3f", wp->get_x0()), "a.u.");
    draw_property_row(x, y, "sigma", TextFormat("%.3f", wp->get_sigma()), "a.u.");
    draw_property_row(x, y, "k0", TextFormat("%.3f", wp->get_k0()), "1/a.u.");

    y += 4;
    draw_section_header(x, y, "Dynamics");
    draw_property_row(x, y, "v_group", TextFormat("%.3f", wp->get_group_velocity()), "a.u.");
    draw_property_row(x, y, "v_phase", TextFormat("%.3f", wp->get_phase_velocity()), "a.u.");
}

void PropertiesPanel::render_laser(int x, int& y, const IQuantumObject* obj) {
    auto* laser = dynamic_cast<const LaserSystem*>(obj);
    if (!laser) return;

    draw_section_header(x, y, "Laser Transition");
    draw_property_row(x, y, "lambda", TextFormat("%.1f", laser->get_transition().wavelength), "nm");

    double freq = laser_utils::wavelength_to_frequency(laser->get_transition().wavelength);
    draw_property_row(x, y, "freq", TextFormat("%.3e", freq), "Hz");

    y += 4;
    draw_section_header(x, y, "Population");
    double inversion = laser->get_population_inversion();
    Color inv_color = inversion > 0 ? SUCCESS : DANGER;
    draw_text("Inversion", static_cast<float>(x), static_cast<float>(y), 13, TEXT_SECONDARY);
    draw_text(TextFormat("%.4f", inversion), static_cast<float>(x + 100), static_cast<float>(y), 13, inv_color);
    y += 18;

    draw_property_row(x, y, "Above thr.", laser->is_above_threshold() ? "YES" : "NO", "");
}

void PropertiesPanel::render_entanglement(int x, int& y, const IQuantumObject* obj) {
    auto* ent = dynamic_cast<const EntanglementSystem*>(obj);
    if (!ent) return;

    draw_section_header(x, y, "Entangled State");
    draw_property_row(x, y, "Concurrence", TextFormat("%.4f", ent->concurrence()), "");
    draw_property_row(x, y, "Entropy", TextFormat("%.4f", ent->entanglement_entropy()), "bits");
    draw_property_row(x, y, "Purity", TextFormat("%.4f", ent->purity()), "");

    y += 4;
    draw_section_header(x, y, "Probabilities");
    auto probs = ent->get_state().probabilities();
    draw_property_row(x, y, "|00>", TextFormat("%.4f", probs[0]), "");
    draw_property_row(x, y, "|01>", TextFormat("%.4f", probs[1]), "");
    draw_property_row(x, y, "|10>", TextFormat("%.4f", probs[2]), "");
    draw_property_row(x, y, "|11>", TextFormat("%.4f", probs[3]), "");
}

void PropertiesPanel::render_solver_info(int x, int& y, const GridSolver* solver) {
    draw_section_header(x, y, "Grid Solver");

    double norm = solver->get_norm();
    Color norm_color = (std::abs(norm - 1.0) < 0.01) ? SUCCESS : DANGER;
    draw_text("Norm", static_cast<float>(x), static_cast<float>(y), 13, TEXT_SECONDARY);
    draw_text(TextFormat("%.6f", norm), static_cast<float>(x + 100), static_cast<float>(y), 13, norm_color);
    y += 18;

    draw_property_row(x, y, "Energy", TextFormat("%.4f", solver->get_energy()), "a.u.");
    draw_property_row(x, y, "<x>", TextFormat("%.4f", solver->get_position_expectation()), "a.u.");
    draw_property_row(x, y, "<p>", TextFormat("%.4f", solver->get_momentum_expectation()), "a.u.");
    draw_property_row(x, y, "Time", TextFormat("%.4f", solver->get_time()), "a.u.");
    draw_property_row(x, y, "Grid", TextFormat("%d pts", solver->get_nx()), "");
}
