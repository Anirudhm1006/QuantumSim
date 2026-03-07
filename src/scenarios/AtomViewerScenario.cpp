#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>

#include <raylib.h>
#include <rlgl.h>

#include "AtomViewerScenario.hpp"

namespace {
    constexpr Color NUCLEUS_COLOR     = {220, 80, 60, 255};
    constexpr Color ELECTRON_COLOR    = {100, 180, 255, 255};
    constexpr Color ORBIT_LINE_COLOR  = {60, 70, 90, 180};
    constexpr Color SHELL_LABEL_COLOR = {120, 130, 150, 255};
    constexpr Color ENERGY_LEVEL_COL  = {100, 140, 180, 255};
    constexpr Color MEASURE_RING_COL  = {255, 220, 60, 180};
    constexpr Color CLOUD_CORE_COL    = {80, 160, 255, 255};
    constexpr Color CLOUD_OUTER_COL   = {40, 80, 160, 100};
    constexpr Color COORD_LABEL_COL   = {180, 180, 190, 255};

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
    , sample_count_slider_("Sample Count", 500.0f, 5000.0f, 2000.0f, "%.0f")
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
    measuring_ = false;
    dots_in_circle_ = 0;
    measure_radius_ = 0.0f;
    regenerate_model();
    regenerate_samples();
}

