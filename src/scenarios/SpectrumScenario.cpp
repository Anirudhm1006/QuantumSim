#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>

#include <raylib.h>

#include "SpectrumScenario.hpp"

namespace {
    constexpr Color LEVEL_LINE_COLOR   = {100, 140, 180, 255};
    constexpr Color LEVEL_SELECT_COLOR = {220, 180, 60, 255};
    constexpr Color BAR_BG             = {15, 15, 20, 255};
    constexpr Color EMISSION_BG        = {8, 8, 12, 255};
}

// ---------------------------------------------------------------------------
// Construction / lifecycle
// ---------------------------------------------------------------------------

SpectrumScenario::SpectrumScenario() = default;

void SpectrumScenario::on_enter() {
    element_index_ = 0;
    selected_level_from_ = -1;
    selected_level_to_ = -1;
    emission_mode_ = true;
    clamp_level_indices();
}

void SpectrumScenario::update(double /*dt*/) {}

void SpectrumScenario::select_element(int index) {
    if (index < 0 || index >= ElementData::element_count()) return;
    element_index_ = index;
    selected_level_from_ = -1;
    selected_level_to_ = -1;
    clamp_level_indices();
}

void SpectrumScenario::clamp_level_indices() {
    const auto& el = ElementData::get_element_by_index(element_index_);
    int n = static_cast<int>(el.energy_levels.size());
    if (n == 0) {
        selected_level_from_ = -1;
        selected_level_to_ = -1;
        return;
    }
    if (selected_level_from_ < 0) selected_level_from_ = std::min(2, n - 1);
    if (selected_level_to_ < 0)   selected_level_to_   = std::min(1, n - 1);
    selected_level_from_ = std::clamp(selected_level_from_, 0, n - 1);
    selected_level_to_   = std::clamp(selected_level_to_,   0, n - 1);
}

// ---------------------------------------------------------------------------
// Transition calculations (from element energy level data)
// ---------------------------------------------------------------------------

double SpectrumScenario::selected_transition_energy_eV() const {
    const auto& levels = ElementData::get_element_by_index(element_index_).energy_levels;
    if (selected_level_from_ < 0 || selected_level_to_ < 0) return 0.0;
    if (selected_level_from_ >= static_cast<int>(levels.size())) return 0.0;
    if (selected_level_to_   >= static_cast<int>(levels.size())) return 0.0;
    if (selected_level_from_ == selected_level_to_) return 0.0;
    return std::abs(levels[selected_level_from_].energy_eV -
                    levels[selected_level_to_].energy_eV);
}

double SpectrumScenario::selected_transition_wavelength_nm() const {
    double dE = selected_transition_energy_eV();
    if (dE < 1e-12) return 0.0;
    return HC_EV_NM / dE;
}

double SpectrumScenario::selected_transition_frequency_hz() const {
    double wl = selected_transition_wavelength_nm();
    if (wl < 1e-12) return 0.0;
    return SPEED_OF_LIGHT / (wl * 1e-9);
}

bool SpectrumScenario::has_valid_transition() const {
    return selected_level_from_ >= 0 && selected_level_to_ >= 0 &&
           selected_level_from_ != selected_level_to_ &&
           selected_transition_energy_eV() > 1e-12;
}

// ---------------------------------------------------------------------------
// Coordinate mapping (log-scale)
// ---------------------------------------------------------------------------

