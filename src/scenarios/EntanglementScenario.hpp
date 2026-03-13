#pragma once

#include <vector>
#include <random>
#include <raylib.h>

#include "../IScenario.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class EntanglementScenario : public IScenario {
public:
    EntanglementScenario();
    ~EntanglementScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Entanglement"; }
    [[nodiscard]] int get_view_count() const override { return 4; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    Slider alice_angle_slider_;
    Slider bob_angle_slider_;
    HelpPopup help_popup_;

    int bell_state_ = 0;

    struct MeasResult {
        int alice;
        int bob;
    };
    std::vector<MeasResult> epr_log_;
    int epr_same_ = 0;
    int epr_diff_ = 0;

    struct BellProbs {
        double p00, p01, p10, p11;
    };
    [[nodiscard]] BellProbs get_bell_probs() const;
    [[nodiscard]] double quantum_correlation(double angle_diff) const;

    int creation_step_ = 0;

    struct FlyingParticle {
        float x;
        int direction;
        float spawn_time;
    };
    std::vector<FlyingParticle> flying_particles_;
    double fly_timer_ = 0.0;

    std::mt19937 rng_{42};

    [[nodiscard]] double alice_angle_rad() const;
    [[nodiscard]] double bob_angle_rad() const;

    void do_epr_measurement();

    void render_epr(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_bell_explorer(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_correlation(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_creation(int vp_x, int vp_y, int vp_w, int vp_h);
};
