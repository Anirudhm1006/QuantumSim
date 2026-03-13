#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "SpinScenario.hpp"

SpinScenario::SpinScenario()
    : theta_slider_("\xCE\xB8 (polar)", 0.0f, 3.14159f, 0.0f, "%.2f rad")
    , phi_slider_("\xCF\x86 (azimuthal)", 0.0f, 6.28318f, 0.0f, "%.2f rad")
    , sg_angle_slider_("Magnet axis", 0.0f, 180.0f, 0.0f, "%.0f\xC2\xB0")
{}

void SpinScenario::on_enter() {
    current_view_ = 0;
    sg_particles_.clear();
    seq_particles_.clear();
    stats_up_ = 0;
    stats_down_ = 0;
    sg_spawn_timer_ = 0.0;
    seq_spawn_timer_ = 0.0;
    seq_final_up_ = 0;
    seq_final_down_ = 0;
    stat_run_count_ = 0;
    stat_results_up_ = 0;
    stat_results_down_ = 0;
}

double SpinScenario::prob_up() const {
    double measure_angle = sg_angle() * PI / 180.0;
    double angle_diff = theta() - measure_angle;
    return std::cos(angle_diff / 2.0) * std::cos(angle_diff / 2.0);
}

double SpinScenario::prob_down() const {
    return 1.0 - prob_up();
}

const char* SpinScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Stern-Gerlach";
        case 1: return "Bloch Sphere";
        case 2: return "Sequential";
        case 3: return "Statistics";
        default: return "Unknown";
    }
}

void SpinScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
}

void SpinScenario::update(double dt) {
    if (current_view_ == 0) {
        sg_spawn_timer_ += dt;
        if (sg_spawn_timer_ > 0.3) {
            sg_spawn_timer_ = 0.0;
            SGParticle p;
            p.x = 0.0f;
            p.y = 0.5f;
            p.measured = false;
            p.result = 0;
            std::uniform_real_distribution<double> coin(0.0, 1.0);
            p.result = (coin(rng_) < prob_up()) ? 1 : -1;
            sg_particles_.push_back(p);
        }

        float magnet_x = 0.4f;
        for (auto& p : sg_particles_) {
            p.x += static_cast<float>(dt) * 0.3f;
            if (!p.measured && p.x > magnet_x) {
                p.measured = true;
                if (p.result == 1) stats_up_++;
                else stats_down_++;
            }
            if (p.measured) {
                float target_y = (p.result == 1) ? 0.25f : 0.75f;
                p.y += (target_y - p.y) * static_cast<float>(dt) * 3.0f;
            }
        }

        sg_particles_.erase(
            std::remove_if(sg_particles_.begin(), sg_particles_.end(),
                           [](const SGParticle& p) { return p.x > 1.1f; }),
            sg_particles_.end());
    }

    if (current_view_ == 2) {
        seq_spawn_timer_ += dt;
        if (seq_spawn_timer_ > 0.5) {
            seq_spawn_timer_ = 0.0;
            SeqParticle sp;
            sp.x = 0.0f;
            sp.stage = 0;
            sp.result1 = sp.result2 = sp.result3 = 0;
            sp.y_offset = 0.0f;

            std::uniform_real_distribution<double> coin(0.0, 1.0);
            double p_up1 = std::cos(theta() / 2.0) * std::cos(theta() / 2.0);
            sp.result1 = (coin(rng_) < p_up1) ? 1 : -1;

            if (seq_filter2_enabled_) {
                sp.result2 = (coin(rng_) < 0.5) ? 1 : -1;
            }

            double p_up3 = seq_filter2_enabled_ ? 0.5 : p_up1;
            sp.result3 = (coin(rng_) < p_up3) ? 1 : -1;

            seq_particles_.push_back(sp);
        }

        for (auto& sp : seq_particles_) {
            sp.x += static_cast<float>(dt) * 0.2f;

            if (sp.x > 0.25f && sp.stage == 0) {
                sp.stage = 1;
                if (sp.result1 == -1) sp.stage = -1;
            }
            if (sp.x > 0.5f && sp.stage == 1 && seq_filter2_enabled_) {
                sp.stage = 2;
            }
            if (sp.x > 0.5f && sp.stage == 1 && !seq_filter2_enabled_) {
                sp.stage = 3;
            }
            if (sp.x > 0.75f && sp.stage == 2) {
                sp.stage = 3;
            }

            if (sp.stage == 1) sp.y_offset += (sp.result1 == 1 ? -0.02f : 0.02f) * static_cast<float>(dt);
            if (sp.stage == 3 && sp.x > 0.85f) {
                if (sp.result3 == 1) seq_final_up_++;
                else seq_final_down_++;
                sp.stage = 4;
            }
        }

        seq_particles_.erase(
            std::remove_if(seq_particles_.begin(), seq_particles_.end(),
                           [](const SeqParticle& sp) { return sp.x > 1.1f || sp.stage == -1; }),
            seq_particles_.end());
    }
}

void SpinScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    switch (current_view_) {
        case 0: render_stern_gerlach(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_bloch_sphere(cam, vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_sequential(vp_x, vp_y, vp_w, vp_h); break;
        case 3: render_statistics(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void SpinScenario::draw_magnet(int cx, int cy, int w, int h, float angle_deg, const char* label) {
    (void)angle_deg;
    DrawRectangle(cx - w / 2, cy - h / 2, w, h / 2, Color{200, 60, 60, 200});
    DrawRectangle(cx - w / 2, cy, w, h / 2, Color{60, 60, 200, 200});
    draw_text("N", static_cast<float>(cx - 4), static_cast<float>(cy - h / 2 + 2), 10, WHITE);
    draw_text("S", static_cast<float>(cx - 4), static_cast<float>(cy + 2), 10, WHITE);
    if (label) {
        draw_text(label, static_cast<float>(cx - w / 2), static_cast<float>(cy + h / 2 + 3), 9, ui_colors::TEXT_SECONDARY);
    }
}

void SpinScenario::render_stern_gerlach(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("STERN-GERLACH EXPERIMENT", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Spin-1/2 particles split into exactly TWO beams", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int area_x = vp_x + 30;
    int area_y = vp_y + 50;
    int area_w = vp_w - 60;
    int area_h = vp_h - 130;

    DrawRectangle(area_x, area_y, area_w, area_h, Color{20, 20, 28, 255});
    DrawRectangleLinesEx({static_cast<float>(area_x), static_cast<float>(area_y),
                          static_cast<float>(area_w), static_cast<float>(area_h)}, 1.0f, ui_colors::PANEL_BORDER);

    DrawRectangle(area_x + 5, area_y + area_h / 2 - 3, 30, 6, Color{160, 160, 60, 200});
    draw_text("source", static_cast<float>(area_x + 5), static_cast<float>(area_y + area_h / 2 + 6), 9, ui_colors::TEXT_SECONDARY);

    int magnet_cx = area_x + static_cast<int>(0.4f * area_w);
    int magnet_cy = area_y + area_h / 2;
    draw_magnet(magnet_cx, magnet_cy, 40, 80, static_cast<float>(sg_angle()), "magnet");

    int screen_x = area_x + area_w - 20;
    DrawRectangle(screen_x, area_y + 20, 8, area_h - 40, Color{60, 80, 60, 200});

    int up_y = area_y + static_cast<int>(0.25f * area_h);
    int down_y = area_y + static_cast<int>(0.75f * area_h);
    DrawCircle(screen_x + 4, up_y, 5, Color{100, 200, 100, 150});
    DrawCircle(screen_x + 4, down_y, 5, Color{200, 100, 100, 150});
    draw_text("\xE2\x86\x91", static_cast<float>(screen_x + 12), static_cast<float>(up_y - 6), 12, Color{100, 200, 100, 255});
    draw_text("\xE2\x86\x93", static_cast<float>(screen_x + 12), static_cast<float>(down_y - 6), 12, Color{200, 100, 100, 255});

    for (const auto& p : sg_particles_) {
        int px = area_x + static_cast<int>(p.x * area_w);
        int py = area_y + static_cast<int>(p.y * area_h);
        Color pc = p.measured ? (p.result == 1 ? Color{100, 200, 100, 200} : Color{200, 100, 100, 200})
                              : Color{180, 180, 60, 200};
        DrawCircle(px, py, 3, pc);
    }

    int info_y = area_y + area_h + 10;
    int total = stats_up_ + stats_down_;
    if (total > 0) {
        float frac_up = static_cast<float>(stats_up_) / total;
        float frac_down = static_cast<float>(stats_down_) / total;

        int bar_w = area_w / 2;
        int bar_h = 18;
        int bar_x = area_x + area_w / 4;

        DrawRectangle(bar_x, info_y, static_cast<int>(bar_w * frac_up), bar_h, Color{100, 200, 100, 180});
        DrawRectangle(bar_x + static_cast<int>(bar_w * frac_up), info_y,
                      static_cast<int>(bar_w * frac_down), bar_h, Color{200, 100, 100, 180});

        draw_text(TextFormat("\xE2\x86\x91 %d (%.1f%%)", stats_up_, frac_up * 100.0f),
                  static_cast<float>(area_x), static_cast<float>(info_y + 22), 12, Color{100, 200, 100, 255});
        draw_text(TextFormat("\xE2\x86\x93 %d (%.1f%%)", stats_down_, frac_down * 100.0f),
                  static_cast<float>(area_x + area_w / 2), static_cast<float>(info_y + 22), 12, Color{200, 100, 100, 255});
        draw_text(TextFormat("Expected: P(\xE2\x86\x91) = %.1f%%   P(\xE2\x86\x93) = %.1f%%",
                  prob_up() * 100.0, prob_down() * 100.0),
                  static_cast<float>(area_x), static_cast<float>(info_y + 40), 11, ui_colors::TEXT_SECONDARY);
    }
}

void SpinScenario::render_bloch_sphere(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)vp_x; (void)vp_y; (void)vp_w; (void)vp_h;

    BeginMode3D(cam);
    {
        for (int i = 0; i < 64; ++i) {
            float a1 = 2.0f * PI * i / 64.0f;
            float a2 = 2.0f * PI * (i + 1) / 64.0f;
            DrawLine3D({std::cos(a1) * 3.0f, 0, std::sin(a1) * 3.0f},
                       {std::cos(a2) * 3.0f, 0, std::sin(a2) * 3.0f}, Color{60, 60, 70, 200});
            DrawLine3D({std::cos(a1) * 3.0f, std::sin(a1) * 3.0f, 0},
                       {std::cos(a2) * 3.0f, std::sin(a2) * 3.0f, 0}, Color{60, 60, 70, 200});
            DrawLine3D({0, std::cos(a1) * 3.0f, std::sin(a1) * 3.0f},
                       {0, std::cos(a2) * 3.0f, std::sin(a2) * 3.0f}, Color{60, 60, 70, 200});
        }

        DrawLine3D({0, -3.5f, 0}, {0, 3.5f, 0}, ui_colors::AXIS_Y);
        DrawLine3D({-3.5f, 0, 0}, {3.5f, 0, 0}, ui_colors::AXIS_X);
        DrawLine3D({0, 0, -3.5f}, {0, 0, 3.5f}, ui_colors::AXIS_Z);

        DrawSphere({0, 3.0f, 0}, 0.15f, Color{100, 200, 100, 255});
        DrawSphere({0, -3.0f, 0}, 0.15f, Color{200, 100, 100, 255});

        float th = static_cast<float>(theta());
        float ph = static_cast<float>(phi());
        float nx = 3.0f * std::sin(th) * std::cos(ph);
        float ny = 3.0f * std::cos(th);
        float nz = 3.0f * std::sin(th) * std::sin(ph);

        DrawLine3D({0, 0, 0}, {nx, ny, nz}, ui_colors::ACCENT);
        DrawSphere({nx, ny, nz}, 0.2f, ui_colors::ACCENT);
    }
    EndMode3D();

    draw_text("BLOCH SPHERE", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("|\xE2\x86\x91\xE2\x9F\xA9 = North    |\xE2\x86\x93\xE2\x9F\xA9 = South",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int info_y = vp_y + vp_h - 70;
    draw_text(TextFormat("|\xCF\x88\xE2\x9F\xA9 = cos(\xCE\xB8/2)|\xE2\x86\x91\xE2\x9F\xA9 + e^(i\xCF\x86)sin(\xCE\xB8/2)|\xE2\x86\x93\xE2\x9F\xA9"),
              static_cast<float>(vp_x + 10), static_cast<float>(info_y), 12, ui_colors::TEXT_PRIMARY);
    draw_text(TextFormat("P(\xE2\x86\x91) = %.3f    P(\xE2\x86\x93) = %.3f",
              std::cos(theta() / 2.0) * std::cos(theta() / 2.0),
              std::sin(theta() / 2.0) * std::sin(theta() / 2.0)),
              static_cast<float>(vp_x + 10), static_cast<float>(info_y + 18), 12, ui_colors::ACCENT);

    float bloch_x = static_cast<float>(std::sin(theta()) * std::cos(phi()));
    float bloch_y = static_cast<float>(std::sin(theta()) * std::sin(phi()));
    float bloch_z = static_cast<float>(std::cos(theta()));
    draw_text(TextFormat("Bloch: (%.2f, %.2f, %.2f)", bloch_x, bloch_y, bloch_z),
              static_cast<float>(vp_x + 10), static_cast<float>(info_y + 36), 11, ui_colors::TEXT_SECONDARY);
}

void SpinScenario::render_sequential(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("SEQUENTIAL STERN-GERLACH", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Measuring one axis randomizes the other",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int area_x = vp_x + 20;
    int area_y = vp_y + 50;
    int area_w = vp_w - 40;
    int area_h = vp_h - 150;
    int cy = area_y + area_h / 2;

    DrawRectangle(area_x, area_y, area_w, area_h, Color{20, 20, 28, 255});
    DrawRectangleLinesEx({static_cast<float>(area_x), static_cast<float>(area_y),
                          static_cast<float>(area_w), static_cast<float>(area_h)}, 1.0f, ui_colors::PANEL_BORDER);

    const char* axis_names[] = {"Z", "X", "Y"};

    int m1_x = area_x + static_cast<int>(0.25f * area_w);
    draw_magnet(m1_x, cy, 30, 50, 0, TextFormat("Filter 1: %s\xE2\x86\x91", axis_names[seq_filter1_axis_]));

    if (seq_filter2_enabled_) {
        int m2_x = area_x + static_cast<int>(0.5f * area_w);
        draw_magnet(m2_x, cy, 30, 50, 90, TextFormat("Filter 2: %s", axis_names[seq_filter2_axis_]));
    }

    int m3_x = area_x + static_cast<int>(0.75f * area_w);
    draw_magnet(m3_x, cy, 30, 50, 0, TextFormat("Filter 3: %s ?", axis_names[seq_filter3_axis_]));

    for (const auto& sp : seq_particles_) {
        int px = area_x + static_cast<int>(sp.x * area_w);
        int py = cy + static_cast<int>(sp.y_offset * area_h);
        Color pc = Color{180, 180, 60, 200};
        if (sp.stage >= 3) {
            pc = (sp.result3 == 1) ? Color{100, 200, 100, 200} : Color{200, 100, 100, 200};
        }
        DrawCircle(px, py, 3, pc);
    }

    int info_y = area_y + area_h + 15;
    int total_seq = seq_final_up_ + seq_final_down_;
    if (total_seq > 0) {
        draw_text(TextFormat("Final %s measurement: \xE2\x86\x91 %d (%.1f%%)  \xE2\x86\x93 %d (%.1f%%)",
                  axis_names[seq_filter3_axis_],
                  seq_final_up_, 100.0f * seq_final_up_ / total_seq,
                  seq_final_down_, 100.0f * seq_final_down_ / total_seq),
                  static_cast<float>(area_x), static_cast<float>(info_y), 12, ui_colors::ACCENT);
    }

    if (seq_filter2_enabled_) {
        draw_text(TextFormat("%s \xE2\x86\x92 %s \xE2\x86\x92 %s: middle measurement erases first!",
                  axis_names[seq_filter1_axis_], axis_names[seq_filter2_axis_], axis_names[seq_filter3_axis_]),
                  static_cast<float>(area_x), static_cast<float>(info_y + 20), 11, ui_colors::TEXT_SECONDARY);
        draw_text("Result: ~50/50 (information about first axis is lost)",
                  static_cast<float>(area_x), static_cast<float>(info_y + 34), 11, Color{220, 160, 90, 255});
    } else {
        draw_text(TextFormat("%s \xE2\x86\x92 %s: same axis, result preserved",
                  axis_names[seq_filter1_axis_], axis_names[seq_filter3_axis_]),
                  static_cast<float>(area_x), static_cast<float>(info_y + 20), 11, ui_colors::TEXT_SECONDARY);
        draw_text("Result: 100% spin-up (no new information gained)",
                  static_cast<float>(area_x), static_cast<float>(info_y + 34), 11, ui_colors::SUCCESS);
    }
}

void SpinScenario::render_statistics(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("MEASUREMENT STATISTICS", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Quantum mechanics predicts probabilities, not outcomes",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int cx = vp_x + vp_w / 2;
    int chart_y = vp_y + 70;
    int chart_h = vp_h - 200;
    int bar_w = 80;

    double p_up = prob_up();
    double p_down = prob_down();

    int total = stat_results_up_ + stat_results_down_;
    float meas_up = total > 0 ? static_cast<float>(stat_results_up_) / total : 0.0f;
    float meas_down = total > 0 ? static_cast<float>(stat_results_down_) / total : 0.0f;

    int baseline = chart_y + chart_h;

    int bar1_x = cx - bar_w - 40;
    int h_pred_up = static_cast<int>(p_up * chart_h);
    DrawRectangle(bar1_x, baseline - h_pred_up, bar_w / 2, h_pred_up, Color{100, 200, 100, 80});
    DrawRectangleLinesEx({static_cast<float>(bar1_x), static_cast<float>(baseline - h_pred_up),
                          static_cast<float>(bar_w / 2), static_cast<float>(h_pred_up)}, 1.0f, Color{100, 200, 100, 200});

    int h_meas_up = static_cast<int>(meas_up * chart_h);
    DrawRectangle(bar1_x + bar_w / 2, baseline - h_meas_up, bar_w / 2, h_meas_up, Color{100, 200, 100, 180});
    draw_text("Spin \xE2\x86\x91", static_cast<float>(bar1_x), static_cast<float>(baseline + 5), 12, Color{100, 200, 100, 255});

    int bar2_x = cx + 40;
    int h_pred_down = static_cast<int>(p_down * chart_h);
    DrawRectangle(bar2_x, baseline - h_pred_down, bar_w / 2, h_pred_down, Color{200, 100, 100, 80});
    DrawRectangleLinesEx({static_cast<float>(bar2_x), static_cast<float>(baseline - h_pred_down),
                          static_cast<float>(bar_w / 2), static_cast<float>(h_pred_down)}, 1.0f, Color{200, 100, 100, 200});

    int h_meas_down = static_cast<int>(meas_down * chart_h);
    DrawRectangle(bar2_x + bar_w / 2, baseline - h_meas_down, bar_w / 2, h_meas_down, Color{200, 100, 100, 180});
    draw_text("Spin \xE2\x86\x93", static_cast<float>(bar2_x), static_cast<float>(baseline + 5), 12, Color{200, 100, 100, 255});

    DrawLine(vp_x + 40, baseline, vp_x + vp_w - 40, baseline, ui_colors::TEXT_SECONDARY);

    draw_text("Prediction", static_cast<float>(vp_x + 40), static_cast<float>(baseline + 22), 10, Color{100, 100, 200, 200});
    draw_text("Measured", static_cast<float>(vp_x + 120), static_cast<float>(baseline + 22), 10, ui_colors::TEXT_PRIMARY);

    int btn_y = baseline + 45;
    int btn_w = 80;

    Rectangle run1_rect = {static_cast<float>(cx - btn_w - 50), static_cast<float>(btn_y), static_cast<float>(btn_w), 28.0f};
    DrawRectangleRec(run1_rect, ui_colors::PANEL_HOVER);
    DrawRectangleLinesEx(run1_rect, 1.0f, ui_colors::ACCENT);
    draw_text("Run 1", run1_rect.x + 20, run1_rect.y + 7, 12, ui_colors::TEXT_PRIMARY);

    Rectangle run100_rect = {static_cast<float>(cx - 30), static_cast<float>(btn_y), static_cast<float>(btn_w), 28.0f};
    DrawRectangleRec(run100_rect, ui_colors::PANEL_HOVER);
    DrawRectangleLinesEx(run100_rect, 1.0f, ui_colors::ACCENT);
    draw_text("Run 100", run100_rect.x + 14, run100_rect.y + 7, 12, ui_colors::TEXT_PRIMARY);

    Rectangle reset_rect = {static_cast<float>(cx + btn_w - 10), static_cast<float>(btn_y), static_cast<float>(btn_w), 28.0f};
    DrawRectangleRec(reset_rect, ui_colors::PANEL_HOVER);
    DrawRectangleLinesEx(reset_rect, 1.0f, ui_colors::DANGER);
    draw_text("Reset", reset_rect.x + 22, reset_rect.y + 7, 12, ui_colors::TEXT_PRIMARY);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, run1_rect)) {
            std::uniform_real_distribution<double> coin(0.0, 1.0);
            if (coin(rng_) < prob_up()) stat_results_up_++;
            else stat_results_down_++;
            stat_run_count_++;
        }
        if (CheckCollisionPointRec(mouse, run100_rect)) {
            std::uniform_real_distribution<double> coin(0.0, 1.0);
            for (int i = 0; i < 100; ++i) {
                if (coin(rng_) < prob_up()) stat_results_up_++;
                else stat_results_down_++;
            }
            stat_run_count_ += 100;
        }
        if (CheckCollisionPointRec(mouse, reset_rect)) {
            stat_run_count_ = 0;
            stat_results_up_ = 0;
            stat_results_down_ = 0;
        }
    }

    draw_text(TextFormat("Runs: %d    \xE2\x86\x91: %d    \xE2\x86\x93: %d", stat_run_count_, stat_results_up_, stat_results_down_),
              static_cast<float>(vp_x + 40), static_cast<float>(btn_y + 35), 12, ui_colors::TEXT_PRIMARY);
}

void SpinScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("SPIN-1/2", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    bool th_changed = theta_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 4;
    bool ph_changed = phi_slider_.render(font_, has_font_, cx, cy, w - 20);
    cy += Slider::HEIGHT + 4;

    if (current_view_ == 0 || current_view_ == 3) {
        sg_angle_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
    }

    if (th_changed || ph_changed) {
        stats_up_ = 0;
        stats_down_ = 0;
        sg_particles_.clear();
        stat_results_up_ = 0;
        stat_results_down_ = 0;
        stat_run_count_ = 0;
    }

    draw_separator(cx, cy, w - 20);
    cy += 4;

    if (current_view_ == 2) {
        draw_text("Middle filter:", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
        cy += 16;

        Rectangle toggle_rect = {static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(w - 20), 22.0f};
        DrawRectangleRec(toggle_rect, seq_filter2_enabled_ ? ui_colors::ACCENT : ui_colors::PANEL_HOVER);
        DrawRectangleLinesEx(toggle_rect, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(seq_filter2_enabled_ ? "X-filter ON" : "X-filter OFF",
                  static_cast<float>(cx + 6), static_cast<float>(cy + 5), 11, ui_colors::TEXT_PRIMARY);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, toggle_rect)) {
                seq_filter2_enabled_ = !seq_filter2_enabled_;
                seq_particles_.clear();
                seq_final_up_ = 0;
                seq_final_down_ = 0;
            }
        }
        cy += 28;
    }

    draw_text("Spin is NOT rotation.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("It is an intrinsic quantum", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("property with only 2 values.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 24;

    draw_text("Views: 1-4 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);

    cy += 20;
    if (HelpPopup::render_help_button(font_, has_font_, cx, cy)) {
        help_popup_.show({"\xCF\x83", "Quantum Spin",
            "Spin is an intrinsic form of angular momentum carried by quantum particles. "
            "Unlike classical rotation, spin is quantized: spin-1/2 particles (electrons, protons, neutrons) "
            "can only be measured as 'spin up' (+\xE2\x84\x8F/2) or 'spin down' (-\xE2\x84\x8F/2) along any axis. "
            "Measuring spin along one axis completely randomizes the result along perpendicular axes.",
            "S = \xE2\x84\x8F/2 \xC2\xB7 \xCF\x83",
            "\xCF\x83 = Pauli matrices"});
    }
}

void SpinScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    draw_section("Spin State", px, py);
    draw_prop("\xCE\xB8", TextFormat("%.3f rad", theta()), px, py);
    draw_prop("\xCF\x86", TextFormat("%.3f rad", phi()), px, py);
    py += 4;

    draw_section("Bloch Vector", px, py);
    float nx = static_cast<float>(std::sin(theta()) * std::cos(phi()));
    float ny = static_cast<float>(std::sin(theta()) * std::sin(phi()));
    float nz = static_cast<float>(std::cos(theta()));
    draw_prop("nx", TextFormat("%.3f", nx), px, py);
    draw_prop("ny", TextFormat("%.3f", ny), px, py);
    draw_prop("nz", TextFormat("%.3f", nz), px, py);
    py += 4;

    draw_section("Probabilities", px, py);
    draw_prop("P(\xE2\x86\x91)", TextFormat("%.4f", prob_up()), px, py, Color{100, 200, 100, 255});
    draw_prop("P(\xE2\x86\x93)", TextFormat("%.4f", prob_down()), px, py, Color{200, 100, 100, 255});
    py += 4;

    draw_section("Expectation", px, py);
    draw_prop("<Sz>", TextFormat("%.3f \xE2\x84\x8F/2", nz), px, py);
    draw_prop("<Sx>", TextFormat("%.3f \xE2\x84\x8F/2", nx), px, py);
    draw_prop("<Sy>", TextFormat("%.3f \xE2\x84\x8F/2", ny), px, py);

    py += 8;
    draw_separator(px, py, w - 24);
    draw_section("Key Fact", px, py);
    draw_text("Classical: any angular", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("momentum value possible.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("Quantum: only \xC2\xB1\xE2\x84\x8F/2.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::ACCENT);
}
