#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "PhotoelectricScenario.hpp"

PhotoelectricScenario::PhotoelectricScenario()
    : wavelength_slider_("Wavelength (nm)", 50.0f, 800.0f, 400.0f, "%.0f")
    , intensity_slider_("Intensity", 1.0f, 10.0f, 5.0f, "%.0f")
{}

void PhotoelectricScenario::on_enter() {
    current_view_ = 0;
    photons_.clear();
    electrons_.clear();
    photon_count_ = 0;
    electron_count_ = 0;
    spawn_timer_ = 0.0;
}

void PhotoelectricScenario::update(double dt) {
    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    auto Ek = PhotoelectricEffect::compute_kinetic_energy(wavelength_nm(), metal.work_function_eV);

    spawn_timer_ += dt;
    double spawn_interval = 0.3 / intensity();
    while (spawn_timer_ >= spawn_interval) {
        spawn_timer_ -= spawn_interval;
        Photon p;
        p.x = -0.8f;
        p.y = 0.1f * static_cast<float>(GetRandomValue(-10, 10));
        p.vx = 2.0f;
        p.vy = 0.0f;
        p.active = true;
        photons_.push_back(p);
        photon_count_++;
    }

    for (auto& p : photons_) {
        if (!p.active) continue;
        p.x += p.vx * static_cast<float>(dt);
        p.y += p.vy * static_cast<float>(dt);

        if (p.x >= 0.0f) {
            p.active = false;
            if (Ek.has_value()) {
                Electron e;
                e.x = 0.05f;
                e.y = p.y;
                float speed = static_cast<float>(std::sqrt(Ek.value())) * 1.5f;
                float angle = static_cast<float>(GetRandomValue(-60, 60)) * 3.14159f / 180.0f;
                e.vx = speed * std::cos(angle);
                e.vy = -speed * std::abs(std::sin(angle));
                e.life = 2.0f;
                electrons_.push_back(e);
                electron_count_++;
            }
        }
        if (p.x > 2.0f) p.active = false;
    }

    for (auto& e : electrons_) {
        e.x += e.vx * static_cast<float>(dt);
        e.y += e.vy * static_cast<float>(dt);
        e.vy += 0.5f * static_cast<float>(dt);
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

void PhotoelectricScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;

    render_em_bar(vp_x, vp_y, vp_w);
    render_setup(vp_x, vp_y + 45, vp_w, vp_h / 2 - 20);
    render_ek_graph(vp_x, vp_y + vp_h / 2 + 20, vp_w, vp_h / 2 - 40);

    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void PhotoelectricScenario::render_em_bar(int vp_x, int vp_y, int vp_w) {
    int bar_x = vp_x + 40;
    int bar_w = vp_w - 80;
    int bar_y = vp_y + 8;
    int bar_h = 18;

    for (int px = 0; px < bar_w; ++px) {
        double wl = 50.0 + 750.0 * static_cast<double>(px) / bar_w;
        Color c = SpectralTransition::wavelength_to_rgb(wl);
        DrawLine(bar_x + px, bar_y, bar_x + px, bar_y + bar_h, c);
    }

    draw_text("50nm", static_cast<float>(bar_x - 5), static_cast<float>(bar_y + bar_h + 2), 9, ui_colors::TEXT_SECONDARY);
    draw_text("800nm", static_cast<float>(bar_x + bar_w - 20), static_cast<float>(bar_y + bar_h + 2), 9, ui_colors::TEXT_SECONDARY);

    int marker = static_cast<int>((wavelength_nm() - 50.0) / 750.0 * bar_w);
    marker = std::clamp(marker, 0, bar_w);
    DrawTriangle({static_cast<float>(bar_x + marker), static_cast<float>(bar_y - 2)},
                 {static_cast<float>(bar_x + marker - 4), static_cast<float>(bar_y - 8)},
                 {static_cast<float>(bar_x + marker + 4), static_cast<float>(bar_y - 8)}, WHITE);
}

void PhotoelectricScenario::render_setup(int vp_x, int vp_y, int vp_w, int vp_h) {
    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    Color photon_color = SpectralTransition::wavelength_to_rgb(wavelength_nm());
    auto Ek = PhotoelectricEffect::compute_kinetic_energy(wavelength_nm(), metal.work_function_eV);

    int cx = vp_x + vp_w / 2;
    int cy = vp_y + vp_h / 2;

    // Metal surface
    DrawRectangle(cx - 10, cy - 50, 100, 100, Color{80, 80, 95, 255});
    draw_text(metal.name.c_str(), static_cast<float>(cx), static_cast<float>(cy - 42), 13, ui_colors::TEXT_PRIMARY);
    draw_text(TextFormat("phi = %.2f eV", metal.work_function_eV), static_cast<float>(cx), static_cast<float>(cy - 26), 11, ui_colors::TEXT_SECONDARY);

    // Emitter label
    draw_text("Photon source", static_cast<float>(vp_x + 30), static_cast<float>(cy - 60), 12, ui_colors::TEXT_SECONDARY);
    DrawRectangle(vp_x + 30, cy - 30, 30, 60, Color{60, 60, 70, 255});

    // Draw photons
    for (const auto& p : photons_) {
        float sx = static_cast<float>(cx - 10) + (p.x + 0.8f) / 0.8f * static_cast<float>(cx - vp_x - 70);
        float sy = static_cast<float>(cy) + p.y * 40.0f;
        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 3, photon_color);
    }

    // Draw electrons
    for (const auto& e : electrons_) {
        float sx = static_cast<float>(cx) + e.x * 150.0f;
        float sy = static_cast<float>(cy) + e.y * 40.0f;
        unsigned char alpha = static_cast<unsigned char>(std::clamp(e.life / 2.0f * 255.0f, 0.0f, 255.0f));
        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 4, Color{60, 200, 255, alpha});
    }

    if (!Ek.has_value()) {
        draw_text("NO EMISSION", static_cast<float>(cx + 100), static_cast<float>(cy - 10), 14, ui_colors::DANGER);
        draw_text("E_photon < work function", static_cast<float>(cx + 100), static_cast<float>(cy + 10), 11, ui_colors::TEXT_SECONDARY);
    } else {
        draw_text(TextFormat("Ek = %.3f eV", Ek.value()), static_cast<float>(cx + 100), static_cast<float>(cy - 10), 14, ui_colors::SUCCESS);
    }
}

