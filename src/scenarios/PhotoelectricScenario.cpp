#include <cmath>
#include <cstdio>
#include <algorithm>
#include <raylib.h>

#include "PhotoelectricScenario.hpp"

namespace {
    constexpr Color METAL_SURFACE   = {90, 95, 110, 255};
    constexpr Color METAL_EDGE      = {120, 125, 140, 255};
    constexpr Color ELECTRON_COLOR  = {60, 200, 255, 255};
    constexpr Color AMMETER_BG      = {35, 40, 50, 255};
    constexpr Color WIRE_COLOR      = {140, 140, 150, 200};
    constexpr Color BAR_PHOTON      = {255, 200, 60, 255};
    constexpr Color BAR_WORK        = {200, 70, 70, 255};
    constexpr Color BAR_KINETIC     = {60, 200, 120, 255};
    constexpr Color LABEL_DIM       = {130, 130, 140, 255};
}

PhotoelectricScenario::PhotoelectricScenario()
    : frequency_slider_("Frequency (THz)", 300.0f, 3000.0f, 1200.0f, "%.0f")
    , intensity_slider_("Intensity", 1.0f, 10.0f, 5.0f, "%.0f")
{}

void PhotoelectricScenario::on_enter() {
    current_view_ = 0;
    photons_.clear();
    electrons_.clear();
    photon_count_ = 0;
    electron_count_ = 0;
    spawn_timer_ = 0.0;
    anim_pulse_ = 0.0;
}

double PhotoelectricScenario::wavelength_nm() const {
    return 2.998e8 / frequency_Hz() * 1e9;
}

double PhotoelectricScenario::photon_energy_eV() const {
    return PhotoelectricEffect::photon_energy_eV(wavelength_nm());
}

// --------------------------------------------------------------------------
// Update — shared physics for all views
// --------------------------------------------------------------------------

void PhotoelectricScenario::update(double dt) {
    anim_pulse_ += dt;

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    auto Ek = PhotoelectricEffect::compute_kinetic_energy(wavelength_nm(), metal.work_function_eV);

    spawn_timer_ += dt;
    double spawn_interval = 0.3 / intensity();
    while (spawn_timer_ >= spawn_interval) {
        spawn_timer_ -= spawn_interval;
        Photon p;
        p.x = -0.8f;
        p.y = 0.08f * static_cast<float>(GetRandomValue(-10, 10));
        p.active = true;
        photons_.push_back(p);
        photon_count_++;
    }

    for (auto& p : photons_) {
        if (!p.active) continue;
        p.x += 2.0f * static_cast<float>(dt);

        if (p.x >= 0.0f) {
            p.active = false;
            if (Ek.has_value()) {
                Electron e;
                e.x = 0.05f;
                e.y = p.y;
                float speed = static_cast<float>(std::sqrt(Ek.value())) * 1.2f;
                float angle = static_cast<float>(GetRandomValue(-50, 50)) * 3.14159f / 180.0f;
                e.vx = speed * std::cos(angle);
                e.vy = -speed * std::abs(std::sin(angle));
                e.life = 2.5f;
                electrons_.push_back(e);
                electron_count_++;
            }
        }
        if (p.x > 2.0f) p.active = false;
    }

    for (auto& e : electrons_) {
        e.x += e.vx * static_cast<float>(dt);
        e.y += e.vy * static_cast<float>(dt);
        e.vy += 0.4f * static_cast<float>(dt);
        e.life -= static_cast<float>(dt);
    }

    photons_.erase(std::remove_if(photons_.begin(), photons_.end(),
                   [](const Photon& p) { return !p.active; }), photons_.end());
    electrons_.erase(std::remove_if(electrons_.begin(), electrons_.end(),
                     [](const Electron& e) { return e.life <= 0; }), electrons_.end());
}

void PhotoelectricScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
    if (IsKeyPressed(KEY_UP)) {
        metal_idx_ = (metal_idx_ + 1) % static_cast<int>(PhotoelectricEffect::get_metal_presets().size());
    }
    if (IsKeyPressed(KEY_DOWN)) {
        metal_idx_--;
        if (metal_idx_ < 0) metal_idx_ = static_cast<int>(PhotoelectricEffect::get_metal_presets().size()) - 1;
    }
}

// --------------------------------------------------------------------------
// View names
// --------------------------------------------------------------------------