const char* AtomViewerScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Bohr Model";
        case 1: return "Orbital Cloud";
        case 2: return "Dot Density";
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

    if (current_view_ == 2) {
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            measuring_ = true;
            measure_start_ = mouse;
            measure_end_ = mouse;
            dots_in_circle_ = 0;
            measure_radius_ = 0.0f;
        }
        if (measuring_ && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            measure_end_ = mouse;
        }
        if (measuring_ && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            measure_end_ = mouse;
            float dx = measure_end_.x - measure_start_.x;
            float dy = measure_end_.y - measure_start_.y;
            measure_radius_ = std::sqrt(dx * dx + dy * dy);
        }
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
// Viewport routing
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y,
                                         int vp_w, int vp_h) {
    DrawRectangle(vp_x, vp_y, vp_w, vp_h, ui_colors::BG_DARK);

    switch (current_view_) {
        case 0: render_bohr_model(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_orbital_cloud(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_dot_density(cam, vp_x, vp_y, vp_w, vp_h); break;
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

    // Element name and Z at the top
    char header[64];
    std::snprintf(header, sizeof(header), "%s (Z=%d)", el.name.c_str(), Z);
    float header_w = measure_text(header, 18);
    draw_text(header, cx - header_w * 0.5f,
              static_cast<float>(vp_y) + 12.0f, 18, ui_colors::ACCENT);

    // Nucleus
    DrawCircleV({cx, cy}, NUCLEUS_RADIUS, NUCLEUS_COLOR);
    char nuc_label[16];
    std::snprintf(nuc_label, sizeof(nuc_label), "%d p", Z);
    float nuc_tw = measure_text(nuc_label, 10);
    draw_text(nuc_label, cx - nuc_tw * 0.5f, cy - 5.0f, 10, ui_colors::TEXT_PRIMARY);

    // Orbits and electrons
    for (int s = 0; s < num_shells; ++s) {
        float r = orbit_spacing * static_cast<float>(s + 1);

        DrawCircleLinesV({cx, cy}, r, ORBIT_LINE_COLOR);

        // Shell label
        const char* shell_name = (s < 7) ? SHELL_LABELS[s] : "?";
        char shell_lbl[32];
        std::snprintf(shell_lbl, sizeof(shell_lbl), "%s (%d)", shell_name, shells[s]);
        draw_text(shell_lbl, cx + r + 6.0f, cy - 8.0f, 11, SHELL_LABEL_COLOR);

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

    // Energy levels on the right side
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

        DrawLineEx({level_x, ly}, {level_x + 80.0f, ly}, 1.5f, ENERGY_LEVEL_COL);

        char elbl[48];
        std::snprintf(elbl, sizeof(elbl), "n=%d  %.2f eV", n, e_n);
        draw_text(elbl, level_x + 4.0f, ly - 14.0f, 10, ui_colors::TEXT_SECONDARY);
    }
}

// ---------------------------------------------------------------------------
// View 1: Orbital Cloud (3D)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_orbital_cloud(Camera3D& cam, int vp_x, int vp_y,
                                              int vp_w, int vp_h) {
    BeginMode3D(cam);

    // Coordinate axes
    float axis_len = 8.0f;
    DrawLine3D({0, 0, 0}, {axis_len, 0, 0}, ui_colors::AXIS_X);
    DrawLine3D({0, 0, 0}, {0, axis_len, 0}, ui_colors::AXIS_Y);
    DrawLine3D({0, 0, 0}, {0, 0, axis_len}, ui_colors::AXIS_Z);

    // Nucleus dot
    DrawSphere({0, 0, 0}, 0.15f, NUCLEUS_COLOR);

    // Render sample points colored by distance from origin
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
        auto a = static_cast<unsigned char>(static_cast<float>(CLOUD_CORE_COL.a) * (1.0f - t) +
                                            static_cast<float>(CLOUD_OUTER_COL.a) * t);

        DrawSphere(p, 0.08f, {r, g, b, a});
    }

    EndMode3D();

    // Axis labels (2D overlay)
    draw_text("x", static_cast<float>(vp_x + vp_w) - 30.0f,
              static_cast<float>(vp_y + vp_h) - 20.0f, 14, ui_colors::AXIS_X);
    draw_text("y", static_cast<float>(vp_x + vp_w) - 18.0f,
              static_cast<float>(vp_y + vp_h) - 34.0f, 14, ui_colors::AXIS_Y);
    draw_text("z", static_cast<float>(vp_x + vp_w) - 44.0f,
              static_cast<float>(vp_y + vp_h) - 20.0f, 14, ui_colors::AXIS_Z);

    // Orbital name overlay
    std::string orb_name = model_.get_orbital_name();
    char orb_buf[64];
    std::snprintf(orb_buf, sizeof(orb_buf), "Orbital: %s", orb_name.c_str());
    draw_text(orb_buf, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 12.0f, 16, ui_colors::ACCENT);

    char sample_info[64];
    std::snprintf(sample_info, sizeof(sample_info), "%d samples", static_cast<int>(samples_.size()));
    draw_text(sample_info, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 32.0f, 12, ui_colors::TEXT_SECONDARY);
}

// ---------------------------------------------------------------------------
// View 2: Dot Density (3D with measurement)
// ---------------------------------------------------------------------------

void AtomViewerScenario::render_dot_density(Camera3D& cam, int vp_x, int vp_y,
                                            int vp_w, int vp_h) {
    BeginMode3D(cam);

    // Coordinate axes
    float axis_len = 8.0f;
    DrawLine3D({0, 0, 0}, {axis_len, 0, 0}, ui_colors::AXIS_X);
    DrawLine3D({0, 0, 0}, {0, axis_len, 0}, ui_colors::AXIS_Y);
    DrawLine3D({0, 0, 0}, {0, 0, axis_len}, ui_colors::AXIS_Z);

    DrawSphere({0, 0, 0}, 0.12f, NUCLEUS_COLOR);

    for (const auto& p : samples_) {
        DrawSphere(p, 0.06f, ELECTRON_COLOR);
    }

    EndMode3D();

    // Measurement circle (screen space)
    if (measuring_ && measure_radius_ > 2.0f) {
        float mid_x = (measure_start_.x + measure_end_.x) * 0.5f;
        float mid_y = (measure_start_.y + measure_end_.y) * 0.5f;

        DrawCircleLinesV({mid_x, mid_y}, measure_radius_, MEASURE_RING_COL);

        count_dots_in_measurement_circle(cam, vp_x, vp_y, vp_w, vp_h);

        int total = static_cast<int>(samples_.size());
        double pct = (total > 0) ? 100.0 * static_cast<double>(dots_in_circle_) /
                                   static_cast<double>(total) : 0.0;

        float r_bohr = measure_radius_ / 30.0f;

        char meas_buf[128];
        std::snprintf(meas_buf, sizeof(meas_buf),
                      "%d/%d dots = %.1f%% probability within r ~ %.1f a0",
                      dots_in_circle_, total, pct, static_cast<double>(r_bohr));

        float meas_tw = measure_text(meas_buf, 13);
        float label_x = mid_x - meas_tw * 0.5f;
        float label_y = mid_y - measure_radius_ - 22.0f;

        DrawRectangle(static_cast<int>(label_x) - 4, static_cast<int>(label_y) - 2,
                      static_cast<int>(meas_tw) + 8, 18, {0, 0, 0, 180});
        draw_text(meas_buf, label_x, label_y, 13, MEASURE_RING_COL);
    }

    // Orbital label
    std::string orb_name = model_.get_orbital_name();
    char orb_buf[64];
    std::snprintf(orb_buf, sizeof(orb_buf), "Dot Density: %s", orb_name.c_str());
    draw_text(orb_buf, static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y) + 12.0f, 16, ui_colors::ACCENT);

    draw_text("Click + drag to measure", static_cast<float>(vp_x) + 12.0f,
              static_cast<float>(vp_y + vp_h) - 22.0f, 11, ui_colors::TEXT_DISABLED);
}

