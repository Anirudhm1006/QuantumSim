#include <cmath>
#include <algorithm>
#include <raylib.h>

#include "SuperpositionScenario.hpp"

SuperpositionScenario::SuperpositionScenario()
    : alpha_sq_slider_("|a|\xC2\xB2 (prob |0\xE2\x9F\xA9)", 0.0f, 1.0f, 0.5f, "%.2f")
    , phase_slider_("Relative phase", 0.0f, 6.28318f, 0.0f, "%.2f rad")
    , mix_slider_("n=2 fraction", 0.0f, 1.0f, 0.5f, "%.2f")
    , time_speed_slider_("Time speed", 0.1f, 5.0f, 1.0f, "%.1f")
{}

void SuperpositionScenario::on_enter() {
    current_view_ = 0;
    sim_time_ = 0.0;
    measure_count_0_ = 0;
    measure_count_1_ = 0;
    last_result_ = -1;
    collapse_flash_ = 0.0f;
    mz_photons_.clear();
    mz_spawn_timer_ = 0.0;
    mz_d1_count_ = 0;
    mz_d2_count_ = 0;
}

const char* SuperpositionScenario::get_view_name(int idx) const {
    switch (idx) {
        case 0: return "Two-State";
        case 1: return "Collapse";
        case 2: return "Mach-Zehnder";
        case 3: return "Quantum Beating";
        default: return "Unknown";
    }
}

void SuperpositionScenario::handle_input() {
    if (help_popup_.is_open()) {
        help_popup_.handle_input();
        return;
    }
}

void SuperpositionScenario::update(double dt) {
    sim_time_ += dt * static_cast<double>(time_speed_slider_.get_value());

    if (collapse_flash_ > 0.0f) {
        collapse_flash_ -= static_cast<float>(dt) * 2.0f;
        if (collapse_flash_ < 0.0f) collapse_flash_ = 0.0f;
    }

    if (current_view_ == 2) {
        mz_spawn_timer_ += dt;
        if (mz_spawn_timer_ > 0.4) {
            mz_spawn_timer_ = 0.0;
            MZPhoton p;
            p.x = 0.0f;
            p.path = 0;
            p.past_splitter1 = false;
            p.past_splitter2 = false;
            mz_photons_.push_back(p);
        }

        for (auto& p : mz_photons_) {
            p.x += static_cast<float>(dt) * 0.25f;

            if (!p.past_splitter1 && p.x > 0.3f) {
                p.past_splitter1 = true;
                if (mz_blocker_) {
                    std::uniform_int_distribution<int> coin(0, 1);
                    p.path = coin(rng_);
                }
            }

            if (!p.past_splitter2 && p.x > 0.7f) {
                p.past_splitter2 = true;
                if (mz_blocker_) {
                    std::uniform_int_distribution<int> coin(0, 1);
                    int detector = coin(rng_);
                    if (detector == 0) mz_d1_count_++;
                    else mz_d2_count_++;
                    p.path = detector + 10;
                } else {
                    mz_d1_count_++;
                    p.path = 10;
                }
            }
        }

        mz_photons_.erase(
            std::remove_if(mz_photons_.begin(), mz_photons_.end(),
                           [](const MZPhoton& p) { return p.x > 1.1f; }),
            mz_photons_.end());
    }
}

void SuperpositionScenario::render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) {
    (void)cam;
    switch (current_view_) {
        case 0: render_two_state(vp_x, vp_y, vp_w, vp_h); break;
        case 1: render_collapse(vp_x, vp_y, vp_w, vp_h); break;
        case 2: render_mach_zehnder(vp_x, vp_y, vp_w, vp_h); break;
        case 3: render_quantum_beat(vp_x, vp_y, vp_w, vp_h); break;
        default: break;
    }
    help_popup_.render(font_, has_font_, GetScreenWidth(), GetScreenHeight());
}

