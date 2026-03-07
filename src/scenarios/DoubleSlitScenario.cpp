#include <cmath>
#include <cstdio>
#include <algorithm>
#include <random>

#include <raylib.h>
#include <rlgl.h>

#include "DoubleSlitScenario.hpp"

namespace {
    constexpr Color EMITTER_COLOR   = {60, 180, 220, 255};
    constexpr Color BARRIER_COLOR   = {100, 100, 110, 255};
    constexpr Color SCREEN_COLOR    = {70, 70, 80, 255};
    constexpr Color WAVE_COLOR      = {80, 160, 240, 120};
    constexpr Color PARTICLE_COLOR  = {220, 200, 80, 255};
    constexpr Color DETECT_HIT      = {120, 220, 120, 255};
    constexpr Color HISTOGRAM_COLOR = {80, 200, 120, 200};
    constexpr Color PLOT_LINE       = {90, 160, 220, 255};
    constexpr Color PLOT_FILL       = {90, 160, 220, 60};
    constexpr Color DETECTOR_GLOW   = {220, 80, 80, 200};

    std::mt19937& rng() {
        static std::mt19937 gen{std::random_device{}()};
        return gen;
    }

    float lerp_f(float a, float b, float t) {
        return a + t * (b - a);
    }

    Color color_from_intensity(float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        auto r = static_cast<unsigned char>(lerp_f(10.0f, 90.0f, t) + t * t * 165.0f);
        auto g = static_cast<unsigned char>(lerp_f(10.0f, 160.0f, t));
        auto b = static_cast<unsigned char>(lerp_f(30.0f, 220.0f, t));
        return {r, g, b, 255};
    }
}

// --------------------------------------------------------------------------
// Construction / lifecycle
// --------------------------------------------------------------------------

DoubleSlitScenario::DoubleSlitScenario()
    : solver_(GRID_NX, GRID_X_MIN, GRID_X_MAX, GRID_DT)
    , potential_(GRID_NX, GRID_X_MIN, GRID_X_MAX)
    , slit_width_slider_("Slit Width (a)", 0.1f, 3.0f, 0.8f, "%.2f")
    , slit_sep_slider_("Slit Separation (d)", 0.5f, 5.0f, 2.0f, "%.2f")
    , wavelength_slider_("Wavelength (λ)", 0.3f, 3.0f, 1.0f, "%.2f")
    , particle_rate_slider_("Particle Rate", 1.0f, 50.0f, 15.0f, "%.0f")
    , histogram_(HISTOGRAM_BINS, 0.0f)
{}

void DoubleSlitScenario::on_enter() {
    reset_simulation();
}

void DoubleSlitScenario::reset_simulation() {
    solver_.reset();
    rebuild_potential();
    solver_.inject_gaussian(-6.0, 0.8, 2.0 * PI / wavelength());

    histogram_.assign(HISTOGRAM_BINS, 0.0f);
    particles_.clear();
    particles_detected_ = 0;
    sim_time_ = 0.0;
    particle_spawn_accum_ = 0.0;
}

void DoubleSlitScenario::rebuild_potential() {
    potential_.clear();
    potential_.set_double_slit(0.0, 0.4, slit_width(), slit_sep(), 1e6);
    solver_.set_potential(potential_.values());
}

// --------------------------------------------------------------------------
// Physics helpers
// --------------------------------------------------------------------------

double DoubleSlitScenario::sinc(double x) {
    if (std::abs(x) < 1e-12) return 1.0;
    return std::sin(x) / x;
}

double DoubleSlitScenario::intensity_at_angle(double sin_theta) const {
    double a = slit_width();
    double d = slit_sep();
    double lam = wavelength();

    double alpha = PI * a * sin_theta / lam;
    double beta  = PI * d * sin_theta / lam;

    double s = sinc(alpha);
    double c = std::cos(beta);
    return (s * s) * (c * c);
}

