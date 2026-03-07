#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "DeBroglieScenario.hpp"

constexpr DeBroglieScenario::ParticlePreset DeBroglieScenario::PRESETS[];

DeBroglieScenario::DeBroglieScenario()
    : solver_(512, -10.0, 10.0, 0.001)
    , potential_(512, -10.0, 10.0)
    , velocity_slider_("Velocity", 0.5f, 15.0f, 5.0f, "%.1f")
{}

void DeBroglieScenario::on_enter() {
    current_view_ = 0;
    potential_.clear();
    reset_solver();
}

double DeBroglieScenario::k0() const {
    return static_cast<double>(velocity_slider_.get_value());
}

double DeBroglieScenario::momentum_au() const {
    return HBAR * k0();
}

double DeBroglieScenario::lambda_au() const {
    return 2.0 * PI / k0();
}

double DeBroglieScenario::energy_au() const {
    return 0.5 * k0() * k0();
}

double DeBroglieScenario::lambda_si() const {
    double v = static_cast<double>(velocity_slider_.get_value()) * 1e5;
    double mass = PRESETS[particle_type_].mass_kg;
    double p = mass * v;
    if (p < 1e-50) return 0.0;
    return H_SI / p;
}

void DeBroglieScenario::reset_solver() {
    solver_.reset();
    potential_.clear();
    solver_.set_potential(potential_.values());
    solver_.inject_gaussian(-3.0, 1.5, k0());
}

const char* DeBroglieScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Wave Packet";
        case 1: return "Comparison";
        default: return "Unknown";
    }
}

void DeBroglieScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
    if (IsKeyPressed(KEY_P)) {
        particle_type_ = (particle_type_ + 1) % NUM_PRESETS;
    }
}

void DeBroglieScenario::update(double dt) {
    (void)dt;
    if (current_view_ == 0) {
        for (int i = 0; i < 5; ++i) solver_.time_step();
    }
}

void DeBroglieScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    switch (current_view_) {
        case 0: render_wavepacket_view(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_comparison_view(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void DeBroglieScenario::render_wavepacket_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    BeginMode3D(cam);
    {
        for (int i = -10; i <= 10; ++i) {
            DrawLine3D({(float)i, 0, -10}, {(float)i, 0, 10}, ui_colors::GRID_LINE);
            DrawLine3D({-10, 0, (float)i}, {10, 0, (float)i}, ui_colors::GRID_LINE);
        }
        DrawLine3D({0,0,0}, {3,0,0}, ui_colors::AXIS_X);
        DrawLine3D({0,0,0}, {0,3,0}, ui_colors::AXIS_Y);
        DrawLine3D({0,0,0}, {0,0,3}, ui_colors::AXIS_Z);

        auto density = solver_.get_probability_density();
        int nx = solver_.get_nx();
        double dx = solver_.get_dx();
        double x_min = solver_.get_x_min();

        double max_p = 0.0;
        for (double p : density) if (p > max_p) max_p = p;

        if (max_p > 1e-15) {
            int step = std::max(1, nx / 300);
            for (int i = 0; i < nx; i += step) {
                float x = static_cast<float>(x_min + i * dx);
                float h = static_cast<float>(density[i] / max_p) * 4.0f;
                if (h > 0.01f) {
                    DrawCube({x, h * 0.5f, 0.0f}, static_cast<float>(dx * step * 0.8), h, 0.15f, ui_colors::WAVEFUNCTION);
                }
            }
        }
    }
    EndMode3D();

    draw_text("DE BROGLIE WAVE PACKET", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text(TextFormat("\xCE\xBB = %.3f a.u.   p = %.3f   E = %.3f", lambda_au(), momentum_au(), energy_au()),
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 30), 12, ui_colors::TEXT_PRIMARY);
}

void DeBroglieScenario::render_comparison_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("WAVE-PARTICLE DUALITY", static_cast<float>(vp_x + 20), static_cast<float>(vp_y + 10), 16, ui_colors::ACCENT);
    draw_text(TextFormat("Particle: %s", PRESETS[particle_type_].name),
              static_cast<float>(vp_x + 20), static_cast<float>(vp_y + 32), 13, ui_colors::TEXT_PRIMARY);
    draw_text("[P] key: cycle particle type", static_cast<float>(vp_x + 20), static_cast<float>(vp_y + 50), 11, ui_colors::TEXT_SECONDARY);

    int cx = vp_x + vp_w / 2;
    int cy = vp_y + vp_h / 2;
    int half_w = vp_w / 2 - 40;

    // Left: particle
    int part_x = vp_x + 40;
    int part_y = cy - 40;
    draw_text("PARTICLE", static_cast<float>(part_x + half_w / 2 - 30), static_cast<float>(part_y - 20), 14, ui_colors::ACCENT);

    DrawCircle(part_x + half_w / 2, cy, 25, Color{100, 140, 200, 255});
    draw_text("m", static_cast<float>(part_x + half_w / 2 - 4), static_cast<float>(cy - 6), 13, ui_colors::TEXT_PRIMARY);

    double v = static_cast<double>(velocity_slider_.get_value()) * 1e5;
    DrawLine(part_x + half_w / 2 + 30, cy, part_x + half_w / 2 + 80, cy, ui_colors::TEXT_PRIMARY);
    DrawTriangle({static_cast<float>(part_x + half_w / 2 + 80), static_cast<float>(cy)},
                 {static_cast<float>(part_x + half_w / 2 + 72), static_cast<float>(cy - 5)},
                 {static_cast<float>(part_x + half_w / 2 + 72), static_cast<float>(cy + 5)}, ui_colors::TEXT_PRIMARY);
    draw_text(TextFormat("v = %.1e m/s", v), static_cast<float>(part_x + half_w / 2 + 30), static_cast<float>(cy + 10), 11, ui_colors::TEXT_SECONDARY);
    draw_text(TextFormat("m = %.2e kg", PRESETS[particle_type_].mass_kg),
              static_cast<float>(part_x + 10), static_cast<float>(cy + 40), 11, ui_colors::TEXT_SECONDARY);

    // Right: wave
    int wave_x = cx + 20;
    draw_text("WAVE", static_cast<float>(wave_x + half_w / 2 - 15), static_cast<float>(part_y - 20), 14, ui_colors::ACCENT);

    double lam_si = lambda_si();
    double lam_display = lam_si;
    const char* unit = "m";
    if (lam_si < 1e-12 && lam_si > 0) { lam_display = lam_si * 1e15; unit = "fm"; }
    else if (lam_si < 1e-9) { lam_display = lam_si * 1e12; unit = "pm"; }
    else if (lam_si < 1e-6) { lam_display = lam_si * 1e9; unit = "nm"; }
    else if (lam_si < 1e-3) { lam_display = lam_si * 1e6; unit = "um"; }
    else if (lam_si < 1.0) { lam_display = lam_si * 1e3; unit = "mm"; }

    double wave_k = 2.0 * PI / std::max(lambda_au(), 0.3);
    int wave_points = half_w;
    int prev_wx = 0, prev_wy = 0;
    for (int px = 0; px < wave_points; ++px) {
        double x = static_cast<double>(px) / 30.0;
        double y_wave = 30.0 * std::sin(wave_k * x);
        int wx = wave_x + px;
        int wy = cy - static_cast<int>(y_wave);
        if (px > 0) DrawLine(prev_wx, prev_wy, wx, wy, ui_colors::WAVEFUNCTION);
        prev_wx = wx;
        prev_wy = wy;
    }

    // Lambda bracket
    double pixels_per_lambda = 30.0 * lambda_au();
    if (pixels_per_lambda > 20 && pixels_per_lambda < half_w) {
        int lx1 = wave_x + 10;
        int lx2 = lx1 + static_cast<int>(pixels_per_lambda);
        int ly = cy + 45;
        DrawLine(lx1, ly, lx2, ly, ui_colors::ACCENT);
        DrawLine(lx1, ly - 5, lx1, ly + 5, ui_colors::ACCENT);
        DrawLine(lx2, ly - 5, lx2, ly + 5, ui_colors::ACCENT);
        draw_text("\xCE\xBB", static_cast<float>((lx1 + lx2) / 2 - 3), static_cast<float>(ly + 6), 13, ui_colors::ACCENT);
    }

    if (lam_si > 0) {
        draw_text(TextFormat("\xCE\xBB = %.3f %s", lam_display, unit),
                  static_cast<float>(wave_x + 10), static_cast<float>(cy + 70), 14, ui_colors::ACCENT);
    } else {
        draw_text("\xCE\xBB ~ 0 (macroscopic)", static_cast<float>(wave_x + 10), static_cast<float>(cy + 70), 14, ui_colors::DANGER);
    }

    // Formula
    draw_text("\xCE\xBB = h / p = h / (mv)", static_cast<float>(vp_x + vp_w / 2 - 60),
              static_cast<float>(vp_y + vp_h - 40), 15, ui_colors::ACCENT);

    // Dividing line
    DrawLine(cx, vp_y + 70, cx, vp_y + vp_h - 50, ui_colors::PANEL_BORDER);
}

void DeBroglieScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("DE BROGLIE", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    bool vel_changed = velocity_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 8;
    if (vel_changed && current_view_ == 0) {
        reset_solver();
    }

    draw_text("Particle type:", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 16;

    for (int i = 0; i < NUM_PRESETS; ++i) {
        Color bg = (i == particle_type_) ? ui_colors::ACCENT : ui_colors::PANEL_BG;
        DrawRectangle(cx, cy, w - 20, 24, bg);
        DrawRectangleLinesEx({(float)cx, (float)cy, (float)(w - 20), 24.0f}, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(PRESETS[i].name, static_cast<float>(cx + 6), static_cast<float>(cy + 5), 12,
                  (i == particle_type_) ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (mouse.x >= cx && mouse.x < cx + w - 20 && mouse.y >= cy && mouse.y < cy + 24) {
                particle_type_ = i;
            }
        }
        cy += 26;
    }

    cy += 8;
    draw_text("[P] cycle particles", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("Views: 1-2 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
}

void DeBroglieScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    draw_section("Particle", px, py);
    draw_prop("Type", PRESETS[particle_type_].name, px, py);
    draw_prop("Mass", TextFormat("%.2e kg", PRESETS[particle_type_].mass_kg), px, py);

    double v = static_cast<double>(velocity_slider_.get_value()) * 1e5;
    draw_prop("v", TextFormat("%.1e m/s", v), px, py);

    py += 4;
    draw_section("De Broglie", px, py);

    double p_si = PRESETS[particle_type_].mass_kg * v;
    draw_prop("p", TextFormat("%.2e kg*m/s", p_si), px, py);

    double lam = lambda_si();
    if (lam > 1e-9) {
        draw_prop("\xCE\xBB", TextFormat("%.3e m", lam), px, py, ui_colors::ACCENT);
    } else if (lam > 1e-12) {
        draw_prop("\xCE\xBB", TextFormat("%.3f pm", lam * 1e12), px, py, ui_colors::ACCENT);
    } else if (lam > 0) {
        draw_prop("\xCE\xBB", TextFormat("%.3e m", lam), px, py, ui_colors::ACCENT);
    } else {
        draw_prop("\xCE\xBB", "~0", px, py, ui_colors::DANGER);
    }

    py += 4;
    draw_section("Formula", px, py);
    draw_text("\xCE\xBB = h / p", static_cast<float>(px), static_cast<float>(py), 13, ui_colors::ACCENT);
    py += 18;
    draw_text("= h / (m * v)", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 18;
    draw_text(TextFormat("h = %.3e J*s", H_SI), static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);

    if (current_view_ == 0) {
        py += 12;
        draw_separator(px, py, w - 24);
        draw_section("Solver (a.u.)", px, py);
        draw_prop("k0", TextFormat("%.2f", k0()), px, py);
        draw_prop("\xCE\xBB", TextFormat("%.3f", lambda_au()), px, py);
        draw_prop("E", TextFormat("%.3f", energy_au()), px, py);
        draw_prop("Norm", TextFormat("%.6f", solver_.get_norm()), px, py);
    }
}