void SuperpositionScenario::render_two_state(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("TWO-STATE SUPERPOSITION", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);

    double a_sq = alpha_sq();
    double b_sq = beta_sq();
    double alpha = std::sqrt(a_sq);
    double beta = std::sqrt(b_sq);
    double phase = rel_phase();

    draw_text(TextFormat("|\xCF\x88\xE2\x9F\xA9 = %.2f|0\xE2\x9F\xA9 + %.2f e^(i%.1f)|1\xE2\x9F\xA9", alpha, beta, phase),
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 30), 13, ui_colors::TEXT_PRIMARY);

    int cx = vp_x + vp_w / 2;
    int cy = vp_y + vp_h / 2 - 20;

    int orb_r = std::min(vp_w, vp_h) / 6;
    int orb_spacing = orb_r * 3;

    int orb0_x = cx - orb_spacing / 2;
    int orb1_x = cx + orb_spacing / 2;

    auto alpha_byte = static_cast<unsigned char>(a_sq * 220 + 35);
    auto beta_byte = static_cast<unsigned char>(b_sq * 220 + 35);

    DrawCircle(orb0_x, cy, static_cast<float>(orb_r), Color{80, 160, 220, alpha_byte});
    DrawCircleLines(orb0_x, cy, static_cast<float>(orb_r), ui_colors::WAVEFUNCTION);
    draw_text("|0\xE2\x9F\xA9", static_cast<float>(orb0_x - 8), static_cast<float>(cy - 8), 16, ui_colors::TEXT_PRIMARY);

    DrawCircle(orb1_x, cy, static_cast<float>(orb_r), Color{220, 140, 80, beta_byte});
    DrawCircleLines(orb1_x, cy, static_cast<float>(orb_r), Color{220, 160, 90, 255});
    draw_text("|1\xE2\x9F\xA9", static_cast<float>(orb1_x - 8), static_cast<float>(cy - 8), 16, ui_colors::TEXT_PRIMARY);

    int bar_y = cy + orb_r + 30;
    int bar_w = vp_w - 80;
    int bar_h = 30;
    int bar_x = vp_x + 40;

    int w0 = static_cast<int>(a_sq * bar_w);
    DrawRectangle(bar_x, bar_y, w0, bar_h, Color{80, 160, 220, 180});
    DrawRectangle(bar_x + w0, bar_y, bar_w - w0, bar_h, Color{220, 140, 80, 180});
    DrawRectangleLinesEx({static_cast<float>(bar_x), static_cast<float>(bar_y),
                          static_cast<float>(bar_w), static_cast<float>(bar_h)}, 1.0f, ui_colors::PANEL_BORDER);

    draw_text(TextFormat("P(|0\xE2\x9F\xA9) = %.1f%%", a_sq * 100.0), static_cast<float>(bar_x),
              static_cast<float>(bar_y + bar_h + 5), 12, ui_colors::WAVEFUNCTION);
    draw_text(TextFormat("P(|1\xE2\x9F\xA9) = %.1f%%", b_sq * 100.0), static_cast<float>(bar_x + bar_w - 100),
              static_cast<float>(bar_y + bar_h + 5), 12, Color{220, 160, 90, 255});

    int circle_y = cy - orb_r - 50;
    int circle_r = 35;
    int circle_x = cx;
    DrawCircleLines(circle_x, circle_y, static_cast<float>(circle_r), ui_colors::TEXT_SECONDARY);

    float ax = static_cast<float>(circle_r) * static_cast<float>(alpha);
    float bx = static_cast<float>(circle_r) * static_cast<float>(beta) * std::cos(static_cast<float>(phase));
    float by = static_cast<float>(circle_r) * static_cast<float>(beta) * std::sin(static_cast<float>(phase));

    DrawLine(circle_x, circle_y, circle_x + static_cast<int>(ax), circle_y, Color{80, 160, 220, 200});
    DrawLine(circle_x, circle_y, circle_x + static_cast<int>(bx), circle_y - static_cast<int>(by), Color{220, 140, 80, 200});
    draw_text("amplitudes", static_cast<float>(circle_x - 25), static_cast<float>(circle_y + circle_r + 5), 9, ui_colors::TEXT_SECONDARY);
}