void PhotoelectricScenario::render_ek_graph(int vp_x, int vp_y, int vp_w, int vp_h) {
    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    double phi = metal.work_function_eV;

    int gx = vp_x + 70;
    int gw = vp_w - 140;
    int gy = vp_y + 10;
    int gh = vp_h - 30;

    // Axes
    DrawLine(gx, gy + gh, gx + gw, gy + gh, ui_colors::TEXT_SECONDARY);
    DrawLine(gx, gy, gx, gy + gh, ui_colors::TEXT_SECONDARY);
    draw_text("f (x10^14 Hz)", static_cast<float>(gx + gw / 2 - 40), static_cast<float>(gy + gh + 4), 10, ui_colors::TEXT_SECONDARY);
    draw_text("Ek (eV)", static_cast<float>(gx - 50), static_cast<float>(gy - 2), 10, ui_colors::TEXT_SECONDARY);

    double f_max = 20.0;
    double ek_max = 8.0;
    double f_threshold = phi / 4.136e-15 / 1e14;

    // Ek = hf - phi line
    int prev_x = 0, prev_y = 0;
    for (int px = 0; px <= gw; ++px) {
        double f = f_max * static_cast<double>(px) / gw;
        double ek = 4.136e-15 * f * 1e14 - phi;
        if (ek < 0) ek = 0;
        int py_val = gy + gh - static_cast<int>((ek / ek_max) * gh);
        int px_val = gx + px;
        if (px > 0 && ek > 0) {
            DrawLine(prev_x, prev_y, px_val, py_val, ui_colors::ACCENT);
        }
        prev_x = px_val;
        prev_y = py_val;
    }

    // Threshold line
    int thresh_x = gx + static_cast<int>((f_threshold / f_max) * gw);
    if (thresh_x > gx && thresh_x < gx + gw) {
        DrawLine(thresh_x, gy, thresh_x, gy + gh, Color{180, 60, 60, 120});
        draw_text("f0", static_cast<float>(thresh_x - 6), static_cast<float>(gy + gh + 4), 10, ui_colors::DANGER);
    }

    // Current point
    double current_f = PhotoelectricEffect::frequency_hz(wavelength_nm()) / 1e14;
    double current_ek = std::max(0.0, PhotoelectricEffect::photon_energy_eV(wavelength_nm()) - phi);
    int dot_x = gx + static_cast<int>((current_f / f_max) * gw);
    int dot_y = gy + gh - static_cast<int>((current_ek / ek_max) * gh);
    dot_x = std::clamp(dot_x, gx, gx + gw);
    dot_y = std::clamp(dot_y, gy, gy + gh);
    DrawCircle(dot_x, dot_y, 5, WHITE);
    DrawCircleLines(dot_x, dot_y, 5, ui_colors::ACCENT);

    draw_text("Ek = hf - phi (linear relationship)", static_cast<float>(gx + 10), static_cast<float>(gy + 4), 11, ui_colors::TEXT_SECONDARY);
}