double DoubleSlitScenario::fringe_spacing() const {
    double d = slit_sep();
    if (d < 1e-9) return 0.0;
    return wavelength() / d;
}

// --------------------------------------------------------------------------
// Input
// --------------------------------------------------------------------------

void DoubleSlitScenario::handle_input() {
    help_popup_.handle_input();
    if (help_popup_.is_open()) return;

    if (IsKeyPressed(KEY_W)) {
        wave_mode_ = !wave_mode_;
        reset_simulation();
    }
    if (IsKeyPressed(KEY_D)) {
        detector_on_ = !detector_on_;
        histogram_.assign(HISTOGRAM_BINS, 0.0f);
        particles_.clear();
        particles_detected_ = 0;
    }
}

// --------------------------------------------------------------------------
// Update
// --------------------------------------------------------------------------

void DoubleSlitScenario::update(double dt) {
    sim_time_ += dt;

    if (wave_mode_) {
        for (int i = 0; i < 4; ++i) {
            solver_.time_step();
        }
    } else {
        double rate = static_cast<double>(particle_rate_slider_.get_value());
        particle_spawn_accum_ += rate * dt;
        while (particle_spawn_accum_ >= 1.0) {
            spawn_particle();
            particle_spawn_accum_ -= 1.0;
        }
        advance_particles(dt);
    }
}

void DoubleSlitScenario::spawn_particle() {
    Vector2 p;
    p.x = 0.05f;
    std::uniform_real_distribution<float> angle_dist(-0.3f, 0.3f);
    p.y = 0.5f + angle_dist(rng()) * 0.1f;
    particles_.push_back(p);
}

void DoubleSlitScenario::advance_particles(double dt) {
    float speed = 0.4f;
    float screen_x = 0.92f;

    for (auto& p : particles_) {
        p.x += speed * static_cast<float>(dt);
    }

    float barrier_x = 0.48f;
    float barrier_x_end = 0.52f;
    float a_norm = static_cast<float>(slit_width()) / 6.0f;
    float d_norm = static_cast<float>(slit_sep()) / 10.0f;
    float slit1_center = 0.5f + d_norm * 0.5f;
    float slit2_center = 0.5f - d_norm * 0.5f;

    std::vector<Vector2> surviving;
    surviving.reserve(particles_.size());

    for (auto& p : particles_) {
        if (p.x >= barrier_x && p.x <= barrier_x_end) {
            bool in_slit1 = std::abs(p.y - slit1_center) < a_norm * 0.5f;
            bool in_slit2 = std::abs(p.y - slit2_center) < a_norm * 0.5f;
            if (!in_slit1 && !in_slit2) continue;

            if (detector_on_ && in_slit1) {
                // Detected at slit 1 — particle goes straight
            }
        }

        if (p.x > barrier_x_end && p.x < barrier_x_end + speed * static_cast<float>(dt) + 0.01f) {
            if (!detector_on_) {
                double sin_theta = static_cast<double>(p.y - 0.5f) * 4.0;
                double prob = intensity_at_angle(sin_theta);
                std::normal_distribution<float> jitter(0.0f, static_cast<float>(wavelength()) * 0.06f);
                double rand_val = static_cast<double>(std::abs(jitter(rng())));
                p.y = 0.5f + static_cast<float>(sin_theta * 0.1) + jitter(rng());
            }
        }

        if (p.x >= screen_x) {
            float y_pos = std::clamp(p.y, 0.0f, 1.0f);
            int bin = static_cast<int>(y_pos * static_cast<float>(HISTOGRAM_BINS - 1));
            bin = std::clamp(bin, 0, HISTOGRAM_BINS - 1);
            histogram_[bin] += 1.0f;
            particles_detected_++;
            continue;
        }

        surviving.push_back(p);
    }
    particles_ = std::move(surviving);
}

// --------------------------------------------------------------------------
// Toggle button helper
// --------------------------------------------------------------------------