void SuperpositionScenario::render_collapse(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("MEASUREMENT & COLLAPSE", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Each click = one measurement. Build statistics!",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int cx = vp_x + vp_w / 2;
    int cy = vp_y + vp_h / 3;

    Color flash_0 = Color{80, 160, 220, 60};
    Color flash_1 = Color{220, 140, 80, 60};
    if (collapse_flash_ > 0.0f) {
        auto intensity = static_cast<unsigned char>(collapse_flash_ * 200);
        if (last_result_ == 0)
            flash_0 = Color{80, 160, 220, intensity};
        else
            flash_1 = Color{220, 140, 80, intensity};
    }

    int orb_r = 50;
    DrawCircle(cx - 80, cy, static_cast<float>(orb_r), flash_0);
    DrawCircleLines(cx - 80, cy, static_cast<float>(orb_r), ui_colors::WAVEFUNCTION);
    draw_text("|0\xE2\x9F\xA9", static_cast<float>(cx - 88), static_cast<float>(cy - 8), 16, ui_colors::TEXT_PRIMARY);

    DrawCircle(cx + 80, cy, static_cast<float>(orb_r), flash_1);
    DrawCircleLines(cx + 80, cy, static_cast<float>(orb_r), Color{220, 160, 90, 255});
    draw_text("|1\xE2\x9F\xA9", static_cast<float>(cx + 72), static_cast<float>(cy - 8), 16, ui_colors::TEXT_PRIMARY);

    if (last_result_ == 0 && collapse_flash_ > 0.0f) {
        draw_text("COLLAPSED TO |0\xE2\x9F\xA9", static_cast<float>(cx - 60), static_cast<float>(cy + orb_r + 10), 14, ui_colors::WAVEFUNCTION);
    } else if (last_result_ == 1 && collapse_flash_ > 0.0f) {
        draw_text("COLLAPSED TO |1\xE2\x9F\xA9", static_cast<float>(cx - 60), static_cast<float>(cy + orb_r + 10), 14, Color{220, 160, 90, 255});
    }

    int btn_y = cy + orb_r + 40;
    Rectangle measure_btn = {static_cast<float>(cx - 60), static_cast<float>(btn_y), 120.0f, 32.0f};
    DrawRectangleRec(measure_btn, ui_colors::ACCENT);
    DrawRectangleLinesEx(measure_btn, 1.0f, ui_colors::PANEL_BORDER);
    draw_text("MEASURE", measure_btn.x + 28, measure_btn.y + 9, 13, ui_colors::TEXT_PRIMARY);

    Rectangle run50_btn = {static_cast<float>(cx - 60), static_cast<float>(btn_y + 38), 55.0f, 26.0f};
    DrawRectangleRec(run50_btn, ui_colors::PANEL_HOVER);
    DrawRectangleLinesEx(run50_btn, 1.0f, ui_colors::ACCENT);
    draw_text("x50", run50_btn.x + 14, run50_btn.y + 6, 11, ui_colors::TEXT_PRIMARY);

    Rectangle reset_btn = {static_cast<float>(cx + 5), static_cast<float>(btn_y + 38), 55.0f, 26.0f};
    DrawRectangleRec(reset_btn, ui_colors::PANEL_HOVER);
    DrawRectangleLinesEx(reset_btn, 1.0f, ui_colors::DANGER);
    draw_text("Reset", reset_btn.x + 12, reset_btn.y + 6, 11, ui_colors::TEXT_PRIMARY);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        auto do_measure = [&]() {
            std::uniform_real_distribution<double> coin(0.0, 1.0);
            if (coin(rng_) < alpha_sq()) {
                measure_count_0_++;
                last_result_ = 0;
            } else {
                measure_count_1_++;
                last_result_ = 1;
            }
            collapse_flash_ = 1.0f;
        };

        if (CheckCollisionPointRec(mouse, measure_btn)) do_measure();
        if (CheckCollisionPointRec(mouse, run50_btn)) {
            for (int i = 0; i < 50; ++i) do_measure();
        }
        if (CheckCollisionPointRec(mouse, reset_btn)) {
            measure_count_0_ = 0;
            measure_count_1_ = 0;
            last_result_ = -1;
        }
    }

    int hist_y = btn_y + 75;
    int total = measure_count_0_ + measure_count_1_;
    if (total > 0) {
        int hist_w = vp_w - 80;
        int hist_h = 25;
        float f0 = static_cast<float>(measure_count_0_) / total;
        DrawRectangle(vp_x + 40, hist_y, static_cast<int>(f0 * hist_w), hist_h, Color{80, 160, 220, 180});
        DrawRectangle(vp_x + 40 + static_cast<int>(f0 * hist_w), hist_y,
                      hist_w - static_cast<int>(f0 * hist_w), hist_h, Color{220, 140, 80, 180});

        draw_text(TextFormat("|0\xE2\x9F\xA9: %d (%.1f%%)  |1\xE2\x9F\xA9: %d (%.1f%%)  Total: %d",
                  measure_count_0_, f0 * 100, measure_count_1_, (1 - f0) * 100, total),
                  static_cast<float>(vp_x + 40), static_cast<float>(hist_y + hist_h + 5), 11, ui_colors::TEXT_PRIMARY);
        draw_text(TextFormat("Expected: |0\xE2\x9F\xA9 %.1f%%  |1\xE2\x9F\xA9 %.1f%%", alpha_sq() * 100, beta_sq() * 100),
                  static_cast<float>(vp_x + 40), static_cast<float>(hist_y + hist_h + 20), 11, ui_colors::TEXT_SECONDARY);
    }
}

