#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "EntanglementScenario.hpp"

EntanglementScenario::EntanglementScenario()
    : alice_angle_slider_("Alice angle", 0.0f, 360.0f, 0.0f, "%.0f\xC2\xB0")
    , bob_angle_slider_("Bob angle", 0.0f, 360.0f, 0.0f, "%.0f\xC2\xB0")
{}

void EntanglementScenario::on_enter() {
    current_view_ = 0;
    epr_log_.clear();
    epr_same_ = 0;
    epr_diff_ = 0;
    creation_step_ = 0;
    flying_particles_.clear();
    fly_timer_ = 0.0;
}

double EntanglementScenario::alice_angle_rad() const {
    return static_cast<double>(alice_angle_slider_.get_value()) * PI / 180.0;
}

double EntanglementScenario::bob_angle_rad() const {
    return static_cast<double>(bob_angle_slider_.get_value()) * PI / 180.0;
}

EntanglementScenario::BellProbs EntanglementScenario::get_bell_probs() const {
    BellProbs bp;
    switch (bell_state_) {
        case 0: bp = {0.5, 0.0, 0.0, 0.5}; break;
        case 1: bp = {0.5, 0.0, 0.0, 0.5}; break;
        case 2: bp = {0.0, 0.5, 0.5, 0.0}; break;
        case 3: bp = {0.0, 0.5, 0.5, 0.0}; break;
        default: bp = {0.5, 0.0, 0.0, 0.5}; break;
    }
    return bp;
}

double EntanglementScenario::quantum_correlation(double angle_diff) const {
    switch (bell_state_) {
        case 0: return std::cos(2.0 * angle_diff);
        case 1: return -std::cos(2.0 * angle_diff);
        case 2: return -std::cos(2.0 * angle_diff);
        case 3: return std::cos(2.0 * angle_diff);
        default: return 0.0;
    }
}

void EntanglementScenario::do_epr_measurement() {
    double angle_diff = alice_angle_rad() - bob_angle_rad();
    double p_same;

    switch (bell_state_) {
        case 0: p_same = std::cos(angle_diff) * std::cos(angle_diff); break;
        case 1: p_same = std::cos(angle_diff) * std::cos(angle_diff); break;
        case 2: p_same = std::sin(angle_diff) * std::sin(angle_diff); break;
        case 3: p_same = std::sin(angle_diff) * std::sin(angle_diff); break;
        default: p_same = 0.5; break;
    }

    std::uniform_real_distribution<double> coin(0.0, 1.0);
    std::uniform_int_distribution<int> updown(0, 1);

    MeasResult r;
    if (coin(rng_) < p_same) {
        r.alice = updown(rng_);
        r.bob = r.alice;
        if (bell_state_ == 1) r.bob = 1 - r.alice;
        if (bell_state_ == 1) { r.alice = updown(rng_); r.bob = r.alice; }
        epr_same_++;
    } else {
        r.alice = updown(rng_);
        r.bob = 1 - r.alice;
        epr_diff_++;
    }

    epr_log_.push_back(r);
    if (epr_log_.size() > 30) epr_log_.erase(epr_log_.begin());
}

const char* EntanglementScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "EPR Experiment";
        case 1: return "Bell States";
        case 2: return "Correlations";
        case 3: return "Creation";
        default: return "Unknown";
    }
}

void EntanglementScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
}

void EntanglementScenario::update(double dt) {
    if (current_view_ == 0) {
        fly_timer_ += dt;
        for (auto& p : flying_particles_) {
            p.x += p.direction * static_cast<float>(dt) * 0.3f;
        }
        flying_particles_.erase(
            std::remove_if(flying_particles_.begin(), flying_particles_.end(),
                           [](const FlyingParticle& p) { return std::abs(p.x) > 0.6f; }),
            flying_particles_.end());
    }
}

void EntanglementScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;
    switch (current_view_) {
        case 0: render_epr(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_bell_explorer(vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_correlation(vp_x, vp_y, vp_w, vp_h); break;
        case 3: render_creation(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void EntanglementScenario::render_epr(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("EPR EXPERIMENT", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Entangled pairs: measuring one instantly determines the other",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int cx = vp_x + vp_w / 2;
    int cy = vp_y + vp_h / 3;

    DrawCircle(cx, cy, 15, Color{220, 180, 50, 200});
    draw_text("Source", static_cast<float>(cx - 18), static_cast<float>(cy + 18), 10, ui_colors::TEXT_SECONDARY);

    int alice_x = cx - vp_w / 3;
    int bob_x = cx + vp_w / 3;

    DrawRectangle(alice_x - 25, cy - 25, 50, 50, Color{80, 160, 220, 150});
    DrawRectangleLinesEx({static_cast<float>(alice_x - 25), static_cast<float>(cy - 25), 50.0f, 50.0f},
                         1.0f, ui_colors::WAVEFUNCTION);
    draw_text("Alice", static_cast<float>(alice_x - 15), static_cast<float>(cy + 30), 11, ui_colors::WAVEFUNCTION);

    DrawRectangle(bob_x - 25, cy - 25, 50, 50, Color{220, 140, 80, 150});
    DrawRectangleLinesEx({static_cast<float>(bob_x - 25), static_cast<float>(cy - 25), 50.0f, 50.0f},
                         1.0f, Color{220, 160, 90, 255});
    draw_text("Bob", static_cast<float>(bob_x - 10), static_cast<float>(cy + 30), 11, Color{220, 160, 90, 255});

    DrawLine(cx - 15, cy, alice_x + 25, cy, Color{90, 160, 220, 100});
    DrawLine(cx + 15, cy, bob_x - 25, cy, Color{220, 140, 80, 100});

    for (const auto& p : flying_particles_) {
        int px = cx + static_cast<int>(p.x * vp_w * 0.6f);
        Color pc = (p.direction < 0) ? Color{90, 160, 220, 200} : Color{220, 140, 80, 200};
        DrawCircle(px, cy, 4, pc);
    }

    float alice_a = static_cast<float>(alice_angle_rad());
    int arrow_len = 20;
    DrawLine(alice_x, cy, alice_x + static_cast<int>(arrow_len * std::cos(alice_a)),
             cy - static_cast<int>(arrow_len * std::sin(alice_a)), Color{100, 200, 100, 255});
    draw_text(TextFormat("%.0f\xC2\xB0", alice_angle_slider_.get_value()),
              static_cast<float>(alice_x - 30), static_cast<float>(cy - 40), 10, ui_colors::TEXT_SECONDARY);

    float bob_a = static_cast<float>(bob_angle_rad());
    DrawLine(bob_x, cy, bob_x + static_cast<int>(arrow_len * std::cos(bob_a)),
             cy - static_cast<int>(arrow_len * std::sin(bob_a)), Color{100, 200, 100, 255});
    draw_text(TextFormat("%.0f\xC2\xB0", bob_angle_slider_.get_value()),
              static_cast<float>(bob_x + 30), static_cast<float>(cy - 40), 10, ui_colors::TEXT_SECONDARY);

    int btn_y = cy + 60;
    Rectangle meas_btn = {static_cast<float>(cx - 55), static_cast<float>(btn_y), 110.0f, 28.0f};
    DrawRectangleRec(meas_btn, ui_colors::ACCENT);
    DrawRectangleLinesEx(meas_btn, 1.0f, ui_colors::PANEL_BORDER);
    draw_text("MEASURE", meas_btn.x + 24, meas_btn.y + 7, 12, ui_colors::TEXT_PRIMARY);

    Rectangle run100_btn = {static_cast<float>(cx - 55), static_cast<float>(btn_y + 32), 50.0f, 24.0f};
    DrawRectangleRec(run100_btn, ui_colors::PANEL_HOVER);
    DrawRectangleLinesEx(run100_btn, 1.0f, ui_colors::ACCENT);
    draw_text("x100", run100_btn.x + 10, run100_btn.y + 5, 11, ui_colors::TEXT_PRIMARY);

    Rectangle reset_btn = {static_cast<float>(cx + 5), static_cast<float>(btn_y + 32), 50.0f, 24.0f};
    DrawRectangleRec(reset_btn, ui_colors::PANEL_HOVER);
    DrawRectangleLinesEx(reset_btn, 1.0f, ui_colors::DANGER);
    draw_text("Reset", reset_btn.x + 10, reset_btn.y + 5, 11, ui_colors::TEXT_PRIMARY);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, meas_btn)) {
            do_epr_measurement();
            FlyingParticle fp1, fp2;
            fp1.x = 0.0f; fp1.direction = -1; fp1.spawn_time = static_cast<float>(fly_timer_);
            fp2.x = 0.0f; fp2.direction = 1; fp2.spawn_time = static_cast<float>(fly_timer_);
            flying_particles_.push_back(fp1);
            flying_particles_.push_back(fp2);
        }
        if (CheckCollisionPointRec(mouse, run100_btn)) {
            for (int i = 0; i < 100; ++i) do_epr_measurement();
        }
        if (CheckCollisionPointRec(mouse, reset_btn)) {
            epr_log_.clear();
            epr_same_ = 0;
            epr_diff_ = 0;
        }
    }

    int log_y = btn_y + 65;
    int total = epr_same_ + epr_diff_;
    if (total > 0) {
        draw_text(TextFormat("Same: %d (%.1f%%)   Different: %d (%.1f%%)   Total: %d",
                  epr_same_, 100.0f * epr_same_ / total, epr_diff_, 100.0f * epr_diff_ / total, total),
                  static_cast<float>(vp_x + 30), static_cast<float>(log_y), 12, ui_colors::TEXT_PRIMARY);

        double angle_diff = alice_angle_rad() - bob_angle_rad();
        double expected_same = std::cos(angle_diff) * std::cos(angle_diff);
        draw_text(TextFormat("Quantum prediction: same = %.1f%%", expected_same * 100.0),
                  static_cast<float>(vp_x + 30), static_cast<float>(log_y + 18), 11, ui_colors::ACCENT);
    }

    int last_y = log_y + 40;
    draw_text("Last measurements:", static_cast<float>(vp_x + 30), static_cast<float>(last_y), 10, ui_colors::TEXT_SECONDARY);
    last_y += 14;
    int show = std::min(static_cast<int>(epr_log_.size()), 10);
    for (int i = static_cast<int>(epr_log_.size()) - show; i < static_cast<int>(epr_log_.size()); ++i) {
        const auto& r = epr_log_[i];
        const char* a_str = r.alice ? "\xE2\x86\x91" : "\xE2\x86\x93";
        const char* b_str = r.bob ? "\xE2\x86\x91" : "\xE2\x86\x93";
        bool same = (r.alice == r.bob);
        draw_text(TextFormat("A:%s  B:%s  %s", a_str, b_str, same ? "same" : "diff"),
                  static_cast<float>(vp_x + 30 + (i - (static_cast<int>(epr_log_.size()) - show)) * 90),
                  static_cast<float>(last_y), 9, same ? ui_colors::SUCCESS : ui_colors::DANGER);
    }
}

void EntanglementScenario::render_bell_explorer(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("BELL STATE EXPLORER", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    const char* state_names[] = {
        "|\xCE\xA6\xE2\x81\xBA\xE2\x9F\xA9 = (|00\xE2\x9F\xA9+|11\xE2\x9F\xA9)/\xE2\x88\x9A""2",
        "|\xCE\xA6\xE2\x81\xBB\xE2\x9F\xA9 = (|00\xE2\x9F\xA9-|11\xE2\x9F\xA9)/\xE2\x88\x9A""2",
        "|\xCE\xA8\xE2\x81\xBA\xE2\x9F\xA9 = (|01\xE2\x9F\xA9+|10\xE2\x9F\xA9)/\xE2\x88\x9A""2",
        "|\xCE\xA8\xE2\x81\xBB\xE2\x9F\xA9 = (|01\xE2\x9F\xA9-|10\xE2\x9F\xA9)/\xE2\x88\x9A""2"
    };

    const char* descriptions[] = {
        "Both always SAME (both 0 or both 1)",
        "Both always SAME (both 0 or both 1)",
        "Both always OPPOSITE (one 0, one 1)",
        "Both always OPPOSITE (one 0, one 1)"
    };

    draw_text(state_names[bell_state_], static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 32), 13, ui_colors::TEXT_PRIMARY);
    draw_text(descriptions[bell_state_], static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 50), 11, ui_colors::TEXT_SECONDARY);

    auto bp = get_bell_probs();
    double probs[] = {bp.p00, bp.p01, bp.p10, bp.p11};
    const char* labels[] = {"|00\xE2\x9F\xA9", "|01\xE2\x9F\xA9", "|10\xE2\x9F\xA9", "|11\xE2\x9F\xA9"};
    Color bar_colors[] = {
        Color{80, 160, 220, 200}, Color{220, 140, 80, 200},
        Color{140, 220, 80, 200}, Color{220, 80, 140, 200}
    };

    int chart_x = vp_x + 60;
    int chart_w = vp_w - 120;
    int chart_y = vp_y + 80;
    int chart_h = vp_h / 2 - 60;
    int baseline = chart_y + chart_h;
    int bar_w = chart_w / 6;

    DrawLine(chart_x, baseline, chart_x + chart_w, baseline, ui_colors::TEXT_SECONDARY);

    for (int i = 0; i < 4; ++i) {
        int bx = chart_x + (i + 1) * chart_w / 5 - bar_w / 2;
        int bh = static_cast<int>(probs[i] * chart_h);
        DrawRectangle(bx, baseline - bh, bar_w, bh, bar_colors[i]);
        DrawRectangleLinesEx({static_cast<float>(bx), static_cast<float>(baseline - bh),
                              static_cast<float>(bar_w), static_cast<float>(bh)}, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(labels[i], static_cast<float>(bx + bar_w / 2 - 10), static_cast<float>(baseline + 5), 12, ui_colors::TEXT_PRIMARY);
        draw_text(TextFormat("%.0f%%", probs[i] * 100.0), static_cast<float>(bx + bar_w / 2 - 10),
                  static_cast<float>(baseline - bh - 16), 11, bar_colors[i]);
    }

    int vis_y = baseline + 40;
    int cx = vp_x + vp_w / 2;

    DrawCircle(cx - 50, vis_y + 30, 25, Color{80, 160, 220, 150});
    DrawCircle(cx + 50, vis_y + 30, 25, Color{220, 140, 80, 150});

    DrawLine(cx - 25, vis_y + 30, cx + 25, vis_y + 30, Color{220, 180, 50, 200});
    DrawLine(cx - 24, vis_y + 31, cx + 24, vis_y + 31, Color{220, 180, 50, 100});

    draw_text("A", static_cast<float>(cx - 55), static_cast<float>(vis_y + 24), 12, ui_colors::TEXT_PRIMARY);
    draw_text("B", static_cast<float>(cx + 45), static_cast<float>(vis_y + 24), 12, ui_colors::TEXT_PRIMARY);
    draw_text("Entangled", static_cast<float>(cx - 30), static_cast<float>(vis_y + 58), 11, Color{220, 180, 50, 255});

    draw_text("Concurrence = 1.0 (maximally entangled)", static_cast<float>(vp_x + 30),
              static_cast<float>(vis_y + 80), 12, ui_colors::SUCCESS);
}

void EntanglementScenario::render_correlation(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("QUANTUM vs CLASSICAL CORRELATIONS", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Bell's inequality proves quantum correlations exceed classical limits",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int plot_x = vp_x + 70;
    int plot_w = vp_w - 120;
    int plot_y = vp_y + 55;
    int plot_h = vp_h - 160;
    int cy = plot_y + plot_h / 2;
    int baseline = plot_y + plot_h;

    DrawLine(plot_x, cy, plot_x + plot_w, cy, Color{60, 60, 70, 150});
    DrawLine(plot_x, plot_y, plot_x, baseline, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, baseline, plot_x + plot_w, baseline, ui_colors::TEXT_SECONDARY);
    draw_text("\xCE\xB8 (degrees)", static_cast<float>(plot_x + plot_w / 2 - 30), static_cast<float>(baseline + 20), 11, ui_colors::TEXT_SECONDARY);
    draw_text("E(\xCE\xB8)", static_cast<float>(plot_x - 35), static_cast<float>(plot_y - 2), 11, ui_colors::TEXT_SECONDARY);

    draw_text("+1", static_cast<float>(plot_x - 20), static_cast<float>(plot_y), 9, ui_colors::TEXT_SECONDARY);
    draw_text("-1", static_cast<float>(plot_x - 20), static_cast<float>(baseline - 10), 9, ui_colors::TEXT_SECONDARY);
    draw_text("0", static_cast<float>(plot_x - 12), static_cast<float>(cy - 5), 9, ui_colors::TEXT_SECONDARY);

    int prev_qx = 0, prev_qy = 0;
    int prev_cx_cl = 0, prev_cy_cl = 0;
    for (int i = 0; i <= plot_w; ++i) {
        double angle = PI * static_cast<double>(i) / plot_w;

        double q_corr = -std::cos(2.0 * angle);
        int qx = plot_x + i;
        int qy = cy - static_cast<int>(q_corr * (plot_h / 2) * 0.9);
        if (i > 0) DrawLine(prev_qx, prev_qy, qx, qy, ui_colors::ACCENT);
        prev_qx = qx;
        prev_qy = qy;

        double c_corr;
        double a_norm = angle / PI;
        if (a_norm < 0.5) c_corr = 1.0 - 4.0 * a_norm;
        else c_corr = -1.0 + 4.0 * (a_norm - 0.5);
        int clx = plot_x + i;
        int cly = cy - static_cast<int>(c_corr * (plot_h / 2) * 0.9);
        if (i > 0) DrawLine(prev_cx_cl, prev_cy_cl, clx, cly, Color{200, 100, 100, 150});
        prev_cx_cl = clx;
        prev_cy_cl = cly;
    }

    double cur_angle = std::abs(alice_angle_rad() - bob_angle_rad());
    if (cur_angle > PI) cur_angle = 2.0 * PI - cur_angle;
    double cur_corr = quantum_correlation(cur_angle);
    double cur_frac = cur_angle / PI;
    int dot_x = plot_x + static_cast<int>(cur_frac * plot_w);
    int dot_y = cy - static_cast<int>(cur_corr * (plot_h / 2) * 0.9);
    DrawCircle(dot_x, dot_y, 6, Color{255, 220, 50, 255});
    DrawLine(dot_x, cy, dot_x, dot_y, Color{255, 220, 50, 100});

    draw_text(TextFormat("E = %.3f", cur_corr), static_cast<float>(dot_x + 10),
              static_cast<float>(dot_y - 10), 11, Color{255, 220, 50, 255});

    for (int t = 0; t <= 4; ++t) {
        int tx = plot_x + t * plot_w / 4;
        DrawLine(tx, baseline, tx, baseline + 4, ui_colors::TEXT_SECONDARY);
        draw_text(TextFormat("%d", t * 45), static_cast<float>(tx - 6), static_cast<float>(baseline + 6), 9, ui_colors::TEXT_SECONDARY);
    }

    int legend_y = baseline + 35;
    DrawLine(vp_x + 40, legend_y, vp_x + 60, legend_y, ui_colors::ACCENT);
    draw_text("Quantum (cos 2\xCE\xB8)", static_cast<float>(vp_x + 65), static_cast<float>(legend_y - 5), 11, ui_colors::ACCENT);

    DrawLine(vp_x + 40, legend_y + 16, vp_x + 60, legend_y + 16, Color{200, 100, 100, 200});
    draw_text("Classical (hidden variables)", static_cast<float>(vp_x + 65), static_cast<float>(legend_y + 11), 11, Color{200, 100, 100, 255});

    double S_quantum = 2.0 * std::sqrt(2.0);
    draw_text(TextFormat("CHSH: S_quantum = %.2f > 2.0 (classical max)", S_quantum),
              static_cast<float>(vp_x + 40), static_cast<float>(legend_y + 35), 13, ui_colors::SUCCESS);
    draw_text("BELL INEQUALITY VIOLATED", static_cast<float>(vp_x + 40),
              static_cast<float>(legend_y + 53), 14, Color{255, 220, 50, 255});
}

void EntanglementScenario::render_creation(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("ENTANGLEMENT CREATION (Quantum Circuit)", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    int cx = vp_x + vp_w / 2;
    int wire_y1 = vp_y + vp_h / 3;
    int wire_y2 = vp_y + vp_h * 2 / 3;
    int left_x = vp_x + 80;
    int right_x = vp_x + vp_w - 80;

    draw_text("|0\xE2\x9F\xA9", static_cast<float>(left_x - 30), static_cast<float>(wire_y1 - 7), 13, ui_colors::TEXT_PRIMARY);
    draw_text("|0\xE2\x9F\xA9", static_cast<float>(left_x - 30), static_cast<float>(wire_y2 - 7), 13, ui_colors::TEXT_PRIMARY);

    DrawLine(left_x, wire_y1, right_x, wire_y1, ui_colors::TEXT_SECONDARY);
    DrawLine(left_x, wire_y2, right_x, wire_y2, ui_colors::TEXT_SECONDARY);

    int h_gate_x = left_x + (right_x - left_x) / 3;
    Color h_color = (creation_step_ >= 1) ? ui_colors::ACCENT : ui_colors::PANEL_HOVER;
    DrawRectangle(h_gate_x - 18, wire_y1 - 18, 36, 36, h_color);
    DrawRectangleLinesEx({static_cast<float>(h_gate_x - 18), static_cast<float>(wire_y1 - 18), 36.0f, 36.0f},
                         1.0f, ui_colors::ACCENT);
    draw_text("H", static_cast<float>(h_gate_x - 5), static_cast<float>(wire_y1 - 8), 16, ui_colors::TEXT_PRIMARY);

    int cnot_x = left_x + 2 * (right_x - left_x) / 3;
    Color cnot_color = (creation_step_ >= 2) ? Color{100, 200, 100, 255} : ui_colors::PANEL_HOVER;
    DrawCircle(cnot_x, wire_y1, 6, cnot_color);
    DrawLine(cnot_x, wire_y1, cnot_x, wire_y2, cnot_color);
    DrawCircle(cnot_x, wire_y2, 12, Color{0, 0, 0, 0});
    DrawCircleLines(cnot_x, wire_y2, 12, cnot_color);
    DrawLine(cnot_x - 12, wire_y2, cnot_x + 12, wire_y2, cnot_color);
    DrawLine(cnot_x, wire_y2 - 12, cnot_x, wire_y2 + 12, cnot_color);

    const char* state_text;
    const char* concurrence_text;
    Color state_color;

    switch (creation_step_) {
        case 0:
            state_text = "|00\xE2\x9F\xA9";
            concurrence_text = "C = 0 (separable)";
            state_color = ui_colors::TEXT_PRIMARY;
            break;
        case 1:
            state_text = "(|0\xE2\x9F\xA9+|1\xE2\x9F\xA9)|0\xE2\x9F\xA9 / \xE2\x88\x9A""2";
            concurrence_text = "C = 0 (still separable!)";
            state_color = ui_colors::ACCENT;
            break;
        case 2:
        default:
            state_text = "(|00\xE2\x9F\xA9+|11\xE2\x9F\xA9) / \xE2\x88\x9A""2 = |\xCE\xA6\xE2\x81\xBA\xE2\x9F\xA9";
            concurrence_text = "C = 1.0 (MAXIMALLY ENTANGLED!)";
            state_color = Color{255, 220, 50, 255};
            break;
    }

    int state_y = wire_y2 + 50;
    draw_text("Current state:", static_cast<float>(vp_x + 40), static_cast<float>(state_y), 12, ui_colors::TEXT_SECONDARY);
    draw_text(state_text, static_cast<float>(vp_x + 40), static_cast<float>(state_y + 18), 16, state_color);
    draw_text(concurrence_text, static_cast<float>(vp_x + 40), static_cast<float>(state_y + 42), 13,
              creation_step_ == 2 ? ui_colors::SUCCESS : ui_colors::TEXT_SECONDARY);

    int btn_y = state_y + 70;
    Rectangle step_btn = {static_cast<float>(cx - 50), static_cast<float>(btn_y), 100.0f, 28.0f};
    DrawRectangleRec(step_btn, ui_colors::ACCENT);
    DrawRectangleLinesEx(step_btn, 1.0f, ui_colors::PANEL_BORDER);
    const char* btn_text = creation_step_ >= 2 ? "Reset" : "Next Step";
    draw_text(btn_text, step_btn.x + 18, step_btn.y + 7, 12, ui_colors::TEXT_PRIMARY);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, step_btn)) {
            creation_step_++;
            if (creation_step_ > 2) creation_step_ = 0;
        }
    }

    if (creation_step_ >= 1) {
        float progress = (creation_step_ == 1) ? 0.33f : 0.67f;
        int marker_x = left_x + static_cast<int>(progress * (right_x - left_x));
        DrawLine(marker_x, wire_y1 - 30, marker_x, wire_y2 + 30, Color{255, 220, 50, 100});
    }

    draw_text("Step 1: Hadamard creates superposition (|0\xE2\x9F\xA9\xE2\x86\x92|+\xE2\x9F\xA9)",
              static_cast<float>(vp_x + 40), static_cast<float>(btn_y + 40), 11,
              creation_step_ >= 1 ? ui_colors::ACCENT : ui_colors::TEXT_SECONDARY);
    draw_text("Step 2: CNOT correlates qubits (creates entanglement!)",
              static_cast<float>(vp_x + 40), static_cast<float>(btn_y + 56), 11,
              creation_step_ >= 2 ? ui_colors::SUCCESS : ui_colors::TEXT_SECONDARY);
}

void EntanglementScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("ENTANGLEMENT", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    if (current_view_ == 0 || current_view_ == 2) {
        alice_angle_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
        bob_angle_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 8;
    }

    draw_text("Bell state:", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 16;
    const char* bell_names[] = {"|\xCE\xA6\xE2\x81\xBA\xE2\x9F\xA9", "|\xCE\xA6\xE2\x81\xBB\xE2\x9F\xA9",
                                 "|\xCE\xA8\xE2\x81\xBA\xE2\x9F\xA9", "|\xCE\xA8\xE2\x81\xBB\xE2\x9F\xA9"};
    for (int i = 0; i < 4; ++i) {
        Color bg = (i == bell_state_) ? ui_colors::ACCENT : ui_colors::PANEL_BG;
        DrawRectangle(cx, cy, w - 20, 22, bg);
        DrawRectangleLinesEx({static_cast<float>(cx), static_cast<float>(cy),
                              static_cast<float>(w - 20), 22.0f}, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(bell_names[i], static_cast<float>(cx + 6), static_cast<float>(cy + 4), 11,
                  (i == bell_state_) ? ui_colors::TEXT_PRIMARY : ui_colors::TEXT_SECONDARY);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (mouse.x >= cx && mouse.x < cx + w - 20 && mouse.y >= cy && mouse.y < cy + 22) {
                bell_state_ = i;
                epr_log_.clear();
                epr_same_ = 0;
                epr_diff_ = 0;
            }
        }
        cy += 24;
    }

    cy += 8;
    draw_separator(cx, cy, w - 20);
    cy += 4;

    draw_text("Two particles linked", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("regardless of distance.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("Measuring one instantly", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("determines the other.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);

    cy += 20;
    draw_text("Views: 1-4 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);

    cy += 20;
    if (HelpPopup::render_help_button(font_, has_font_, cx, cy)) {
        help_popup_.show({"\xCE\xA8", "Quantum Entanglement",
            "When two particles are entangled, measuring one instantly determines the state of the other, "
            "no matter how far apart they are. Einstein called this 'spooky action at a distance'. "
            "Bell's theorem (1964) proved that no classical hidden-variable theory can reproduce "
            "quantum correlations. The CHSH inequality S <= 2 is violated by quantum mechanics "
            "(S = 2\xE2\x88\x9A""2 \xE2\x89\x88 2.83). This has been confirmed experimentally.",
            "|\xCE\xA6\xE2\x81\xBA\xE2\x9F\xA9 = (|00\xE2\x9F\xA9+|11\xE2\x9F\xA9)/\xE2\x88\x9A""2",
            "CHSH: S = 2\xE2\x88\x9A""2 > 2"});
    }
}

void EntanglementScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    const char* bell_full[] = {
        "|\xCE\xA6\xE2\x81\xBA\xE2\x9F\xA9 (|00\xE2\x9F\xA9+|11\xE2\x9F\xA9)/\xE2\x88\x9A""2",
        "|\xCE\xA6\xE2\x81\xBB\xE2\x9F\xA9 (|00\xE2\x9F\xA9-|11\xE2\x9F\xA9)/\xE2\x88\x9A""2",
        "|\xCE\xA8\xE2\x81\xBA\xE2\x9F\xA9 (|01\xE2\x9F\xA9+|10\xE2\x9F\xA9)/\xE2\x88\x9A""2",
        "|\xCE\xA8\xE2\x81\xBB\xE2\x9F\xA9 (|01\xE2\x9F\xA9-|10\xE2\x9F\xA9)/\xE2\x88\x9A""2"
    };

    draw_section("Bell State", px, py);
    draw_text(bell_full[bell_state_], static_cast<float>(px), static_cast<float>(py), 11, ui_colors::ACCENT);
    py += 16;

    auto bp = get_bell_probs();
    draw_section("Probabilities", px, py);
    draw_prop("P(|00\xE2\x9F\xA9)", TextFormat("%.0f%%", bp.p00 * 100), px, py);
    draw_prop("P(|01\xE2\x9F\xA9)", TextFormat("%.0f%%", bp.p01 * 100), px, py);
    draw_prop("P(|10\xE2\x9F\xA9)", TextFormat("%.0f%%", bp.p10 * 100), px, py);
    draw_prop("P(|11\xE2\x9F\xA9)", TextFormat("%.0f%%", bp.p11 * 100), px, py);
    py += 4;

    draw_section("Entanglement", px, py);
    draw_prop("Concurrence", "1.000", px, py, ui_colors::SUCCESS);
    draw_prop("Entropy", "1.000", px, py);
    draw_prop("Purity", "1.000", px, py);
    py += 4;

    if (current_view_ == 0 || current_view_ == 2) {
        draw_section("Angles", px, py);
        draw_prop("Alice", TextFormat("%.0f\xC2\xB0", alice_angle_slider_.get_value()), px, py, ui_colors::WAVEFUNCTION);
        draw_prop("Bob", TextFormat("%.0f\xC2\xB0", bob_angle_slider_.get_value()), px, py, Color{220, 160, 90, 255});
        double diff = std::abs(alice_angle_rad() - bob_angle_rad());
        if (diff > PI) diff = 2.0 * PI - diff;
        draw_prop("\xCE\x94\xCE\xB8", TextFormat("%.0f\xC2\xB0", diff * 180.0 / PI), px, py);
        py += 4;

        draw_section("Correlation", px, py);
        double corr = quantum_correlation(diff);
        draw_prop("E(\xCE\xB8)", TextFormat("%.4f", corr), px, py, ui_colors::ACCENT);
    }

    py += 12;
    draw_separator(px, py, w - 24);
    draw_section("Key Fact", px, py);
    draw_text("No information travels", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("faster than light.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("Correlations are real,", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("but not communication.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::ACCENT);
}
