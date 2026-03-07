#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "FreeParticleScenario.hpp"

FreeParticleScenario::FreeParticleScenario()
    : solver_(512, -15.0, 15.0, 0.001)
    , potential_(512, -15.0, 15.0)
    , x0_slider_("x0 (initial pos)", -8.0f, 8.0f, -5.0f, "%.1f")
    , sigma_slider_("\xCF\x83 (width)", 0.3f, 4.0f, 1.5f, "%.2f")
    , k0_slider_("k0 (momentum)", -10.0f, 10.0f, 5.0f, "%.1f")
{}

void FreeParticleScenario::on_enter() {
    current_view_ = 0;
    paused_ = false;
    reset_solver();
}

void FreeParticleScenario::reset_solver() {
    solver_.reset();
    potential_.clear();
    solver_.set_potential(potential_.values());
    solver_.inject_gaussian(x0(), sigma(), k0());
}

void FreeParticleScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
    if (IsKeyPressed(KEY_SPACE)) paused_ = !paused_;
    if (IsKeyPressed(KEY_R)) reset_solver();
    if (IsKeyPressed(KEY_UP)) speed_mult_ = std::min(speed_mult_ + 1, 20);
    if (IsKeyPressed(KEY_DOWN)) speed_mult_ = std::max(speed_mult_ - 1, 1);
}

void FreeParticleScenario::update(double dt) {
    (void)dt;
    if (!paused_) {
        for (int i = 0; i < speed_mult_; ++i) solver_.time_step();
    }
}

void FreeParticleScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    BeginMode3D(cam);
    {
        for (int i = -15; i <= 15; ++i) {
            DrawLine3D({(float)i, 0, -15}, {(float)i, 0, 15}, ui_colors::GRID_LINE);
            DrawLine3D({-15, 0, (float)i}, {15, 0, (float)i}, ui_colors::GRID_LINE);
        }
        DrawLine3D({0,0,0}, {4,0,0}, ui_colors::AXIS_X);
        DrawLine3D({0,0,0}, {0,4,0}, ui_colors::AXIS_Y);
        DrawLine3D({0,0,0}, {0,0,4}, ui_colors::AXIS_Z);

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
                float h = static_cast<float>(density[i] / max_p) * 5.0f;
                if (h > 0.01f) {
                    DrawCube({x, h * 0.5f, 0.0f}, static_cast<float>(dx * step * 0.8), h, 0.2f, ui_colors::WAVEFUNCTION);
                }
            }
        }

        // x-axis label markers
        for (int i = -12; i <= 12; i += 4) {
            DrawLine3D({(float)i, 0, -0.3f}, {(float)i, 0, 0.3f}, ui_colors::TEXT_SECONDARY);
        }
    }
    EndMode3D();

    draw_text("FREE PARTICLE", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text(TextFormat("t = %.3f    speed x%d    %s", solver_.get_time(), speed_mult_, paused_ ? "PAUSED" : "RUNNING"),
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 30), 12, ui_colors::TEXT_PRIMARY);
    draw_text("Space: pause    R: reset    Up/Down: speed", static_cast<float>(vp_x + 10),
              static_cast<float>(vp_y + vp_h - 20), 11, ui_colors::TEXT_SECONDARY);

    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void FreeParticleScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("FREE PARTICLE", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    bool x0_changed = x0_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 4;
    bool sig_changed = sigma_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 4;
    bool k0_changed = k0_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 8;

    if (x0_changed || sig_changed || k0_changed) reset_solver();

    draw_separator(cx, cy, w - 20);
    cy += 4;
    draw_text("Space: pause/resume", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("R: reset packet", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("Up/Down: sim speed", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 20;

    draw_text("Observe the wave packet", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("spreading over time", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("(dispersion) while", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("moving at group velocity.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
}

void FreeParticleScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    draw_section("Initial Params", px, py);
    draw_prop("x0", TextFormat("%.2f", x0()), px, py);
    draw_prop("\xCF\x83", TextFormat("%.3f", sigma()), px, py);
    draw_prop("k0", TextFormat("%.2f", k0()), px, py);

    py += 4;
    draw_section("Solver State", px, py);
    draw_prop("Norm", TextFormat("%.6f", solver_.get_norm()), px, py);
    draw_prop("Time", TextFormat("%.4f", solver_.get_time()), px, py);
    draw_prop("<x>", TextFormat("%.4f", solver_.get_position_expectation()), px, py);
    draw_prop("<p>", TextFormat("%.4f", solver_.get_momentum_expectation()), px, py);

    py += 4;
    draw_section("Dynamics", px, py);
    double vg = k0();
    draw_prop("v_group", TextFormat("%.3f", vg), px, py);
    draw_prop("E", TextFormat("%.3f", k0() * k0() / 2.0), px, py);

    py += 8;
    draw_separator(px, py, w - 24);
    draw_section("Physics", px, py);
    draw_text("Gaussian packet in free", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("space disperses as it", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("evolves: width grows as", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("\xCF\x83(t) = \xCF\x83_0 * sqrt(1+(t/\xCF\x84)^2)", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::ACCENT);
    py += 14;
    draw_text("Center moves at v = k0", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
}
