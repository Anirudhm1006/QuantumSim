#include <cmath>
#include <algorithm>
#include <numeric>
#include <raylib.h>

#include "HeisenbergScenario.hpp"

HeisenbergScenario::HeisenbergScenario()
    : solver_(512, -10.0, 10.0, 0.001)
    , potential_(512, -10.0, 10.0)
    , sigma_slider_("\xCF\x83 (position spread)", 0.2f, 5.0f, 1.0f, "%.2f")
    , probe_lambda_slider_("\xCE\xBB probe (photon \xCE\xBB)", 0.05f, 2.0f, 0.5f, "%.2f")
{}

void HeisenbergScenario::on_enter() {
    current_view_ = 0;
    reset_microscope();
    reset_samples();
    reset_solver();
}

void HeisenbergScenario::reset_solver() {
    solver_.reset();
    potential_.clear();
    solver_.set_potential(potential_.values());
    solver_.inject_gaussian(0.0, sigma(), 4.0);
}

void HeisenbergScenario::reset_microscope() {
    electron_x_ = 0.5f;
    electron_y_ = 0.5f;
    electron_vx_ = 0.0f;
    electron_vy_ = 0.0f;
    photon_active_ = false;
    photon_timer_ = 0.0f;
    photon_x_ = -0.1f;
    show_measurement_ = false;
    measure_fade_ = 0.0f;
}

void HeisenbergScenario::reset_samples() {
    pos_samples_.clear();
    mom_samples_.clear();
    phase_points_.clear();
    sample_timer_ = 0.0;
}

const char* HeisenbergScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Microscope";
        case 1: return "Live Sampling";
        case 2: return "Phase Space";
        case 3: return "Wave Packet";
        default: return "Unknown";
    }
}

void HeisenbergScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
}

void HeisenbergScenario::update(double dt) {
    if (current_view_ == 0) {
        std::uniform_real_distribution<float> drift(-0.3f, 0.3f);
        electron_vx_ += drift(rng_) * static_cast<float>(dt);
        electron_vy_ += drift(rng_) * static_cast<float>(dt);
        electron_vx_ *= 0.98f;
        electron_vy_ *= 0.98f;
        electron_x_ += electron_vx_ * static_cast<float>(dt);
        electron_y_ += electron_vy_ * static_cast<float>(dt);
        electron_x_ = std::clamp(electron_x_, 0.15f, 0.85f);
        electron_y_ = std::clamp(electron_y_, 0.15f, 0.85f);

        photon_timer_ += static_cast<float>(dt);
        if (!photon_active_ && photon_timer_ > 1.5f) {
            photon_active_ = true;
            photon_x_ = -0.05f;
            photon_timer_ = 0.0f;
        }

        if (photon_active_) {
            photon_x_ += static_cast<float>(dt) * 0.6f;
            if (photon_x_ >= electron_x_ - 0.02f) {
                photon_active_ = false;
                show_measurement_ = true;
                measure_fade_ = 1.0f;

                float lam = static_cast<float>(probe_lambda());
                measure_circle_r_ = lam * 0.3f;

                float dp = static_cast<float>(HBAR / probe_lambda());
                std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * PI);
                momentum_arrow_angle_ = angle_dist(rng_);
                momentum_arrow_mag_ = dp * 0.2f;

                electron_vx_ += std::cos(momentum_arrow_angle_) * dp * 0.1f;
                electron_vy_ += std::sin(momentum_arrow_angle_) * dp * 0.1f;
            }
        }

        if (show_measurement_) {
            measure_fade_ -= static_cast<float>(dt) * 0.4f;
            if (measure_fade_ <= 0.0f) {
                show_measurement_ = false;
                measure_fade_ = 0.0f;
            }
        }
    }

    if (current_view_ == 1 || current_view_ == 2) {
        sample_timer_ += dt;
        double interval = 0.03;
        while (sample_timer_ >= interval && pos_samples_.size() < MAX_SAMPLES) {
            sample_timer_ -= interval;
            std::normal_distribution<float> pos_dist(0.0f, static_cast<float>(sigma()));
            std::normal_distribution<float> mom_dist(0.0f, static_cast<float>(delta_p()));
            float px = pos_dist(rng_);
            float pm = mom_dist(rng_);
            pos_samples_.push_back(px);
            mom_samples_.push_back(pm);
            phase_points_.push_back({px, pm});
        }
    }

    if (current_view_ == 3) {
        for (int i = 0; i < 5; ++i) solver_.time_step();
    }
}

void HeisenbergScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;
    switch (current_view_) {
        case 0: render_microscope_view(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_sampling_view(vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_phase_space_view(vp_x, vp_y, vp_w, vp_h); break;
        case 3: render_wavepacket_view(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void HeisenbergScenario::render_microscope_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("HEISENBERG MICROSCOPE", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Observing an electron with a photon", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int area_x = vp_x + 40;
    int area_y = vp_y + 50;
    int area_w = vp_w - 80;
    int area_h = vp_h - 150;

    DrawRectangle(area_x, area_y, area_w, area_h, Color{20, 20, 28, 255});
    DrawRectangleLinesEx({static_cast<float>(area_x), static_cast<float>(area_y),
                          static_cast<float>(area_w), static_cast<float>(area_h)}, 1.0f, ui_colors::PANEL_BORDER);

    int ex = area_x + static_cast<int>(electron_x_ * area_w);
    int ey = area_y + static_cast<int>(electron_y_ * area_h);

    if (photon_active_) {
        int px_screen = area_x + static_cast<int>(photon_x_ * area_w);
        int py_screen = ey;

        float wave_k = 2.0f * PI / std::max(static_cast<float>(probe_lambda()) * 80.0f, 10.0f);
        for (int i = -30; i <= 0; ++i) {
            int wx = px_screen + i * 2;
            if (wx < area_x) continue;
            float wave_y = 6.0f * std::sin(wave_k * static_cast<float>(i));
            DrawCircle(wx, py_screen + static_cast<int>(wave_y), 2, Color{255, 200, 50, 200});
        }
        DrawCircle(px_screen, py_screen, 5, Color{255, 220, 50, 255});
    }

    DrawCircle(ex, ey, 8, Color{70, 130, 220, 255});
    draw_text("e\xE2\x81\xBB", static_cast<float>(ex - 5), static_cast<float>(ey - 5), 10, WHITE);

    if (show_measurement_) {
        auto alpha = static_cast<unsigned char>(measure_fade_ * 200.0f);
        int circle_r = static_cast<int>(measure_circle_r_ * area_w);
        circle_r = std::max(circle_r, 8);
        DrawCircleLines(ex, ey, static_cast<float>(circle_r), Color{100, 200, 100, alpha});
        DrawCircleLines(ex, ey, static_cast<float>(circle_r + 1), Color{100, 200, 100, static_cast<unsigned char>(alpha / 2)});

        draw_text("\xCE\x94x", static_cast<float>(ex + circle_r + 4), static_cast<float>(ey - 8), 12,
                  Color{100, 200, 100, alpha});

        float arrow_len = momentum_arrow_mag_ * area_w;
        arrow_len = std::clamp(arrow_len, 15.0f, 120.0f);
        int ax2 = ex + static_cast<int>(std::cos(momentum_arrow_angle_) * arrow_len);
        int ay2 = ey + static_cast<int>(std::sin(momentum_arrow_angle_) * arrow_len);
        DrawLine(ex, ey, ax2, ay2, Color{220, 100, 100, alpha});

        float head_angle = momentum_arrow_angle_ + PI;
        int h1x = ax2 + static_cast<int>(std::cos(head_angle + 0.4f) * 10.0f);
        int h1y = ay2 + static_cast<int>(std::sin(head_angle + 0.4f) * 10.0f);
        int h2x = ax2 + static_cast<int>(std::cos(head_angle - 0.4f) * 10.0f);
        int h2y = ay2 + static_cast<int>(std::sin(head_angle - 0.4f) * 10.0f);
        DrawLine(ax2, ay2, h1x, h1y, Color{220, 100, 100, alpha});
        DrawLine(ax2, ay2, h2x, h2y, Color{220, 100, 100, alpha});

        draw_text("\xCE\x94p", static_cast<float>(ax2 + 4), static_cast<float>(ay2 - 8), 12,
                  Color{220, 100, 100, alpha});
    }

    int info_y = area_y + area_h + 10;
    draw_text(TextFormat("Probe \xCE\xBB = %.2f", probe_lambda()),
              static_cast<float>(area_x), static_cast<float>(info_y), 13, Color{255, 220, 50, 255});
    draw_text(TextFormat("\xCE\x94x \xE2\x89\x88 \xCE\xBB = %.3f", delta_x_microscope()),
              static_cast<float>(area_x), static_cast<float>(info_y + 18), 13, Color{100, 200, 100, 255});
    draw_text(TextFormat("\xCE\x94p \xE2\x89\x88 \xE2\x84\x8F/\xCE\xBB = %.3f", delta_p_microscope()),
              static_cast<float>(area_x), static_cast<float>(info_y + 36), 13, Color{220, 100, 100, 255});
    draw_text(TextFormat("\xCE\x94x\xC2\xB7\xCE\x94p = %.3f >= \xE2\x84\x8F/2 = %.3f",
              delta_x_microscope() * delta_p_microscope(), HBAR / 2.0),
              static_cast<float>(area_x + area_w / 2), static_cast<float>(info_y + 18), 14, ui_colors::ACCENT);

    draw_text("Short \xCE\xBB \xE2\x86\x92 precise position, blurry momentum",
              static_cast<float>(area_x + area_w / 2), static_cast<float>(info_y + 40), 11, ui_colors::TEXT_SECONDARY);
    draw_text("Long \xCE\xBB \xE2\x86\x92 blurry position, precise momentum",
              static_cast<float>(area_x + area_w / 2), static_cast<float>(info_y + 54), 11, ui_colors::TEXT_SECONDARY);
}

void HeisenbergScenario::draw_histogram(int px, int py, int pw, int ph,
                                         const std::vector<float>& data, double sig,
                                         const char* xlabel, const char* title,
                                         Color bar_color, Color curve_color) {
    draw_text(title, static_cast<float>(px + pw / 2 - 60), static_cast<float>(py + 4), 13, ui_colors::ACCENT);

    int plot_x = px + 40;
    int plot_w = pw - 60;
    int plot_y = py + 28;
    int plot_h = ph - 60;
    int baseline = plot_y + plot_h;

    DrawLine(plot_x, baseline, plot_x + plot_w, baseline, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, baseline, ui_colors::TEXT_SECONDARY);
    draw_text(xlabel, static_cast<float>(plot_x + plot_w / 2 - 10), static_cast<float>(baseline + 8), 11, ui_colors::TEXT_SECONDARY);

    double range = std::max(sig * 4.0, 1.0);
    constexpr int NBINS = 40;
    int bins[NBINS] = {};
    int max_bin = 1;

    for (float v : data) {
        double frac = (v + range) / (2.0 * range);
        int bi = static_cast<int>(frac * NBINS);
        bi = std::clamp(bi, 0, NBINS - 1);
        bins[bi]++;
        if (bins[bi] > max_bin) max_bin = bins[bi];
    }

    float bin_w = static_cast<float>(plot_w) / NBINS;
    for (int i = 0; i < NBINS; ++i) {
        float h = (static_cast<float>(bins[i]) / max_bin) * plot_h * 0.9f;
        float bx = plot_x + i * bin_w;
        DrawRectangle(static_cast<int>(bx), baseline - static_cast<int>(h),
                      std::max(static_cast<int>(bin_w - 1), 1), static_cast<int>(h), bar_color);
    }

    double max_gauss = 1.0 / (sig * std::sqrt(2.0 * PI));
    int prev_gx = 0, prev_gy = 0;
    for (int i = 0; i <= plot_w; ++i) {
        double x = -range + 2.0 * range * static_cast<double>(i) / plot_w;
        double g = std::exp(-x * x / (2.0 * sig * sig)) / (sig * std::sqrt(2.0 * PI));
        double frac = g / (max_gauss * 1.1);
        int gx = plot_x + i;
        int gy = baseline - static_cast<int>(frac * plot_h * 0.9);
        if (i > 0) DrawLine(prev_gx, prev_gy, gx, gy, curve_color);
        prev_gx = gx;
        prev_gy = gy;
    }

    draw_text(TextFormat("\xCE\x94 = %.3f", sig), static_cast<float>(plot_x + plot_w - 70),
              static_cast<float>(plot_y + 8), 12, curve_color);
    draw_text(TextFormat("N = %d", static_cast<int>(data.size())), static_cast<float>(plot_x + 4),
              static_cast<float>(plot_y + 8), 10, ui_colors::TEXT_SECONDARY);
}

void HeisenbergScenario::render_sampling_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    int half_w = vp_w / 2 - 5;

    draw_histogram(vp_x + 5, vp_y + 10, half_w, vp_h - 80,
                   pos_samples_, sigma(), "x", "Position Measurements",
                   Color{90, 160, 220, 120}, ui_colors::WAVEFUNCTION);

    DrawLine(vp_x + vp_w / 2, vp_y + 30, vp_x + vp_w / 2, vp_y + vp_h - 80, ui_colors::PANEL_BORDER);

    draw_histogram(vp_x + vp_w / 2 + 5, vp_y + 10, half_w, vp_h - 80,
                   mom_samples_, delta_p() * 2.0, "p", "Momentum Measurements",
                   Color{220, 160, 90, 120}, Color{220, 160, 90, 255});

    int bottom_y = vp_y + vp_h - 60;
    draw_text(TextFormat("\xCE\x94x = %.3f", delta_x()), static_cast<float>(vp_x + 30),
              static_cast<float>(bottom_y), 14, ui_colors::WAVEFUNCTION);
    draw_text(TextFormat("\xCE\x94p = %.3f", delta_p()), static_cast<float>(vp_x + vp_w / 2 + 30),
              static_cast<float>(bottom_y), 14, Color{220, 160, 90, 255});

    double prod = product();
    Color prod_color = (prod >= HBAR / 2.0 - 0.001) ? ui_colors::SUCCESS : ui_colors::DANGER;
    draw_text(TextFormat("\xCE\x94x \xC2\xB7 \xCE\x94p = %.4f >= \xE2\x84\x8F/2 = %.3f", prod, HBAR / 2.0),
              static_cast<float>(vp_x + vp_w / 2 - 100), static_cast<float>(bottom_y + 20), 14, prod_color);

    if (pos_samples_.size() >= MAX_SAMPLES) {
        draw_text("Max samples reached. Adjust \xCF\x83 to reset.",
                  static_cast<float>(vp_x + vp_w / 2 - 100), static_cast<float>(bottom_y + 40), 11, ui_colors::TEXT_SECONDARY);
    }
}

void HeisenbergScenario::render_phase_space_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("PHASE SPACE (\xCE\x94x \xC2\xB7 \xCE\x94p >= \xE2\x84\x8F/2)",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    int plot_size = std::min(vp_w - 100, vp_h - 100);
    int cx = vp_x + vp_w / 2;
    int cy = vp_y + 20 + plot_size / 2 + 20;

    DrawLine(cx - plot_size / 2, cy, cx + plot_size / 2, cy, ui_colors::TEXT_SECONDARY);
    DrawLine(cx, cy - plot_size / 2, cx, cy + plot_size / 2, ui_colors::TEXT_SECONDARY);
    draw_text("x", static_cast<float>(cx + plot_size / 2 + 5), static_cast<float>(cy - 8), 13, ui_colors::TEXT_SECONDARY);
    draw_text("p", static_cast<float>(cx + 5), static_cast<float>(cy - plot_size / 2 - 5), 13, ui_colors::TEXT_SECONDARY);

    double dx = delta_x();
    double dp = delta_p();
    double scale = plot_size * 0.08;

    float rx = static_cast<float>(dx * scale);
    float ry = static_cast<float>(dp * scale);
    rx = std::clamp(rx, 10.0f, static_cast<float>(plot_size / 2 - 10));
    ry = std::clamp(ry, 10.0f, static_cast<float>(plot_size / 2 - 10));

    constexpr int ELLIPSE_PTS = 80;
    for (int i = 0; i < ELLIPSE_PTS; ++i) {
        float a1 = 2.0f * PI * i / ELLIPSE_PTS;
        float a2 = 2.0f * PI * (i + 1) / ELLIPSE_PTS;
        int x1 = cx + static_cast<int>(rx * std::cos(a1));
        int y1 = cy - static_cast<int>(ry * std::sin(a1));
        int x2 = cx + static_cast<int>(rx * std::cos(a2));
        int y2 = cy - static_cast<int>(ry * std::sin(a2));
        DrawLine(x1, y1, x2, y2, ui_colors::ACCENT);
    }

    float min_area_r = static_cast<float>(std::sqrt(HBAR / 2.0) * scale);
    min_area_r = std::max(min_area_r, 5.0f);
    for (int i = 0; i < ELLIPSE_PTS; ++i) {
        float a1 = 2.0f * PI * i / ELLIPSE_PTS;
        float a2 = 2.0f * PI * (i + 1) / ELLIPSE_PTS;
        int x1 = cx + static_cast<int>(min_area_r * std::cos(a1));
        int y1 = cy - static_cast<int>(min_area_r * std::sin(a1));
        int x2 = cx + static_cast<int>(min_area_r * std::cos(a2));
        int y2 = cy - static_cast<int>(min_area_r * std::sin(a2));
        DrawLine(x1, y1, x2, y2, Color{180, 60, 60, 120});
    }

    for (const auto& pt : phase_points_) {
        int sx = cx + static_cast<int>(pt.x * scale);
        int sy = cy - static_cast<int>(pt.y * scale);
        if (std::abs(sx - cx) < plot_size / 2 && std::abs(sy - cy) < plot_size / 2) {
            DrawCircle(sx, sy, 2, Color{90, 160, 220, 100});
        }
    }

    DrawLine(cx - static_cast<int>(rx), cy + 3, cx + static_cast<int>(rx), cy + 3, Color{100, 200, 100, 180});
    draw_text("\xCE\x94x", static_cast<float>(cx + static_cast<int>(rx) + 4), static_cast<float>(cy), 11, Color{100, 200, 100, 255});

    DrawLine(cx - 3, cy - static_cast<int>(ry), cx - 3, cy + static_cast<int>(ry), Color{220, 160, 90, 180});
    draw_text("\xCE\x94p", static_cast<float>(cx - 25), static_cast<float>(cy - static_cast<int>(ry) - 5), 11, Color{220, 160, 90, 255});

    int info_y = cy + plot_size / 2 + 15;
    draw_text(TextFormat("Area = \xCE\x94x\xC2\xB7\xCE\x94p = %.4f", product()),
              static_cast<float>(vp_x + 30), static_cast<float>(info_y), 14, ui_colors::ACCENT);
    draw_text(TextFormat("Min area = \xE2\x84\x8F/2 = %.3f", HBAR / 2.0),
              static_cast<float>(vp_x + 30), static_cast<float>(info_y + 18), 12, Color{180, 60, 60, 255});
    draw_text("Red circle = minimum uncertainty", static_cast<float>(vp_x + 30),
              static_cast<float>(info_y + 36), 11, ui_colors::TEXT_SECONDARY);
    draw_text("Blue ellipse can be squeezed but never smaller than red",
              static_cast<float>(vp_x + 30), static_cast<float>(info_y + 50), 11, ui_colors::TEXT_SECONDARY);
}

void HeisenbergScenario::render_wavepacket_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("WAVE PACKET TIME EVOLUTION", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text(TextFormat("t = %.3f   Narrow \xCF\x83 -> fast spreading", solver_.get_time()),
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int plot_x = vp_x + 50;
    int plot_w = vp_w - 80;
    int plot_y = vp_y + 50;
    int plot_h = vp_h - 100;
    int baseline = plot_y + plot_h;

    DrawLine(plot_x, baseline, plot_x + plot_w, baseline, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, baseline, ui_colors::TEXT_SECONDARY);
    draw_text("x", static_cast<float>(plot_x + plot_w + 5), static_cast<float>(baseline - 8), 11, ui_colors::TEXT_SECONDARY);
    draw_text("|\xCF\x88(x,t)|\xC2\xB2", static_cast<float>(plot_x - 45), static_cast<float>(plot_y - 2), 11, ui_colors::TEXT_SECONDARY);

    auto density = solver_.get_probability_density();
    int nx = solver_.get_nx();
    double dx_grid = solver_.get_dx();
    double x_min = solver_.get_x_min();

    double max_p = 0.0;
    for (double p : density) if (p > max_p) max_p = p;
    if (max_p < 1e-15) max_p = 1.0;

    int prev_px = 0, prev_py = 0;
    for (int i = 0; i < nx; ++i) {
        double x = x_min + i * dx_grid;
        double frac_x = (x - x_min) / (solver_.get_x_max() - x_min);
        int sx = plot_x + static_cast<int>(frac_x * plot_w);
        double frac_y = density[i] / (max_p * 1.1);
        int sy = baseline - static_cast<int>(frac_y * plot_h);

        int fill_h = baseline - sy;
        if (fill_h > 0) {
            DrawLine(sx, baseline, sx, sy, Color{90, 160, 220, 40});
        }

        if (i > 0) DrawLine(prev_px, prev_py, sx, sy, ui_colors::WAVEFUNCTION);
        prev_px = sx;
        prev_py = sy;
    }

    for (int t = 0; t <= 4; ++t) {
        int tx = plot_x + t * plot_w / 4;
        double val = x_min + (solver_.get_x_max() - x_min) * t / 4.0;
        DrawLine(tx, baseline, tx, baseline + 4, ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("%.0f", val), static_cast<float>(tx - 8), static_cast<float>(baseline + 6), 9, ui_colors::TEXT_SECONDARY);
    }

    int info_y = baseline + 20;
    draw_text(TextFormat("Initial \xCF\x83 = %.2f    Norm = %.6f", sigma(), solver_.get_norm()),
              static_cast<float>(plot_x), static_cast<float>(info_y), 12, ui_colors::TEXT_SECONDARY);
}

void HeisenbergScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("HEISENBERG", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    if (current_view_ == 0) {
        bool changed = probe_lambda_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 8;
        if (changed) reset_microscope();
    } else {
        bool changed = sigma_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 8;
        if (changed) {
            reset_samples();
            if (current_view_ == 3) reset_solver();
        }
    }

    draw_separator(cx, cy, w - 20);
    cy += 4;

    if (current_view_ == 0) {
        draw_text("Short \xCE\xBB photon:", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 14;
        draw_text("  \xE2\x86\x92 good position info", static_cast<float>(cx), static_cast<float>(cy), 11, Color{100, 200, 100, 255});
        cy += 14;
        draw_text("  \xE2\x86\x92 big momentum kick", static_cast<float>(cx), static_cast<float>(cy), 11, Color{220, 100, 100, 255});
        cy += 20;
        draw_text("Long \xCE\xBB photon:", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 14;
        draw_text("  \xE2\x86\x92 blurry position", static_cast<float>(cx), static_cast<float>(cy), 11, Color{100, 200, 100, 255});
        cy += 14;
        draw_text("  \xE2\x86\x92 gentle momentum", static_cast<float>(cx), static_cast<float>(cy), 11, Color{220, 100, 100, 255});
    } else {
        draw_text("Narrow \xCF\x83 \xE2\x86\x92 wide \xCE\x94p", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 14;
        draw_text("Wide \xCF\x83 \xE2\x86\x92 narrow \xCE\x94p", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 20;
        draw_text("You cannot know both", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 14;
        draw_text("position and momentum", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 14;
        draw_text("precisely at once.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    }

    cy += 24;
    draw_text("Views: 1-4 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);

    cy += 20;
    if (HelpPopup::render_help_button(font_, has_font_, cx, cy)) {
        help_popup_.show({"\xCE\x94x\xC2\xB7\xCE\x94p", "Heisenberg Uncertainty",
            "It is impossible to simultaneously know the exact position and momentum of a particle. "
            "This is not a measurement limitation but a fundamental property of quantum mechanics. "
            "A particle does not have a definite position AND momentum at the same time.",
            "\xCE\x94x \xC2\xB7 \xCE\x94p \xE2\x89\xA5 \xE2\x84\x8F/2",
            "\xE2\x84\x8F = h/(2\xCF\x80) = 1.055 \xC3\x97 10\xE2\x81\xBB\xC2\xB3\xE2\x81\xB4 J\xC2\xB7s"});
    }
}

void HeisenbergScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    if (current_view_ == 0) {
        draw_section("Microscope", px, py);
        draw_prop("Probe \xCE\xBB", TextFormat("%.3f", probe_lambda()), px, py, Color{255, 220, 50, 255});
        draw_prop("\xCE\x94x", TextFormat("%.4f", delta_x_microscope()), px, py, Color{100, 200, 100, 255});
        draw_prop("\xCE\x94p", TextFormat("%.4f", delta_p_microscope()), px, py, Color{220, 100, 100, 255});
        py += 4;
        draw_section("Product", px, py);
        double prod_m = delta_x_microscope() * delta_p_microscope();
        draw_prop("\xCE\x94x\xC2\xB7\xCE\x94p", TextFormat("%.4f", prod_m), px, py, ui_colors::ACCENT);
        draw_prop("\xE2\x84\x8F/2", TextFormat("%.3f", HBAR / 2.0), px, py);
        draw_text("= \xE2\x84\x8F (always)", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::SUCCESS);
    } else {
        draw_section("Gaussian Packet", px, py);
        draw_prop("\xCF\x83", TextFormat("%.3f", sigma()), px, py);
        py += 4;
        draw_section("Uncertainties", px, py);
        draw_prop("\xCE\x94x", TextFormat("%.4f", delta_x()), px, py, ui_colors::WAVEFUNCTION);
        draw_prop("\xCE\x94p", TextFormat("%.4f", delta_p()), px, py, Color{220, 160, 90, 255});
        py += 4;
        draw_section("Product", px, py);
        Color prod_color = (product() >= HBAR / 2.0 - 0.001) ? ui_colors::SUCCESS : ui_colors::DANGER;
        draw_prop("\xCE\x94x\xC2\xB7\xCE\x94p", TextFormat("%.4f", product()), px, py, prod_color);
        draw_prop("\xE2\x84\x8F/2", TextFormat("%.3f", HBAR / 2.0), px, py);
        py += 4;
        const char* status = (product() >= HBAR / 2.0 - 0.001) ? "SATISFIED" : "VIOLATED";
        draw_text(status, static_cast<float>(px), static_cast<float>(py), 14, prod_color);
        py += 24;
        draw_section("Samples", px, py);
        draw_prop("N", TextFormat("%d / %d", static_cast<int>(pos_samples_.size()), MAX_SAMPLES), px, py);
    }

    py += 16;
    draw_separator(px, py, w - 24);
    draw_section("Principle", px, py);
    draw_text("\xCE\x94x \xC2\xB7 \xCE\x94p \xE2\x89\xA5 \xE2\x84\x8F / 2", static_cast<float>(px), static_cast<float>(py), 13, ui_colors::ACCENT);
    py += 18;
    draw_text("For a Gaussian, equality", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("holds (minimum uncertainty).", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
}
