#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>
#include <complex>

#include <raylib.h>
#include <rlgl.h>

#include "AtomViewerScenario.hpp"

namespace {
    constexpr Color NUCLEUS_COLOR     = {220, 80, 60, 255};
    constexpr Color ELECTRON_COLOR    = {100, 180, 255, 255};
    constexpr Color ORBIT_LINE_COLOR  = {60, 70, 90, 180};
    constexpr Color SHELL_LABEL_COLOR = {120, 130, 150, 255};
    constexpr Color ENERGY_LEVEL_COL  = {100, 140, 180, 255};
    constexpr Color CLOUD_CORE_COL    = {80, 160, 255, 255};
    constexpr Color CLOUD_OUTER_COL   = {40, 80, 160, 100};
    constexpr Color MEASURE_SPHERE_COL = {255, 220, 60, 40};
    constexpr Color MEASURE_RING_COL  = {255, 220, 60, 180};
    constexpr Color LOBE_POSITIVE     = {60, 140, 255, 160};
    constexpr Color LOBE_NEGATIVE     = {255, 110, 50, 160};

    constexpr float BOHR_ORBIT_BASE_RADIUS = 40.0f;
    constexpr float NUCLEUS_RADIUS         = 12.0f;
    constexpr float ELECTRON_RADIUS        = 5.0f;

    constexpr double RYDBERG_EV = 13.605693009;

    constexpr float PI_F = 3.14159265358979323846f;
}

// ---------------------------------------------------------------------------
// Construction / lifecycle
// ---------------------------------------------------------------------------

AtomViewerScenario::AtomViewerScenario()
    : n_slider_("n (Principal)", 1.0f, 6.0f, 1.0f, "%.0f")
    , l_slider_("l (Orbital)", 0.0f, 0.0f, 0.0f, "%.0f")
    , m_slider_("m (Magnetic)", 0.0f, 0.0f, 0.0f, "%.0f")
    , sample_count_slider_("Samples", 500.0f, 5000.0f, 2000.0f, "%.0f")
    , measure_radius_slider_("Measure r (a\xe2\x82\x80)", 0.5f, 30.0f, 3.0f, "%.1f")
    , iso_threshold_slider_("Iso Threshold", 0.01f, 0.50f, 0.10f, "%.2f")
{}

void AtomViewerScenario::on_enter() {
    element_index_ = 0;
    n_slider_.set_value(1.0f);
    l_slider_.set_range(0.0f, 0.0f);
    l_slider_.set_value(0.0f);
    m_slider_.set_range(0.0f, 0.0f);
    m_slider_.set_value(0.0f);
    sample_count_slider_.set_value(2000.0f);
    sample_count_ = 2000;
    orbit_angle_ = 0.0f;
    zoom_delta_ = 0.0f;
    regenerate_model();
    regenerate_samples();
    regenerate_orbital_shape();
}

const char* AtomViewerScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Bohr Model";
        case 1: return "Orbital Cloud";
        case 2: return "Dot Density";
        case 3: return "Orbital Shape";
        default: return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// Element / quantum number management
// ---------------------------------------------------------------------------

void AtomViewerScenario::select_element(int index) {
    if (index < 0 || index >= ElementData::element_count()) return;
    element_index_ = index;
    validate_quantum_numbers();
    regenerate_model();
    regenerate_samples();
    regenerate_orbital_shape();
}

int AtomViewerScenario::current_Z() const {
    return ElementData::get_element_by_index(element_index_).atomic_number;
}

double AtomViewerScenario::energy_eV() const {
    int n = current_n();
    int Z = current_Z();
    if (n <= 0) return 0.0;
    return -RYDBERG_EV * static_cast<double>(Z * Z) / static_cast<double>(n * n);
}

std::string AtomViewerScenario::orbital_name_from_nl(int n, int l) const {
    static const char subshell_letters[] = "spdfghijk";
    char buf[16];
    char letter = (l >= 0 && l < static_cast<int>(sizeof(subshell_letters) - 1))
                      ? subshell_letters[l] : '?';
    std::snprintf(buf, sizeof(buf), "%d%c", n, letter);
    return std::string(buf);
}

std::string AtomViewerScenario::orbital_description() const {
    int n = current_n();
    int l = current_l();

    int radial_nodes = n - l - 1;
    int angular_nodes = l;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%d radial + %d angular = %d total nodes",
                  radial_nodes, angular_nodes, radial_nodes + angular_nodes);
    return std::string(buf);
}

