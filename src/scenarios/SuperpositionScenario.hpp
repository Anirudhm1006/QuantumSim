#pragma once

#include <vector>
#include <random>
#include <raylib.h>

#include "../IScenario.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class SuperpositionScenario : public IScenario {
public:
    SuperpositionScenario();
    ~SuperpositionScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Superposition"; }
    [[nodiscard]] int get_view_count() const override { return 4; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    Slider alpha_sq_slider_;
    Slider phase_slider_;
    Slider mix_slider_;
    Slider time_speed_slider_;
    HelpPopup help_popup_;

    double sim_time_ = 0.0;
    int measure_count_0_ = 0;
    int measure_count_1_ = 0;
    int last_result_ = -1;
    float collapse_flash_ = 0.0f;

    bool mz_blocker_ = false;
    struct MZPhoton {
        float x;
        int path;
        bool past_splitter1;
        bool past_splitter2;
    };
    std::vector<MZPhoton> mz_photons_;
    double mz_spawn_timer_ = 0.0;
    int mz_d1_count_ = 0;
    int mz_d2_count_ = 0;

    std::mt19937 rng_{42};

    [[nodiscard]] double alpha_sq() const { return static_cast<double>(alpha_sq_slider_.get_value()); }
    [[nodiscard]] double beta_sq() const { return 1.0 - alpha_sq(); }
    [[nodiscard]] double rel_phase() const { return static_cast<double>(phase_slider_.get_value()); }
    [[nodiscard]] double mix_ratio() const { return static_cast<double>(mix_slider_.get_value()); }

    void render_two_state(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_collapse(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_mach_zehnder(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_quantum_beat(int vp_x, int vp_y, int vp_w, int vp_h);
};