bool PhotoelectricScenario::render_button(const char* label, int x, int y, int w) {
    int h = 28;
    Vector2 mouse = GetMousePosition();
    bool hovered = mouse.x >= x && mouse.x < x + w && mouse.y >= y && mouse.y < y + h;

    DrawRectangle(x, y, w, h, hovered ? ui_colors::PANEL_HOVER : ui_colors::PANEL_BG);
    DrawRectangleLinesEx({(float)x, (float)y, (float)w, (float)h}, 1.0f, ui_colors::PANEL_BORDER);

    float tw = measure_text(label, 13);
    draw_text(label, static_cast<float>(x) + (w - tw) / 2, static_cast<float>(y + 7), 13, ui_colors::TEXT_PRIMARY);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void PhotoelectricScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("PHOTOELECTRIC", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    wavelength_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 4;

    intensity_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 8;

    draw_text("Metal:", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 16;

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    for (int i = 0; i < static_cast<int>(metals.size()); ++i) {
        Color bg = (i == metal_idx_) ? ui_colors::ACCENT : ui_colors::PANEL_BG;
        DrawRectangle(cx, cy, w - 20, 22, bg);
        DrawRectangleLinesEx({(float)cx, (float)cy, (float)(w - 20), 22.0f}, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(metals[i].name.c_str(), static_cast<float>(cx + 6), static_cast<float>(cy + 4), 12,
                  (i == metal_idx_) ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("%.2f eV", metals[i].work_function_eV),
                  static_cast<float>(cx + w - 75), static_cast<float>(cy + 4), 11, ui_colors::TEXT_SECONDARY);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (mouse.x >= cx && mouse.x < cx + w - 20 && mouse.y >= cy && mouse.y < cy + 22) {
                metal_idx_ = i;
            }
        }
        cy += 24;
    }

    cy += 8;
    draw_text("Up/Down: cycle metals", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
}

void PhotoelectricScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    const auto& metals = PhotoelectricEffect::get_metal_presets();
    const auto& metal = metals[metal_idx_];
    double E_ph = PhotoelectricEffect::photon_energy_eV(wavelength_nm());
    auto Ek = PhotoelectricEffect::compute_kinetic_energy(wavelength_nm(), metal.work_function_eV);
    double freq = PhotoelectricEffect::frequency_hz(wavelength_nm());
    double threshold = PhotoelectricEffect::threshold_wavelength_nm(metal.work_function_eV);

    draw_section("Photon", px, py);
    draw_prop("\xCE\xBB", TextFormat("%.0f nm", wavelength_nm()), px, py);
    draw_prop("E", TextFormat("%.3f eV", E_ph), px, py);
    draw_prop("f", TextFormat("%.2e Hz", freq), px, py);

    py += 4;
    draw_section("Metal", px, py);
    draw_prop("Name", metal.name.c_str(), px, py);
    draw_prop("\xCF\x86", TextFormat("%.2f eV", metal.work_function_eV), px, py);
    draw_prop("\xCE\xBB_thresh", TextFormat("%.0f nm", threshold), px, py);

    py += 4;
    draw_section("Result", px, py);
    if (Ek.has_value()) {
        draw_prop("Ek", TextFormat("%.4f eV", Ek.value()), px, py, ui_colors::SUCCESS);
        draw_text("Ek = hf - phi", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
        py += 16;
    } else {
        draw_text("No emission", static_cast<float>(px), static_cast<float>(py), 13, ui_colors::DANGER);
        py += 18;
        draw_text("hf < phi", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
        py += 16;
    }

    py += 8;
    draw_separator(px, py, w - 24);
    draw_section("Note", px, py);
    draw_text("Intensity affects emission", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("RATE but NOT kinetic", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("energy of electrons.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);

    py += 20;
    draw_prop("Photons", TextFormat("%d", photon_count_), px, py);
    draw_prop("Electrons", TextFormat("%d", electron_count_), px, py);
}
