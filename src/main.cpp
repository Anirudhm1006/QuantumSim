#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <raylib.h>

#include "IScenario.hpp"
#include "scenarios/DoubleSlitScenario.hpp"
#include "scenarios/TunnelingScenario.hpp"
#include "scenarios/InfiniteWellScenario.hpp"
#include "scenarios/AtomViewerScenario.hpp"
#include "scenarios/FreeParticleScenario.hpp"
#include "scenarios/PhotoelectricScenario.hpp"
#include "scenarios/SpectrumScenario.hpp"
#include "scenarios/DeBroglieScenario.hpp"
#include "scenarios/HeisenbergScenario.hpp"
#include "MenuBar.hpp"

static void update_orbit_camera(Camera3D& cam, bool in_viewport) {
    if (!in_viewport) return;

    float dx = cam.position.x - cam.target.x;
    float dy = cam.position.y - cam.target.y;
    float dz = cam.position.z - cam.target.z;
    float radius = std::sqrt(dx * dx + dy * dy + dz * dz);
    float azimuth = std::atan2(dz, dx);
    float elevation = std::asin(std::clamp(dy / radius, -1.0f, 1.0f));

    constexpr float ORBIT_SPEED = 0.003f;
    constexpr float PAN_SPEED = 0.01f;
    constexpr float ZOOM_SPEED = 1.5f;
    constexpr float MIN_RADIUS = 1.0f;
    constexpr float MAX_RADIUS = 60.0f;
    constexpr float MAX_ELEVATION = 1.5f;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 delta = GetMouseDelta();
        azimuth -= delta.x * ORBIT_SPEED;
        elevation += delta.y * ORBIT_SPEED;
        elevation = std::clamp(elevation, -MAX_ELEVATION, MAX_ELEVATION);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        Vector3 forward = {cam.target.x - cam.position.x, 0, cam.target.z - cam.position.z};
        float flen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
        if (flen > 1e-6f) {
            forward.x /= flen;
            forward.z /= flen;
        }
        Vector3 right = {-forward.z, 0, forward.x};

        float pan_x = -delta.x * PAN_SPEED * radius * 0.05f;
        float pan_y = delta.y * PAN_SPEED * radius * 0.05f;

        cam.target.x += right.x * pan_x;
        cam.target.z += right.z * pan_x;
        cam.target.y += pan_y;
    }

    float wheel = GetMouseWheelMove();
    if (std::abs(wheel) > 0.0f) {
        radius -= wheel * ZOOM_SPEED;
        radius = std::clamp(radius, MIN_RADIUS, MAX_RADIUS);
    }

    cam.position.x = cam.target.x + radius * std::cos(elevation) * std::cos(azimuth);
    cam.position.y = cam.target.y + radius * std::sin(elevation);
    cam.position.z = cam.target.z + radius * std::cos(elevation) * std::sin(azimuth);
}

