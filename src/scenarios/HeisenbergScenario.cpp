#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "HeisenbergScenario.hpp"

HeisenbergScenario::HeisenbergScenario()
    : solver_(512, -10.0, 10.0, 0.001)
    , potential_(512, -10.0, 10.0)
    , sigma_slider_("\xCF\x83 (position uncertainty)", 0.1f, 5.0f, 1.0f, "%.2f")
{}

void HeisenbergScenario::on_enter() {
    current_view_ = 2;
    reset_solver();
}

void HeisenbergScenario::reset_solver() {
    solver_.reset();
    potential_.clear();
    solver_.set_potential(potential_.values());
    solver_.inject_gaussian(0.0, sigma(), 4.0);
}

const char* HeisenbergScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Position Space";
        case 1: return "Momentum Space";
        case 2: return "Split View";
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
    (void)dt;
}

void HeisenbergScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;
    switch (current_view_) {
        case 0: render_x_space(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_p_space(vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_split_view(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void HeisenbergScenario::draw_gaussian_plot(int px, int py, int pw, int ph, double sig,
                                             const char* xlabel, const char* title,
                                             Color curve_color, Color fill_color) {
    draw_text(title, static_cast<float>(px + pw / 2 - 60), static_cast<float>(py + 4), 14, ui_colors::ACCENT);

    int plot_x = px + 50;
    int plot_w = pw - 70;
    int plot_y = py + 30;
    int plot_h = ph - 60;
    int baseline = plot_y + plot_h;

    // Axes
    DrawLine(plot_x, baseline, plot_x + plot_w, baseline, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, baseline, ui_colors::TEXT_SECONDARY);
    draw_text(xlabel, static_cast<float>(plot_x + plot_w / 2 - 10), static_cast<float>(baseline + 8), 11, ui_colors::TEXT_SECONDARY);
    draw_text("|\xCF\x88|^2", static_cast<float>(plot_x - 35), static_cast<float>(plot_y - 2), 11, ui_colors::TEXT_SECONDARY);

    double range = sig * 5.0;
    if (range < 1.0) range = 1.0;

    // Filled area + curve
    int prev_cx = 0, prev_cy = 0;
    for (int i = 0; i <= plot_w; ++i) {
        double x = -range + 2.0 * range * static_cast<double>(i) / plot_w;
        double gauss = std::exp(-x * x / (2.0 * sig * sig)) / (sig * std::sqrt(2.0 * PI));
        double max_val = 1.0 / (sig * std::sqrt(2.0 * PI));
        double frac = gauss / (max_val * 1.2);

        int cx = plot_x + i;
        int cy_val = baseline - static_cast<int>(frac * plot_h);

        DrawLine(cx, baseline, cx, cy_val, fill_color);

        if (i > 0) DrawLine(prev_cx, prev_cy, cx, cy_val, curve_color);
        prev_cx = cx;
        prev_cy = cy_val;
    }

    // Delta bracket (1-sigma range)
    int sigma_px = static_cast<int>((sig / range) * plot_w * 0.5);
    int center_px = plot_x + plot_w / 2;
    int bracket_y = baseline + 20;

    DrawLine(center_px - sigma_px, bracket_y, center_px + sigma_px, bracket_y, ui_colors::ACCENT);
    DrawLine(center_px - sigma_px, bracket_y - 4, center_px - sigma_px, bracket_y + 4, ui_colors::ACCENT);
    DrawLine(center_px + sigma_px, bracket_y - 4, center_px + sigma_px, bracket_y + 4, ui_colors::ACCENT);
    draw_text(TextFormat("\xCE\x94 = %.3f", sig), static_cast<float>(center_px - 20), static_cast<float>(bracket_y + 5), 11, ui_colors::ACCENT);

    // Grid lines
    for (int g = 1; g <= 4; ++g) {
        int gy = baseline - static_cast<int>((g / 5.0) * plot_h);
        DrawLine(plot_x, gy, plot_x + plot_w, gy, Color{40, 40, 50, 100});
    }

    // Tick marks
    for (int t = 0; t <= 4; ++t) {
        int tx = plot_x + t * plot_w / 4;
        double val = -range + 2.0 * range * t / 4.0;
        DrawLine(tx, baseline, tx, baseline + 4, ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("%.1f", val), static_cast<float>(tx - 10), static_cast<float>(baseline + 5), 9, ui_colors::TEXT_SECONDARY);
    }
}

void HeisenbergScenario::render_x_space(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_gaussian_plot(vp_x + 20, vp_y + 10, vp_w - 40, vp_h - 30, sigma(),
                       "x (position)", "POSITION-SPACE WAVEFUNCTION",
                       ui_colors::WAVEFUNCTION, Color{90, 160, 220, 40});

    draw_text(TextFormat("\xCE\x94x = \xCF\x83 = %.3f", delta_x()),
              static_cast<float>(vp_x + 30), static_cast<float>(vp_y + vp_h - 20), 13, ui_colors::TEXT_PRIMARY);
}

void HeisenbergScenario::render_p_space(int vp_x, int vp_y, int vp_w, int vp_h) {
    double sigma_p = 1.0 / (2.0 * sigma());
    draw_gaussian_plot(vp_x + 20, vp_y + 10, vp_w - 40, vp_h - 30, sigma_p,
                       "p (momentum)", "MOMENTUM-SPACE WAVEFUNCTION",
                       Color{220, 160, 90, 255}, Color{220, 160, 90, 40});

    draw_text(TextFormat("\xCE\x94p = \xE2\x84\x8F/(2\xCF\x83) = %.3f", delta_p()),
              static_cast<float>(vp_x + 30), static_cast<float>(vp_y + vp_h - 20), 13, ui_colors::TEXT_PRIMARY);
}

void HeisenbergScenario::render_split_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    int half_w = vp_w / 2 - 10;
    double sigma_p = 1.0 / (2.0 * sigma());

    draw_gaussian_plot(vp_x + 5, vp_y + 10, half_w, vp_h - 80, sigma(),
                       "x", "Position |\xCF\x88(x)|^2",
                       ui_colors::WAVEFUNCTION, Color{90, 160, 220, 40});

    DrawLine(vp_x + vp_w / 2, vp_y + 30, vp_x + vp_w / 2, vp_y + vp_h - 80, ui_colors::PANEL_BORDER);

    draw_gaussian_plot(vp_x + vp_w / 2 + 5, vp_y + 10, half_w, vp_h - 80, sigma_p,
                       "p", "Momentum |\xCF\x86(p)|^2",
                       Color{220, 160, 90, 255}, Color{220, 160, 90, 40});

    // Product display at bottom
    double prod = product();
    Color prod_color = (prod >= HBAR / 2.0 - 0.001) ? ui_colors::SUCCESS : ui_colors::DANGER;
    int bottom_y = vp_y + vp_h - 55;

    draw_text(TextFormat("\xCE\x94x = %.3f", delta_x()), static_cast<float>(vp_x + 30), static_cast<float>(bottom_y), 14, ui_colors::WAVEFUNCTION);
    draw_text(TextFormat("\xCE\x94p = %.3f", delta_p()), static_cast<float>(vp_x + vp_w / 2 + 30), static_cast<float>(bottom_y), 14, Color{220, 160, 90, 255});

    draw_text(TextFormat("\xCE\x94x \xC2\xB7 \xCE\x94p = %.4f", prod),
              static_cast<float>(vp_x + vp_w / 2 - 80), static_cast<float>(bottom_y + 22), 16, prod_color);
    draw_text(TextFormat(">= \xE2\x84\x8F/2 = %.3f", HBAR / 2.0),
              static_cast<float>(vp_x + vp_w / 2 + 40), static_cast<float>(bottom_y + 24), 13, ui_colors::TEXT_SECONDARY);

    const char* verdict = (prod >= HBAR / 2.0 - 0.001) ? "SATISFIED" : "VIOLATED";
    draw_text(verdict, static_cast<float>(vp_x + vp_w / 2 - 30), static_cast<float>(bottom_y + 42), 14, prod_color);
}

void HeisenbergScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("HEISENBERG", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    bool changed = sigma_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 8;
    if (changed) reset_solver();

    draw_separator(cx, cy, w - 20);
    cy += 4;

    draw_text("Narrow \xCF\x83 -> wide \xCE\x94p", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("Wide \xCF\x83 -> narrow \xCE\x94p", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 20;

    draw_text("The more precisely you", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("know position, the less", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("precisely you can know", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("momentum, and vice versa.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 24;

    draw_text("Views: 1-3 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
}

void HeisenbergScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    draw_section("Parameters", px, py);
    draw_prop("\xCF\x83", TextFormat("%.3f", sigma()), px, py);
    py += 4;

    draw_section("Uncertainties", px, py);
    draw_prop("\xCE\x94x", TextFormat("%.4f", delta_x()), px, py);
    draw_prop("\xCE\x94p", TextFormat("%.4f", delta_p()), px, py);
    py += 4;

    draw_section("Product", px, py);
    Color prod_color = (product() >= HBAR / 2.0 - 0.001) ? ui_colors::SUCCESS : ui_colors::DANGER;
    draw_prop("\xCE\x94x\xC2\xB7\xCE\x94p", TextFormat("%.4f", product()), px, py, prod_color);
    draw_prop("\xE2\x84\x8F/2", TextFormat("%.3f", HBAR / 2.0), px, py);
    py += 4;

    const char* status = (product() >= HBAR / 2.0 - 0.001) ? "SATISFIED" : "VIOLATED";
    draw_text(status, static_cast<float>(px), static_cast<float>(py), 14, prod_color);
    py += 24;

    draw_separator(px, py, w - 24);
    draw_section("Principle", px, py);
    draw_text("\xCE\x94x \xC2\xB7 \xCE\x94p >= \xE2\x84\x8F / 2", static_cast<float>(px), static_cast<float>(py), 13, ui_colors::ACCENT);
    py += 20;
    draw_text("For a Gaussian packet,", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("equality holds (minimum", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("uncertainty state).", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
}