bool DoubleSlitScenario::render_toggle_button(const char* label_on, const char* label_off, bool state, int x, int y, int w) {
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

// --------------------------------------------------------------------------
// View queries
// --------------------------------------------------------------------------

const char* DoubleSlitScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Setup";
        case 1: return "Wavefunction";
        case 2: return "Pattern";
        case 3: return "Heatmap";
        default: return "Unknown";
    }
}

bool DoubleSlitScenario::uses_3d() const {
    return current_view_ == 1;
}

// --------------------------------------------------------------------------
// Viewport dispatch
// --------------------------------------------------------------------------

void DoubleSlitScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    switch (current_view_) {
        case 0: render_setup_view(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_wavefunction_view(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_pattern_view(vp_x, vp_y, vp_w, vp_h); break;
        case 3: render_heatmap_view(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }

    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

// --------------------------------------------------------------------------
// View 0 — Setup (2D top-down)
// --------------------------------------------------------------------------

void DoubleSlitScenario::render_setup_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    int margin = 30;
    int ax = vp_x + margin;
    int ay = vp_y + margin;
    int aw = vp_w - 2 * margin;
    int ah = vp_h - 2 * margin;

    // Emitter
    int emitter_x = ax + 10;
    int emitter_cy = ay + ah / 2;
    DrawRectangle(emitter_x, emitter_cy - 15, 12, 30, EMITTER_COLOR);
    draw_text("Source", static_cast<float>(emitter_x - 4), static_cast<float>(emitter_cy + 20), 11, ui_colors::TEXT_SECONDARY);

    // Barrier with slits
    int barrier_x = ax + aw / 2 - 4;
    int barrier_w = 8;
    float a_frac = static_cast<float>(slit_width()) / 6.0f;
    float d_frac = static_cast<float>(slit_sep()) / 10.0f;
    int slit_h = static_cast<int>(a_frac * static_cast<float>(ah));
    int slit_offset = static_cast<int>(d_frac * 0.5f * static_cast<float>(ah));

    int slit1_top = emitter_cy - slit_offset - slit_h / 2;
    int slit2_top = emitter_cy + slit_offset - slit_h / 2;

    DrawRectangle(barrier_x, ay, barrier_w, slit1_top - ay, BARRIER_COLOR);
    DrawRectangle(barrier_x, slit1_top + slit_h, barrier_w, slit2_top - (slit1_top + slit_h), BARRIER_COLOR);
    DrawRectangle(barrier_x, slit2_top + slit_h, barrier_w, (ay + ah) - (slit2_top + slit_h), BARRIER_COLOR);

    if (detector_on_) {
        DrawCircle(barrier_x + barrier_w / 2, slit1_top + slit_h / 2, 6.0f, DETECTOR_GLOW);
        draw_text("DET", static_cast<float>(barrier_x - 8), static_cast<float>(slit1_top - 14), 10, DETECTOR_GLOW);
    }

    // Detection screen
    int screen_x = ax + aw - 20;
    DrawRectangle(screen_x, ay, 6, ah, SCREEN_COLOR);

    if (wave_mode_) {
        // Animate circular wavefronts
        float phase = static_cast<float>(sim_time_) * 3.0f;
        float lam_px = static_cast<float>(wavelength()) / 3.0f * static_cast<float>(aw) * 0.3f;

        for (int ring = 0; ring < 20; ++ring) {
            float radius = std::fmod(static_cast<float>(ring) * lam_px + phase * lam_px * 0.3f,
                                     static_cast<float>(aw));
            if (radius < 5.0f) continue;

            float alpha_factor = 1.0f - radius / static_cast<float>(aw);
            if (alpha_factor <= 0.0f) continue;
            auto alpha = static_cast<unsigned char>(alpha_factor * 100.0f);

            Color wc = {WAVE_COLOR.r, WAVE_COLOR.g, WAVE_COLOR.b, alpha};

            // Wavefronts from slit 1
            DrawCircleLines(barrier_x + barrier_w, slit1_top + slit_h / 2, radius, wc);
            // Wavefronts from slit 2
            DrawCircleLines(barrier_x + barrier_w, slit2_top + slit_h / 2, radius, wc);
        }

        // Interference pattern on screen
        for (int sy = 0; sy < ah; ++sy) {
            float y_norm = (static_cast<float>(sy) / static_cast<float>(ah) - 0.5f) * 2.0f;
            double sin_theta = static_cast<double>(y_norm) * 0.5;
            double I = intensity_at_angle(sin_theta);
            auto brightness = static_cast<unsigned char>(std::clamp(I * 255.0, 0.0, 255.0));
            DrawRectangle(screen_x, ay + sy, 6, 1, {brightness, brightness, static_cast<unsigned char>(brightness / 2), 255});
        }
    } else {
        // Draw active particles
        for (const auto& p : particles_) {
            int px = ax + static_cast<int>(p.x * static_cast<float>(aw));
            int py = ay + static_cast<int>(p.y * static_cast<float>(ah));
            DrawCircle(px, py, 2.5f, PARTICLE_COLOR);
        }

        // Draw histogram on detection screen
        float max_hist = *std::max_element(histogram_.begin(), histogram_.end());
        if (max_hist < 1.0f) max_hist = 1.0f;

        for (int i = 0; i < HISTOGRAM_BINS; ++i) {
            float frac = histogram_[i] / max_hist;
            int bar_w = static_cast<int>(frac * 40.0f);
            int by = ay + static_cast<int>(static_cast<float>(i) / static_cast<float>(HISTOGRAM_BINS) * static_cast<float>(ah));
            if (bar_w > 0) {
                DrawRectangle(screen_x - bar_w, by, bar_w, std::max(1, ah / HISTOGRAM_BINS), HISTOGRAM_COLOR);
            }
        }
    }

    // Labels
    draw_text(wave_mode_ ? "WAVE MODE" : "PARTICLE MODE",
              static_cast<float>(vp_x + vp_w / 2 - 40), static_cast<float>(vp_y + 8), 14, ui_colors::ACCENT);
}

// --------------------------------------------------------------------------
// View 1 — Wavefunction (3D with GridSolver)
// --------------------------------------------------------------------------

void DoubleSlitScenario::render_wavefunction_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
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

    for (int i = 0; i < nx - 1; ++i) {
        float x0 = static_cast<float>(x_min + i * dx);
        float x1 = static_cast<float>(x_min + (i + 1) * dx);
        float y0 = static_cast<float>(prob[i]) * scale_y;
        float y1 = static_cast<float>(prob[i + 1]) * scale_y;

        // Probability density bars
        if (y0 > 0.001f) {
            DrawCubeV({x0, y0 * 0.5f, 0.0f}, {static_cast<float>(dx) * 0.8f, y0, 0.2f}, ui_colors::WAVEFUNCTION);
        }

        // Potential as red cubes
        float pot_h = static_cast<float>(pot[i]) * scale_pot;
        if (pot_h > 0.01f) {
            pot_h = std::min(pot_h, 3.0f);
            DrawCubeV({x0, pot_h * 0.5f, 0.0f}, {static_cast<float>(dx) * 0.8f, pot_h, 0.4f}, ui_colors::POTENTIAL);
        }
    }

    // Ground plane grid
    DrawGrid(20, 1.0f);

    EndMode3D();
    EndScissorMode();

    draw_text("Wavefunction |ψ|²", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 8), 14, ui_colors::ACCENT);

    char info[64];
    std::snprintf(info, sizeof(info), "Norm: %.4f  t=%.3f", solver_.get_norm(), solver_.get_time());
    draw_text(info, static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 26), 12, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// View 2 — Pattern (2D intensity plot)
// --------------------------------------------------------------------------

void DoubleSlitScenario::render_pattern_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    int margin = 50;
    int plot_x = vp_x + margin;
    int plot_y = vp_y + margin;
    int plot_w = vp_w - 2 * margin;
    int plot_h = vp_h - 2 * margin;

    // Axes
    DrawLine(plot_x, plot_y + plot_h, plot_x + plot_w, plot_y + plot_h, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, plot_y + plot_h, ui_colors::TEXT_SECONDARY);

    draw_text("sin(θ)", static_cast<float>(plot_x + plot_w / 2 - 20),
              static_cast<float>(plot_y + plot_h + 8), 12, ui_colors::TEXT_SECONDARY);
    draw_text("I(θ)", static_cast<float>(plot_x - 35), static_cast<float>(plot_y - 5), 12, ui_colors::TEXT_SECONDARY);

    // Grid lines
    for (int i = 1; i < 10; ++i) {
        int gx = plot_x + plot_w * i / 10;
        DrawLine(gx, plot_y, gx, plot_y + plot_h, ui_colors::GRID_LINE);
    }
    for (int i = 1; i < 5; ++i) {
        int gy = plot_y + plot_h * i / 5;
        DrawLine(plot_x, gy, plot_x + plot_w, gy, ui_colors::GRID_LINE);
    }

    // Plot I(theta)
    int n_points = plot_w;
    float prev_px = 0.0f, prev_py = 0.0f;

    for (int i = 0; i < n_points; ++i) {
        float frac = static_cast<float>(i) / static_cast<float>(n_points - 1);
        double sin_theta = (static_cast<double>(frac) - 0.5) * 2.0;
        double I = intensity_at_angle(sin_theta);

        float px = static_cast<float>(plot_x) + frac * static_cast<float>(plot_w);
        float py = static_cast<float>(plot_y + plot_h) - static_cast<float>(I) * static_cast<float>(plot_h);

        // Fill under curve
        if (static_cast<float>(plot_y + plot_h) - py > 1.0f) {
            DrawLine(static_cast<int>(px), static_cast<int>(py),
                     static_cast<int>(px), plot_y + plot_h, PLOT_FILL);
        }

        if (i > 0) {
            DrawLineEx({prev_px, prev_py}, {px, py}, 2.0f, PLOT_LINE);
        }
        prev_px = px;
        prev_py = py;
    }

    // Mark central maximum
    float center_px = static_cast<float>(plot_x + plot_w / 2);
    DrawLineEx({center_px, static_cast<float>(plot_y)},
               {center_px, static_cast<float>(plot_y + plot_h)}, 1.0f, ui_colors::ACCENT);

    draw_text("I(θ) = I₀ cos²(πd sinθ/λ) sinc²(πa sinθ/λ)",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 8), 13, ui_colors::ACCENT);

    char params[128];
    std::snprintf(params, sizeof(params), "a=%.2f  d=%.2f  λ=%.2f  Δy=%.3f",
                  slit_width(), slit_sep(), wavelength(), fringe_spacing());
    draw_text(params, static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 26), 12, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// View 3 — Heatmap (2D color)