void SuperpositionScenario::render_mach_zehnder(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("MACH-ZEHNDER INTERFEROMETER", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text(mz_blocker_ ? "Path blocker ON: which-path info destroys interference"
                           : "Both paths: superposition \xE2\x86\x92 interference \xE2\x86\x92 100% D1",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11,
              mz_blocker_ ? ui_colors::DANGER : ui_colors::SUCCESS);

    int area_x = vp_x + 40;
    int area_y = vp_y + 55;
    int area_w = vp_w - 80;
    int area_h = vp_h - 170;

    DrawRectangle(area_x, area_y, area_w, area_h, Color{20, 20, 28, 255});
    DrawRectangleLinesEx({static_cast<float>(area_x), static_cast<float>(area_y),
                          static_cast<float>(area_w), static_cast<float>(area_h)}, 1.0f, ui_colors::PANEL_BORDER);

    int cy = area_y + area_h / 2;
    int top_y = area_y + area_h / 4;
    int bot_y = area_y + area_h * 3 / 4;

    int bs1_x = area_x + static_cast<int>(0.3f * area_w);
    int bs2_x = area_x + static_cast<int>(0.7f * area_w);
    int mirror_top_x = area_x + static_cast<int>(0.7f * area_w);
    int mirror_bot_x = area_x + static_cast<int>(0.3f * area_w);

    DrawRectangle(area_x + 10, cy - 2, bs1_x - area_x - 15, 4, Color{255, 200, 50, 150});
    draw_text("source", static_cast<float>(area_x + 10), static_cast<float>(cy + 6), 9, ui_colors::TEXT_SECONDARY);

    DrawRectangle(bs1_x - 5, cy - 15, 10, 30, Color{120, 180, 220, 200});
    draw_text("BS1", static_cast<float>(bs1_x - 8), static_cast<float>(cy + 18), 9, ui_colors::ACCENT);

    DrawLine(bs1_x, cy, mirror_top_x, top_y, Color{90, 160, 220, 150});
    DrawLine(bs1_x, cy, mirror_bot_x, bot_y, Color{220, 140, 80, 150});

    DrawRectangle(mirror_top_x - 5, top_y - 5, 10, 10, Color{180, 180, 180, 200});
    draw_text("M1", static_cast<float>(mirror_top_x + 8), static_cast<float>(top_y - 5), 9, ui_colors::TEXT_SECONDARY);
    DrawRectangle(mirror_bot_x - 5, bot_y - 5, 10, 10, Color{180, 180, 180, 200});
    draw_text("M2", static_cast<float>(mirror_bot_x - 20), static_cast<float>(bot_y + 5), 9, ui_colors::TEXT_SECONDARY);

    if (mz_blocker_) {
        int block_x = (bs1_x + mirror_bot_x) / 2;
        int block_y = (cy + bot_y) / 2;
        DrawRectangle(block_x - 8, block_y - 8, 16, 16, Color{200, 60, 60, 200});
        draw_text("BLOCK", static_cast<float>(block_x - 15), static_cast<float>(block_y + 10), 9, ui_colors::DANGER);
    }

    DrawLine(mirror_top_x, top_y, bs2_x, cy, Color{90, 160, 220, 150});
    if (!mz_blocker_) {
        DrawLine(mirror_bot_x, bot_y, bs2_x, cy, Color{220, 140, 80, 150});
    }

    DrawRectangle(bs2_x - 5, cy - 15, 10, 30, Color{120, 180, 220, 200});
    draw_text("BS2", static_cast<float>(bs2_x - 8), static_cast<float>(cy + 18), 9, ui_colors::ACCENT);

    int d1_x = area_x + area_w - 30;
    int d2_y = cy + (area_h / 4);
    DrawRectangle(d1_x, cy - 10, 20, 20, Color{100, 200, 100, 200});
    draw_text("D1", static_cast<float>(d1_x + 2), static_cast<float>(cy - 8), 10, WHITE);
    DrawRectangle(bs2_x - 10, d2_y - 10, 20, 20, Color{200, 100, 100, 200});
    draw_text("D2", static_cast<float>(bs2_x - 8), static_cast<float>(d2_y - 8), 10, WHITE);

    DrawLine(bs2_x, cy, d1_x, cy, Color{255, 200, 50, 150});
    DrawLine(bs2_x, cy, bs2_x, d2_y, Color{255, 200, 50, 80});

    for (const auto& p : mz_photons_) {
        int px_screen, py_screen;
        Color pc = Color{255, 220, 50, 220};

        if (p.x < 0.3f) {
            px_screen = area_x + static_cast<int>(p.x * area_w);
            py_screen = cy;
        } else if (p.x < 0.7f) {
            float t = (p.x - 0.3f) / 0.4f;
            if (!mz_blocker_) {
                int px1 = bs1_x + static_cast<int>(t * (mirror_top_x - bs1_x));
                int py1 = cy + static_cast<int>(t * (top_y - cy));
                int px2 = bs1_x + static_cast<int>(t * (mirror_bot_x - bs1_x));
                int py2 = cy + static_cast<int>(t * (bot_y - cy));
                DrawCircle(px1, py1, 3, Color{90, 160, 220, 180});
                DrawCircle(px2, py2, 3, Color{220, 140, 80, 180});
                continue;
            } else {
                if (p.path == 0) {
                    px_screen = bs1_x + static_cast<int>(t * (mirror_top_x - bs1_x));
                    py_screen = cy + static_cast<int>(t * (top_y - cy));
                    pc = Color{90, 160, 220, 220};
                } else {
                    continue;
                }
            }
        } else {
            px_screen = area_x + static_cast<int>(p.x * area_w);
            py_screen = cy;
        }
        DrawCircle(px_screen, py_screen, 3, pc);
    }

    int info_y = area_y + area_h + 10;
    int mz_total = mz_d1_count_ + mz_d2_count_;
    if (mz_total > 0) {
        draw_text(TextFormat("D1: %d (%.1f%%)   D2: %d (%.1f%%)",
                  mz_d1_count_, 100.0f * mz_d1_count_ / mz_total,
                  mz_d2_count_, 100.0f * mz_d2_count_ / mz_total),
                  static_cast<float>(area_x), static_cast<float>(info_y), 13, ui_colors::TEXT_PRIMARY);
    }
    if (!mz_blocker_) {
        draw_text("Superposition \xE2\x86\x92 constructive interference at D1, destructive at D2",
                  static_cast<float>(area_x), static_cast<float>(info_y + 20), 11, ui_colors::SUCCESS);
    } else {
        draw_text("Which-path info \xE2\x86\x92 no interference \xE2\x86\x92 50/50 at both detectors",
                  static_cast<float>(area_x), static_cast<float>(info_y + 20), 11, ui_colors::DANGER);
    }
}

void SuperpositionScenario::render_quantum_beat(int vp_x, int vp_y, int vp_w, int vp_h) {
    draw_text("QUANTUM BEATING (Energy Superposition)", static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 10), 14, ui_colors::ACCENT);
    draw_text("Superposition of n=1 and n=2 infinite well states",
              static_cast<float>(vp_x + 10), static_cast<float>(vp_y + 28), 11, ui_colors::TEXT_SECONDARY);

    int plot_x = vp_x + 50;
    int plot_w = vp_w - 80;
    int plot_y = vp_y + 55;
    int plot_h = vp_h - 130;
    int baseline = plot_y + plot_h;

    DrawLine(plot_x, baseline, plot_x + plot_w, baseline, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x, plot_y, plot_x, baseline, ui_colors::TEXT_SECONDARY);
    draw_text("x / L", static_cast<float>(plot_x + plot_w + 5), static_cast<float>(baseline - 8), 11, ui_colors::TEXT_SECONDARY);
    draw_text("|\xCF\x88|\xC2\xB2", static_cast<float>(plot_x - 30), static_cast<float>(plot_y), 11, ui_colors::TEXT_SECONDARY);

    double c2 = mix_ratio();
    double c1 = 1.0 - c2;
    double omega = 3.0 * PI * PI;

    int prev_sx = 0, prev_sy = 0;
    int prev_g1x = 0, prev_g1y = 0;
    int prev_g2x = 0, prev_g2y = 0;

    double max_val = 0.0;
    for (int i = 0; i <= plot_w; ++i) {
        double x = static_cast<double>(i) / plot_w;
        double psi1 = std::sqrt(2.0) * std::sin(PI * x);
        double psi2 = std::sqrt(2.0) * std::sin(2.0 * PI * x);
        double psi_re = std::sqrt(c1) * psi1 * std::cos(0.0) + std::sqrt(c2) * psi2 * std::cos(-omega * sim_time_);
        double psi_im = std::sqrt(c1) * psi1 * std::sin(0.0) + std::sqrt(c2) * psi2 * std::sin(-omega * sim_time_);
        double prob = psi_re * psi_re + psi_im * psi_im;
        if (prob > max_val) max_val = prob;
    }
    if (max_val < 0.01) max_val = 1.0;

    for (int i = 0; i <= plot_w; ++i) {
        double x = static_cast<double>(i) / plot_w;
        double psi1 = std::sqrt(2.0) * std::sin(PI * x);
        double psi2 = std::sqrt(2.0) * std::sin(2.0 * PI * x);

        double psi_re = std::sqrt(c1) * psi1 * std::cos(0.0) + std::sqrt(c2) * psi2 * std::cos(-omega * sim_time_);
        double psi_im = std::sqrt(c1) * psi1 * std::sin(0.0) + std::sqrt(c2) * psi2 * std::sin(-omega * sim_time_);
        double prob = psi_re * psi_re + psi_im * psi_im;

        int sx = plot_x + i;
        int sy = baseline - static_cast<int>((prob / (max_val * 1.1)) * plot_h);

        int fill_h = baseline - sy;
        if (fill_h > 0) DrawLine(sx, baseline, sx, sy, Color{90, 160, 220, 40});

        if (i > 0) DrawLine(prev_sx, prev_sy, sx, sy, ui_colors::WAVEFUNCTION);
        prev_sx = sx;
        prev_sy = sy;

        double g1 = c1 * psi1 * psi1;
        double g2 = c2 * psi2 * psi2;
        int g1y = baseline - static_cast<int>((g1 / (max_val * 1.1)) * plot_h);
        int g2y = baseline - static_cast<int>((g2 / (max_val * 1.1)) * plot_h);
        if (i > 0) {
            DrawLine(prev_g1x, prev_g1y, sx, g1y, Color{100, 200, 100, 80});
            DrawLine(prev_g2x, prev_g2y, sx, g2y, Color{220, 140, 80, 80});
        }
        prev_g1x = sx;
        prev_g1y = g1y;
        prev_g2x = sx;
        prev_g2y = g2y;
    }

    DrawLine(plot_x, baseline + 2, plot_x, baseline + 6, ui_colors::TEXT_SECONDARY);
    DrawLine(plot_x + plot_w, baseline + 2, plot_x + plot_w, baseline + 6, ui_colors::TEXT_SECONDARY);
    draw_text("0", static_cast<float>(plot_x - 4), static_cast<float>(baseline + 8), 9, ui_colors::TEXT_SECONDARY);
    draw_text("L", static_cast<float>(plot_x + plot_w - 2), static_cast<float>(baseline + 8), 9, ui_colors::TEXT_SECONDARY);

    int info_y = baseline + 20;
    draw_text(TextFormat("c\xE2\x82\x81 = %.2f (n=1)   c\xE2\x82\x82 = %.2f (n=2)   t = %.2f",
              c1, c2, sim_time_), static_cast<float>(plot_x), static_cast<float>(info_y), 12, ui_colors::TEXT_PRIMARY);
    draw_text("Blue = |\xCF\x88(x,t)|\xC2\xB2    Green/Orange = individual n=1, n=2 components",
              static_cast<float>(plot_x), static_cast<float>(info_y + 16), 11, ui_colors::TEXT_SECONDARY);
    draw_text("Probability density oscillates because E\xE2\x82\x81 \xE2\x89\xA0 E\xE2\x82\x82",
              static_cast<float>(plot_x), static_cast<float>(info_y + 30), 11, ui_colors::ACCENT);
}

