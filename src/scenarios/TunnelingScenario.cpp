#include <cmath>
#include <cstdio>
#include <algorithm>

#include <raylib.h>
#include <rlgl.h>

#include "TunnelingScenario.hpp"

namespace {
    constexpr Color INCIDENT_COLOR    = {80, 160, 240, 200};
    constexpr Color REFLECTED_COLOR   = {240, 160, 60, 200};
    constexpr Color TRANSMITTED_COLOR = {60, 220, 100, 200};
    constexpr Color BARRIER_FILL      = {180, 60, 60, 120};
    constexpr Color BARRIER_OUTLINE   = {220, 80, 80, 255};
    constexpr Color PLOT_LINE         = {90, 160, 220, 255};
    constexpr Color PLOT_FILL         = {90, 160, 220, 60};
    constexpr Color MARKER_COLOR      = {255, 220, 60, 255};
    constexpr Color CURVE_ABOVE       = {120, 200, 120, 255};
}

// --------------------------------------------------------------------------
// Construction / lifecycle
// --------------------------------------------------------------------------

TunnelingScenario::TunnelingScenario()
    : solver_(GRID_NX, GRID_X_MIN, GRID_X_MAX, GRID_DT)
    , potential_(GRID_NX, GRID_X_MIN, GRID_X_MAX)
    , v0_slider_("Barrier Height (V\xe2\x82\x80)", 1.0f, 50.0f, 10.0f, "%.1f")
    , l_slider_("Barrier Width (L)", 0.1f, 3.0f, 1.0f, "%.2f")
    , k0_slider_("Wave Number (k\xe2\x82\x80)", 1.0f, 15.0f, 4.0f, "%.1f")
{}

void TunnelingScenario::on_enter() {
    reset_simulation();
}

void TunnelingScenario::reset_simulation() {
    solver_.reset();
    rebuild_barrier();
    inject_packet();
    auto_fire_timer_ = 0.0;
}

void TunnelingScenario::rebuild_barrier() {
    potential_.clear();
    potential_.set_barrier(0.0, barrier_l(), v0());
    solver_.set_potential(potential_.values());
}

void TunnelingScenario::inject_packet() {
    solver_.inject_gaussian(-6.0, 1.0, k0());
}

// --------------------------------------------------------------------------
// Physics
// --------------------------------------------------------------------------

double TunnelingScenario::transmission_coefficient(double E, double V0, double L) {
    if (E <= 0.0 || V0 <= 0.0 || L <= 0.0) return 0.0;

    if (E >= V0) {
        double k2 = std::sqrt(2.0 * (E - V0));
        double s = std::sin(k2 * L);
        double denom = 1.0 + (V0 * V0 * s * s) / (4.0 * E * (E - V0));
        return 1.0 / denom;
    } else {
        double kappa = std::sqrt(2.0 * (V0 - E));
        double sh = std::sinh(kappa * L);
        double denom = 1.0 + (V0 * V0 * sh * sh) / (4.0 * E * (V0 - E));
        return 1.0 / denom;
    }
}

double TunnelingScenario::numerical_transmission() const {
    auto prob = solver_.get_probability_density();
    double dx = solver_.get_dx();
    double x_min = solver_.get_x_min();
    double barrier_end = barrier_l();

    double right_prob = 0.0;
    for (int i = 0; i < solver_.get_nx(); ++i) {
        double x = x_min + i * dx;
        if (x > barrier_end + 0.5) {
            right_prob += prob[i] * dx;
        }
    }
    return right_prob;
}

// --------------------------------------------------------------------------
// Input
// --------------------------------------------------------------------------

void TunnelingScenario::handle_input() {
    help_popup_.handle_input();
    if (help_popup_.is_open()) return;

    if (IsKeyPressed(KEY_F)) {
        auto_fire_ = !auto_fire_;
        auto_fire_timer_ = 0.0;
    }
    if (IsKeyPressed(KEY_R)) {
        reset_simulation();
    }
}

// --------------------------------------------------------------------------
// Update
// --------------------------------------------------------------------------