// --------------------------------------------------------------------------

void DoubleSlitScenario::render_heatmap_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    int margin = 40;
    int hm_x = vp_x + margin;
    int hm_y = vp_y + margin;
    int hm_w = vp_w - 2 * margin;
    int hm_h = vp_h - 2 * margin;

    int res_x = std::min(hm_w, 400);
    int res_y = std::min(hm_h, 300);

    float cell_w = static_cast<float>(hm_w) / static_cast<float>(res_x);
    float cell_h = static_cast<float>(hm_h) / static_cast<float>(res_y);

    double a = slit_width();
    double d = slit_sep();
    double lam = wavelength();
    double screen_dist = 5.0;

    for (int iy = 0; iy < res_y; ++iy) {
        double y_phys = (static_cast<double>(iy) / static_cast<double>(res_y - 1) - 0.5) * 10.0;
        for (int ix = 0; ix < res_x; ++ix) {
            double x_phys = (static_cast<double>(ix) / static_cast<double>(res_x - 1)) * 10.0 + 1.0;

            double r1 = std::sqrt(x_phys * x_phys + (y_phys - d * 0.5) * (y_phys - d * 0.5));
            double r2 = std::sqrt(x_phys * x_phys + (y_phys + d * 0.5) * (y_phys + d * 0.5));
            double path_diff = r2 - r1;

            double sin_theta = y_phys / std::sqrt(x_phys * x_phys + y_phys * y_phys);
            double I = intensity_at_angle(sin_theta);

            double distance_decay = screen_dist / std::max(x_phys, 0.5);
            I *= std::min(distance_decay, 1.0);

            float fx = static_cast<float>(hm_x) + static_cast<float>(ix) * cell_w;
            float fy = static_cast<float>(hm_y) + static_cast<float>(iy) * cell_h;

            DrawRectangleV({fx, fy}, {cell_w + 1.0f, cell_h + 1.0f}, color_from_intensity(static_cast<float>(I)));
        }
    }

    // Axis labels
    draw_text("Distance from slits →", static_cast<float>(hm_x + hm_w / 2 - 60),
              static_cast<float>(hm_y + hm_h + 6), 11, ui_colors::TEXT_SECONDARY);
    draw_text("y", static_cast<float>(hm_x - 16), static_cast<float>(hm_y + hm_h / 2), 12, ui_colors::TEXT_SECONDARY);

    // Slit position marker
    int slit_px = hm_x;
    int slit_py1 = hm_y + hm_h / 2 - static_cast<int>(d / 10.0 * 0.5 * static_cast<double>(hm_h));
    int slit_py2 = hm_y + hm_h / 2 + static_cast<int>(d / 10.0 * 0.5 * static_cast<double>(hm_h));
    DrawCircle(slit_px, slit_py1, 3.0f, EMITTER_COLOR);
    DrawCircle(slit_px, slit_py2, 3.0f, EMITTER_COLOR);

    draw_text("Double-Slit Diffraction Heatmap",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 8), 14, ui_colors::ACCENT);
}

