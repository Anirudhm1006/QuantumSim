#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "DeBroglieScenario.hpp"

const DeBroglieScenario::ParticlePreset DeBroglieScenario::PRESETS[NUM_PRESETS] = {
    {"Electron",       9.109e-31,  1e-18,  true},
    {"Proton",         1.673e-27,  1e-15,  true},
    {"Neutron",        1.675e-27,  1e-15,  true},
    {"Alpha particle", 6.644e-27,  3e-15,  true},
    {"C60 Buckyball",  1.197e-24,  7e-10,  true},
    {"Baseball",       0.145,      0.07,   false}
};

DeBroglieScenario::DeBroglieScenario()
    : velocity_slider_("Velocity (m/s)", 3.0f, 8.0f, 6.0f, "10^%.1f")
    , crystal_d_slider_("Crystal d (\xC3\x85)", 0.5f, 5.0f, 2.0f, "%.1f")
{}

void DeBroglieScenario::on_enter() {
    current_view_ = 0;
    anim_time_ = 0.0;
    particle_anim_x_ = 0.0f;
}

double DeBroglieScenario::velocity_mps() const {
    return std::pow(10.0, static_cast<double>(velocity_slider_.get_value()));
}

double DeBroglieScenario::momentum_si() const {
    return PRESETS[particle_type_].mass_kg * velocity_mps();
}

double DeBroglieScenario::lambda_si() const {
    double p = momentum_si();
    if (p < 1e-100) return 0.0;
    return H_SI / p;
}

double DeBroglieScenario::lambda_display_val() const {
    double lam = lambda_si();
    if (lam <= 0) return 0;
    if (lam < 1e-15) return lam * 1e18;
    if (lam < 1e-12) return lam * 1e15;
    if (lam < 1e-9)  return lam * 1e12;
    if (lam < 1e-6)  return lam * 1e9;
    if (lam < 1e-3)  return lam * 1e6;
    if (lam < 1.0)   return lam * 1e3;
    return lam;
}

const char* DeBroglieScenario::lambda_display_unit() const {
    double lam = lambda_si();
    if (lam <= 0) return "";
    if (lam < 1e-15) return "am";
    if (lam < 1e-12) return "fm";
    if (lam < 1e-9)  return "pm";
    if (lam < 1e-6)  return "nm";
    if (lam < 1e-3)  return "\xCE\xBCm";
    if (lam < 1.0)   return "mm";
    return "m";
}

bool DeBroglieScenario::is_quantum_regime() const {
    double lam = lambda_si();
    double size = PRESETS[particle_type_].typical_size_m;
    return lam > size * 0.01;
}

const char* DeBroglieScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Wave Animation";
        case 1: return "Scale Comparison";
        case 2: return "Diffraction";
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
    anim_time_ += dt;
    particle_anim_x_ += static_cast<float>(dt) * 0.15f;
    if (particle_anim_x_ > 1.1f) particle_anim_x_ = -0.1f;
}

void DeBroglieScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;
    switch (current_view_) {
        case 0: render_wave_animation(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_scale_comparison(vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_diffraction(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void DeBroglieScenario::render_wave_animation(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("DE BROGLIE WAVE-PARTICLE DUALITY",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text(TextFormat("Particle: %s", PRESETS[particle_type_].name),
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 12, ui_colors::TEXT_PRIMARY);

    int area_x = vp_x + 40;
    int area_y = vp_y + 55;
    int area_w = vp_w - 80;
    int area_h = vp_h / 2 - 40;
    int cy = area_y + area_h / 2;

    DrawRectangle(area_x, area_y, area_w, area_h, Color{20, 20, 28, 255});
    DrawRectangleLinesEx({static_cast<float>(area_x), static_cast<float>(area_y),
                          static_cast<float>(area_w), static_cast<float>(area_h)}, 1.0f, ui_colors::PANEL_BORDER);

    int px = area_x + static_cast<int>(particle_anim_x_ * area_w);
    int particle_r = PRESETS[particle_type_].is_quantum ? 6 : 14;

    bool quantum = is_quantum_regime();

    if (quantum && PRESETS[particle_type_].is_quantum) {
        double wave_lambda_px = std::min(static_cast<double>(area_w) * 0.15,
                                          std::max(20.0, 300.0 / velocity_mps() * 1e5));
        float wave_k = static_cast<float>(2.0 * PI / wave_lambda_px);
        float amplitude = static_cast<float>(area_h) * 0.25f;

        int prev_wx = 0, prev_wy = 0;
        for (int i = 0; i < area_w; ++i) {
            int wx = area_x + i;
            float env = std::exp(-std::pow(static_cast<float>(i - (px - area_x)), 2.0f) /
                                  (2.0f * 3000.0f));
            float wave_y = amplitude * env * std::sin(wave_k * i - static_cast<float>(anim_time_) * 3.0f);
            int wy = cy - static_cast<int>(wave_y);
            if (i > 0) DrawLine(prev_wx, prev_wy, wx, wy, Color{90, 160, 220, 150});
            prev_wx = wx;
            prev_wy = wy;
        }

        if (wave_lambda_px > 15 && wave_lambda_px < area_w / 2) {
            int lx1 = px;
            int lx2 = px + static_cast<int>(wave_lambda_px);
            if (lx2 < area_x + area_w - 10) {
                int ly = cy + static_cast<int>(amplitude) + 15;
                DrawLine(lx1, ly, lx2, ly, ui_colors::ACCENT);
                DrawLine(lx1, ly - 4, lx1, ly + 4, ui_colors::ACCENT);
                DrawLine(lx2, ly - 4, lx2, ly + 4, ui_colors::ACCENT);
                draw_text("\xCE\xBB", static_cast<float>((lx1 + lx2) / 2 - 3),
                          static_cast<float>(ly + 5), 12, ui_colors::ACCENT);
            }
        }
    }

    DrawCircle(px, cy, static_cast<float>(particle_r), Color{100, 160, 220, 220});
    if (particle_r >= 10) {
        draw_text("m", static_cast<float>(px - 4), static_cast<float>(cy - 5), 10, ui_colors::TEXT_PRIMARY);
    }

    float arrow_len = 40.0f;
    DrawLine(px + particle_r + 5, cy, px + particle_r + 5 + static_cast<int>(arrow_len), cy, ui_colors::TEXT_PRIMARY);
    DrawTriangle(
        {static_cast<float>(px + particle_r + 5 + static_cast<int>(arrow_len)), static_cast<float>(cy)},
        {static_cast<float>(px + particle_r + 5 + static_cast<int>(arrow_len) - 6), static_cast<float>(cy - 4)},
        {static_cast<float>(px + particle_r + 5 + static_cast<int>(arrow_len) - 6), static_cast<float>(cy + 4)},
        ui_colors::TEXT_PRIMARY);

    int info_y = area_y + area_h + 20;
    double lam = lambda_si();

    if (!PRESETS[particle_type_].is_quantum || !quantum) {
        draw_text("WAVE NATURE UNDETECTABLE",
                  static_cast<float>(area_x + area_w / 2 - 80), static_cast<float>(cy - 8), 14, ui_colors::DANGER);

        draw_text(TextFormat("\xCE\xBB = %.2e m", lam),
                  static_cast<float>(area_x), static_cast<float>(info_y), 14, ui_colors::DANGER);
        draw_text("Wavelength is astronomically smaller than any detector",
                  static_cast<float>(area_x), static_cast<float>(info_y + 18), 11, ui_colors::TEXT_SECONDARY);
    } else {
        draw_text(TextFormat("\xCE\xBB = %.3f %s", lambda_display_val(), lambda_display_unit()),
                  static_cast<float>(area_x), static_cast<float>(info_y), 14, ui_colors::ACCENT);
        draw_text("Wave nature is observable at this scale!",
                  static_cast<float>(area_x), static_cast<float>(info_y + 18), 11, ui_colors::SUCCESS);
    }

    draw_text("\xCE\xBB = h / p = h / (mv)", static_cast<float>(area_x),
              static_cast<float>(info_y + 40), 14, ui_colors::ACCENT);
    draw_text(TextFormat("v = %.2e m/s    m = %.2e kg    p = %.2e kg\xC2\xB7m/s",
              velocity_mps(), PRESETS[particle_type_].mass_kg, momentum_si()),
              static_cast<float>(area_x), static_cast<float>(info_y + 58), 11, ui_colors::TEXT_SECONDARY);
}

void DeBroglieScenario::render_scale_comparison(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("WAVELENGTH SCALE COMPARISON", static_cast<float>(vp_x + 10),
              static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    struct ScaleRef {
        const char* name;
        double size_m;
        Color color;
    };
    ScaleRef refs[] = {
        {"Proton",        1e-15,  Color{220, 80, 80, 200}},
        {"Atom",          1e-10,  Color{80, 160, 220, 200}},
        {"Molecule",      1e-9,   Color{80, 220, 120, 200}},
        {"Virus",         1e-7,   Color{200, 180, 60, 200}},
        {"Human Hair",    7e-5,   Color{200, 140, 80, 200}},
        {"Baseball",      0.07,   Color{180, 100, 60, 200}},
    };
    int nrefs = 6;

    int bar_x = vp_x + 100;
    int bar_w = vp_w - 160;
    int bar_y = vp_y + 60;
    int bar_h = vp_h - 160;

    double log_min = -36.0;
    double log_max = 0.0;

    DrawRectangle(bar_x, bar_y, bar_w, bar_h, Color{20, 20, 28, 255});

    for (int i = 0; i < nrefs; ++i) {
        double log_size = std::log10(refs[i].size_m);
        double frac = (log_size - log_min) / (log_max - log_min);
        int ry = bar_y + bar_h - static_cast<int>(frac * bar_h);
        DrawLine(bar_x, ry, bar_x + bar_w, ry, Color{refs[i].color.r, refs[i].color.g, refs[i].color.b, 60});
        DrawCircle(bar_x + bar_w + 10, ry, 4, refs[i].color);
        draw_text(TextFormat("%s (~%.0e m)", refs[i].name, refs[i].size_m),
                  static_cast<float>(bar_x + bar_w + 20), static_cast<float>(ry - 6), 11, refs[i].color);
    }

    double lam = lambda_si();
    if (lam > 0) {
        double log_lam = std::log10(lam);
        double frac = (log_lam - log_min) / (log_max - log_min);
        frac = std::clamp(frac, 0.0, 1.0);
        int ly = bar_y + bar_h - static_cast<int>(frac * bar_h);

        DrawLine(bar_x - 20, ly, bar_x + bar_w, ly, ui_colors::ACCENT);
        DrawTriangle(
            {static_cast<float>(bar_x - 20), static_cast<float>(ly)},
            {static_cast<float>(bar_x - 28), static_cast<float>(ly - 5)},
            {static_cast<float>(bar_x - 28), static_cast<float>(ly + 5)},
            ui_colors::ACCENT);

        draw_text(TextFormat("\xCE\xBB = %.2e m", lam),
                  static_cast<float>(bar_x - 30), static_cast<float>(ly - 20), 13, ui_colors::ACCENT);
        draw_text(TextFormat("(%s)", PRESETS[particle_type_].name),
                  static_cast<float>(bar_x - 30), static_cast<float>(ly + 8), 11, ui_colors::TEXT_SECONDARY);
    }

    for (int e = -35; e <= 0; e += 5) {
        double frac = (static_cast<double>(e) - log_min) / (log_max - log_min);
        int ty = bar_y + bar_h - static_cast<int>(frac * bar_h);
        DrawLine(bar_x - 5, ty, bar_x, ty, ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("10^%d", e), static_cast<float>(bar_x - 50), static_cast<float>(ty - 5), 9, ui_colors::TEXT_SECONDARY);
    }

    draw_text("Metres", static_cast<float>(bar_x - 50), static_cast<float>(bar_y - 15), 11, ui_colors::TEXT_SECONDARY);

    int info_y = bar_y + bar_h + 20;
    bool quantum = is_quantum_regime();
    if (quantum && PRESETS[particle_type_].is_quantum) {
        draw_text("\xCE\xBB is comparable to atomic scales \xE2\x86\x92 WAVE EFFECTS OBSERVABLE",
                  static_cast<float>(vp_x + 30), static_cast<float>(info_y), 13, ui_colors::SUCCESS);
    } else {
        draw_text("\xCE\xBB is far smaller than any structure \xE2\x86\x92 NO WAVE EFFECTS",
                  static_cast<float>(vp_x + 30), static_cast<float>(info_y), 13, ui_colors::DANGER);
    }
}

void DeBroglieScenario::render_diffraction(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("ELECTRON DIFFRACTION (Davisson-Germer)",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    double d_angstrom = static_cast<double>(crystal_d_slider_.get_value());
    double d_m = d_angstrom * 1e-10;
    double lam = lambda_si();

    int left_w = vp_w / 2 - 20;
    int right_w = vp_w / 2 - 20;

    int setup_x = vp_x + 20;
    int setup_y = vp_y + 45;
    int setup_h = vp_h - 120;
    int cy = setup_y + setup_h / 2;

    draw_text("Setup", static_cast<float>(setup_x + left_w / 2 - 15), static_cast<float>(setup_y - 2), 12, ui_colors::TEXT_SECONDARY);

    DrawRectangle(setup_x + left_w / 4 - 5, cy - 3, 10, 6, Color{255, 200, 50, 200});
    draw_text("source", static_cast<float>(setup_x + left_w / 4 - 15), static_cast<float>(cy + 8), 9, ui_colors::TEXT_SECONDARY);

    for (int i = 0; i < 8; ++i) {
        float fx = static_cast<float>(setup_x + left_w / 4 + 10 + i * 8);
        DrawLine(static_cast<int>(fx), cy, static_cast<int>(fx + 5), cy, Color{90, 160, 220, 180});
    }

    int crystal_x = setup_x + left_w / 2;
    DrawRectangle(crystal_x, cy - setup_h / 3, 8, setup_h * 2 / 3, Color{120, 100, 80, 200});
    for (int row = -4; row <= 4; ++row) {
        int dot_y = cy + row * (setup_h / 12);
        DrawCircle(crystal_x + 4, dot_y, 2, Color{200, 180, 100, 255});
    }
    draw_text("crystal", static_cast<float>(crystal_x - 8), static_cast<float>(cy + setup_h / 3 + 5), 9, ui_colors::TEXT_SECONDARY);

    int screen_x = setup_x + left_w * 3 / 4;
    DrawRectangle(screen_x, cy - setup_h / 3, 4, setup_h * 2 / 3, Color{60, 80, 60, 200});
    draw_text("detector", static_cast<float>(screen_x - 12), static_cast<float>(cy + setup_h / 3 + 5), 9, ui_colors::TEXT_SECONDARY);

    int pattern_x = vp_x + vp_w / 2 + 10;
    int pattern_y = setup_y;
    int pattern_h = setup_h;
    int pattern_w = right_w;

    draw_text("Diffraction Pattern", static_cast<float>(pattern_x + pattern_w / 2 - 50), static_cast<float>(pattern_y - 2), 12, ui_colors::TEXT_SECONDARY);

    DrawRectangle(pattern_x, pattern_y, pattern_w, pattern_h, Color{15, 15, 20, 255});

    int pcx = pattern_x + pattern_w / 2;
    int pcy = pattern_y + pattern_h / 2;

    if (lam > 0 && lam < d_m * 2.0 && PRESETS[particle_type_].is_quantum) {
        double sin_theta_max = lam / d_m;
        sin_theta_max = std::min(sin_theta_max, 1.0);

        int max_order = std::min(5, static_cast<int>(d_m / lam));
        float max_radius = static_cast<float>(pattern_h) * 0.4f;

        for (int n = 0; n <= max_order; ++n) {
            double sin_theta = n * lam / d_m;
            if (sin_theta > 1.0) break;
            float ring_r = static_cast<float>(sin_theta / sin_theta_max) * max_radius;
            if (n == 0) ring_r = 5.0f;

            auto intensity = static_cast<unsigned char>(220 - n * 35);
            intensity = std::max(intensity, static_cast<unsigned char>(60));

            if (n == 0) {
                DrawCircle(pcx, pcy, ring_r, Color{90, 200, 90, intensity});
            } else {
                for (int seg = 0; seg < 120; ++seg) {
                    float a1 = 2.0f * PI * seg / 120.0f;
                    float a2 = 2.0f * PI * (seg + 1) / 120.0f;
                    DrawLine(pcx + static_cast<int>(ring_r * std::cos(a1)),
                             pcy + static_cast<int>(ring_r * std::sin(a1)),
                             pcx + static_cast<int>(ring_r * std::cos(a2)),
                             pcy + static_cast<int>(ring_r * std::sin(a2)),
                             Color{90, 200, 90, intensity});
                    DrawLine(pcx + static_cast<int>((ring_r + 1) * std::cos(a1)),
                             pcy + static_cast<int>((ring_r + 1) * std::sin(a1)),
                             pcx + static_cast<int>((ring_r + 1) * std::cos(a2)),
                             pcy + static_cast<int>((ring_r + 1) * std::sin(a2)),
                             Color{90, 200, 90, static_cast<unsigned char>(intensity / 2)});
                }
            }
        }

        draw_text(TextFormat("n\xCE\xBB = d sin\xCE\xB8    (%d orders visible)", max_order),
                  static_cast<float>(pattern_x + 10), static_cast<float>(pattern_y + pattern_h + 5), 11, ui_colors::SUCCESS);
    } else if (!PRESETS[particle_type_].is_quantum) {
        DrawCircle(pcx, pcy, 3, Color{90, 200, 90, 200});
        draw_text("No diffraction: \xCE\xBB << d",
                  static_cast<float>(pattern_x + 10), static_cast<float>(pattern_y + pattern_h + 5), 11, ui_colors::DANGER);
    } else {
        DrawCircle(pcx, pcy, 3, Color{90, 200, 90, 200});
        draw_text("\xCE\xBB too large or too small for this d",
                  static_cast<float>(pattern_x + 10), static_cast<float>(pattern_y + pattern_h + 5), 11, ui_colors::TEXT_SECONDARY);
    }

    int info_y = setup_y + setup_h + 25;
    draw_text(TextFormat("\xCE\xBB = %.3f %s    d = %.1f \xC3\x85",
              lambda_display_val(), lambda_display_unit(), d_angstrom),
              static_cast<float>(vp_x + 30), static_cast<float>(info_y), 13, ui_colors::ACCENT);
    draw_text("Diffraction occurs when \xCE\xBB ~ d (lattice spacing)",
              static_cast<float>(vp_x + 30), static_cast<float>(info_y + 18), 11, ui_colors::TEXT_SECONDARY);
}

void DeBroglieScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("DE BROGLIE", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    velocity_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 8;

    if (current_view_ == 2) {
        crystal_d_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 8;
    }

    draw_text("Particle type:", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 16;

    for (int i = 0; i < NUM_PRESETS; ++i) {
        Color bg = (i == particle_type_) ? ui_colors::ACCENT : ui_colors::PANEL_BG;
        DrawRectangle(cx, cy, w - 20, 22, bg);
        DrawRectangleLinesEx({static_cast<float>(cx), static_cast<float>(cy),
                              static_cast<float>(w - 20), 22.0f}, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(PRESETS[i].name, static_cast<float>(cx + 6), static_cast<float>(cy + 4), 11,
                  (i == particle_type_) ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (mouse.x >= cx && mouse.x < cx + w - 20 && mouse.y >= cy && mouse.y < cy + 22) {
                particle_type_ = i;
            }
        }
        cy += 24;
    }

    cy += 8;
    draw_text("[P] cycle particles", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("Views: 1-3 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);

    cy += 20;
    if (HelpPopup::render_help_button(font_, has_font_, cx, cy)) {
        help_popup_.show({"\xCE\xBB", "De Broglie Wavelength",
            "Every particle has an associated wavelength \xCE\xBB = h/p. "
            "For macroscopic objects, this wavelength is so incredibly small (~10\xE2\x81\xBB\xC2\xB3\xE2\x81\xB4 m for a baseball) "
            "that wave behavior is completely undetectable. "
            "Only particles with masses near atomic scale have wavelengths large enough to produce "
            "observable wave effects like diffraction and interference.",
            "\xCE\xBB = h / p = h / (mv)",
            "h = 6.626 \xC3\x97 10\xE2\x81\xBB\xC2\xB3\xE2\x81\xB4 J\xC2\xB7s"});
    }
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
    draw_prop("Size", TextFormat("~%.0e m", PRESETS[particle_type_].typical_size_m), px, py);
    draw_prop("v", TextFormat("%.2e m/s", velocity_mps()), px, py);
    py += 4;

    draw_section("De Broglie", px, py);
    draw_prop("p", TextFormat("%.2e kg\xC2\xB7m/s", momentum_si()), px, py);

    double lam = lambda_si();
    Color lam_color = (is_quantum_regime() && PRESETS[particle_type_].is_quantum)
                       ? ui_colors::ACCENT : ui_colors::DANGER;
    draw_prop("\xCE\xBB", TextFormat("%.3f %s", lambda_display_val(), lambda_display_unit()), px, py, lam_color);
    draw_prop("\xCE\xBB (SI)", TextFormat("%.2e m", lam), px, py);
    py += 4;

    draw_section("Regime", px, py);
    bool quantum = is_quantum_regime() && PRESETS[particle_type_].is_quantum;
    draw_text(quantum ? "QUANTUM" : "CLASSICAL",
              static_cast<float>(px), static_cast<float>(py), 14,
              quantum ? ui_colors::SUCCESS : ui_colors::DANGER);
    py += 18;

    double ratio = lam / PRESETS[particle_type_].typical_size_m;
    draw_prop("\xCE\xBB/size", TextFormat("%.2e", ratio), px, py);
    py += 4;

    if (quantum) {
        draw_text("Wave effects are", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
        py += 14;
        draw_text("experimentally observable", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    } else {
        draw_text("\xCE\xBB is negligible compared", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
        py += 14;
        draw_text("to the particle size.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
        py += 14;
        draw_text("No wave behaviour.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    }

    py += 20;
    draw_separator(px, py, w - 24);
    draw_section("Formula", px, py);
    draw_text("\xCE\xBB = h / p = h / (mv)", static_cast<float>(px), static_cast<float>(py), 13, ui_colors::ACCENT);
    py += 18;
    draw_text(TextFormat("h = %.3e J\xC2\xB7s", H_SI), static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
}