void TunnelingScenario::update(double dt) {
    for (int i = 0; i < 4; ++i) {
        solver_.time_step();
    }

    if (auto_fire_) {
        auto_fire_timer_ += dt;
        if (auto_fire_timer_ >= AUTO_FIRE_INTERVAL) {
            auto_fire_timer_ = 0.0;
            solver_.reset();
            rebuild_barrier();
            inject_packet();
        }
    }
}

// --------------------------------------------------------------------------
// View queries
// --------------------------------------------------------------------------

const char* TunnelingScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Setup";
        case 1: return "Wavefunction";
        case 2: return "T(E) Curve";
        default: return "Unknown";
    }
}

bool TunnelingScenario::uses_3d() const {
    return current_view_ == 1;
}

// --------------------------------------------------------------------------
// Viewport dispatch
// --------------------------------------------------------------------------

void TunnelingScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    switch (current_view_) {
        case 0: render_setup_view(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_wavefunction_view(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_te_curve_view(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }

    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

// --------------------------------------------------------------------------
// View 0 — Setup (2D side view)
// --------------------------------------------------------------------------

void TunnelingScenario::render_setup_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    int margin = 50;
    int plot_x = vp_x + margin;
    int plot_y = vp_y + margin;
    int plot_w = vp_w - 2 * margin;
    int plot_h = vp_h - 2 * margin;

    DrawLine(plot_x, plot_y + plot_h, plot_x + plot_w, plot_y + plot_h, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, plot_y + plot_h, ui_colors::TEXT_SECONDARY);
    draw_text("x", static_cast<float>(plot_x + plot_w + 4),
              static_cast<float>(plot_y + plot_h - 6), 12, ui_colors::TEXT_SECONDARY);
    draw_text("|\xcf\x88(x)|\xc2\xb2", static_cast<float>(plot_x - 40),
              static_cast<float>(plot_y - 14), 12, ui_colors::TEXT_SECONDARY);

    for (int i = 1; i < 10; ++i) {
        int gx = plot_x + plot_w * i / 10;
        DrawLine(gx, plot_y, gx, plot_y + plot_h, ui_colors::GRID_LINE);
    }
    for (int i = 1; i < 5; ++i) {
        int gy = plot_y + plot_h * i / 5;
        DrawLine(plot_x, gy, plot_x + plot_w, gy, ui_colors::GRID_LINE);
    }

    double x_min = solver_.get_x_min();
    double x_max = solver_.get_x_max();
    double x_range = x_max - x_min;

    auto x_to_screen = [&](double x) -> float {
        double frac = (x - x_min) / x_range;
        return static_cast<float>(plot_x) + static_cast<float>(frac) * static_cast<float>(plot_w);
    };

    // Barrier rectangle
    float bar_sx = x_to_screen(0.0);
    float bar_ex = x_to_screen(barrier_l());
    double e_frac = particle_energy() / v0();
    float barrier_visual_h = static_cast<float>(plot_h) * 0.6f;

    DrawRectangleV({bar_sx, static_cast<float>(plot_y + plot_h) - barrier_visual_h},
                   {bar_ex - bar_sx, barrier_visual_h}, BARRIER_FILL);
    DrawRectangleLinesEx({bar_sx, static_cast<float>(plot_y + plot_h) - barrier_visual_h,
                          bar_ex - bar_sx, barrier_visual_h}, 1.5f, BARRIER_OUTLINE);

    char v0_label[32];
    std::snprintf(v0_label, sizeof(v0_label), "V\xe2\x82\x80=%.1f", v0());
    draw_text(v0_label, (bar_sx + bar_ex) * 0.5f - 15.0f,
              static_cast<float>(plot_y + plot_h) - barrier_visual_h - 16.0f, 11, BARRIER_OUTLINE);

    // Energy level line across barrier
    if (e_frac > 0.0 && e_frac < 1.5) {
        float e_screen_y = static_cast<float>(plot_y + plot_h) -
                           static_cast<float>(e_frac) * barrier_visual_h;
        DrawLineEx({bar_sx - 30.0f, e_screen_y}, {bar_ex + 30.0f, e_screen_y},
                   1.0f, MARKER_COLOR);
        draw_text("E", bar_ex + 34.0f, e_screen_y - 6.0f, 11, MARKER_COLOR);
    }

    // Probability density curve with color-coded regions
    auto prob = solver_.get_probability_density();
    int nx = solver_.get_nx();
    double dx = solver_.get_dx();

    double max_prob = *std::max_element(prob.begin(), prob.end());
    if (max_prob < 1e-15) max_prob = 1.0;
    float scale = static_cast<float>(plot_h) * 0.85f / static_cast<float>(max_prob);

    double barrier_start = 0.0;
    double barrier_end = barrier_l();

    float base_y = static_cast<float>(plot_y + plot_h);

    for (int i = 1; i < nx; ++i) {
        double x0 = x_min + (i - 1) * dx;
        double x1 = x_min + i * dx;
        float sx0 = x_to_screen(x0);
        float sx1 = x_to_screen(x1);
        float sy0 = base_y - static_cast<float>(prob[i - 1]) * scale;
        float sy1 = base_y - static_cast<float>(prob[i]) * scale;

        sy0 = std::max(sy0, static_cast<float>(plot_y));
        sy1 = std::max(sy1, static_cast<float>(plot_y));

        Color line_color;
        if (x1 < barrier_start) {
            line_color = INCIDENT_COLOR;
        } else if (x1 > barrier_end) {
            line_color = TRANSMITTED_COLOR;
        } else {
            line_color = REFLECTED_COLOR;
        }

        Color fill = {line_color.r, line_color.g, line_color.b, 60};
        DrawTriangle({sx0, base_y}, {sx0, sy0}, {sx1, sy1}, fill);
        DrawTriangle({sx0, base_y}, {sx1, sy1}, {sx1, base_y}, fill);

        DrawLineEx({sx0, sy0}, {sx1, sy1}, 1.5f, line_color);
    }

    // Transmission / Reflection percentages
    double T = transmission_coefficient(particle_energy(), v0(), barrier_l());
    double R = 1.0 - T;
    double T_num = numerical_transmission();

    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "T(analytical) = %.2f%%", T * 100.0);
    draw_text(tbuf, static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 8), 13, TRANSMITTED_COLOR);

    std::snprintf(tbuf, sizeof(tbuf), "R(analytical) = %.2f%%", R * 100.0);
    draw_text(tbuf, static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 24), 13, INCIDENT_COLOR);

    std::snprintf(tbuf, sizeof(tbuf), "T(numerical)  = %.2f%%", T_num * 100.0);
    draw_text(tbuf, static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 40), 13, ui_colors::TEXT_SECONDARY);

    // Legend
    int lx = vp_x + vp_w - 160;
    int ly = vp_y + 8;
    DrawRectangle(lx - 2, ly - 2, 154, 54, {30, 30, 38, 200});
    DrawRectangle(lx, ly + 2, 10, 10, INCIDENT_COLOR);
    draw_text("Incident/Reflected", static_cast<float>(lx + 14), static_cast<float>(ly), 11, ui_colors::TEXT_SECONDARY);
    DrawRectangle(lx, ly + 18, 10, 10, REFLECTED_COLOR);
    draw_text("Inside Barrier", static_cast<float>(lx + 14), static_cast<float>(ly + 16), 11, ui_colors::TEXT_SECONDARY);
    DrawRectangle(lx, ly + 34, 10, 10, TRANSMITTED_COLOR);
    draw_text("Transmitted", static_cast<float>(lx + 14), static_cast<float>(ly + 32), 11, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// View 1 — Wavefunction (3D with GridSolver)
// --------------------------------------------------------------------------

void TunnelingScenario::render_wavefunction_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    BeginScissorMode(vp_x, vp_y, vp_w, vp_h);
    ClearBackground(ui_colors::BG_DARK);

    BeginMode3D(cam);

    auto prob = solver_.get_probability_density();
    auto& pot = solver_.get_potential();
    int nx = solver_.get_nx();
    double dx = solver_.get_dx();
    double x_min = solver_.get_x_min();

    float scale_y = 200.0f;
    float scale_pot = 0.001f;

    for (int i = 0; i < nx; ++i) {
        float x0 = static_cast<float>(x_min + i * dx);
        float y0 = static_cast<float>(prob[i]) * scale_y;

        if (y0 > 0.001f) {
            DrawCubeV({x0, y0 * 0.5f, 0.0f},
                      {static_cast<float>(dx) * 0.8f, y0, 0.2f}, ui_colors::WAVEFUNCTION);
        }

        float pot_h = static_cast<float>(pot[i]) * scale_pot;
        if (pot_h > 0.01f) {
            pot_h = std::min(pot_h, 3.0f);
            DrawCubeV({x0, pot_h * 0.5f, 0.0f},
                      {static_cast<float>(dx) * 0.8f, pot_h, 0.4f}, ui_colors::POTENTIAL);
        }
    }

    DrawGrid(20, 1.0f);

    EndMode3D();
    EndScissorMode();

    draw_text("Wavefunction |\xcf\x88|\xc2\xb2",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 8), 14, ui_colors::ACCENT);

    char info[64];
    std::snprintf(info, sizeof(info), "Norm: %.4f  t=%.3f", solver_.get_norm(), solver_.get_time());
    draw_text(info, static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 26), 12, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// View 2 — T(E) curve
