#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "InfiniteWellScenario.hpp"

InfiniteWellScenario::InfiniteWellScenario()
    : solver_(GRID_NX, GRID_X_MIN, GRID_X_MAX, GRID_DT)
    , potential_(GRID_NX, GRID_X_MIN, GRID_X_MAX)
    , well_width_slider_("Well Width L", 2.0f, 12.0f, 6.0f, "%.1f")
    , n_slider_("Quantum Number n", 1.0f, 8.0f, 1.0f, "%.0f")
    , superpos_n1_("c1 (n=1)", 0.0f, 1.0f, 1.0f, "%.2f")
    , superpos_n2_("c2 (n=2)", 0.0f, 1.0f, 0.5f, "%.2f")
    , superpos_n3_("c3 (n=3)", 0.0f, 1.0f, 0.0f, "%.2f")
{}

void InfiniteWellScenario::on_enter() {
    current_view_ = 0;
    rebuild_potential();
    reset_simulation();
}

double InfiniteWellScenario::energy_n(int n) const {
    double L = well_width();
    return (n * n * PI * PI) / (2.0 * L * L);
}

double InfiniteWellScenario::psi_n(int n, double x) const {
    double L = well_width();
    double x_min = -L / 2.0;
    double x_max = L / 2.0;
    if (x < x_min || x > x_max) return 0.0;
    double x_shifted = x - x_min;
    return std::sqrt(2.0 / L) * std::sin(n * PI * x_shifted / L);
}

void InfiniteWellScenario::rebuild_potential() {
    double L = well_width();
    potential_.clear();
    potential_.set_barrier(GRID_X_MIN, -L / 2.0, 1000.0);
    potential_.set_barrier(L / 2.0, GRID_X_MAX, 1000.0);
    solver_.set_potential(potential_.values());
}

void InfiniteWellScenario::reset_simulation() {
    solver_.reset();
    solver_.inject_gaussian(0.0, well_width() / 4.0, 3.0);
    solver_.set_potential(potential_.values());
}

const char* InfiniteWellScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Standing Waves";
        case 1: return "Time Evolution";
        case 2: return "Superposition";
        default: return "Unknown";
    }
}

bool InfiniteWellScenario::uses_3d() const {
    return current_view_ == 1;
}

void InfiniteWellScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        // no-op for standing wave view, but useful for time evolution
    }
}

void InfiniteWellScenario::update(double dt) {
    if (current_view_ == 1) {
        for (int i = 0; i < 5; ++i) solver_.time_step();
    }
    (void)dt;
}

void InfiniteWellScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    switch (current_view_) {
        case 0: render_standing_wave_view(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_wavefunction_view(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_superposition_view(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }

    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void InfiniteWellScenario::render_standing_wave_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("STANDING WAVES IN INFINITE WELL", static_cast<float>(vp_x + 20), static_cast<float>(vp_y + 10), 16, ui_colors::ACCENT);

    double L = well_width();
    int n = quantum_n();

    int plot_x = vp_x + 60;
    int plot_w = vp_w - 120;
    int plot_top = vp_y + 50;
    int plot_bottom = vp_y + vp_h - 60;
    int plot_h = plot_bottom - plot_top;

    // Draw well walls
    int wall_left = plot_x;
    int wall_right = plot_x + plot_w;
    DrawLine(wall_left, plot_top, wall_left, plot_bottom, ui_colors::POTENTIAL);
    DrawLine(wall_right, plot_top, wall_right, plot_bottom, ui_colors::POTENTIAL);
    DrawLine(wall_left, plot_bottom, wall_right, plot_bottom, ui_colors::TEXT_SECONDARY);

    // Draw energy levels and wavefunctions
    int max_n = std::min(n + 2, 8);
    double E_max = energy_n(max_n);

    for (int k = 1; k <= max_n; ++k) {
        double E = energy_n(k);
        int level_y = plot_bottom - static_cast<int>((E / (E_max * 1.2)) * plot_h);

        Color level_color = (k == n) ? ui_colors::ACCENT : Color{60, 60, 70, 200};
        DrawLine(wall_left, level_y, wall_right, level_y, level_color);

        char label[32];
        std::snprintf(label, sizeof(label), "n=%d  E=%.2f", k, E);
        draw_text(label, static_cast<float>(wall_right + 10), static_cast<float>(level_y - 7), 11, level_color);

        if (k == n) {
            int prev_px = wall_left;
            int prev_py = level_y;
            double amplitude = plot_h * 0.06;

            for (int px = 0; px <= plot_w; ++px) {
                double frac = static_cast<double>(px) / plot_w;
                double x = -L / 2.0 + frac * L;
                double psi = psi_n(k, x);
                int cur_px = wall_left + px;
                int cur_py = level_y - static_cast<int>(psi * amplitude);

                if (px > 0) {
                    DrawLine(prev_px, prev_py, cur_px, cur_py, ui_colors::WAVEFUNCTION);
                }
                prev_px = cur_px;
                prev_py = cur_py;
            }

            // Draw |psi|^2 filled
            for (int px = 0; px <= plot_w; px += 2) {
                double frac = static_cast<double>(px) / plot_w;
                double x = -L / 2.0 + frac * L;
                double psi = psi_n(k, x);
                double prob = psi * psi;
                int h = static_cast<int>(prob * amplitude * 2.0);
                int cur_px = wall_left + px;
                DrawLine(cur_px, level_y, cur_px, level_y - h, Color{90, 160, 220, 80});
            }
        }
    }

    // Labels
    draw_text(TextFormat("L = %.1f a.u.", L), static_cast<float>(plot_x + plot_w / 2 - 40),
              static_cast<float>(plot_bottom + 10), 13, ui_colors::TEXT_PRIMARY);
    draw_text(TextFormat("E_%d = %.4f a.u.", n, energy_n(n)), static_cast<float>(plot_x + plot_w / 2 - 50),
              static_cast<float>(plot_bottom + 28), 13, ui_colors::ACCENT);
}

void InfiniteWellScenario::render_wavefunction_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)vp_x; (void)vp_y; (void)vp_w; (void)vp_h;

    BeginMode3D(cam);
    {
        for (int i = -10; i <= 10; ++i) {
            DrawLine3D({(float)i, 0, -10}, {(float)i, 0, 10}, ui_colors::GRID_LINE);
            DrawLine3D({-10, 0, (float)i}, {10, 0, (float)i}, ui_colors::GRID_LINE);
        }
        DrawLine3D({0,0,0}, {3,0,0}, ui_colors::AXIS_X);
        DrawLine3D({0,0,0}, {0,3,0}, ui_colors::AXIS_Y);
        DrawLine3D({0,0,0}, {0,0,3}, ui_colors::AXIS_Z);

        // Potential walls
        const auto& vals = potential_.values();
        int nx = potential_.get_nx();
        double dx = potential_.get_dx();
        double x_min = potential_.get_x_min();
        int step = std::max(1, nx / 200);
        for (int i = 0; i < nx; i += step) {
            if (std::abs(vals[i]) < 1e-10) continue;
            float x = static_cast<float>(x_min + i * dx);
            DrawCube({x, 1.5f, 0.0f}, static_cast<float>(dx * step), 3.0f, 0.3f, ui_colors::POTENTIAL);
        }

        // Probability density
        auto density = solver_.get_probability_density();
        double max_p = 0.0;
        for (double p : density) if (p > max_p) max_p = p;
        if (max_p > 1e-15) {
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

    draw_text("TIME EVOLUTION", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text(TextFormat("t = %.3f", solver_.get_time()), static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 12, ui_colors::TEXT_SECONDARY);
}

void InfiniteWellScenario::render_superposition_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("SUPERPOSITION OF EIGENSTATES", static_cast<float>(vp_x + 20), static_cast<float>(vp_y + 10), 16, ui_colors::ACCENT);

    double c1 = static_cast<double>(superpos_n1_.get_value());
    double c2 = static_cast<double>(superpos_n2_.get_value());
    double c3 = static_cast<double>(superpos_n3_.get_value());
    double norm = std::sqrt(c1*c1 + c2*c2 + c3*c3);
    if (norm < 1e-10) norm = 1.0;

    double L = well_width();
    double time = GetTime();

    int plot_x = vp_x + 60;
    int plot_w = vp_w - 120;
    int plot_cy = vp_y + vp_h / 2;
    int plot_h = vp_h / 2 - 60;

    // Draw well walls
    DrawLine(plot_x, plot_cy - plot_h, plot_x, plot_cy + plot_h, ui_colors::POTENTIAL);
    DrawLine(plot_x + plot_w, plot_cy - plot_h, plot_x + plot_w, plot_cy + plot_h, ui_colors::POTENTIAL);
    DrawLine(plot_x, plot_cy, plot_x + plot_w, plot_cy, Color{50, 50, 60, 255});

    // Compute and draw |psi(x,t)|^2
    int prev_px = 0, prev_py_psi = 0, prev_py_prob = 0;
    for (int px = 0; px <= plot_w; ++px) {
        double frac = static_cast<double>(px) / plot_w;
        double x = -L / 2.0 + frac * L;

        double psi_re = 0.0, psi_im = 0.0;
        double coeffs[] = {c1, c2, c3};
        for (int n = 1; n <= 3; ++n) {
            double cn = coeffs[n - 1] / norm;
            double psi_spatial = psi_n(n, x);
            double E = energy_n(n);
            psi_re += cn * psi_spatial * std::cos(-E * time);
            psi_im += cn * psi_spatial * std::sin(-E * time);
        }

        double prob = psi_re * psi_re + psi_im * psi_im;
        double amplitude_scale = static_cast<double>(plot_h) * 0.5;

        int cur_px = plot_x + px;
        int cur_py_psi = plot_cy - static_cast<int>(psi_re * amplitude_scale);
        int cur_py_prob = plot_cy - static_cast<int>(prob * amplitude_scale * 2.0);

        if (px > 0) {
            DrawLine(prev_px, prev_py_psi, cur_px, cur_py_psi, Color{90, 160, 220, 150});
            DrawLine(prev_px, prev_py_prob, cur_px, cur_py_prob, ui_colors::ACCENT);
        }

        // Filled probability
        DrawLine(cur_px, plot_cy, cur_px, cur_py_prob, Color{70, 130, 180, 40});

        prev_px = cur_px;
        prev_py_psi = cur_py_psi;
        prev_py_prob = cur_py_prob;
    }

    draw_text("Blue line: Re(psi)    Bold: |psi|^2", static_cast<float>(plot_x),
              static_cast<float>(vp_y + vp_h - 40), 12, ui_colors::TEXT_SECONDARY);
    draw_text(TextFormat("c1=%.2f  c2=%.2f  c3=%.2f", c1, c2, c3),
              static_cast<float>(plot_x), static_cast<float>(vp_y + vp_h - 24), 12, ui_colors::TEXT_PRIMARY);
}

void InfiniteWellScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("INFINITE WELL", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    bool width_changed = well_width_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 4;

    if (current_view_ == 0) {
        bool n_changed = n_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
        if (width_changed || n_changed) {
            rebuild_potential();
        }
    } else if (current_view_ == 1) {
        if (width_changed) {
            rebuild_potential();
            reset_simulation();
        }
        cy += 4;
        draw_text("Space: pause/resume", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 16;
    } else if (current_view_ == 2) {
        if (width_changed) rebuild_potential();
        superpos_n1_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 2;
        superpos_n2_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 2;
        superpos_n3_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
    }

    cy += 8;
    draw_separator(cx, cy, w - 20);
    draw_text("Views: 1-3 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
}

void InfiniteWellScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    draw_section("Well Parameters", px, py);
    draw_prop("L", TextFormat("%.2f a.u.", well_width()), px, py);

    if (current_view_ == 0) {
        int n = quantum_n();
        draw_prop("n", TextFormat("%d", n), px, py);
        draw_prop("E_n", TextFormat("%.4f a.u.", energy_n(n)), px, py);
        py += 4;
        draw_section("Formulas", px, py);
        draw_text("E_n = n^2 * pi^2 / (2L^2)", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
        py += 16;
        draw_text("psi_n = sqrt(2/L) sin(n*pi*x/L)", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
        py += 20;

        py += 4;
        draw_section("Energy Levels", px, py);
        for (int k = 1; k <= std::min(quantum_n() + 3, 8); ++k) {
            Color c = (k == quantum_n()) ? ui_colors::ACCENT : ui_colors::TEXT_SECONDARY;
            draw_text(TextFormat("E_%d = %.4f", k, energy_n(k)), static_cast<float>(px), static_cast<float>(py), 12, c);
            py += 16;
        }
    } else if (current_view_ == 1) {
        py += 4;
        draw_section("Solver", px, py);
        draw_prop("Norm", TextFormat("%.6f", solver_.get_norm()), px, py);
        draw_prop("Time", TextFormat("%.4f", solver_.get_time()), px, py);
        draw_prop("<x>", TextFormat("%.4f", solver_.get_position_expectation()), px, py);
        draw_prop("<p>", TextFormat("%.4f", solver_.get_momentum_expectation()), px, py);
    } else if (current_view_ == 2) {
        py += 4;
        draw_section("Superposition", px, py);
        draw_text("psi = c1*psi_1 + c2*psi_2 + c3*psi_3", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
        py += 18;
        draw_text("Time evolution shown", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
        py += 18;
        draw_text("with beating frequencies", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    }
}
