#pragma once

#include <vector>
#include <random>
#include <raylib.h>

#include "../IScenario.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class SpinScenario : public IScenario {
public:
    SpinScenario();
    ~SpinScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Spin"; }
    [[nodiscard]] int get_view_count() const override { return 4; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return current_view_ == 1; }

private:
    Slider theta_slider_;
    Slider phi_slider_;
    Slider sg_angle_slider_;
    HelpPopup help_popup_;

    [[nodiscard]] double theta() const { return static_cast<double>(theta_slider_.get_value()); }
    [[nodiscard]] double phi() const { return static_cast<double>(phi_slider_.get_value()); }
    [[nodiscard]] double sg_angle() const { return static_cast<double>(sg_angle_slider_.get_value()); }

    [[nodiscard]] double prob_up() const;
    [[nodiscard]] double prob_down() const;

    struct SGParticle {
        float x, y;
        int result;
        bool measured;
    };
    std::vector<SGParticle> sg_particles_;
    double sg_spawn_timer_ = 0.0;

    int stats_up_ = 0;
    int stats_down_ = 0;

    int seq_filter1_axis_ = 0;
    int seq_filter2_axis_ = 1;
    int seq_filter3_axis_ = 0;
    bool seq_filter2_enabled_ = true;
    struct SeqParticle {
        float x;
        int stage;
        int result1, result2, result3;
        float y_offset;
    };
    std::vector<SeqParticle> seq_particles_;
    double seq_spawn_timer_ = 0.0;
    int seq_final_up_ = 0;
    int seq_final_down_ = 0;

    int stat_run_count_ = 0;
    int stat_results_up_ = 0;
    int stat_results_down_ = 0;

    std::mt19937 rng_{42};

    void render_stern_gerlach(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_bloch_sphere(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);
    void render_sequential(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_statistics(int vp_x, int vp_y, int vp_w, int vp_h);

    void draw_magnet(int cx, int cy, int w, int h, float angle_deg, const char* label);
};