const char* PhotoelectricScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Experiment";
        case 1: return "Energy Diagram";
        case 2: return "Ek vs f Graph";
        default: return "Unknown";
    }
}

// --------------------------------------------------------------------------
// Viewport dispatch
// --------------------------------------------------------------------------

void PhotoelectricScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;

    switch (current_view_) {
        case 0: render_experiment_view(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_energy_diagram_view(vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_graph_view(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }

    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

// --------------------------------------------------------------------------
// EM spectrum bar (shared)
// --------------------------------------------------------------------------

void PhotoelectricScenario::render_em_bar(int x, int y, int w) {
    int bar_x = x + 40;
    int bar_w = w - 80;
    int bar_y = y;
    int bar_h = 16;

    for (int px = 0; px < bar_w; ++px) {
        double wl = 100.0 + 700.0 * static_cast<double>(px) / bar_w;
        Color c = SpectralTransition::wavelength_to_rgb(wl);
        DrawLine(bar_x + px, bar_y, bar_x + px, bar_y + bar_h, c);
    }

    draw_text("UV", static_cast<float>(bar_x - 5), static_cast<float>(bar_y + bar_h + 2), 9, ui_colors::TEXT_SECONDARY);
    draw_text("IR", static_cast<float>(bar_x + bar_w - 10), static_cast<float>(bar_y + bar_h + 2), 9, ui_colors::TEXT_SECONDARY);

    double wl = wavelength_nm();
    int marker = static_cast<int>((wl - 100.0) / 700.0 * bar_w);
    marker = std::clamp(marker, 0, bar_w);
    DrawTriangle({static_cast<float>(bar_x + marker), static_cast<float>(bar_y - 2)},
                 {static_cast<float>(bar_x + marker - 4), static_cast<float>(bar_y - 8)},
                 {static_cast<float>(bar_x + marker + 4), static_cast<float>(bar_y - 8)}, WHITE);
}

// --------------------------------------------------------------------------
// View 0 — The Experiment (lab setup with photocell circuit)
// --------------------------------------------------------------------------

void PhotoelectricScenario::render_experiment_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    render_em_bar(vp_x, vp_y + 8, vp_w);

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    auto Ek = PhotoelectricEffect::compute_kinetic_energy(wavelength_nm(), metal.work_function_eV);
    Color photon_color = SpectralTransition::wavelength_to_rgb(wavelength_nm());

    int cx = vp_x + vp_w / 2;
    int cy = vp_y + vp_h / 2 + 10;

    int lamp_x = vp_x + 50;
    int lamp_cy = cy;
    DrawRectangle(lamp_x, lamp_cy - 35, 40, 70, Color{50, 55, 65, 255});
    DrawRectangleLines(lamp_x, lamp_cy - 35, 40, 70, Color{80, 85, 95, 255});
    draw_text("LAMP", static_cast<float>(lamp_x + 4), static_cast<float>(lamp_cy - 20), 10, ui_colors::TEXT_SECONDARY);

    float pulse = static_cast<float>(std::sin(anim_pulse_ * 4.0) * 0.3 + 0.7);
    Color glow = {photon_color.r, photon_color.g, photon_color.b,
                  static_cast<unsigned char>(pulse * 120)};
    DrawCircle(lamp_x + 20, lamp_cy, 25, glow);

    int metal_x = cx + 20;
    int metal_top = cy - 60;
    int metal_w = 16;
    int metal_h = 120;
    DrawRectangle(metal_x, metal_top, metal_w, metal_h, METAL_SURFACE);
    DrawRectangleLines(metal_x, metal_top, metal_w, metal_h, METAL_EDGE);
    draw_text("Cathode", static_cast<float>(metal_x - 15), static_cast<float>(metal_top - 16), 11, ui_colors::TEXT_SECONDARY);
    draw_text(metal.name.c_str(), static_cast<float>(metal_x - 10), static_cast<float>(metal_top + metal_h + 4), 11, ui_colors::ACCENT);

    int collector_x = metal_x + 120;
    DrawRectangle(collector_x, metal_top + 20, 10, metal_h - 40, Color{60, 70, 80, 255});
    DrawRectangleLines(collector_x, metal_top + 20, 10, metal_h - 40, Color{90, 100, 110, 255});
    draw_text("Anode", static_cast<float>(collector_x - 8), static_cast<float>(metal_top + 4), 11, ui_colors::TEXT_SECONDARY);

    int tube_left = metal_x - 15;
    int tube_right = collector_x + 25;
    int tube_top = metal_top - 10;
    int tube_bot = metal_top + metal_h + 10;
    DrawRectangleLinesEx({static_cast<float>(tube_left), static_cast<float>(tube_top),
                          static_cast<float>(tube_right - tube_left), static_cast<float>(tube_bot - tube_top)},
                         1.5f, Color{70, 80, 100, 120});
    draw_text("vacuum tube", static_cast<float>(tube_left + 10), static_cast<float>(tube_bot + 2), 9, LABEL_DIM);

    int ammeter_x = collector_x + 50;
    int ammeter_cy = metal_top + metal_h + 40;
    DrawCircle(ammeter_x, ammeter_cy, 22, AMMETER_BG);
    DrawCircleLines(ammeter_x, ammeter_cy, 22, ui_colors::PANEL_BORDER);
    draw_text("A", static_cast<float>(ammeter_x - 4), static_cast<float>(ammeter_cy - 6), 13, ui_colors::TEXT_PRIMARY);

    DrawLine(metal_x + metal_w / 2, metal_top + metal_h, metal_x + metal_w / 2, ammeter_cy + 22, WIRE_COLOR);
    DrawLine(metal_x + metal_w / 2, ammeter_cy + 22, ammeter_x - 22, ammeter_cy, WIRE_COLOR);
    DrawLine(ammeter_x + 22, ammeter_cy, collector_x + 5, ammeter_cy, WIRE_COLOR);
    DrawLine(collector_x + 5, ammeter_cy, collector_x + 5, metal_top + metal_h - 20, WIRE_COLOR);

    for (const auto& p : photons_) {
        float sx = static_cast<float>(lamp_x + 40) + (p.x + 0.8f) / 0.8f * static_cast<float>(metal_x - lamp_x - 40);
        float sy = static_cast<float>(cy) + p.y * 30.0f;

        DrawLine(static_cast<int>(sx) - 5, static_cast<int>(sy),
                 static_cast<int>(sx) + 5, static_cast<int>(sy), photon_color);
        float wave_y = 3.0f * std::sin(sx * 0.3f + static_cast<float>(anim_pulse_) * 10.0f);
        DrawLine(static_cast<int>(sx) - 5, static_cast<int>(sy + wave_y),
                 static_cast<int>(sx) + 5, static_cast<int>(sy), photon_color);
    }

    for (const auto& e : electrons_) {
        float sx = static_cast<float>(metal_x + metal_w) + e.x * 100.0f;
        float sy = static_cast<float>(cy) + e.y * 30.0f;
        auto alpha = static_cast<unsigned char>(std::clamp(e.life / 2.5f * 255.0f, 0.0f, 255.0f));
        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 3.5f, {ELECTRON_COLOR.r, ELECTRON_COLOR.g, ELECTRON_COLOR.b, alpha});
        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 1.5f, {255, 255, 255, alpha});
    }

    float current = Ek.has_value() ? static_cast<float>(intensity()) * 0.5f : 0.0f;
    char current_str[32];
    std::snprintf(current_str, sizeof(current_str), "%.1f \xc2\xb5\x41", current);
    draw_text(current_str, static_cast<float>(ammeter_x - 15), static_cast<float>(ammeter_cy + 28), 11,
              Ek.has_value() ? ui_colors::SUCCESS : ui_colors::DANGER);

    int info_x = vp_x + 14;
    int info_y = vp_y + vp_h - 70;

    if (Ek.has_value()) {
        draw_text("ELECTRONS EMITTED", static_cast<float>(info_x), static_cast<float>(info_y), 14, ui_colors::SUCCESS);
        char ek_str[64];
        std::snprintf(ek_str, sizeof(ek_str), "Ek = %.3f eV", Ek.value());
        draw_text(ek_str, static_cast<float>(info_x), static_cast<float>(info_y + 18), 13, ui_colors::TEXT_PRIMARY);
        draw_text("Increasing intensity \xe2\x86\x92 more electrons (higher current)",
                  static_cast<float>(info_x), static_cast<float>(info_y + 36), 11, ui_colors::TEXT_SECONDARY);
        draw_text("Increasing frequency \xe2\x86\x92 faster electrons (higher Ek)",
                  static_cast<float>(info_x), static_cast<float>(info_y + 50), 11, ui_colors::TEXT_SECONDARY);
    } else {
        draw_text("NO EMISSION", static_cast<float>(info_x), static_cast<float>(info_y), 14, ui_colors::DANGER);
        draw_text("Photon energy < work function", static_cast<float>(info_x), static_cast<float>(info_y + 18), 12, ui_colors::TEXT_SECONDARY);
        draw_text("No matter how intense the light, electrons won't escape!",
                  static_cast<float>(info_x), static_cast<float>(info_y + 36), 11, ui_colors::DANGER);
        draw_text("This contradicts classical wave theory \xe2\x80\x94 evidence for photons.",
                  static_cast<float>(info_x), static_cast<float>(info_y + 50), 11, ui_colors::TEXT_SECONDARY);
    }
}