void AtomViewerScenario::validate_quantum_numbers() {
    int n = current_n();
    int max_l = n - 1;
    l_slider_.set_range(0.0f, static_cast<float>(max_l));

    int l = current_l();
    if (l > max_l) {
        l_slider_.set_value(static_cast<float>(max_l));
        l = max_l;
    }

    m_slider_.set_range(static_cast<float>(-l), static_cast<float>(l));
    int m = current_m();
    if (m < -l) m_slider_.set_value(static_cast<float>(-l));
    if (m > l)  m_slider_.set_value(static_cast<float>(l));
}

void AtomViewerScenario::regenerate_model() {
    int n = current_n();
    int l = current_l();
    int m = current_m();
    int Z = current_Z();
    model_ = HydrogenModel(n, l, m, Z);
}

void AtomViewerScenario::regenerate_samples() {
    samples_.clear();
    samples_.reserve(static_cast<size_t>(sample_count_));
    for (int i = 0; i < sample_count_; ++i) {
        auto pos = model_.sample_position();
        if (pos.size() >= 3) {
            samples_.push_back({static_cast<float>(pos[0]),
                                static_cast<float>(pos[1]),
                                static_cast<float>(pos[2])});
        }
    }
}

void AtomViewerScenario::regenerate_orbital_shape() {
    orbital_voxels_.clear();

    int n = current_n();
    int Z = current_Z();
    float extent = 2.5f * static_cast<float>(n * n) / std::max(1.0f, static_cast<float>(Z));
    extent = std::max(extent, 4.0f);
    orbital_extent_ = extent;

    constexpr int RES = ORBITAL_GRID_RES;
    float step = 2.0f * extent / static_cast<float>(RES - 1);
    orbital_voxel_size_ = step * 0.85f;

    struct GridPt { float x, y, z; double psi_re; double prob; };
    std::vector<GridPt> grid;
    grid.reserve(RES * RES * RES);

    double max_prob = 0.0;

    for (int ix = 0; ix < RES; ++ix) {
        float x = -extent + step * static_cast<float>(ix);
        for (int iy = 0; iy < RES; ++iy) {
            float y = -extent + step * static_cast<float>(iy);
            for (int iz = 0; iz < RES; ++iz) {
                float z = -extent + step * static_cast<float>(iz);

                double r = std::sqrt(static_cast<double>(x * x + y * y + z * z));
                if (r < 1e-8) r = 1e-8;
                double theta = std::acos(std::clamp(static_cast<double>(z) / r, -1.0, 1.0));
                double phi = std::atan2(static_cast<double>(y), static_cast<double>(x));

                auto psi = model_.wavefunction(r, theta, phi);
                double prob = std::norm(psi);

                if (prob > max_prob) max_prob = prob;
                grid.push_back({x, y, z, psi.real(), prob});
            }
        }
    }

    double threshold_frac = static_cast<double>(iso_threshold_slider_.get_value());
    double threshold = threshold_frac * max_prob;

    for (const auto& gp : grid) {
        if (gp.prob >= threshold) {
            float sign = (gp.psi_re >= 0.0) ? 1.0f : -1.0f;
            orbital_voxels_.push_back({{gp.x, gp.y, gp.z}, sign});
        }
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void AtomViewerScenario::update(double dt) {
    orbit_angle_ += static_cast<float>(dt) * 0.8f;
    if (orbit_angle_ > 2.0f * PI_F) orbit_angle_ -= 2.0f * PI_F;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void AtomViewerScenario::handle_input() {
    periodic_table_.handle_input();
    if (periodic_table_.is_open()) return;

    help_popup_.handle_input();
    if (help_popup_.is_open()) return;

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        zoom_delta_ = -2.0f;
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        zoom_delta_ = 2.0f;
    }
}

// ---------------------------------------------------------------------------
// Button helper
// ---------------------------------------------------------------------------

bool AtomViewerScenario::render_button(const char* label, int x, int y, int w,
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

// ---------------------------------------------------------------------------
// 3D helpers
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_3d_axes(float axis_len) {
    DrawLine3D({0, 0, 0}, {axis_len, 0, 0}, ui_colors::AXIS_X);
    DrawLine3D({0, 0, 0}, {0, axis_len, 0}, ui_colors::AXIS_Y);
    DrawLine3D({0, 0, 0}, {0, 0, axis_len}, ui_colors::AXIS_Z);

    DrawSphere({axis_len, 0, 0}, 0.08f, ui_colors::AXIS_X);
    DrawSphere({0, axis_len, 0}, 0.08f, ui_colors::AXIS_Y);
    DrawSphere({0, 0, axis_len}, 0.08f, ui_colors::AXIS_Z);
}

void AtomViewerScenario::render_zoom_buttons(int vp_x, int vp_y, int vp_w, int vp_h) {
    int bx = vp_x + vp_w - 42;
    int by = vp_y + vp_h - 70;
    int bw = 32;
    int bh = 28;

    auto draw_zoom_btn = [&](const char* label, int btn_y) -> bool {
        Vector2 mouse = GetMousePosition();
        bool hov = mouse.x >= bx && mouse.x < bx + bw && mouse.y >= btn_y && mouse.y < btn_y + bh;

        Color bg = hov ? Color{70, 70, 80, 220} : Color{45, 45, 55, 200};
        DrawRectangleRounded({static_cast<float>(bx), static_cast<float>(btn_y),
                              static_cast<float>(bw), static_cast<float>(bh)}, 0.3f, 4, bg);
        DrawRectangleRoundedLines({static_cast<float>(bx), static_cast<float>(btn_y),
                                   static_cast<float>(bw), static_cast<float>(bh)},
                                  0.3f, 4, 1.0f, ui_colors::PANEL_BORDER);

        float tw = measure_text(label, 16);
        draw_text(label, static_cast<float>(bx) + (static_cast<float>(bw) - tw) * 0.5f,
                  static_cast<float>(btn_y) + 5.0f, 16, ui_colors::TEXT_PRIMARY);

        return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    };

    if (draw_zoom_btn("+", by)) {
        zoom_delta_ = -2.0f;
    }
    if (draw_zoom_btn("-", by + bh + 4)) {
        zoom_delta_ = 2.0f;
    }
}

// ---------------------------------------------------------------------------
// Viewport routing
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y,
                                         int vp_w, int vp_h) {
    if (std::abs(zoom_delta_) > 0.01f) {
        float dx = cam.position.x - cam.target.x;
        float dy = cam.position.y - cam.target.y;
        float dz = cam.position.z - cam.target.z;
        float radius = std::sqrt(dx * dx + dy * dy + dz * dz);
        float new_radius = std::clamp(radius + zoom_delta_, 1.0f, 80.0f);
        if (radius > 1e-6f) {
            float scale = new_radius / radius;
            cam.position.x = cam.target.x + dx * scale;
            cam.position.y = cam.target.y + dy * scale;
            cam.position.z = cam.target.z + dz * scale;
        }
        zoom_delta_ = 0.0f;
    }

    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    switch (current_view_) {
        case 0: render_bohr_model(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_orbital_cloud(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_dot_density(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 3: render_orbital_shape(cam, vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }

    periodic_table_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

// ---------------------------------------------------------------------------
// View 0: Bohr Model (2D)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_bohr_model(int vp_x, int vp_y, int vp_w, int vp_h) {
    const auto& el = ElementData::get_element_by_index(element_index_);
    int Z = el.atomic_number;
    auto shells = ElementData::get_electron_shell_config(Z);
    int num_shells = static_cast<int>(shells.size());

    float cx = static_cast<float>(vp_x) + static_cast<float>(vp_w) * 0.45f;
    float cy = static_cast<float>(vp_y) + static_cast<float>(vp_h) * 0.5f;

    float max_available = std::min(static_cast<float>(vp_w) * 0.4f,
                                   static_cast<float>(vp_h) * 0.42f);
    float orbit_spacing = (num_shells > 0) ? max_available / static_cast<float>(num_shells) : BOHR_ORBIT_BASE_RADIUS;
    orbit_spacing = std::max(orbit_spacing, 25.0f);

    char header[64];
    std::snprintf(header, sizeof(header), "%s (Z=%d)", el.name.c_str(), Z);
    float header_w = measure_text(header, 18);
    draw_text(header, cx - header_w * 0.5f,
              static_cast<float>(vp_y) + 12.0f, 18, ui_colors::ACCENT);

    DrawCircleV({cx, cy}, NUCLEUS_RADIUS, NUCLEUS_COLOR);
    char nuc_label[16];
    std::snprintf(nuc_label, sizeof(nuc_label), "%d p", Z);
    float nuc_tw = measure_text(nuc_label, 10);
    draw_text(nuc_label, cx - nuc_tw * 0.5f, cy - 5.0f, 10, ui_colors::TEXT_PRIMARY);

    int highlight_shell = current_n() - 1;

    for (int s = 0; s < num_shells; ++s) {
        float r = orbit_spacing * static_cast<float>(s + 1);
        bool is_active = (s == highlight_shell);

        Color ring_col = is_active ? ui_colors::ACCENT : ORBIT_LINE_COLOR;
        DrawCircleLinesV({cx, cy}, r, ring_col);
        if (is_active) {
            DrawCircleLinesV({cx, cy}, r + 1.0f, ring_col);
        }

        const char* shell_name = (s < 7) ? SHELL_LABELS[s] : "?";
        char shell_lbl[32];
        std::snprintf(shell_lbl, sizeof(shell_lbl), "%s (n=%d, %de\xe2\x81\xbb)",
                      shell_name, s + 1, shells[s]);
        draw_text(shell_lbl, cx + r + 6.0f, cy - 8.0f,
                  11, is_active ? ui_colors::ACCENT : SHELL_LABEL_COLOR);

        int electron_count = shells[s];
        for (int e = 0; e < electron_count; ++e) {
            float angle_offset = static_cast<float>(e) * 2.0f * PI_F / static_cast<float>(electron_count);
            float speed_factor = 1.0f / std::sqrt(static_cast<float>(s + 1));
            float angle = orbit_angle_ * speed_factor + angle_offset;
            float ex = cx + r * std::cos(angle);
            float ey = cy + r * std::sin(angle);
            DrawCircleV({ex, ey}, ELECTRON_RADIUS, ELECTRON_COLOR);
        }
    }

    float level_x = static_cast<float>(vp_x + vp_w) - 140.0f;
    float level_y_start = static_cast<float>(vp_y) + 50.0f;
    float level_y_end = static_cast<float>(vp_y + vp_h) - 30.0f;
    float level_height = level_y_end - level_y_start;

    draw_text("Energy Levels", level_x, level_y_start - 18.0f, 12, ui_colors::ACCENT);

    int max_n = std::max(num_shells, 4);
    for (int n = 1; n <= max_n; ++n) {
        double e_n = -RYDBERG_EV * static_cast<double>(Z * Z) / static_cast<double>(n * n);
        float frac = 1.0f - static_cast<float>((e_n - (-RYDBERG_EV * Z * Z)) /
                     (0.0 - (-RYDBERG_EV * Z * Z)));
        float ly = level_y_start + frac * level_height;

        bool active_level = (n == current_n());
        Color lc = active_level ? ui_colors::ACCENT : ENERGY_LEVEL_COL;
        float thick = active_level ? 2.5f : 1.5f;
        DrawLineEx({level_x, ly}, {level_x + 80.0f, ly}, thick, lc);

        char elbl[48];
        std::snprintf(elbl, sizeof(elbl), "n=%d  %.2f eV", n, e_n);
        draw_text(elbl, level_x + 4.0f, ly - 14.0f, 10,
                  active_level ? ui_colors::ACCENT : ui_colors::TEXT_SECONDARY);
    }
}

// ---------------------------------------------------------------------------
// View 1: Orbital Cloud (3D)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_orbital_cloud(Camera3D& cam, int vp_x, int vp_y,
                                              int vp_w, int vp_h) {
    BeginScissorMode(vp_x, vp_y, vp_w, vp_h);
    BeginMode3D(cam);

    float axis_len = orbital_extent_ * 0.7f;
    render_3d_axes(axis_len);
    DrawSphere({0, 0, 0}, 0.15f, NUCLEUS_COLOR);

    float max_dist = 0.0f;
    for (const auto& p : samples_) {
        float d = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        if (d > max_dist) max_dist = d;
    }
    if (max_dist < 1e-6f) max_dist = 1.0f;

    for (const auto& p : samples_) {
        float d = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        float t = d / max_dist;

        auto r = static_cast<unsigned char>(static_cast<float>(CLOUD_CORE_COL.r) * (1.0f - t) +
                                            static_cast<float>(CLOUD_OUTER_COL.r) * t);
        auto g = static_cast<unsigned char>(static_cast<float>(CLOUD_CORE_COL.g) * (1.0f - t) +
                                            static_cast<float>(CLOUD_OUTER_COL.g) * t);
        auto b = static_cast<unsigned char>(static_cast<float>(CLOUD_CORE_COL.b) * (1.0f - t) +
                                            static_cast<float>(CLOUD_OUTER_COL.b) * t);
        auto a = static_cast<unsigned char>(200.0f * (1.0f - t * 0.6f));

        float sz = 0.06f + 0.02f * (1.0f - t);
        DrawCubeV(p, {sz, sz, sz}, {r, g, b, a});
    }

    EndMode3D();
    EndScissorMode();

    draw_text("x", static_cast<float>(vp_x + vp_w) - 30.0f,
              static_cast<float>(vp_y + vp_h) - 20.0f, 14, ui_colors::AXIS_X);
    draw_text("y", static_cast<float>(vp_x + vp_w) - 18.0f,
              static_cast<float>(vp_y + vp_h) - 34.0f, 14, ui_colors::AXIS_Y);
    draw_text("z", static_cast<float>(vp_x + vp_w) - 44.0f,
              static_cast<float>(vp_y + vp_h) - 20.0f, 14, ui_colors::AXIS_Z);

    std::string orb_name = model_.get_orbital_name();
    char orb_buf[64];
    std::snprintf(orb_buf, sizeof(orb_buf), "Orbital: %s", orb_name.c_str());
    draw_text(orb_buf, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 12.0f, 16, ui_colors::ACCENT);

    char sample_info[64];
    std::snprintf(sample_info, sizeof(sample_info), "%d samples | %s",
                  static_cast<int>(samples_.size()), orbital_description().c_str());
    draw_text(sample_info, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 32.0f, 12, ui_colors::TEXT_SECONDARY);

    draw_text("Left-click drag: rotate | Scroll: zoom | Middle drag: pan",
              static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y + vp_h) - 18.0f, 10, ui_colors::TEXT_DISABLED);

    render_zoom_buttons(vp_x, vp_y, vp_w, vp_h);
}

// ---------------------------------------------------------------------------
// View 2: Dot Density (3D with measurement sphere)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_dot_density(Camera3D& cam, int vp_x, int vp_y,
                                            int vp_w, int vp_h) {
    float measure_r = measure_radius_slider_.get_value();

    BeginScissorMode(vp_x, vp_y, vp_w, vp_h);
    BeginMode3D(cam);

    float axis_len = orbital_extent_ * 0.7f;
    render_3d_axes(axis_len);
    DrawSphere({0, 0, 0}, 0.12f, NUCLEUS_COLOR);

    int dots_inside = 0;
    float r_sq = measure_r * measure_r;

    for (const auto& p : samples_) {
        float dist_sq = p.x * p.x + p.y * p.y + p.z * p.z;
        bool inside = dist_sq <= r_sq;
        if (inside) dots_inside++;

        Color col = inside ? Color{255, 220, 80, 255} : ELECTRON_COLOR;
        float sz = inside ? 0.08f : 0.06f;
        DrawCubeV(p, {sz, sz, sz}, col);
    }

    constexpr int RING_SEGMENTS = 48;
    for (int i = 0; i < RING_SEGMENTS; ++i) {
        float a1 = 2.0f * PI_F * static_cast<float>(i) / RING_SEGMENTS;
        float a2 = 2.0f * PI_F * static_cast<float>(i + 1) / RING_SEGMENTS;
        DrawLine3D({measure_r * std::cos(a1), 0, measure_r * std::sin(a1)},
                   {measure_r * std::cos(a2), 0, measure_r * std::sin(a2)}, MEASURE_RING_COL);
        DrawLine3D({measure_r * std::cos(a1), measure_r * std::sin(a1), 0},
                   {measure_r * std::cos(a2), measure_r * std::sin(a2), 0}, MEASURE_RING_COL);
        DrawLine3D({0, measure_r * std::cos(a1), measure_r * std::sin(a1)},
                   {0, measure_r * std::cos(a2), measure_r * std::sin(a2)}, MEASURE_RING_COL);
    }

    EndMode3D();
    EndScissorMode();

    int total = static_cast<int>(samples_.size());
    double pct = (total > 0) ? 100.0 * static_cast<double>(dots_inside) / static_cast<double>(total) : 0.0;

    std::string orb_name = model_.get_orbital_name();
    char title[64];
    std::snprintf(title, sizeof(title), "Dot Density: %s", orb_name.c_str());
    draw_text(title, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 12.0f, 16, ui_colors::ACCENT);

    char meas_buf[128];
    std::snprintf(meas_buf, sizeof(meas_buf),
                  "r = %.1f a\xe2\x82\x80  |  %d/%d dots  |  %.1f%% probability",
                  static_cast<double>(measure_r), dots_inside, total, pct);
    draw_text(meas_buf, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 32.0f, 12, MEASURE_RING_COL);

    draw_text("Adjust measurement radius with slider in controls panel",
              static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y + vp_h) - 18.0f, 10, ui_colors::TEXT_DISABLED);

    render_zoom_buttons(vp_x, vp_y, vp_w, vp_h);
}

// ---------------------------------------------------------------------------
// View 3: Orbital Shape (3D isosurface)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_orbital_shape(Camera3D& cam, int vp_x, int vp_y,
                                              int vp_w, int vp_h) {
    BeginScissorMode(vp_x, vp_y, vp_w, vp_h);
    BeginMode3D(cam);

    float axis_len = orbital_extent_ * 0.7f;
    render_3d_axes(axis_len);
    DrawSphere({0, 0, 0}, 0.12f, NUCLEUS_COLOR);

    float vs = orbital_voxel_size_;
    for (const auto& v : orbital_voxels_) {
        Color col = (v.psi_sign > 0) ? LOBE_POSITIVE : LOBE_NEGATIVE;
        DrawCubeV(v.pos, {vs, vs, vs}, col);
    }

    constexpr int RING_SEGS = 48;
    Color ring_col = {80, 80, 90, 60};
    for (int i = 0; i < RING_SEGS; ++i) {
        float a1 = 2.0f * PI_F * static_cast<float>(i) / RING_SEGS;
        float a2 = 2.0f * PI_F * static_cast<float>(i + 1) / RING_SEGS;
        float r = orbital_extent_ * 0.8f;
        DrawLine3D({r * std::cos(a1), 0, r * std::sin(a1)},
                   {r * std::cos(a2), 0, r * std::sin(a2)}, ring_col);
    }

    EndMode3D();
    EndScissorMode();

    std::string orb_name = model_.get_orbital_name();
    char title[64];
    std::snprintf(title, sizeof(title), "Orbital Shape: %s (m=%d)", orb_name.c_str(), current_m());
    draw_text(title, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 12.0f, 16, ui_colors::ACCENT);

    draw_text(orbital_description().c_str(), static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 32.0f, 12, ui_colors::TEXT_SECONDARY);

    int l = current_l();
    const char* shape_desc = "";
    switch (l) {
        case 0: shape_desc = "l=0 (s): Spherical symmetry"; break;
        case 1: shape_desc = "l=1 (p): Dumbbell / directional lobes"; break;
        case 2: shape_desc = "l=2 (d): Cloverleaf / complex lobes"; break;
        case 3: shape_desc = "l=3 (f): Multi-lobe structure"; break;
        default: shape_desc = "Higher angular momentum"; break;
    }
    draw_text(shape_desc, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 48.0f, 11, ui_colors::TEXT_SECONDARY);

    float legend_x = static_cast<float>(vp_x) + 12.0f;
    float legend_y = static_cast<float>(vp_y + vp_h) - 42.0f;
    DrawRectangle(static_cast<int>(legend_x), static_cast<int>(legend_y), 12, 12, LOBE_POSITIVE);
    draw_text("\xcf\x88 > 0 (positive phase)", legend_x + 16.0f, legend_y, 11, ui_colors::TEXT_SECONDARY);
    DrawRectangle(static_cast<int>(legend_x), static_cast<int>(legend_y) + 16, 12, 12, LOBE_NEGATIVE);
    draw_text("\xcf\x88 < 0 (negative phase)", legend_x + 16.0f, legend_y + 16.0f, 11, ui_colors::TEXT_SECONDARY);

    render_zoom_buttons(vp_x, vp_y, vp_w, vp_h);
}

// ---------------------------------------------------------------------------
// Controls panel (left)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);

    int cx = x + 10;
    int cy = y + 10;
    int cw = w - 20;

    draw_section("ELEMENT", cx, cy);
    cy += 4;

    const auto& el = ElementData::get_element_by_index(element_index_);
    char btn_label[64];
    std::snprintf(btn_label, sizeof(btn_label), "%s - %s (Z=%d)",
                  el.symbol.c_str(), el.name.c_str(), el.atomic_number);

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

    draw_section("QUANTUM NUMBERS", cx, cy);
    cy += 4;

    bool changed = false;

    if (n_slider_.render(font_, has_font_, cx, cy, cw)) {
        n_slider_.set_value(std::round(n_slider_.get_value()));
        validate_quantum_numbers();
        changed = true;
    }
    cy += Slider::HEIGHT;

    if (l_slider_.render(font_, has_font_, cx, cy, cw)) {
        l_slider_.set_value(std::round(l_slider_.get_value()));
        int l = current_l();
        m_slider_.set_range(static_cast<float>(-l), static_cast<float>(l));
        changed = true;
    }
    cy += Slider::HEIGHT;

    if (m_slider_.render(font_, has_font_, cx, cy, cw)) {
        m_slider_.set_value(std::round(m_slider_.get_value()));
        changed = true;
    }
    cy += Slider::HEIGHT;

    if (changed) {
        regenerate_model();
        regenerate_samples();
        regenerate_orbital_shape();
    }

    std::string orb = orbital_name_from_nl(current_n(), current_l());
    char orb_display[64];
    std::snprintf(orb_display, sizeof(orb_display), "Orbital:  %s  (m=%d)",
                  orb.c_str(), current_m());
    draw_text(orb_display, static_cast<float>(cx), static_cast<float>(cy), 13,
              ui_colors::ACCENT);
    cy += 22;

    draw_separator(cx, cy, cw);

    if (current_view_ == 2) {
        draw_section("MEASUREMENT", cx, cy);
        cy += 4;
        measure_radius_slider_.render(font_, has_font_, cx, cy, cw);
        cy += Slider::HEIGHT + 4;
        draw_separator(cx, cy, cw);
    }

    if (current_view_ == 3) {
        draw_section("ISO-SURFACE", cx, cy);
        cy += 4;
        if (iso_threshold_slider_.render(font_, has_font_, cx, cy, cw)) {
            regenerate_orbital_shape();
        }
        cy += Slider::HEIGHT + 4;

        char voxel_info[64];
        std::snprintf(voxel_info, sizeof(voxel_info), "%d voxels rendered",
                      static_cast<int>(orbital_voxels_.size()));
        draw_text(voxel_info, static_cast<float>(cx), static_cast<float>(cy), 11,
                  ui_colors::TEXT_SECONDARY);
        cy += 16;
        draw_separator(cx, cy, cw);
    }

    draw_section("SAMPLING", cx, cy);
    cy += 4;

    if (sample_count_slider_.render(font_, has_font_, cx, cy, cw)) {
        sample_count_slider_.set_value(std::round(sample_count_slider_.get_value()));
        sample_count_ = static_cast<int>(sample_count_slider_.get_value());
    }
    cy += Slider::HEIGHT;

    if (render_button("Regenerate", cx, cy, cw, 26, Color{55, 55, 65, 255})) {
        sample_count_ = static_cast<int>(sample_count_slider_.get_value());
        regenerate_samples();
        regenerate_orbital_shape();
    }
    cy += 34;

    draw_separator(cx, cy, cw);
    cy += 4;

    draw_text("[+/-] Zoom in/out", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 14;
    draw_text("[1-4] Switch views", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_DISABLED);
    cy += 20;

    if (HelpPopup::render_help_button(font_, has_font_, cx + 10, cy + 4)) {
        help_popup_.show({"Atom Viewer", "Atomic Orbitals & Quantum Numbers",
            "Each orbital is defined by 3 quantum numbers:\n"
            "  n = principal (1,2,3...) \xe2\x86\x92 energy shell\n"
            "  l = angular (0..n-1) \xe2\x86\x92 shape (s,p,d,f)\n"
            "  m = magnetic (-l..+l) \xe2\x86\x92 orientation\n\n"
            "Number of nodes = n-1 total:\n"
            "  Radial nodes = n-l-1 (spherical shells of \xcf\x88=0)\n"
            "  Angular nodes = l (planes/cones of \xcf\x88=0)\n\n"
            "Views:\n"
            "  Bohr Model: classical orbits (simplified)\n"
            "  Orbital Cloud: probability sampling\n"
            "  Dot Density: with measurement sphere\n"
            "  Orbital Shape: isosurface showing +/- lobes",
            "E_n = -13.6 Z\xc2\xb2 / n\xc2\xb2 eV",
            "eV (energy), a\xe2\x82\x80 (Bohr radius)"});
    }
    draw_text("Help", static_cast<float>(cx + 24),
              static_cast<float>(cy - 2), 12, ui_colors::TEXT_SECONDARY);
}

// ---------------------------------------------------------------------------
// Properties panel (right)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_properties(int x, int y, int w, int h) {
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

    draw_section("QUANTUM NUMBERS", px, py);

    int n = current_n();
    int l = current_l();
    int m = current_m();

    std::snprintf(buf, sizeof(buf), "%d", n);
    draw_prop("n:", buf, px, py);
    std::snprintf(buf, sizeof(buf), "%d", l);
    draw_prop("l:", buf, px, py);
    std::snprintf(buf, sizeof(buf), "%d", m);
    draw_prop("m:", buf, px, py);

    std::string orb = orbital_name_from_nl(n, l);
    draw_prop("Orbital:", orb.c_str(), px, py, ui_colors::ACCENT);

    static const char* subshell_names[] = {"s (sharp)", "p (principal)",
        "d (diffuse)", "f (fundamental)", "g", "h"};
    if (l >= 0 && l < 6) {
        draw_text(subshell_names[l], static_cast<float>(px + 10),
                  static_cast<float>(py), 11, ui_colors::TEXT_DISABLED);
        py += 16;
    }

    py += 4;
    draw_separator(px, py, pw);

    draw_section("ENERGY", px, py);

    double e = energy_eV();
    std::snprintf(buf, sizeof(buf), "%.4f eV", e);
    draw_prop("E_n:", buf, px, py, ui_colors::WAVEFUNCTION);

    draw_text("E = -13.6 Z\xc2\xb2/n\xc2\xb2", static_cast<float>(px + 10),
              static_cast<float>(py), 11, ui_colors::TEXT_DISABLED);
    py += 16;

    py += 4;
    draw_separator(px, py, pw);

    draw_section("NODES", px, py);

    int radial_nodes = n - l - 1;
    int angular_nodes = l;

    std::snprintf(buf, sizeof(buf), "%d", radial_nodes);
    draw_prop("Radial:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%d", angular_nodes);
    draw_prop("Angular:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%d", radial_nodes + angular_nodes);
    draw_prop("Total:", buf, px, py, ui_colors::ACCENT);

    draw_text("Radial: spherical shells", static_cast<float>(px + 4),
              static_cast<float>(py), 10, ui_colors::TEXT_DISABLED);
    py += 13;
    draw_text("  where \xcf\x88(r) = 0", static_cast<float>(px + 4),
              static_cast<float>(py), 10, ui_colors::TEXT_DISABLED);
    py += 13;
    draw_text("Angular: nodal planes", static_cast<float>(px + 4),
              static_cast<float>(py), 10, ui_colors::TEXT_DISABLED);
    py += 13;
    draw_text("  (like standing wave nodes)", static_cast<float>(px + 4),
              static_cast<float>(py), 10, ui_colors::TEXT_DISABLED);
    py += 16;

    py += 4;
    draw_separator(px, py, pw);

    draw_section("SAMPLES", px, py);

    std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(samples_.size()));
    draw_prop("Cloud:", buf, px, py);

    if (current_view_ == 3) {
        std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(orbital_voxels_.size()));
        draw_prop("Voxels:", buf, px, py);
    }

    std::snprintf(buf, sizeof(buf), "%s", get_view_name(current_view_));
    draw_prop("View:", buf, px, py);

    if (current_view_ == 2) {
        py += 4;
        draw_separator(px, py, pw);
        draw_section("MEASUREMENT", px, py);

        float measure_r = measure_radius_slider_.get_value();
        float r_sq = measure_r * measure_r;
        int dots_inside = 0;
        for (const auto& p : samples_) {
            if (p.x * p.x + p.y * p.y + p.z * p.z <= r_sq) dots_inside++;
        }

        int total = static_cast<int>(samples_.size());
        double pct = (total > 0) ? 100.0 * static_cast<double>(dots_inside) / static_cast<double>(total) : 0.0;

        std::snprintf(buf, sizeof(buf), "%.1f a\xe2\x82\x80", static_cast<double>(measure_r));
        draw_prop("Radius:", buf, px, py);

        std::snprintf(buf, sizeof(buf), "%d / %d", dots_inside, total);
        draw_prop("Dots:", buf, px, py);

        std::snprintf(buf, sizeof(buf), "%.1f%%", pct);
        draw_prop("P(r<R):", buf, px, py, MEASURE_RING_COL);
    }
}