// --------------------------------------------------------------------------

void TunnelingScenario::render_te_curve_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    int margin = 60;
    int plot_x = vp_x + margin;
    int plot_y = vp_y + margin;
    int plot_w = vp_w - 2 * margin;
    int plot_h = vp_h - 2 * margin;

    double current_v0 = v0();
    double current_l = barrier_l();
    double current_e = particle_energy();

    double e_min = 0.1;
    double e_max = std::max(current_v0 * 2.5, 5.0);

    // Axes
    DrawLine(plot_x, plot_y + plot_h, plot_x + plot_w, plot_y + plot_h, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, plot_y + plot_h, ui_colors::TEXT_SECONDARY);

    draw_text("Energy E", static_cast<float>(plot_x + plot_w / 2 - 25),
              static_cast<float>(plot_y + plot_h + 12), 12, ui_colors::TEXT_SECONDARY);
    draw_text("T(E)", static_cast<float>(plot_x - 42),
              static_cast<float>(plot_y - 5), 12, ui_colors::TEXT_SECONDARY);

    // Y-axis tick marks and grid (T = 0 to 1)
    for (int i = 0; i <= 4; ++i) {
        float frac = static_cast<float>(i) / 4.0f;
        int gy = plot_y + plot_h - static_cast<int>(frac * static_cast<float>(plot_h));
        DrawLine(plot_x - 4, gy, plot_x, gy, ui_colors::TEXT_SECONDARY);
        DrawLine(plot_x, gy, plot_x + plot_w, gy, ui_colors::GRID_LINE);

        char label[8];
        std::snprintf(label, sizeof(label), "%.2f", static_cast<double>(frac));
        draw_text(label, static_cast<float>(plot_x - 38), static_cast<float>(gy - 5),
                  10, ui_colors::TEXT_SECONDARY);
    }

    // X-axis tick marks and grid
    int n_x_ticks = 5;
    for (int i = 0; i <= n_x_ticks; ++i) {
        double e_val = e_min + (e_max - e_min) * static_cast<double>(i) / static_cast<double>(n_x_ticks);
        int gx = plot_x + static_cast<int>(static_cast<double>(plot_w) *
                 static_cast<double>(i) / static_cast<double>(n_x_ticks));
        DrawLine(gx, plot_y + plot_h, gx, plot_y + plot_h + 4, ui_colors::TEXT_SECONDARY);
        DrawLine(gx, plot_y, gx, plot_y + plot_h, ui_colors::GRID_LINE);

        char label[8];
        std::snprintf(label, sizeof(label), "%.1f", e_val);
        draw_text(label, static_cast<float>(gx - 10), static_cast<float>(plot_y + plot_h + 6),
                  10, ui_colors::TEXT_SECONDARY);
    }

    // V0 vertical marker
    double v0_frac = (current_v0 - e_min) / (e_max - e_min);
    if (v0_frac > 0.0 && v0_frac < 1.0) {
        int v0_screen_x = plot_x + static_cast<int>(v0_frac * static_cast<double>(plot_w));
        DrawLineEx({static_cast<float>(v0_screen_x), static_cast<float>(plot_y)},
                   {static_cast<float>(v0_screen_x), static_cast<float>(plot_y + plot_h)},
                   1.0f, ui_colors::DANGER);
        draw_text("V\xe2\x82\x80", static_cast<float>(v0_screen_x + 3),
                  static_cast<float>(plot_y + 2), 11, ui_colors::DANGER);
    }

    // T(E) curve
    int n_points = plot_w;
    float prev_px = 0.0f, prev_py = 0.0f;

    for (int i = 0; i < n_points; ++i) {
        double frac = static_cast<double>(i) / static_cast<double>(n_points - 1);
        double E = e_min + frac * (e_max - e_min);
        double T = transmission_coefficient(E, current_v0, current_l);

        float px = static_cast<float>(plot_x) + static_cast<float>(frac) * static_cast<float>(plot_w);
        float py = static_cast<float>(plot_y + plot_h) - static_cast<float>(T) * static_cast<float>(plot_h);

        if (static_cast<float>(plot_y + plot_h) - py > 1.0f) {
            DrawLine(static_cast<int>(px), static_cast<int>(py),
                     static_cast<int>(px), plot_y + plot_h, PLOT_FILL);
        }

        Color line_col = (E >= current_v0) ? CURVE_ABOVE : PLOT_LINE;
        if (i > 0) {
            DrawLineEx({prev_px, prev_py}, {px, py}, 2.0f, line_col);
        }
        prev_px = px;
        prev_py = py;
    }

    // Current energy marker dot
    double e_frac_current = (current_e - e_min) / (e_max - e_min);
    if (e_frac_current >= 0.0 && e_frac_current <= 1.0) {
        double T_current = transmission_coefficient(current_e, current_v0, current_l);
        float mark_x = static_cast<float>(plot_x) +
                       static_cast<float>(e_frac_current) * static_cast<float>(plot_w);
        float mark_y = static_cast<float>(plot_y + plot_h) -
                       static_cast<float>(T_current) * static_cast<float>(plot_h);

        DrawCircleV({mark_x, mark_y}, 6.0f, MARKER_COLOR);
        DrawCircleV({mark_x, mark_y}, 3.5f, ui_colors::BG_DARK);

        for (int d = static_cast<int>(mark_y); d < plot_y + plot_h; d += 4) {
            DrawPixel(static_cast<int>(mark_x), d, MARKER_COLOR);
        }

        char ebuf[48];
        std::snprintf(ebuf, sizeof(ebuf), "E=%.2f, T=%.4f", current_e, T_current);
        draw_text(ebuf, mark_x + 8.0f, mark_y - 14.0f, 11, MARKER_COLOR);
    }

    // Formula label
    draw_text("T(E) = 1 / (1 + V\xe2\x82\x80\xc2\xb2sinh\xc2\xb2(\xce\xbaL) / (4E(V\xe2\x82\x80-E)))",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 8), 13, ui_colors::ACCENT);

    char params[128];
    std::snprintf(params, sizeof(params), "V\xe2\x82\x80=%.1f  L=%.2f  E=%.2f",
                  current_v0, current_l, current_e);
    draw_text(params, static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 26), 12,
              ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// UI helpers