// --------------------------------------------------------------------------
// Controls panel (left side)
// --------------------------------------------------------------------------

void DoubleSlitScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int cx = x + 10;
    int cy = y + 10;
    int cw = w - 20;

    draw_section("SLIT PARAMETERS", cx, cy);
    cy += 4;

    bool changed = false;

    if (slit_width_slider_.render(font_, has_font_, cx, cy, cw)) changed = true;
    cy += Slider::HEIGHT + 4;

    if (slit_sep_slider_.render(font_, has_font_, cx, cy, cw)) changed = true;
    cy += Slider::HEIGHT + 4;

    if (wavelength_slider_.render(font_, has_font_, cx, cy, cw)) changed = true;
    cy += Slider::HEIGHT + 8;

    if (changed) {
        rebuild_potential();
    }

    draw_separator(cx, cy, cw);

    draw_section("MODE", cx, cy);
    cy += 4;

    if (render_toggle_button("Mode: WAVE", "Mode: PARTICLE", wave_mode_, cx, cy, cw)) {
        wave_mode_ = !wave_mode_;
        reset_simulation();
    }
    cy += 34;

    if (render_toggle_button("Detector: ON", "Detector: OFF", detector_on_, cx, cy, cw)) {
        detector_on_ = !detector_on_;
        histogram_.assign(HISTOGRAM_BINS, 0.0f);
        particles_.clear();
        particles_detected_ = 0;
    }
    cy += 34;

    if (!wave_mode_) {
        draw_separator(cx, cy, cw);
        draw_section("PARTICLE", cx, cy);
        cy += 4;
        particle_rate_slider_.render(font_, has_font_, cx, cy, cw);
        cy += Slider::HEIGHT + 4;
    }

    draw_separator(cx, cy, cw);
    cy += 4;

    draw_text("[W] Toggle Wave/Particle", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 14;
    draw_text("[D] Toggle Detector", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 14;

    // Help buttons
    cy += 8;
    if (HelpPopup::render_help_button(font_, has_font_, cx + 10, cy + 4)) {
        help_popup_.show({"Double Slit", "Interference & Diffraction",
            "Waves passing through two narrow slits produce an\n"
            "interference pattern. Each slit also diffracts the\n"
            "wave, creating a sinc² envelope.",
            "I = I₀ cos²(πd sinθ/λ) · sinc²(πa sinθ/λ)",
            "Dimensionless intensity"});
    }
    draw_text("Help: Double Slit", static_cast<float>(cx + 24), static_cast<float>(cy - 2), 12, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// Properties panel (right side)
// --------------------------------------------------------------------------

void DoubleSlitScenario::render_properties(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int px = x + 10;
    int py = y + 10;

    draw_section("SLIT PARAMETERS", px, py);

    char buf[64];

    std::snprintf(buf, sizeof(buf), "%.2f", slit_width());
    draw_prop("Width (a):", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%.2f", slit_sep());
    draw_prop("Separation (d):", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%.2f", wavelength());
    draw_prop("Wavelength (λ):", buf, px, py);

    py += 4;
    draw_separator(px, py, w - 20);

    draw_section("DERIVED QUANTITIES", px, py);

    std::snprintf(buf, sizeof(buf), "%.4f", fringe_spacing());
    draw_prop("Fringe Δy:", buf, px, py, ui_colors::ACCENT);

    draw_text("Δy = λ / d", static_cast<float>(px + 10), static_cast<float>(py), 11, ui_colors::TEXT_DISABLED);
    py += 18;

    double ratio = slit_sep() / slit_width();
    std::snprintf(buf, sizeof(buf), "%.2f", ratio);
    draw_prop("d/a ratio:", buf, px, py);

    py += 4;
    draw_separator(px, py, w - 20);

    draw_section("STATUS", px, py);

    draw_prop("Mode:", wave_mode_ ? "Wave" : "Particle", px, py,
              wave_mode_ ? ui_colors::WAVEFUNCTION : PARTICLE_COLOR);

    draw_prop("Detector:", detector_on_ ? "ON" : "OFF", px, py,
              detector_on_ ? ui_colors::DANGER : ui_colors::SUCCESS);

    if (!wave_mode_) {
        std::snprintf(buf, sizeof(buf), "%d", particles_detected_);
        draw_prop("Detected:", buf, px, py, ui_colors::SUCCESS);

        std::snprintf(buf, sizeof(buf), "%zu", particles_.size());
        draw_prop("In flight:", buf, px, py);
    }

    if (wave_mode_) {
        py += 4;
        draw_separator(px, py, w - 20);
        draw_section("GRID SOLVER", px, py);

        std::snprintf(buf, sizeof(buf), "%.6f", solver_.get_norm());
        draw_prop("Norm:", buf, px, py);

        std::snprintf(buf, sizeof(buf), "%.4f", solver_.get_time());
        draw_prop("Sim time:", buf, px, py);

        std::snprintf(buf, sizeof(buf), "%d", solver_.get_nx());
        draw_prop("Grid points:", buf, px, py);
    }

    py += 8;
    draw_separator(px, py, w - 20);
    draw_section("PATTERN INFO", px, py);

    if (detector_on_) {
        draw_text("Detector at slit 1 collapses", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
        py += 14;
        draw_text("interference -> two-peak pattern", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    } else {
        draw_text("Full interference pattern:", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
        py += 14;
        draw_text("cos² fringes × sinc² envelope", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    }
}
