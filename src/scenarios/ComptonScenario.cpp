#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "ComptonScenario.hpp"

const ComptonScenario::TargetPreset ComptonScenario::TARGETS[2] = {
    {"Electron", 9.109e-31, 2.426},
    {"Proton",   1.673e-27, 1.321e-3}
};

ComptonScenario::ComptonScenario()
    : theta_slider_("Scattering angle \xCE\xB8", 0.0f, 180.0f, 90.0f, "%.0f\xC2\xB0")
    , lambda_in_slider_("Incoming \xCE\xBB (pm)", 1.0f, 100.0f, 10.0f, "%.1f pm")
{}

void ComptonScenario::on_enter() {
    current_view_ = 0;
    anim_time_ = 0.0;
    anim_phase_ = 0.0f;
}

double ComptonScenario::theta_rad() const {
    return static_cast<double>(theta_slider_.get_value()) * PI / 180.0;
}

double ComptonScenario::lambda_in_m() const {
    return static_cast<double>(lambda_in_slider_.get_value()) * 1e-12;
}

double ComptonScenario::compton_wavelength() const {
    double m = TARGETS[target_type_].mass_kg;
    return H_SI / (m * C_SI);
}

double ComptonScenario::delta_lambda() const {
    return compton_wavelength() * (1.0 - std::cos(theta_rad()));
}

double ComptonScenario::lambda_out_m() const {
    return lambda_in_m() + delta_lambda();
}

double ComptonScenario::photon_energy_in_eV() const {
    return (H_SI * C_SI / lambda_in_m()) / EV_TO_J;
}

double ComptonScenario::photon_energy_out_eV() const {
    return (H_SI * C_SI / lambda_out_m()) / EV_TO_J;
}

double ComptonScenario::electron_ke_eV() const {
    return photon_energy_in_eV() - photon_energy_out_eV();
}

double ComptonScenario::electron_recoil_angle() const {
    double th = theta_rad();
    if (std::abs(th) < 1e-10) return 0.0;
    double lam_c = compton_wavelength();
    double lam_i = lambda_in_m();
    double cot_phi = (1.0 + lam_i / lam_c) * std::tan(th / 2.0);
    if (std::abs(cot_phi) < 1e-10) return PI / 2.0;
    return std::atan(1.0 / cot_phi);
}

const char* ComptonScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Scattering";
        case 1: return "Wavelengths";
        case 2: return "\xCE\x94\xCE\xBB vs \xCE\xB8";
        case 3: return "Classical vs QM";
        default: return "Unknown";
    }
}

void ComptonScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
    if (IsKeyPressed(KEY_T)) {
        target_type_ = (target_type_ + 1) % 2;
    }
}

void ComptonScenario::update(double dt) {
    anim_time_ += dt;
    anim_phase_ += static_cast<float>(dt) * 2.0f;
    if (anim_phase_ > 2.0f * PI) anim_phase_ -= 2.0f * PI;
}

void ComptonScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;
    switch (current_view_) {
        case 0: render_scattering_anim(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_wavelength_compare(vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_shift_graph(vp_x, vp_y, vp_w, vp_h); break;
        case 3: render_classical_vs_quantum(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void ComptonScenario::render_scattering_anim(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("COMPTON SCATTERING", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Photon collides with electron, loses energy, wavelength increases",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int cx = vp_x + vp_w / 2;
    int cy = vp_y + vp_h / 2 - 20;

    DrawCircle(cx, cy, 12, Color{100, 160, 220, 220});
    draw_text("e\xE2\x81\xBB", static_cast<float>(cx - 5), static_cast<float>(cy - 5), 10, WHITE);

    float cycle = std::fmod(static_cast<float>(anim_time_) * 0.5f, 2.0f);
    float th = static_cast<float>(theta_rad());

    if (cycle < 1.0f) {
        float t = cycle;
        float px = static_cast<float>(cx) - 200.0f * (1.0f - t);
        float wave_k = 2.0f * PI / std::max(5.0f, static_cast<float>(lambda_in_slider_.get_value()) * 0.5f);

        for (int i = -20; i <= 0; ++i) {
            float wx = px + i * 3.0f;
            float wy = 6.0f * std::sin(wave_k * static_cast<float>(i) - anim_phase_ * 5.0f);
            DrawCircle(static_cast<int>(wx), cy + static_cast<int>(wy), 2, Color{255, 200, 50, 200});
        }
        DrawCircle(static_cast<int>(px), cy, 4, Color{255, 220, 50, 255});
    } else {
        float t = cycle - 1.0f;

        float out_dx = std::cos(th) * 200.0f * t;
        float out_dy = -std::sin(th) * 200.0f * t;
        float out_px = static_cast<float>(cx) + out_dx;
        float out_py = static_cast<float>(cy) + out_dy;

        float wave_k_out = 2.0f * PI / std::max(5.0f, static_cast<float>(lambda_out_m() / 1e-12) * 0.5f);
        for (int i = -15; i <= 0; ++i) {
            float wx = out_px + std::cos(th) * i * 3.0f;
            float wy_base = out_py + (-std::sin(th)) * i * 3.0f;
            float perp_x = -std::sin(th);
            float perp_y = -std::cos(th);
            float wave_val = 5.0f * std::sin(wave_k_out * static_cast<float>(i) - anim_phase_ * 4.0f);
            DrawCircle(static_cast<int>(wx + perp_x * wave_val), static_cast<int>(wy_base + perp_y * wave_val),
                       2, Color{255, 150, 50, 180});
        }
        DrawCircle(static_cast<int>(out_px), static_cast<int>(out_py), 4, Color{255, 160, 50, 255});

        float recoil_a = -static_cast<float>(electron_recoil_angle());
        float recoil_dx = std::cos(recoil_a) * 80.0f * t;
        float recoil_dy = std::sin(recoil_a) * 80.0f * t;
        DrawCircle(cx + static_cast<int>(recoil_dx), cy + static_cast<int>(recoil_dy), 8,
                   Color{100, 160, 220, static_cast<unsigned char>(220 - t * 150)});
    }

    DrawLine(cx - 150, cy + 40, cx + 150, cy + 40, Color{60, 60, 70, 100});
    DrawLine(cx - 150, cy + 40, cx - 80, cy + 40, Color{255, 200, 50, 100});
    draw_text("\xCE\xBB", static_cast<float>(cx - 120), static_cast<float>(cy + 44), 11, Color{255, 200, 50, 200});

    float arc_r = 60.0f;
    for (int i = 0; i < 20; ++i) {
        float a1 = -th * static_cast<float>(i) / 20.0f;
        float a2 = -th * static_cast<float>(i + 1) / 20.0f;
        DrawLine(cx + static_cast<int>(arc_r * std::cos(a1)), cy + static_cast<int>(arc_r * std::sin(a1)),
                 cx + static_cast<int>(arc_r * std::cos(a2)), cy + static_cast<int>(arc_r * std::sin(a2)),
                 ui_colors::ACCENT);
    }
    draw_text(TextFormat("\xCE\xB8 = %.0f\xC2\xB0", theta_slider_.get_value()),
              static_cast<float>(cx + 65), static_cast<float>(cy - 20), 12, ui_colors::ACCENT);

    int info_y = cy + 80;
    draw_text(TextFormat("\xCE\xBB = %.1f pm  \xE2\x86\x92  \xCE\xBB' = %.2f pm   (\xCE\x94\xCE\xBB = %.3f pm)",
              lambda_in_slider_.get_value(), lambda_out_m() * 1e12, delta_lambda() * 1e12),
              static_cast<float>(vp_x + 30), static_cast<float>(info_y), 13, ui_colors::TEXT_PRIMARY);
    draw_text(TextFormat("E_photon: %.1f keV \xE2\x86\x92 %.1f keV    E_electron: %.1f keV",
              photon_energy_in_eV() / 1000.0, photon_energy_out_eV() / 1000.0, electron_ke_eV() / 1000.0),
              static_cast<float>(vp_x + 30), static_cast<float>(info_y + 18), 12, ui_colors::TEXT_SECONDARY);
}

void ComptonScenario::render_wavelength_compare(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("WAVELENGTH COMPARISON", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    int wave_x = vp_x + 60;
    int wave_w = vp_w - 100;
    int half_h = vp_h / 2 - 30;

    int top_y = vp_y + 50;
    int top_cy = top_y + half_h / 2;
    draw_text("INCOMING PHOTON", static_cast<float>(wave_x), static_cast<float>(top_y - 5), 12, Color{255, 200, 50, 255});

    float lam_in_px = std::max(10.0f, std::min(static_cast<float>(wave_w) / 3.0f, lambda_in_slider_.get_value() * 3.0f));
    float k_in = 2.0f * PI / lam_in_px;
    int prev_x = 0, prev_y = 0;
    for (int i = 0; i < wave_w; ++i) {
        int wx = wave_x + i;
        int wy = top_cy - static_cast<int>(30.0f * std::sin(k_in * i - anim_phase_ * 3.0f));
        if (i > 0) DrawLine(prev_x, prev_y, wx, wy, Color{255, 200, 50, 220});
        prev_x = wx;
        prev_y = wy;
    }

    if (lam_in_px > 15 && lam_in_px < wave_w / 2) {
        int bx1 = wave_x + 20;
        int bx2 = bx1 + static_cast<int>(lam_in_px);
        int by = top_cy + 40;
        DrawLine(bx1, by, bx2, by, Color{255, 200, 50, 200});
        DrawLine(bx1, by - 4, bx1, by + 4, Color{255, 200, 50, 200});
        DrawLine(bx2, by - 4, bx2, by + 4, Color{255, 200, 50, 200});
        draw_text(TextFormat("\xCE\xBB = %.1f pm", lambda_in_slider_.get_value()),
                  static_cast<float>((bx1 + bx2) / 2 - 20), static_cast<float>(by + 6), 11, Color{255, 200, 50, 255});
    }

    int bot_y = vp_y + vp_h / 2 + 20;
    int bot_cy = bot_y + half_h / 2;
    draw_text("SCATTERED PHOTON", static_cast<float>(wave_x), static_cast<float>(bot_y - 5), 12, Color{255, 150, 50, 255});

    float lam_out_pm = static_cast<float>(lambda_out_m() * 1e12);
    float lam_out_px = std::max(10.0f, std::min(static_cast<float>(wave_w) / 3.0f, lam_out_pm * 3.0f));
    float k_out = 2.0f * PI / lam_out_px;
    prev_x = 0; prev_y = 0;
    for (int i = 0; i < wave_w; ++i) {
        int wx = wave_x + i;
        int wy = bot_cy - static_cast<int>(30.0f * std::sin(k_out * i - anim_phase_ * 2.5f));
        if (i > 0) DrawLine(prev_x, prev_y, wx, wy, Color{255, 150, 50, 220});
        prev_x = wx;
        prev_y = wy;
    }

    if (lam_out_px > 15 && lam_out_px < wave_w / 2) {
        int bx1 = wave_x + 20;
        int bx2 = bx1 + static_cast<int>(lam_out_px);
        int by = bot_cy + 40;
        DrawLine(bx1, by, bx2, by, Color{255, 150, 50, 200});
        DrawLine(bx1, by - 4, bx1, by + 4, Color{255, 150, 50, 200});
        DrawLine(bx2, by - 4, bx2, by + 4, Color{255, 150, 50, 200});
        draw_text(TextFormat("\xCE\xBB' = %.2f pm", lam_out_pm),
                  static_cast<float>((bx1 + bx2) / 2 - 20), static_cast<float>(by + 6), 11, Color{255, 150, 50, 255});
    }

    DrawLine(wave_x, vp_y + vp_h / 2 + 5, wave_x + wave_w, vp_y + vp_h / 2 + 5, ui_colors::PANEL_BORDER);

    int info_y = bot_cy + half_h / 2 + 10;
    draw_text(TextFormat("\xCE\x94\xCE\xBB = \xCE\xBB' - \xCE\xBB = %.4f pm",
              delta_lambda() * 1e12), static_cast<float>(wave_x), static_cast<float>(info_y), 14, ui_colors::ACCENT);
    draw_text(TextFormat("\xCE\xBB_C = h/(m_e c) = %.3f pm (Compton wavelength)", compton_wavelength() * 1e12),
              static_cast<float>(wave_x), static_cast<float>(info_y + 18), 11, ui_colors::TEXT_SECONDARY);
}

void ComptonScenario::render_shift_graph(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("\xCE\x94\xCE\xBB vs SCATTERING ANGLE",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    int plot_x = vp_x + 70;
    int plot_w = vp_w - 110;
    int plot_y = vp_y + 50;
    int plot_h = vp_h - 140;
    int baseline = plot_y + plot_h;

    DrawLine(plot_x, baseline, plot_x + plot_w, baseline, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, baseline, ui_colors::TEXT_SECONDARY);
    draw_text("\xCE\xB8 (degrees)", static_cast<float>(plot_x + plot_w / 2 - 30), static_cast<float>(baseline + 20), 11, ui_colors::TEXT_SECONDARY);
    draw_text("\xCE\x94\xCE\xBB (pm)", static_cast<float>(plot_x - 55), static_cast<float>(plot_y - 2), 11, ui_colors::TEXT_SECONDARY);

    double lam_c = compton_wavelength() * 1e12;
    double max_shift = 2.0 * lam_c;

    int prev_gx = 0, prev_gy = 0;
    for (int i = 0; i <= plot_w; ++i) {
        double angle = PI * static_cast<double>(i) / plot_w;
        double shift = lam_c * (1.0 - std::cos(angle));
        double frac = shift / (max_shift * 1.15);
        int gx = plot_x + i;
        int gy = baseline - static_cast<int>(frac * plot_h);
        if (i > 0) DrawLine(prev_gx, prev_gy, gx, gy, ui_colors::ACCENT);
        prev_gx = gx;
        prev_gy = gy;
    }

    double cur_theta = theta_rad();
    double cur_shift = delta_lambda() * 1e12;
    double cur_frac_x = cur_theta / PI;
    double cur_frac_y = cur_shift / (max_shift * 1.15);
    int dot_x = plot_x + static_cast<int>(cur_frac_x * plot_w);
    int dot_y = baseline - static_cast<int>(cur_frac_y * plot_h);

    DrawLine(dot_x, baseline, dot_x, dot_y, Color{255, 200, 50, 100});
    DrawLine(plot_x, dot_y, dot_x, dot_y, Color{255, 200, 50, 100});
    DrawCircle(dot_x, dot_y, 5, Color{255, 200, 50, 255});

    draw_text(TextFormat("%.3f pm", cur_shift), static_cast<float>(dot_x + 8),
              static_cast<float>(dot_y - 10), 11, Color{255, 200, 50, 255});

    struct KeyPoint { double angle; const char* label; };
    KeyPoint points[] = {
        {0, "\xCE\xB8=0: no shift"},
        {PI / 2.0, "\xCE\xB8=90\xC2\xB0: \xCE\x94\xCE\xBB=\xCE\xBB_C"},
        {PI, "\xCE\xB8=180\xC2\xB0: \xCE\x94\xCE\xBB=2\xCE\xBB_C"}
    };

    for (const auto& kp : points) {
        int kx = plot_x + static_cast<int>((kp.angle / PI) * plot_w);
        double ks = lam_c * (1.0 - std::cos(kp.angle));
        int ky = baseline - static_cast<int>((ks / (max_shift * 1.15)) * plot_h);
        DrawCircle(kx, ky, 3, ui_colors::TEXT_SECONDARY);
        draw_text(kp.label, static_cast<float>(kx - 30), static_cast<float>(ky - 15), 9, ui_colors::TEXT_SECONDARY);
    }

    for (int t = 0; t <= 4; ++t) {
        int tx = plot_x + t * plot_w / 4;
        DrawLine(tx, baseline, tx, baseline + 4, ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("%d", t * 45), static_cast<float>(tx - 6), static_cast<float>(baseline + 6), 9, ui_colors::TEXT_SECONDARY);
    }

    for (int t = 0; t <= 4; ++t) {
        int ty = baseline - t * plot_h / 4;
        DrawLine(plot_x - 4, ty, plot_x, ty, ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("%.2f", t * max_shift / 4.0), static_cast<float>(plot_x - 40), static_cast<float>(ty - 5), 9, ui_colors::TEXT_SECONDARY);
    }

    int info_y = baseline + 30;
    draw_text("\xCE\x94\xCE\xBB = (\xE2\x84\x8E/m_e c)(1 - cos\xCE\xB8)",
              static_cast<float>(plot_x), static_cast<float>(info_y), 14, ui_colors::ACCENT);
}

void ComptonScenario::render_classical_vs_quantum(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("CLASSICAL vs QUANTUM PREDICTION", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    int half_w = vp_w / 2 - 20;
    int col1_x = vp_x + 20;
    int col2_x = vp_x + vp_w / 2 + 10;
    int top_y = vp_y + 45;

    DrawRectangle(col1_x, top_y, half_w, vp_h - 100, Color{25, 20, 20, 255});
    DrawRectangleLinesEx({static_cast<float>(col1_x), static_cast<float>(top_y),
                          static_cast<float>(half_w), static_cast<float>(vp_h - 100)}, 1.0f, Color{180, 60, 60, 100});

    DrawRectangle(col2_x, top_y, half_w, vp_h - 100, Color{20, 20, 25, 255});
    DrawRectangleLinesEx({static_cast<float>(col2_x), static_cast<float>(top_y),
                          static_cast<float>(half_w), static_cast<float>(vp_h - 100)}, 1.0f, Color{60, 130, 180, 100});

    draw_text("THOMSON (Classical)", static_cast<float>(col1_x + 10), static_cast<float>(top_y + 8), 13, ui_colors::DANGER);
    draw_text("COMPTON (Quantum)", static_cast<float>(col2_x + 10), static_cast<float>(top_y + 8), 13, ui_colors::ACCENT);

    int wave_y = top_y + 50;
    int wave_w = half_w - 20;

    float lam_in_px = std::max(10.0f, lambda_in_slider_.get_value() * 2.0f);
    float k_in = 2.0f * PI / lam_in_px;

    int prev_x = 0, prev_y = 0;
    for (int i = 0; i < wave_w; ++i) {
        int wx = col1_x + 10 + i;
        int wy = wave_y + 30 - static_cast<int>(20.0f * std::sin(k_in * i - anim_phase_ * 3.0f));
        if (i > 0) DrawLine(prev_x, prev_y, wx, wy, Color{255, 200, 50, 180});
        prev_x = wx;
        prev_y = wy;
    }
    draw_text("in", static_cast<float>(col1_x + 10), static_cast<float>(wave_y + 5), 10, Color{255, 200, 50, 200});

    int wave_y2 = wave_y + 80;
    prev_x = 0; prev_y = 0;
    for (int i = 0; i < wave_w; ++i) {
        int wx = col1_x + 10 + i;
        int wy = wave_y2 + 30 - static_cast<int>(20.0f * std::sin(k_in * i - anim_phase_ * 3.0f));
        if (i > 0) DrawLine(prev_x, prev_y, wx, wy, Color{255, 200, 50, 180});
        prev_x = wx;
        prev_y = wy;
    }
    draw_text("out", static_cast<float>(col1_x + 10), static_cast<float>(wave_y2 + 5), 10, Color{255, 200, 50, 200});

    draw_text("\xCE\xBB_out = \xCE\xBB_in (NO CHANGE)", static_cast<float>(col1_x + 10),
              static_cast<float>(wave_y2 + 65), 12, ui_colors::DANGER);
    draw_text("Classical EM: wave shakes", static_cast<float>(col1_x + 10),
              static_cast<float>(wave_y2 + 85), 11, ui_colors::TEXT_SECONDARY);
    draw_text("electron, re-radiates at", static_cast<float>(col1_x + 10),
              static_cast<float>(wave_y2 + 99), 11, ui_colors::TEXT_SECONDARY);
    draw_text("same frequency.", static_cast<float>(col1_x + 10),
              static_cast<float>(wave_y2 + 113), 11, ui_colors::TEXT_SECONDARY);

    prev_x = 0; prev_y = 0;
    for (int i = 0; i < wave_w; ++i) {
        int wx = col2_x + 10 + i;
        int wy = wave_y + 30 - static_cast<int>(20.0f * std::sin(k_in * i - anim_phase_ * 3.0f));
        if (i > 0) DrawLine(prev_x, prev_y, wx, wy, Color{255, 200, 50, 180});
        prev_x = wx;
        prev_y = wy;
    }
    draw_text("in", static_cast<float>(col2_x + 10), static_cast<float>(wave_y + 5), 10, Color{255, 200, 50, 200});

    float lam_out_px = std::max(10.0f, static_cast<float>(lambda_out_m() * 1e12) * 2.0f);
    float k_out = 2.0f * PI / lam_out_px;
    prev_x = 0; prev_y = 0;
    for (int i = 0; i < wave_w; ++i) {
        int wx = col2_x + 10 + i;
        int wy = wave_y2 + 30 - static_cast<int>(20.0f * std::sin(k_out * i - anim_phase_ * 2.5f));
        if (i > 0) DrawLine(prev_x, prev_y, wx, wy, Color{255, 150, 50, 180});
        prev_x = wx;
        prev_y = wy;
    }
    draw_text("out", static_cast<float>(col2_x + 10), static_cast<float>(wave_y2 + 5), 10, Color{255, 150, 50, 200});

    draw_text(TextFormat("\xCE\xBB' = \xCE\xBB + %.3f pm", delta_lambda() * 1e12),
              static_cast<float>(col2_x + 10), static_cast<float>(wave_y2 + 65), 12, ui_colors::ACCENT);
    draw_text("Photon billiard ball:", static_cast<float>(col2_x + 10),
              static_cast<float>(wave_y2 + 85), 11, ui_colors::TEXT_SECONDARY);
    draw_text("photon transfers momentum", static_cast<float>(col2_x + 10),
              static_cast<float>(wave_y2 + 99), 11, ui_colors::TEXT_SECONDARY);
    draw_text("to electron, loses energy.", static_cast<float>(col2_x + 10),
              static_cast<float>(wave_y2 + 113), 11, ui_colors::TEXT_SECONDARY);

    int verdict_y = top_y + vp_h - 120;
    draw_text("CLASSICAL FAILS", static_cast<float>(col1_x + half_w / 2 - 40),
              static_cast<float>(verdict_y), 14, ui_colors::DANGER);
    draw_text("QUANTUM CORRECT", static_cast<float>(col2_x + half_w / 2 - 45),
              static_cast<float>(verdict_y), 14, ui_colors::SUCCESS);
}

void ComptonScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("COMPTON", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    theta_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 4;
    lambda_in_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 8;

    draw_text("Target:", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 16;
    for (int i = 0; i < 2; ++i) {
        Color bg = (i == target_type_) ? ui_colors::ACCENT : ui_colors::PANEL_BG;
        DrawRectangle(cx, cy, w - 20, 22, bg);
        DrawRectangleLinesEx({static_cast<float>(cx), static_cast<float>(cy),
                              static_cast<float>(w - 20), 22.0f}, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(TARGETS[i].name, static_cast<float>(cx + 6), static_cast<float>(cy + 4), 11,
                  (i == target_type_) ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (mouse.x >= cx && mouse.x < cx + w - 20 && mouse.y >= cy && mouse.y < cy + 22) {
                target_type_ = i;
            }
        }
        cy += 24;
    }

    cy += 8;
    draw_separator(cx, cy, w - 20);
    cy += 4;

    draw_text("[T] cycle target", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("Views: 1-4 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);

    cy += 20;
    if (HelpPopup::render_help_button(font_, has_font_, cx, cy)) {
        help_popup_.show({"\xCE\x94\xCE\xBB", "Compton Scattering",
            "When a photon collides with a free electron, the photon's wavelength increases "
            "(it loses energy). The shift depends only on the scattering angle, not the initial wavelength. "
            "This effect proved that light consists of particles (photons) with momentum p = h/\xCE\xBB, "
            "and cannot be explained by classical wave theory (Thomson scattering).",
            "\xCE\x94\xCE\xBB = (h/m_e c)(1 - cos\xCE\xB8)",
            "\xCE\xBB_C = h/(m_e c) = 2.426 pm"});
    }
}

void ComptonScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    draw_section("Incoming", px, py);
    draw_prop("\xCE\xBB", TextFormat("%.1f pm", lambda_in_slider_.get_value()), px, py, Color{255, 200, 50, 255});
    draw_prop("E", TextFormat("%.1f keV", photon_energy_in_eV() / 1000.0), px, py);
    py += 4;

    draw_section("Scattered", px, py);
    draw_prop("\xCE\xB8", TextFormat("%.0f\xC2\xB0", theta_slider_.get_value()), px, py, ui_colors::ACCENT);
    draw_prop("\xCE\xBB'", TextFormat("%.2f pm", lambda_out_m() * 1e12), px, py, Color{255, 150, 50, 255});
    draw_prop("E'", TextFormat("%.1f keV", photon_energy_out_eV() / 1000.0), px, py);
    py += 4;

    draw_section("Shift", px, py);
    draw_prop("\xCE\x94\xCE\xBB", TextFormat("%.4f pm", delta_lambda() * 1e12), px, py, ui_colors::ACCENT);
    draw_prop("\xCE\xBB_C", TextFormat("%.3f pm", compton_wavelength() * 1e12), px, py);
    py += 4;

    draw_section("Electron", px, py);
    draw_prop("K.E.", TextFormat("%.1f keV", electron_ke_eV() / 1000.0), px, py);
    draw_prop("Recoil \xCF\x86", TextFormat("%.1f\xC2\xB0", electron_recoil_angle() * 180.0 / PI), px, py);
    py += 4;

    draw_section("Target", px, py);
    draw_prop("Type", TARGETS[target_type_].name, px, py);
    draw_prop("Mass", TextFormat("%.3e kg", TARGETS[target_type_].mass_kg), px, py);

    py += 12;
    draw_separator(px, py, w - 24);
    draw_section("Formula", px, py);
    draw_text("\xCE\x94\xCE\xBB = \xCE\xBB_C(1-cos\xCE\xB8)", static_cast<float>(px), static_cast<float>(py), 13, ui_colors::ACCENT);
    py += 18;
    draw_text("\xCE\xBB_C = h/(mc)", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
}