// --------------------------------------------------------------------------

bool TunnelingScenario::render_toggle_button(const char* label_on, const char* label_off,
                                             bool state, int x, int y, int w) {
    const char* text = state ? label_on : label_off;
    Color bg = state ? ui_colors::ACCENT : Color{55, 55, 65, 255};
    Color fg = state ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY;

    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y),
                          static_cast<float>(w), 26.0f}, 0.3f, 4, bg);

    float tw = measure_text(text, 13);
    draw_text(text, static_cast<float>(x) + (static_cast<float>(w) - tw) * 0.5f,
              static_cast<float>(y) + 6.0f, 13, fg);

    Vector2 mouse = GetMousePosition();
    bool hovered = mouse.x >= static_cast<float>(x) && mouse.x <= static_cast<float>(x + w) &&
                   mouse.y >= static_cast<float>(y) && mouse.y <= static_cast<float>(y + 26);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool TunnelingScenario::render_button(const char* label, int x, int y, int w) {
    Color bg = {55, 55, 65, 255};
    Color fg = ui_colors::TEXT_SECONDARY;

    Vector2 mouse = GetMousePosition();
    bool hovered = mouse.x >= static_cast<float>(x) && mouse.x <= static_cast<float>(x + w) &&
                   mouse.y >= static_cast<float>(y) && mouse.y <= static_cast<float>(y + 26);

    if (hovered) {
        bg = ui_colors::PANEL_HOVER;
        fg = ui_colors::TEXT_PRIMARY;
    }

    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y),
                          static_cast<float>(w), 26.0f}, 0.3f, 4, bg);

    float tw = measure_text(label, 13);
    draw_text(label, static_cast<float>(x) + (static_cast<float>(w) - tw) * 0.5f,
              static_cast<float>(y) + 6.0f, 13, fg);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// --------------------------------------------------------------------------