float SpectrumScenario::wavelength_to_x(double wl_nm, int bar_x, int bar_w) const {
    double log_min = std::log10(SPECTRUM_MIN_NM);
    double log_max = std::log10(SPECTRUM_MAX_NM);
    double log_wl  = std::log10(std::clamp(wl_nm, SPECTRUM_MIN_NM, SPECTRUM_MAX_NM));
    double frac = (log_wl - log_min) / (log_max - log_min);
    return static_cast<float>(bar_x) + static_cast<float>(frac) * static_cast<float>(bar_w);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void SpectrumScenario::handle_input() {
    periodic_table_.handle_input();
    if (periodic_table_.is_open()) return;

    help_popup_.handle_input();
    if (help_popup_.is_open()) return;

    const auto& el = ElementData::get_element_by_index(element_index_);
    int n_levels = static_cast<int>(el.energy_levels.size());

    if (IsKeyPressed(KEY_TAB)) {
        emission_mode_ = !emission_mode_;
    }
    if (n_levels > 0) {
        if (IsKeyPressed(KEY_UP) && selected_level_from_ < n_levels - 1)
            selected_level_from_++;
        if (IsKeyPressed(KEY_DOWN) && selected_level_from_ > 0)
            selected_level_from_--;
        if (IsKeyPressed(KEY_RIGHT) && selected_level_to_ < n_levels - 1)
            selected_level_to_++;
        if (IsKeyPressed(KEY_LEFT) && selected_level_to_ > 0)
            selected_level_to_--;
    }
}

// ---------------------------------------------------------------------------
// Button helpers
// ---------------------------------------------------------------------------

bool SpectrumScenario::render_button(const char* label, int x, int y, int w,
                                     int btn_h, Color bg) {
    auto fx = static_cast<float>(x);
    auto fy = static_cast<float>(y);
    auto fw = static_cast<float>(w);
    auto fh = static_cast<float>(btn_h);

    DrawRectangleRounded({fx, fy, fw, fh}, 0.3f, 4, bg);

    float tw = measure_text(label, 13);
    draw_text(label, fx + (fw - tw) * 0.5f, fy + (fh - 13.0f) * 0.5f,
              13, ui_colors::TEXT_PRIMARY);

    Vector2 mouse = GetMousePosition();
    bool hovered = mouse.x >= fx && mouse.x <= fx + fw &&
                   mouse.y >= fy && mouse.y <= fy + fh;
    if (hovered) {
        DrawRectangleRounded({fx, fy, fw, fh}, 0.3f, 4, {255, 255, 255, 20});
    }
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool SpectrumScenario::render_toggle_button(const char* label_on, const char* label_off,
                                            bool state, int x, int y, int w) {
    const char* text = state ? label_on : label_off;
    Color bg = state ? ui_colors::ACCENT : Color{55, 55, 65, 255};
    Color fg = state ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY;

    auto fx = static_cast<float>(x);
    auto fy = static_cast<float>(y);
    auto fw = static_cast<float>(w);

    DrawRectangleRounded({fx, fy, fw, 26.0f}, 0.3f, 4, bg);
    float tw = measure_text(text, 13);
    draw_text(text, fx + (fw - tw) * 0.5f, fy + 6.0f, 13, fg);

    Vector2 mouse = GetMousePosition();
    bool hovered = mouse.x >= fx && mouse.x <= fx + fw &&
                   mouse.y >= fy && mouse.y <= fy + 26.0f;
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ---------------------------------------------------------------------------
// Viewport
// ---------------------------------------------------------------------------

void SpectrumScenario::render_viewport(Camera3D& /*cam*/, int vp_x, int vp_y,
                                       int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    int margin = 20;
    int inner_x = vp_x + margin;
    int inner_w = vp_w - 2 * margin;

    int bar_h = 50;
    int spectrum_h = 60;
    int region_label_h = 16;
    int gap = 10;

    int bar_y = vp_y + margin + 26;
    render_em_spectrum_bar(inner_x, bar_y, inner_w, bar_h);

    int level_y = bar_y + bar_h + region_label_h + gap;
    int level_h = vp_h - (bar_y - vp_y) - bar_h - region_label_h
                  - spectrum_h - gap * 2 - margin - 16;
    if (level_h < 80) level_h = 80;
    render_energy_levels(inner_x, level_y, inner_w, level_h);

    int spec_y = level_y + level_h + gap;
    render_line_spectrum(inner_x, spec_y, inner_w, spectrum_h);

    periodic_table_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

// ---------------------------------------------------------------------------
// EM Spectrum Bar (top)
// ---------------------------------------------------------------------------

void SpectrumScenario::render_em_spectrum_bar(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, BAR_BG);

    double log_min = std::log10(SPECTRUM_MIN_NM);
    double log_max = std::log10(SPECTRUM_MAX_NM);

    for (int px = 0; px < w; ++px) {
        double frac = static_cast<double>(px) / static_cast<double>(w);
        double wl = std::pow(10.0, log_min + frac * (log_max - log_min));

        Color col;
        if (wl >= 380.0 && wl <= 780.0) {
            col = SpectralTransition::wavelength_to_rgb(wl);
        } else if (wl < 380.0) {
            double fade = std::clamp((380.0 - wl) / 370.0, 0.0, 1.0);
            auto v = static_cast<unsigned char>(60.0 * (1.0 - fade * 0.7));
            col = {static_cast<unsigned char>(v / 2), static_cast<unsigned char>(v / 4),
                   v, 255};
        } else {
            double fade = std::clamp((wl - 780.0) / 220.0, 0.0, 1.0);
            auto v = static_cast<unsigned char>(70.0 * (1.0 - fade * 0.6));
            col = {v, static_cast<unsigned char>(v / 4),
                   static_cast<unsigned char>(v / 4), 255};
        }
        DrawLine(x + px, y, x + px, y + h, col);
    }

    DrawRectangleLines(x, y, w, h, ui_colors::PANEL_BORDER);

    // Region boundary markers
    float vis_left  = wavelength_to_x(380.0, x, w);
    float vis_right = wavelength_to_x(780.0, x, w);
    DrawLine(static_cast<int>(vis_left),  y, static_cast<int>(vis_left),  y + h, {255, 255, 255, 50});
    DrawLine(static_cast<int>(vis_right), y, static_cast<int>(vis_right), y + h, {255, 255, 255, 50});

    // Region labels below bar
    int label_y = y + h + 2;
    float xray_center = (static_cast<float>(x) + wavelength_to_x(10.0, x, w)) * 0.5f;
    draw_text("X-ray", xray_center - 14.0f, static_cast<float>(label_y), 10,
              ui_colors::TEXT_DISABLED);

    float uv_center = (wavelength_to_x(10.0, x, w) + vis_left) * 0.5f;
    draw_text("UV", uv_center - 6.0f, static_cast<float>(label_y), 11,
              ui_colors::TEXT_SECONDARY);

    float vis_center = (vis_left + vis_right) * 0.5f;
    draw_text("Visible", vis_center - 18.0f, static_cast<float>(label_y), 11,
              ui_colors::TEXT_PRIMARY);

    float ir_center = (vis_right + static_cast<float>(x + w)) * 0.5f;
    draw_text("IR", ir_center - 5.0f, static_cast<float>(label_y), 11,
              ui_colors::TEXT_SECONDARY);

    // Triangle pointer at selected transition wavelength
    double sel_wl = selected_transition_wavelength_nm();
    if (sel_wl > 0.0 && sel_wl >= SPECTRUM_MIN_NM && sel_wl <= SPECTRUM_MAX_NM) {
        float tx = wavelength_to_x(sel_wl, x, w);
        Color arrow_col = SpectralTransition::wavelength_to_rgb(sel_wl);

        Vector2 tip = {tx, static_cast<float>(y) - 2.0f};
        Vector2 tl  = {tx - 7.0f, static_cast<float>(y) - 14.0f};
        Vector2 tr  = {tx + 7.0f, static_cast<float>(y) - 14.0f};
        DrawTriangle(tip, tr, tl, arrow_col);

        DrawLineEx({tx, static_cast<float>(y)},
                   {tx, static_cast<float>(y + h)}, 1.5f, {255, 255, 255, 90});

        char wl_buf[32];
        std::snprintf(wl_buf, sizeof(wl_buf), "%.1f nm", sel_wl);
        float tw = measure_text(wl_buf, 11);
        draw_text(wl_buf, tx - tw * 0.5f, static_cast<float>(y) - 26.0f, 11, arrow_col);
    }
}

// ---------------------------------------------------------------------------
// Energy Level Diagram (center)
// ---------------------------------------------------------------------------

void SpectrumScenario::render_energy_levels(int x, int y, int w, int h) {
    const auto& el = ElementData::get_element_by_index(element_index_);
    const auto& levels = el.energy_levels;

    char title[64];
    std::snprintf(title, sizeof(title), "Energy Levels  %s (%s)",
                  el.name.c_str(), el.symbol.c_str());
    draw_text(title, static_cast<float>(x), static_cast<float>(y - 16), 13,
              ui_colors::ACCENT);

    DrawRectangleLines(x, y, w, h, ui_colors::PANEL_BORDER);

    if (levels.empty()) {
        draw_text("No energy level data available",
                  static_cast<float>(x + w / 2 - 90), static_cast<float>(y + h / 2),
                  14, ui_colors::TEXT_SECONDARY);
        return;
    }

    double e_min = levels[0].energy_eV;
    double e_max = levels[0].energy_eV;
    for (const auto& lev : levels) {
        e_min = std::min(e_min, lev.energy_eV);
        e_max = std::max(e_max, lev.energy_eV);
    }
    double e_range = e_max - e_min;
    if (e_range < 1e-9) e_range = 1.0;

    int level_pad_x = 80;
    int level_x0 = x + level_pad_x;
    int level_x1 = x + w - level_pad_x;
    int pad_y = 15;

    auto energy_to_y = [&](double eV) -> int {
        double frac = (eV - e_min) / e_range;
        return y + h - pad_y - static_cast<int>(frac * static_cast<double>(h - 2 * pad_y));
    };

    for (int i = 0; i < static_cast<int>(levels.size()); ++i) {
        int ly = energy_to_y(levels[i].energy_eV);

        bool is_from = (i == selected_level_from_);
        bool is_to   = (i == selected_level_to_);
        Color line_col = (is_from || is_to) ? LEVEL_SELECT_COLOR : LEVEL_LINE_COLOR;
        float thickness = (is_from || is_to) ? 2.5f : 1.0f;

        DrawLineEx({static_cast<float>(level_x0), static_cast<float>(ly)},
                   {static_cast<float>(level_x1), static_cast<float>(ly)},
                   thickness, line_col);

        // Level label (left side)
        float lbl_tw = measure_text(levels[i].label.c_str(), 11);
        draw_text(levels[i].label.c_str(),
                  static_cast<float>(level_x0) - lbl_tw - 8.0f,
                  static_cast<float>(ly) - 6.0f, 11, line_col);

        // Energy value (right side)
        char eval[32];
        std::snprintf(eval, sizeof(eval), "%.2f eV", levels[i].energy_eV);
        draw_text(eval, static_cast<float>(level_x1) + 8.0f,
                  static_cast<float>(ly) - 6.0f, 11, ui_colors::TEXT_SECONDARY);

        if (is_from) {
            draw_text("\xe2\x96\xb6 from",
                      static_cast<float>(level_x0) - lbl_tw - 56.0f,
                      static_cast<float>(ly) - 6.0f, 10, LEVEL_SELECT_COLOR);
        }
        if (is_to) {
            float ev_tw = measure_text(eval, 11);
            draw_text("\xe2\x96\xb6 to",
                      static_cast<float>(level_x1) + 12.0f + ev_tw,
                      static_cast<float>(ly) - 6.0f, 10, LEVEL_SELECT_COLOR);
        }
    }

    // Transition arrow
    if (has_valid_transition()) {
        int y_from = energy_to_y(levels[selected_level_from_].energy_eV);
        int y_to   = energy_to_y(levels[selected_level_to_].energy_eV);

        int arrow_x = (level_x0 + level_x1) / 2;
        double wl = selected_transition_wavelength_nm();
        Color arrow_col = SpectralTransition::wavelength_to_rgb(wl);

        DrawLineEx({static_cast<float>(arrow_x), static_cast<float>(y_from)},
                   {static_cast<float>(arrow_x), static_cast<float>(y_to)},
                   3.0f, arrow_col);

        // Arrowhead at y_to
        float tip_x = static_cast<float>(arrow_x);
        float tip_y = static_cast<float>(y_to);
        float sign  = (y_to > y_from) ? 1.0f : -1.0f;
        float sz    = 9.0f;

        Vector2 av_tip = {tip_x, tip_y};
        Vector2 av_l   = {tip_x - sz, tip_y - sz * sign};
        Vector2 av_r   = {tip_x + sz, tip_y - sz * sign};
        DrawTriangle(av_tip, av_r, av_l, arrow_col);

        // Photon label
        char photon_lbl[48];
        std::snprintf(photon_lbl, sizeof(photon_lbl), "%.1f nm", wl);
        draw_text(photon_lbl, static_cast<float>(arrow_x + 14),
                  static_cast<float>((y_from + y_to) / 2 - 6), 12, arrow_col);

        const char* mode_tag = emission_mode_ ? "emission" : "absorption";
        draw_text(mode_tag, static_cast<float>(arrow_x + 14),
                  static_cast<float>((y_from + y_to) / 2 + 8), 10,
                  ui_colors::TEXT_DISABLED);
    }
}

// ---------------------------------------------------------------------------
// Line Spectrum (bottom)
// ---------------------------------------------------------------------------

void SpectrumScenario::render_line_spectrum(int x, int y, int w, int h) {
    const auto& el = ElementData::get_element_by_index(element_index_);

    double log_min = std::log10(SPECTRUM_MIN_NM);
    double log_max = std::log10(SPECTRUM_MAX_NM);

    if (emission_mode_) {
        DrawRectangle(x, y, w, h, EMISSION_BG);
    } else {
        for (int px = 0; px < w; ++px) {
            double frac = static_cast<double>(px) / static_cast<double>(w);
            double wl = std::pow(10.0, log_min + frac * (log_max - log_min));

            Color col;
            if (wl >= 380.0 && wl <= 780.0) {
                col = SpectralTransition::wavelength_to_rgb(wl);
            } else if (wl < 380.0) {
                col = {35, 18, 55, 255};
            } else {
                col = {55, 12, 12, 255};
            }
            DrawLine(x + px, y, x + px, y + h, col);
        }
    }

    for (const auto& line : el.emission_lines) {
        if (line.wavelength_nm < SPECTRUM_MIN_NM || line.wavelength_nm > SPECTRUM_MAX_NM)
            continue;

        float lx = wavelength_to_x(line.wavelength_nm, x, w);

        if (emission_mode_) {
            Color col = SpectralTransition::wavelength_to_rgb(line.wavelength_nm);
            auto alpha = static_cast<unsigned char>(
                std::clamp(line.intensity * 255.0, 80.0, 255.0));
            col.a = alpha;
            DrawLineEx({lx, static_cast<float>(y)},
                       {lx, static_cast<float>(y + h)}, 2.0f, col);
        } else {
            DrawLineEx({lx, static_cast<float>(y)},
                       {lx, static_cast<float>(y + h)}, 2.5f, {4, 4, 4, 230});
        }
    }

    // Highlight the currently selected transition wavelength
    double sel_wl = selected_transition_wavelength_nm();
    if (sel_wl > 0.0 && sel_wl >= SPECTRUM_MIN_NM && sel_wl <= SPECTRUM_MAX_NM) {
        float sx = wavelength_to_x(sel_wl, x, w);
        DrawLineEx({sx, static_cast<float>(y)},
                   {sx, static_cast<float>(y + h)}, 1.0f, {255, 255, 255, 120});
    }

    DrawRectangleLines(x, y, w, h, ui_colors::PANEL_BORDER);

    const char* mode_label = emission_mode_ ? "Emission Spectrum" : "Absorption Spectrum";
    draw_text(mode_label, static_cast<float>(x), static_cast<float>(y - 14), 12,
              ui_colors::ACCENT);
}

// ---------------------------------------------------------------------------
// Controls panel (left)
// ---------------------------------------------------------------------------

void SpectrumScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int cx = x + 10;
    int cy = y + 10;
    int cw = w - 20;

    draw_section("ELEMENT", cx, cy);
    cy += 4;

    const auto& el = ElementData::get_element_by_index(element_index_);
    char btn_label[64];
    std::snprintf(btn_label, sizeof(btn_label), "%s - %s", el.symbol.c_str(),
                  el.name.c_str());

    if (render_button(btn_label, cx, cy, cw, 28, ui_colors::ACCENT)) {
        periodic_table_.open([this](int z) {
            const auto& elems = ElementData::get_elements();
            for (int i = 0; i < static_cast<int>(elems.size()); ++i) {
                if (elems[i].atomic_number == z) {
                    select_element(i);
                    return;
                }
            }
        });
    }
    cy += 36;

    draw_separator(cx, cy, cw);

    draw_section("MODE", cx, cy);
    cy += 4;

    if (render_toggle_button("Mode: EMISSION", "Mode: ABSORPTION",
                             emission_mode_, cx, cy, cw)) {
        emission_mode_ = !emission_mode_;
    }
    cy += 34;

    draw_separator(cx, cy, cw);

    draw_section("LEVEL SELECTION", cx, cy);
    cy += 4;

    const auto& levels = el.energy_levels;
    if (!levels.empty()) {
        char from_buf[64];
        std::snprintf(from_buf, sizeof(from_buf), "From: %s",
                      levels[selected_level_from_].label.c_str());
        draw_text(from_buf, static_cast<float>(cx), static_cast<float>(cy), 13,
                  LEVEL_SELECT_COLOR);
        cy += 18;

        char to_buf[64];
        std::snprintf(to_buf, sizeof(to_buf), "To:   %s",
                      levels[selected_level_to_].label.c_str());
        draw_text(to_buf, static_cast<float>(cx), static_cast<float>(cy), 13,
                  LEVEL_SELECT_COLOR);
        cy += 22;
    }

    draw_separator(cx, cy, cw);
    cy += 4;

    draw_text("[Tab]          Toggle Em/Ab", static_cast<float>(cx),
              static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 15;
    draw_text("[Up/Down]      Change 'from'", static_cast<float>(cx),
              static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 15;
    draw_text("[Left/Right]   Change 'to'", static_cast<float>(cx),
              static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 22;

    draw_separator(cx, cy, cw);
    cy += 4;

    if (HelpPopup::render_help_button(font_, has_font_, cx + 10, cy + 4)) {
        help_popup_.show({"Spectrum", "Emission & Absorption Spectra",
            "Atoms emit or absorb photons when electrons\n"
            "transition between discrete energy levels.\n"
            "The photon energy equals the energy difference\n"
            "between the two levels: E = hf = hc/\xce\xbb.",
            "\xce\x94" "E = E_upper - E_lower = hc/\xce\xbb",
            "eV (energy), nm (wavelength), Hz (frequency)"});
    }
    draw_text("Help: Spectra", static_cast<float>(cx + 24),
              static_cast<float>(cy - 2), 12, ui_colors::TEXT_SECONDARY);
}

// ---------------------------------------------------------------------------
// Properties panel (right)
// ---------------------------------------------------------------------------

void SpectrumScenario::render_properties(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int px = x + 10;
    int py = y + 10;
    int pw = w - 20;
    char buf[96];

    const auto& el = ElementData::get_element_by_index(element_index_);

    draw_section("ELEMENT", px, py);

    std::snprintf(buf, sizeof(buf), "%d", el.atomic_number);
    draw_prop("Z:", buf, px, py);
    draw_prop("Name:", el.name.c_str(), px, py);
    draw_prop("Symbol:", el.symbol.c_str(), px, py, ui_colors::ACCENT);

    py += 4;
    draw_separator(px, py, pw);

    draw_section("SELECTED TRANSITION", px, py);

    if (has_valid_transition()) {
        const auto& levels = el.energy_levels;

        draw_prop("From:", levels[selected_level_from_].label.c_str(), px, py);
        draw_prop("To:", levels[selected_level_to_].label.c_str(), px, py);

        double dE = selected_transition_energy_eV();
        std::snprintf(buf, sizeof(buf), "%.4f eV", dE);
        draw_prop("\xce\x94" "E:", buf, px, py, ui_colors::ACCENT);

        double wl = selected_transition_wavelength_nm();
        Color wl_color = SpectralTransition::wavelength_to_rgb(wl);
        std::snprintf(buf, sizeof(buf), "%.2f nm", wl);
        draw_prop("\xce\xbb:", buf, px, py, wl_color);

        // Color swatch next to wavelength
        DrawRectangle(px + pw - 30, py - 16, 24, 12, wl_color);
        DrawRectangleLines(px + pw - 30, py - 16, 24, 12, ui_colors::PANEL_BORDER);

        double freq = selected_transition_frequency_hz();
        std::snprintf(buf, sizeof(buf), "%.1f THz", freq / 1e12);
        draw_prop("\xce\xbd:", buf, px, py);

        int n_lower = std::min(selected_level_from_, selected_level_to_) + 1;
        std::string series = SpectralTransition::identify_series(n_lower);
        draw_prop("Series:", series.c_str(), px, py);

        std::string region = SpectralTransition::wavelength_region(wl);
        draw_prop("Region:", region.c_str(), px, py);
    } else {
        draw_text("Select two different levels", static_cast<float>(px),
                  static_cast<float>(py), 12, ui_colors::TEXT_DISABLED);
        py += 18;
    }

    py += 4;
    draw_separator(px, py, pw);

    draw_section("VISIBLE EMISSION LINES", px, py);

    int lines_shown = 0;
    for (const auto& line : el.emission_lines) {
        if (line.wavelength_nm < 380.0 || line.wavelength_nm > 780.0) continue;
        if (py > y + h - 20) {
            draw_text("...", static_cast<float>(px), static_cast<float>(py), 12,
                      ui_colors::TEXT_DISABLED);
            break;
        }

        Color lc = SpectralTransition::wavelength_to_rgb(line.wavelength_nm);
        DrawRectangle(px, py + 2, 10, 10, lc);
        DrawRectangleLines(px, py + 2, 10, 10, ui_colors::PANEL_BORDER);

        std::snprintf(buf, sizeof(buf), "%.1f nm", line.wavelength_nm);
        draw_text(buf, static_cast<float>(px + 16), static_cast<float>(py), 12, lc);

        std::snprintf(buf, sizeof(buf), "(I=%.0f%%)", line.intensity * 100.0);
        draw_text(buf, static_cast<float>(px + 90), static_cast<float>(py), 11,
                  ui_colors::TEXT_DISABLED);

        py += 16;
        lines_shown++;
    }

    if (lines_shown == 0) {
        draw_text("No visible lines", static_cast<float>(px),
                  static_cast<float>(py), 12, ui_colors::TEXT_DISABLED);
    }
}