// --------------------------------------------------------------------------
// View 1 — Energy Diagram (visual bar comparison)
// --------------------------------------------------------------------------

void PhotoelectricScenario::render_energy_diagram_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    double E_photon = photon_energy_eV();
    double phi = metal.work_function_eV;
    auto Ek = PhotoelectricEffect::compute_kinetic_energy(wavelength_nm(), phi);

    draw_text("Energy Comparison", static_cast<float>(vp_x + vp_w / 2 - 60), static_cast<float>(vp_y + 10), 16, ui_colors::ACCENT);

    int diagram_x = vp_x + 60;
    int diagram_w = vp_w - 120;
    int diagram_y = vp_y + 50;
    int diagram_h = vp_h - 120;

    double max_energy = std::max({E_photon, phi, 10.0});
    float eV_to_px = static_cast<float>(diagram_h) / static_cast<float>(max_energy);

    int bar_w = std::min(120, diagram_w / 5);
    int gap = std::min(60, diagram_w / 8);
    int total_bars_w = bar_w * 3 + gap * 2;
    int start_x = diagram_x + (diagram_w - total_bars_w) / 2;

    int base_y = diagram_y + diagram_h;

    DrawLine(diagram_x, base_y, diagram_x + diagram_w, base_y, ui_colors::TEXT_SECONDARY);
    DrawLine(diagram_x, diagram_y, diagram_x, base_y, ui_colors::TEXT_SECONDARY);

    for (int i = 0; i <= static_cast<int>(max_energy); ++i) {
        int gy = base_y - static_cast<int>(static_cast<float>(i) * eV_to_px);
        DrawLine(diagram_x - 3, gy, diagram_x + 3, gy, ui_colors::TEXT_SECONDARY);
        char lbl[16];
        std::snprintf(lbl, sizeof(lbl), "%d eV", i);
        draw_text(lbl, static_cast<float>(diagram_x - 40), static_cast<float>(gy - 5), 10, LABEL_DIM);
        if (gy > diagram_y) {
            DrawLine(diagram_x, gy, diagram_x + diagram_w, gy, Color{50, 50, 60, 80});
        }
    }

    int photon_bar_h = static_cast<int>(E_photon * static_cast<double>(eV_to_px));
    photon_bar_h = std::clamp(photon_bar_h, 2, diagram_h);
    int b1_x = start_x;
    DrawRectangle(b1_x, base_y - photon_bar_h, bar_w, photon_bar_h, BAR_PHOTON);
    DrawRectangleLinesEx({static_cast<float>(b1_x), static_cast<float>(base_y - photon_bar_h),
                          static_cast<float>(bar_w), static_cast<float>(photon_bar_h)}, 1.0f, Color{255, 220, 100, 200});

    draw_text("E photon", static_cast<float>(b1_x + bar_w / 2 - 25), static_cast<float>(base_y + 8), 12, BAR_PHOTON);
    draw_text("= hf", static_cast<float>(b1_x + bar_w / 2 - 12), static_cast<float>(base_y + 22), 11, LABEL_DIM);
    char e_str[32];
    std::snprintf(e_str, sizeof(e_str), "%.2f eV", E_photon);
    draw_text(e_str, static_cast<float>(b1_x + bar_w / 2 - 20), static_cast<float>(base_y - photon_bar_h - 16), 12, BAR_PHOTON);

    int work_bar_h = static_cast<int>(phi * static_cast<double>(eV_to_px));
    work_bar_h = std::clamp(work_bar_h, 2, diagram_h);
    int b2_x = start_x + bar_w + gap;
    DrawRectangle(b2_x, base_y - work_bar_h, bar_w, work_bar_h, BAR_WORK);
    DrawRectangleLinesEx({static_cast<float>(b2_x), static_cast<float>(base_y - work_bar_h),
                          static_cast<float>(bar_w), static_cast<float>(work_bar_h)}, 1.0f, Color{220, 90, 90, 200});

    draw_text("Work func.", static_cast<float>(b2_x + bar_w / 2 - 30), static_cast<float>(base_y + 8), 12, BAR_WORK);
    draw_text("\xcf\x86", static_cast<float>(b2_x + bar_w / 2 - 3), static_cast<float>(base_y + 22), 12, LABEL_DIM);
    char phi_str[32];
    std::snprintf(phi_str, sizeof(phi_str), "%.2f eV", phi);
    draw_text(phi_str, static_cast<float>(b2_x + bar_w / 2 - 20), static_cast<float>(base_y - work_bar_h - 16), 12, BAR_WORK);

    int b3_x = start_x + 2 * (bar_w + gap);
    if (Ek.has_value()) {
        int ek_bar_h = static_cast<int>(Ek.value() * static_cast<double>(eV_to_px));
        ek_bar_h = std::max(ek_bar_h, 4);
        DrawRectangle(b3_x, base_y - ek_bar_h, bar_w, ek_bar_h, BAR_KINETIC);
        DrawRectangleLinesEx({static_cast<float>(b3_x), static_cast<float>(base_y - ek_bar_h),
                              static_cast<float>(bar_w), static_cast<float>(ek_bar_h)}, 1.0f, Color{80, 220, 140, 200});

        char ek_str[32];
        std::snprintf(ek_str, sizeof(ek_str), "%.3f eV", Ek.value());
        draw_text(ek_str, static_cast<float>(b3_x + bar_w / 2 - 25), static_cast<float>(base_y - ek_bar_h - 16), 12, BAR_KINETIC);

        int arrow_y = base_y - photon_bar_h;
        int arrow_y2 = base_y - work_bar_h;
        if (arrow_y < arrow_y2) {
            int mid_x = (b1_x + bar_w + b2_x) / 2;
            DrawLine(b1_x + bar_w, arrow_y, mid_x, arrow_y, Color{255, 255, 255, 100});
            DrawLine(mid_x, arrow_y, mid_x, arrow_y2, Color{255, 255, 255, 100});

            int ek_region_h = arrow_y2 - arrow_y;
            if (ek_region_h > 5) {
                DrawRectangle(b1_x + 2, arrow_y, bar_w - 4, ek_region_h, Color{BAR_KINETIC.r, BAR_KINETIC.g, BAR_KINETIC.b, 60});
                draw_text("Ek", static_cast<float>(b1_x + bar_w / 2 - 6),
                          static_cast<float>(arrow_y + ek_region_h / 2 - 6), 11, BAR_KINETIC);
            }
            int phi_region_y = arrow_y2;
            DrawRectangle(b1_x + 2, phi_region_y, bar_w - 4, base_y - phi_region_y, Color{BAR_WORK.r, BAR_WORK.g, BAR_WORK.b, 40});
        }
    } else {
        int shortfall_h = static_cast<int>((phi - E_photon) * static_cast<double>(eV_to_px));
        shortfall_h = std::max(shortfall_h, 4);
        DrawRectangle(b3_x, base_y - shortfall_h, bar_w, shortfall_h, Color{100, 40, 40, 150});
        DrawRectangleLinesEx({static_cast<float>(b3_x), static_cast<float>(base_y - shortfall_h),
                              static_cast<float>(bar_w), static_cast<float>(shortfall_h)}, 1.0f, Color{180, 60, 60, 150});

        float pulse_t = static_cast<float>(std::sin(anim_pulse_ * 3.0) * 0.5 + 0.5);
        auto pulse_alpha = static_cast<unsigned char>(pulse_t * 200);
        draw_text("DEFICIT", static_cast<float>(b3_x + bar_w / 2 - 22), static_cast<float>(base_y - shortfall_h - 16), 12,
                  {200, 80, 80, pulse_alpha});

        char def_str[32];
        std::snprintf(def_str, sizeof(def_str), "%.3f eV short", phi - E_photon);
        draw_text(def_str, static_cast<float>(b3_x + bar_w / 2 - 30), static_cast<float>(base_y - shortfall_h - 32), 10,
                  ui_colors::TEXT_SECONDARY);
    }

    draw_text("Ek (max)", static_cast<float>(b3_x + bar_w / 2 - 22), static_cast<float>(base_y + 8), 12,
              Ek.has_value() ? BAR_KINETIC : ui_colors::DANGER);
    draw_text("= hf - \xcf\x86", static_cast<float>(b3_x + bar_w / 2 - 18), static_cast<float>(base_y + 22), 11, LABEL_DIM);

    draw_text("Ek = hf \xe2\x88\x92 \xcf\x86  (Einstein, 1905)",
              static_cast<float>(vp_x + vp_w / 2 - 100), static_cast<float>(vp_y + vp_h - 22), 13, ui_colors::ACCENT);
}