// Controls panel (left side)
// --------------------------------------------------------------------------

void TunnelingScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int cx = x + 10;
    int cy = y + 10;
    int cw = w - 20;

    draw_section("BARRIER PARAMETERS", cx, cy);
    cy += 4;

    bool changed = false;

    if (v0_slider_.render(font_, has_font_, cx, cy, cw)) changed = true;
    cy += Slider::HEIGHT + 4;

    if (l_slider_.render(font_, has_font_, cx, cy, cw)) changed = true;
    cy += Slider::HEIGHT + 4;

    if (k0_slider_.render(font_, has_font_, cx, cy, cw)) changed = true;
    cy += Slider::HEIGHT + 8;

    if (changed) {
        rebuild_barrier();
        solver_.reset();
        inject_packet();
        auto_fire_timer_ = 0.0;
    }

    draw_separator(cx, cy, cw);

    draw_section("ACTIONS", cx, cy);
    cy += 4;

    if (render_toggle_button("Auto-Fire: ON", "Auto-Fire: OFF", auto_fire_, cx, cy, cw)) {
        auto_fire_ = !auto_fire_;
        auto_fire_timer_ = 0.0;
    }
    cy += 34;

    if (render_button("Reset Simulation", cx, cy, cw)) {
        reset_simulation();
    }
    cy += 34;

    draw_separator(cx, cy, cw);
    cy += 4;

    draw_text("[F] Toggle Auto-Fire", static_cast<float>(cx), static_cast<float>(cy),
              11, ui_colors::TEXT_DISABLED);
    cy += 14;
    draw_text("[R] Reset", static_cast<float>(cx), static_cast<float>(cy),
              11, ui_colors::TEXT_DISABLED);
    cy += 14;

    cy += 8;
    if (HelpPopup::render_help_button(font_, has_font_, cx + 10, cy + 4)) {
        help_popup_.show({"Quantum Tunneling", "Barrier Penetration",
            "A quantum particle can pass through a potential\n"
            "barrier even when its energy is less than the\n"
            "barrier height. The transmission probability\n"
            "depends exponentially on the barrier width and\n"
            "the energy deficit.",
            "T = 1/(1 + V\xe2\x82\x80\xc2\xb2sinh\xc2\xb2(\xce\xbaL)/(4E(V\xe2\x82\x80-E)))\n"
            "\xce\xba = \xe2\x88\x9a(2m(V\xe2\x82\x80-E))/\xe2\x84\x8f",
            "Dimensionless probability"});
    }
    draw_text("Help: Tunneling", static_cast<float>(cx + 24), static_cast<float>(cy - 2),
              12, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// Properties panel (right side)
// --------------------------------------------------------------------------

void TunnelingScenario::render_properties(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int px = x + 10;
    int py = y + 10;

    draw_section("BARRIER", px, py);

    char buf[64];

    std::snprintf(buf, sizeof(buf), "%.1f", v0());
    draw_prop("V\xe2\x82\x80:", buf, px, py, ui_colors::DANGER);

    std::snprintf(buf, sizeof(buf), "%.2f", barrier_l());
    draw_prop("Width (L):", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%.1f", k0());
    draw_prop("k\xe2\x82\x80:", buf, px, py);

    py += 4;
    draw_separator(px, py, w - 20);

    draw_section("PARTICLE ENERGY", px, py);

    double current_e = particle_energy();

    std::snprintf(buf, sizeof(buf), "%.4f", current_e);
    draw_prop("E = \xe2\x84\x8f\xc2\xb2k\xe2\x82\x80\xc2\xb2/2m:", buf, px, py, ui_colors::ACCENT);

    double ratio = current_e / v0();
    std::snprintf(buf, sizeof(buf), "%.4f", ratio);
    draw_prop("E/V\xe2\x82\x80:", buf, px, py,
              ratio >= 1.0 ? ui_colors::SUCCESS : MARKER_COLOR);

    draw_text(ratio >= 1.0 ? "Above barrier" : "Below barrier (tunneling)",
              static_cast<float>(px + 10), static_cast<float>(py), 11,
              ratio >= 1.0 ? ui_colors::SUCCESS : MARKER_COLOR);
    py += 18;

    py += 4;
    draw_separator(px, py, w - 20);

    draw_section("TRANSMISSION", px, py);

    double T = transmission_coefficient(current_e, v0(), barrier_l());
    double R = 1.0 - T;

    std::snprintf(buf, sizeof(buf), "%.6f (%.2f%%)", T, T * 100.0);
    draw_prop("T:", buf, px, py, TRANSMITTED_COLOR);

    std::snprintf(buf, sizeof(buf), "%.6f (%.2f%%)", R, R * 100.0);
    draw_prop("R:", buf, px, py, INCIDENT_COLOR);

    if (current_e < v0()) {
        double kappa = std::sqrt(2.0 * (v0() - current_e));
        std::snprintf(buf, sizeof(buf), "%.4f", kappa);
        draw_prop("\xce\xba:", buf, px, py);
    } else {
        double k2 = std::sqrt(2.0 * (current_e - v0()));
        std::snprintf(buf, sizeof(buf), "%.4f", k2);
        draw_prop("k\xe2\x82\x82:", buf, px, py);
    }

    double T_num = numerical_transmission();
    std::snprintf(buf, sizeof(buf), "%.4f%%", T_num * 100.0);
    draw_prop("T(numerical):", buf, px, py, ui_colors::TEXT_SECONDARY);

    py += 4;
    draw_separator(px, py, w - 20);

    draw_section("GRID SOLVER", px, py);

    std::snprintf(buf, sizeof(buf), "%.6f", solver_.get_norm());
    draw_prop("Norm:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%.4f", solver_.get_time());
    draw_prop("Sim time:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%d", solver_.get_nx());
    draw_prop("Grid points:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%.5f", solver_.get_dx());
    draw_prop("dx:", buf, px, py);

    py += 8;
    draw_separator(px, py, w - 20);

    draw_section("STATUS", px, py);

    draw_prop("Auto-fire:", auto_fire_ ? "ON" : "OFF", px, py,
              auto_fire_ ? ui_colors::SUCCESS : ui_colors::TEXT_DISABLED);

    if (auto_fire_) {
        std::snprintf(buf, sizeof(buf), "%.1fs / %.1fs", auto_fire_timer_, AUTO_FIRE_INTERVAL);
        draw_prop("Next fire:", buf, px, py);
    }
}