int main() {
    const int screen_width = 1280;
    const int screen_height = 800;

    InitWindow(screen_width, screen_height, "QuantumSim - Fysikk 2 Quantum Physics Simulator");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    int codepoints[512];
    int cp_count = 0;
    for (int i = 32; i < 127; ++i) codepoints[cp_count++] = i;
    for (int i = 0x0370; i <= 0x03FF; ++i) codepoints[cp_count++] = i;
    codepoints[cp_count++] = 0x210F; // hbar
    codepoints[cp_count++] = 0x00B7; // middle dot
    codepoints[cp_count++] = 0x2265; // >=
    codepoints[cp_count++] = 0x00B2; // superscript 2
    Font app_font = LoadFontEx("resources/fonts/inter.ttf", 32, codepoints, cp_count);
    SetTextureFilter(app_font.texture, TEXTURE_FILTER_BILINEAR);

    Camera3D camera = {};
    camera.position = {0.0f, 8.0f, 12.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    MenuBar menu_bar;
    menu_bar.set_font(app_font);

    constexpr int CONTROLS_W = 200;
    constexpr int PROPERTIES_W = 230;

    std::vector<std::unique_ptr<IScenario>> scenarios;
    scenarios.push_back(std::make_unique<DoubleSlitScenario>());
    scenarios.push_back(std::make_unique<TunnelingScenario>());
    scenarios.push_back(std::make_unique<InfiniteWellScenario>());
    scenarios.push_back(std::make_unique<AtomViewerScenario>());
    scenarios.push_back(std::make_unique<FreeParticleScenario>());
    scenarios.push_back(std::make_unique<PhotoelectricScenario>());
    scenarios.push_back(std::make_unique<SpectrumScenario>());
    scenarios.push_back(std::make_unique<DeBroglieScenario>());
    scenarios.push_back(std::make_unique<HeisenbergScenario>());

    for (auto& s : scenarios) s->set_font(app_font);

    int active_idx = 0;
    scenarios[active_idx]->on_enter();

    auto switch_scenario = [&](int idx) {
        if (idx >= 0 && idx < static_cast<int>(scenarios.size()) && idx != active_idx) {
            active_idx = idx;
            camera.position = {0.0f, 8.0f, 12.0f};
            camera.target = {0.0f, 0.0f, 0.0f};
            scenarios[active_idx]->on_enter();
        }
    };

    while (!WindowShouldClose()) {
        MenuAction action = menu_bar.update();
        switch (action.type) {
            case MenuAction::Type::SCENARIO_DOUBLE_SLIT:   switch_scenario(0); break;
            case MenuAction::Type::SCENARIO_TUNNELING:     switch_scenario(1); break;
            case MenuAction::Type::SCENARIO_INFINITE_WELL: switch_scenario(2); break;
            case MenuAction::Type::SCENARIO_HYDROGEN:      switch_scenario(3); break;
            case MenuAction::Type::SCENARIO_FREE_PARTICLE: switch_scenario(4); break;
            case MenuAction::Type::SCENARIO_PHOTOELECTRIC: switch_scenario(5); break;
            case MenuAction::Type::SCENARIO_SPECTRUM:      switch_scenario(6); break;
            case MenuAction::Type::SCENARIO_DE_BROGLIE:    switch_scenario(7); break;
            case MenuAction::Type::SCENARIO_HEISENBERG:    switch_scenario(8); break;
            case MenuAction::Type::NONE: break;
        }

        IScenario* sc = scenarios[active_idx].get();

        for (int key = KEY_ONE; key <= KEY_NINE; ++key) {
            if (IsKeyPressed(key)) {
                int view = key - KEY_ONE;
                if (view < sc->get_view_count()) {
                    sc->set_view(view);
                }
            }
        }

        sc->handle_input();

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        int vp_x = CONTROLS_W;
        int vp_y = menu_bar.get_height();
        int vp_w = sw - CONTROLS_W - PROPERTIES_W;
        int vp_h = sh - vp_y - 26;

        Vector2 mouse = GetMousePosition();
        int mx = static_cast<int>(mouse.x);
        int my = static_cast<int>(mouse.y);
        bool in_viewport = mx > vp_x && mx < vp_x + vp_w && my > vp_y && my < vp_y + vp_h;

        if (sc->uses_3d()) {
            update_orbit_camera(camera, in_viewport);
        }

        sc->update(GetFrameTime());

        BeginDrawing();
        {
            ClearBackground(ui_colors::BG_DARK);

            sc->render_viewport(camera, vp_x, vp_y, vp_w, vp_h);
            sc->render_controls(0, vp_y, CONTROLS_W, vp_h);
            sc->render_properties(sw - PROPERTIES_W, vp_y, PROPERTIES_W, vp_h);

            menu_bar.render_bar();

            // View tabs below menu bar
            int tab_x = CONTROLS_W;
            int vc = sc->get_view_count();
            if (vc > 1) {
                for (int v = 0; v < vc; ++v) {
                    int tw = 120;
                    int tx = tab_x + v * (tw + 2);
                    bool active = (v == sc->get_current_view());
                    Color bg = active ? ui_colors::ACCENT : ui_colors::PANEL_BG;
                    DrawRectangle(tx, vp_y, tw, 22, bg);
                    DrawRectangleLinesEx({(float)tx, (float)vp_y, (float)tw, 22.0f}, 1.0f, ui_colors::PANEL_BORDER);

                    const char* vname = sc->get_view_name(v);
                    DrawTextEx(app_font, vname, {static_cast<float>(tx + 6), static_cast<float>(vp_y + 4)},
                               12, 1, active ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        if (mx >= tx && mx < tx + tw && my >= vp_y && my < vp_y + 22) {
                            sc->set_view(v);
                        }
                    }
                }
            }

            // Status bar
            int status_y = sh - 26;
            DrawRectangle(0, status_y, sw, 26, Color{30, 30, 38, 255});
            DrawTextEx(app_font, sc->get_name(), {8.0f, static_cast<float>(status_y + 6)}, 13, 1, ui_colors::ACCENT);

            const char* view_label = sc->get_view_name(sc->get_current_view());
            DrawTextEx(app_font, view_label, {200.0f, static_cast<float>(status_y + 6)}, 13, 1, ui_colors::TEXT_SECONDARY);

            DrawTextEx(app_font, TextFormat("%d FPS", GetFPS()),
                       {static_cast<float>(sw - 60), static_cast<float>(status_y + 6)}, 13, 1, ui_colors::TEXT_SECONDARY);

            menu_bar.render_dropdowns();
        }
        EndDrawing();
    }

    UnloadFont(app_font);
    CloseWindow();
    return 0;
}