// --------------------------------------------------------------------------
// View 2 — Ek vs frequency graph
// --------------------------------------------------------------------------

void PhotoelectricScenario::render_graph_view(int vp_x, int vp_y, int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    render_em_bar(vp_x, vp_y + 8, vp_w);

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    double phi = metal.work_function_eV;

    int gx = vp_x + 80;
    int gw = vp_w - 160;
    int gy = vp_y + 60;
    int gh = vp_h - 110;

    DrawLine(gx, gy + gh, gx + gw, gy + gh, ui_colors::TEXT_SECONDARY);
    DrawLine(gx, gy, gx, gy + gh, ui_colors::TEXT_SECONDARY);
    draw_text("f (THz)", static_cast<float>(gx + gw / 2 - 20), static_cast<float>(gy + gh + 8), 12, ui_colors::TEXT_SECONDARY);
    draw_text("Ek (eV)", static_cast<float>(gx - 55), static_cast<float>(gy - 2), 12, ui_colors::TEXT_SECONDARY);

    double f_max_THz = 3000.0;
    double ek_max = 10.0;

    for (int i = 0; i <= 6; ++i) {
        double f_val = f_max_THz * i / 6.0;
        int tick_x = gx + gw * i / 6;
        DrawLine(tick_x, gy + gh, tick_x, gy + gh + 4, ui_colors::TEXT_SECONDARY);
        char lbl[16];
        std::snprintf(lbl, sizeof(lbl), "%.0f", f_val);
        draw_text(lbl, static_cast<float>(tick_x - 12), static_cast<float>(gy + gh + 8), 9, LABEL_DIM);
        if (i > 0) DrawLine(tick_x, gy, tick_x, gy + gh, Color{50, 50, 60, 80});
    }
    for (int i = 0; i <= 5; ++i) {
        double ek_val = ek_max * i / 5.0;
        int tick_y = gy + gh - gh * i / 5;
        DrawLine(gx - 4, tick_y, gx, tick_y, ui_colors::TEXT_SECONDARY);
        char lbl[16];
        std::snprintf(lbl, sizeof(lbl), "%.0f", ek_val);
        draw_text(lbl, static_cast<float>(gx - 30), static_cast<float>(tick_y - 5), 9, LABEL_DIM);
        if (i > 0) DrawLine(gx, tick_y, gx + gw, tick_y, Color{50, 50, 60, 80});
    }

    for (int mi = 0; mi < static_cast<int>(metals.size()); ++mi) {
        double m_phi = metals[mi].work_function_eV;
        double f_threshold_THz = m_phi / (4.136e-15) / 1e12;

        Color line_col = (mi == metal_idx_) ? ui_colors::ACCENT : Color{80, 80, 90, 120};
        float line_thick = (mi == metal_idx_) ? 2.5f : 1.0f;

        int prev_x_val = 0, prev_y_val = 0;
        bool started = false;
        for (int px = 0; px <= gw; ++px) {
            double f = f_max_THz * static_cast<double>(px) / gw;
            double ek = 4.136e-15 * f * 1e12 - m_phi;
            if (ek < 0) continue;

            int py_val = gy + gh - static_cast<int>((ek / ek_max) * gh);
            int px_val = gx + px;
            py_val = std::clamp(py_val, gy, gy + gh);

            if (started) {
                DrawLineEx({static_cast<float>(prev_x_val), static_cast<float>(prev_y_val)},
                           {static_cast<float>(px_val), static_cast<float>(py_val)}, line_thick, line_col);
            }
            prev_x_val = px_val;
            prev_y_val = py_val;
            started = true;
        }

        int thresh_x = gx + static_cast<int>((f_threshold_THz / f_max_THz) * gw);
        if (thresh_x > gx && thresh_x < gx + gw) {
            if (mi == metal_idx_) {
                DrawLine(thresh_x, gy, thresh_x, gy + gh, Color{180, 60, 60, 120});
                draw_text("f\xe2\x82\x80", static_cast<float>(thresh_x - 6), static_cast<float>(gy + gh + 18), 10, ui_colors::DANGER);
            }
        }

        if (mi == metal_idx_) {
            draw_text(metals[mi].name.c_str(),
                      static_cast<float>(gx + gw + 8),
                      static_cast<float>(std::max(gy, gy + gh - static_cast<int>((ek_max * 0.6 / ek_max) * gh))),
                      11, ui_colors::ACCENT);
        } else {
            float label_y = static_cast<float>(gy + gh - static_cast<int>(((4.136e-15 * f_max_THz * 1e12 - m_phi) / ek_max) * gh));
            label_y = std::clamp(label_y, static_cast<float>(gy), static_cast<float>(gy + gh - 10));
            draw_text(metals[mi].name.c_str(), static_cast<float>(gx + gw + 8), label_y, 9, LABEL_DIM);
        }
    }

    double current_f_THz = frequency_THz();
    double current_ek = std::max(0.0, photon_energy_eV() - phi);
    int dot_x = gx + static_cast<int>((current_f_THz / f_max_THz) * gw);
    int dot_y = gy + gh - static_cast<int>((current_ek / ek_max) * gh);
    dot_x = std::clamp(dot_x, gx, gx + gw);
    dot_y = std::clamp(dot_y, gy, gy + gh);

    DrawLine(dot_x, gy, dot_x, gy + gh, Color{255, 255, 255, 40});
    DrawLine(gx, dot_y, gx + gw, dot_y, Color{255, 255, 255, 40});
    DrawCircle(dot_x, dot_y, 6, WHITE);
    DrawCircleLines(dot_x, dot_y, 6, ui_colors::ACCENT);

    draw_text("Slope = h (Planck's constant)", static_cast<float>(gx + 10), static_cast<float>(gy + 4), 11, ui_colors::TEXT_SECONDARY);
    draw_text("All metals have the SAME slope \xe2\x80\x94 only the threshold differs",
              static_cast<float>(gx + 10), static_cast<float>(gy + 18), 11, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// Controls panel (left side)
// --------------------------------------------------------------------------

void PhotoelectricScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;
    int cw = w - 20;

    draw_section("LIGHT SOURCE", cx, cy);
    cy += 4;

    frequency_slider_.render(font_, has_font_, cx, cy, cw);
    cy += Slider::HEIGHT + 2;

    char wl_str[48];
    std::snprintf(wl_str, sizeof(wl_str), "\xCE\xBB = %.0f nm", wavelength_nm());
    draw_text(wl_str, static_cast<float>(cx + 4), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 16;

    intensity_slider_.render(font_, has_font_, cx, cy, cw);
    cy += Slider::HEIGHT + 8;

    draw_separator(cx, cy, cw);

    draw_section("METAL", cx, cy);
    cy += 4;

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    for (int i = 0; i < static_cast<int>(metals.size()); ++i) {
        Color bg = (i == metal_idx_) ? ui_colors::ACCENT : ui_colors::PANEL_BG;
        DrawRectangle(cx, cy, cw, 22, bg);
        DrawRectangleLinesEx({static_cast<float>(cx), static_cast<float>(cy),
                              static_cast<float>(cw), 22.0f}, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(metals[i].name.c_str(), static_cast<float>(cx + 6), static_cast<float>(cy + 4), 12,
                  (i == metal_idx_) ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("\xcf\x86 = %.2f eV", metals[i].work_function_eV),
                  static_cast<float>(cx + cw - 80), static_cast<float>(cy + 4), 11, ui_colors::TEXT_SECONDARY);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (mouse.x >= cx && mouse.x < cx + cw && mouse.y >= cy && mouse.y < cy + 22) {
                metal_idx_ = i;
            }
        }
        cy += 24;
    }

    cy += 8;
    draw_separator(cx, cy, cw);
    cy += 4;

    draw_text("[\xe2\x86\x91/\xe2\x86\x93] Cycle metals", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 14;
    draw_text("[1-3] Switch views", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 20;

    if (HelpPopup::render_help_button(font_, has_font_, cx + 10, cy + 4)) {
        help_popup_.show({"Photoelectric Effect", "Einstein's Explanation (1905)",
            "Light consists of photons, each with energy E = hf.\n"
            "A photon can eject an electron from a metal only if\n"
            "its energy exceeds the metal's work function \xcf\x86.\n\n"
            "The maximum kinetic energy of the ejected electron is:\n"
            "  Ek = hf \xe2\x88\x92 \xcf\x86\n\n"
            "Key predictions (confirmed experimentally):\n"
            "  \xe2\x80\xa2 Below threshold frequency f\xe2\x82\x80 = \xcf\x86/h, NO emission\n"
            "  \xe2\x80\xa2 Above f\xe2\x82\x80, Ek depends linearly on f\n"
            "  \xe2\x80\xa2 Intensity affects only electron COUNT, not Ek\n"
            "  \xe2\x80\xa2 The graph Ek vs f has slope h (Planck's constant)",
            "Ek = hf \xe2\x88\x92 \xcf\x86",
            "Energy in electronvolts (eV)"});
    }
    draw_text("Help", static_cast<float>(cx + 24), static_cast<float>(cy - 2), 12, ui_colors::TEXT_SECONDARY);
}

// --------------------------------------------------------------------------
// Properties panel (right side)
// --------------------------------------------------------------------------

void PhotoelectricScenario::render_properties(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int px = x + 12;
    int py = y + 8;

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    double E_ph = photon_energy_eV();
    auto Ek = PhotoelectricEffect::compute_kinetic_energy(wavelength_nm(), metal.work_function_eV);
    double freq_Hz = frequency_Hz();
    double threshold = PhotoelectricEffect::threshold_wavelength_nm(metal.work_function_eV);

    draw_section("PHOTON", px, py);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.0f THz", frequency_THz());
    draw_prop("f:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%.0f nm", wavelength_nm());
    draw_prop("\xCE\xBB:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%.3f eV", E_ph);
    draw_prop("E = hf:", buf, px, py, BAR_PHOTON);

    py += 4;
    draw_separator(px, py, w - 24);

    draw_section("METAL", px, py);

    draw_prop("Name:", metal.name.c_str(), px, py);

    std::snprintf(buf, sizeof(buf), "%.2f eV", metal.work_function_eV);
    draw_prop("\xcf\x86:", buf, px, py, BAR_WORK);

    std::snprintf(buf, sizeof(buf), "%.0f nm", threshold);
    draw_prop("\xCE\xBB threshold:", buf, px, py);

    double f_threshold_THz = metal.work_function_eV / (4.136e-15) / 1e12;
    std::snprintf(buf, sizeof(buf), "%.0f THz", f_threshold_THz);
    draw_prop("f\xe2\x82\x80:", buf, px, py);

    py += 4;
    draw_separator(px, py, w - 24);

    draw_section("RESULT", px, py);

    if (Ek.has_value()) {
        std::snprintf(buf, sizeof(buf), "%.4f eV", Ek.value());
        draw_prop("Ek:", buf, px, py, BAR_KINETIC);

        draw_text("Ek = hf \xe2\x88\x92 \xcf\x86", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
        py += 18;

        double v = std::sqrt(2.0 * Ek.value() * 1.602e-19 / 9.109e-31);
        std::snprintf(buf, sizeof(buf), "%.2e m/s", v);
        draw_prop("v (max):", buf, px, py);
    } else {
        draw_text("No emission", static_cast<float>(px), static_cast<float>(py), 13, ui_colors::DANGER);
        py += 18;
        draw_text("hf < \xcf\x86", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
        py += 16;
    }

    py += 8;
    draw_separator(px, py, w - 24);
    draw_section("KEY INSIGHT", px, py);
    draw_text("Intensity \xe2\x86\x92 # photons/sec", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("  \xe2\x86\x92 affects CURRENT only", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("Frequency \xe2\x86\x92 energy/photon", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("  \xe2\x86\x92 affects Ek of electrons", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);

    py += 20;
    draw_prop("Photons:", TextFormat("%d", photon_count_), px, py);
    draw_prop("Electrons:", TextFormat("%d", electron_count_), px, py);
}