void SuperpositionScenario::render_controls(int x, int y, int w, int h) {
    draw_panel_bg(x, y, w, h);
    int cx = x + 10;
    int cy = y + 8;

    draw_text("SUPERPOSITION", static_cast<float>(cx), static_cast<float>(cy), 12, ui_colors::TEXT_SECONDARY);
    cy += 22;

    if (current_view_ <= 1) {
        alpha_sq_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
        phase_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
    }

    if (current_view_ == 2) {
        Rectangle blocker_btn = {static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(w - 20), 26.0f};
        DrawRectangleRec(blocker_btn, mz_blocker_ ? ui_colors::DANGER : ui_colors::PANEL_HOVER);
        DrawRectangleLinesEx(blocker_btn, 1.0f, ui_colors::PANEL_BORDER);
        draw_text(mz_blocker_ ? "Blocker: ON" : "Blocker: OFF",
                  static_cast<float>(cx + 6), static_cast<float>(cy + 6), 12, ui_colors::TEXT_PRIMARY);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, blocker_btn)) {
                mz_blocker_ = !mz_blocker_;
                mz_photons_.clear();
                mz_d1_count_ = 0;
                mz_d2_count_ = 0;
            }
        }
        cy += 32;
    }

    if (current_view_ == 3) {
        mix_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
        time_speed_slider_.render(font_, has_font_, cx, cy, w - 20);
        cy += Slider::HEIGHT + 4;
    }

    draw_separator(cx, cy, w - 20);
    cy += 4;

    draw_text("A quantum state can be", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("in multiple states at", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 14;
    draw_text("once until measured.", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);
    cy += 24;

    draw_text("Views: 1-4 keys", static_cast<float>(cx), static_cast<float>(cy), 11, ui_colors::TEXT_SECONDARY);

    cy += 20;
    if (HelpPopup::render_help_button(font_, has_font_, cx, cy)) {
        help_popup_.show({"\xCF\x88", "Quantum Superposition",
            "A quantum system can exist in a linear combination of states: |\xCF\x88\xE2\x9F\xA9 = \xCE\xB1|0\xE2\x9F\xA9 + \xCE\xB2|1\xE2\x9F\xA9. "
            "This is NOT ignorance about which state it is in. The system is genuinely in BOTH states "
            "simultaneously. Measurement forces a collapse to one outcome, with probability |\xCE\xB1|\xC2\xB2 or |\xCE\xB2|\xC2\xB2. "
            "Interference proves the reality of superposition: the Mach-Zehnder shows a photon must "
            "travel both paths to explain the detection pattern.",
            "|\xCF\x88\xE2\x9F\xA9 = \xCE\xB1|0\xE2\x9F\xA9 + \xCE\xB2|1\xE2\x9F\xA9, |\xCE\xB1|\xC2\xB2 + |\xCE\xB2|\xC2\xB2 = 1",
            "P(|0\xE2\x9F\xA9) = |\xCE\xB1|\xC2\xB2, P(|1\xE2\x9F\xA9) = |\xCE\xB2|\xC2\xB2"});
    }
}

void SuperpositionScenario::render_properties(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
    DrawLine(x, y, x, y + h, ui_colors::PANEL_BORDER);

    int px = x + 12;
    int py = y + 8;

    draw_text("PROPERTIES", static_cast<float>(px), static_cast<float>(py), 12, ui_colors::TEXT_SECONDARY);
    py += 22;

    if (current_view_ <= 1) {
        draw_section("State", px, py);
        draw_prop("|\xCE\xB1|\xC2\xB2", TextFormat("%.4f", alpha_sq()), px, py, ui_colors::WAVEFUNCTION);
        draw_prop("|\xCE\xB2|\xC2\xB2", TextFormat("%.4f", beta_sq()), px, py, Color{220, 160, 90, 255});
        draw_prop("Phase", TextFormat("%.3f rad", rel_phase()), px, py);
        py += 4;
        draw_section("Probabilities", px, py);
        draw_prop("P(|0\xE2\x9F\xA9)", TextFormat("%.1f%%", alpha_sq() * 100.0), px, py);
        draw_prop("P(|1\xE2\x9F\xA9)", TextFormat("%.1f%%", beta_sq() * 100.0), px, py);
    }

    if (current_view_ == 1) {
        py += 4;
        draw_section("Measurements", px, py);
        draw_prop("|0\xE2\x9F\xA9 count", TextFormat("%d", measure_count_0_), px, py);
        draw_prop("|1\xE2\x9F\xA9 count", TextFormat("%d", measure_count_1_), px, py);
        int total = measure_count_0_ + measure_count_1_;
        if (total > 0) {
            draw_prop("Measured |0\xE2\x9F\xA9", TextFormat("%.1f%%", 100.0f * measure_count_0_ / total), px, py);
        }
    }

    if (current_view_ == 2) {
        draw_section("Interferometer", px, py);
        draw_prop("Blocker", mz_blocker_ ? "ON" : "OFF", px, py, mz_blocker_ ? ui_colors::DANGER : ui_colors::SUCCESS);
        draw_prop("D1 hits", TextFormat("%d", mz_d1_count_), px, py);
        draw_prop("D2 hits", TextFormat("%d", mz_d2_count_), px, py);
        py += 4;
        draw_section("Physics", px, py);
        if (mz_blocker_) {
            draw_text("With blocker: photon", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
            py += 14;
            draw_text("takes ONE path only.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
            py += 14;
            draw_text("No interference.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::DANGER);
        } else {
            draw_text("Photon in superposition", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
            py += 14;
            draw_text("of BOTH paths.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
            py += 14;
            draw_text("Interference: D1=100%", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::SUCCESS);
        }
    }

    if (current_view_ == 3) {
        draw_section("Beating", px, py);
        draw_prop("c\xE2\x82\x81 (n=1)", TextFormat("%.3f", 1.0 - mix_ratio()), px, py, Color{100, 200, 100, 255});
        draw_prop("c\xE2\x82\x82 (n=2)", TextFormat("%.3f", mix_ratio()), px, py, Color{220, 140, 80, 255});
        draw_prop("Time", TextFormat("%.3f", sim_time_), px, py);
        py += 4;
        draw_section("Frequency", px, py);
        double omega_beat = 3.0 * PI * PI;
        draw_prop("\xCF\x89 beat", TextFormat("%.2f", omega_beat), px, py, ui_colors::ACCENT);
        draw_text("= (E\xE2\x82\x82-E\xE2\x82\x81)/\xE2\x84\x8F", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    }

    py += 20;
    draw_separator(px, py, w - 24);
    draw_section("Key Insight", px, py);
    draw_text("Superposition is not", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("\"we don't know\". The", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("system is genuinely in", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::TEXT_SECONDARY);
    py += 14;
    draw_text("both states at once.", static_cast<float>(px), static_cast<float>(py), 11, ui_colors::ACCENT);
}