void AtomViewerScenario::count_dots_in_measurement_circle(Camera3D& cam,
                                                          int vp_x, int vp_y,
                                                          int vp_w, int vp_h) {
    float mid_x = (measure_start_.x + measure_end_.x) * 0.5f;
    float mid_y = (measure_start_.y + measure_end_.y) * 0.5f;
    float r_sq = measure_radius_ * measure_radius_;

    dots_in_circle_ = 0;
    for (const auto& p : samples_) {
        Vector2 screen = GetWorldToScreenEx(p, cam, vp_w, vp_h);
        screen.x += static_cast<float>(vp_x);
        screen.y += static_cast<float>(vp_y);

        float dx = screen.x - mid_x;
        float dy = screen.y - mid_y;
        if (dx * dx + dy * dy <= r_sq) {
            dots_in_circle_++;
        }
    }
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

    // Quantum numbers
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
    }

    // Orbital label
    std::string orb = orbital_name_from_nl(current_n(), current_l());
    char orb_display[64];
    std::snprintf(orb_display, sizeof(orb_display), "Orbital:  %s  (m=%d)",
                  orb.c_str(), current_m());
    draw_text(orb_display, static_cast<float>(cx), static_cast<float>(cy), 13,
              ui_colors::ACCENT);
    cy += 22;

    draw_separator(cx, cy, cw);

    // Sample count (only relevant for Views 1 & 2)
    draw_section("SAMPLING", cx, cy);
    cy += 4;

    if (sample_count_slider_.render(font_, has_font_, cx, cy, cw)) {
        sample_count_slider_.set_value(std::round(sample_count_slider_.get_value()));
        sample_count_ = static_cast<int>(sample_count_slider_.get_value());
    }
    cy += Slider::HEIGHT;

    if (render_button("Reset Samples", cx, cy, cw, 26, Color{55, 55, 65, 255})) {
        sample_count_ = static_cast<int>(sample_count_slider_.get_value());
        regenerate_samples();
    }
    cy += 34;

    draw_separator(cx, cy, cw);
    cy += 4;

    if (HelpPopup::render_help_button(font_, has_font_, cx + 10, cy + 4)) {
        help_popup_.show({"Atom Viewer", "Atomic Orbitals & Bohr Model",
            "View the atom in three ways:\n"
            "- Bohr Model: classical circular orbits\n"
            "- Orbital Cloud: quantum probability distribution\n"
            "- Dot Density: Monte Carlo sampling with\n"
            "  interactive measurement of probability.",
            "E_n = -13.6 Z\xc2\xb2 / n\xc2\xb2 eV",
            "eV (energy), a\xe2\x82\x80 (Bohr radius)"});
    }
    draw_text("Help: Atom Viewer", static_cast<float>(cx + 24),
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

    draw_section("CURRENT ORBITAL", px, py);

    std::snprintf(buf, sizeof(buf), "%d", current_n());
    draw_prop("n:", buf, px, py);
    std::snprintf(buf, sizeof(buf), "%d", current_l());
    draw_prop("l:", buf, px, py);
    std::snprintf(buf, sizeof(buf), "%d", current_m());
    draw_prop("m:", buf, px, py);

    std::string orb = orbital_name_from_nl(current_n(), current_l());
    draw_prop("Orbital:", orb.c_str(), px, py, ui_colors::ACCENT);

    double e = energy_eV();
    std::snprintf(buf, sizeof(buf), "%.4f eV", e);
    draw_prop("E_n:", buf, px, py, ui_colors::WAVEFUNCTION);

    py += 4;
    draw_separator(px, py, pw);

    draw_section("SAMPLES", px, py);

    std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(samples_.size()));
    draw_prop("Count:", buf, px, py);

    std::snprintf(buf, sizeof(buf), "%s", get_view_name(current_view_));
    draw_prop("View:", buf, px, py);

    py += 4;
    draw_separator(px, py, pw);

    // Measurement result (View 2 only)
    if (current_view_ == 2) {
        draw_section("MEASUREMENT", px, py);

        if (measuring_ && measure_radius_ > 2.0f) {
            int total = static_cast<int>(samples_.size());
            double pct = (total > 0) ? 100.0 * static_cast<double>(dots_in_circle_) /
                                       static_cast<double>(total) : 0.0;
            float r_bohr = measure_radius_ / 30.0f;

            std::snprintf(buf, sizeof(buf), "%d / %d", dots_in_circle_, total);
            draw_prop("Dots:", buf, px, py);

            std::snprintf(buf, sizeof(buf), "%.1f%%", pct);
            draw_prop("Probability:", buf, px, py, MEASURE_RING_COL);

            std::snprintf(buf, sizeof(buf), "~ %.1f a\xe2\x82\x80",
                          static_cast<double>(r_bohr));
            draw_prop("Radius:", buf, px, py);
        } else {
            draw_text("Drag in viewport to measure",
                      static_cast<float>(px), static_cast<float>(py), 12,
                      ui_colors::TEXT_DISABLED);
            py += 18;
        }
    }
}
