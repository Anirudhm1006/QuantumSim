#pragma once

#include <vector>
#include <random>
#include <raylib.h>

#include "../IScenario.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class DeBroglieScenario : public IScenario {
public:
    DeBroglieScenario();
    ~DeBroglieScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "De Broglie"; }
    [[nodiscard]] int get_view_count() const override { return 3; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    Slider velocity_slider_;
    Slider crystal_d_slider_;
    int particle_type_ = 0;
    HelpPopup help_popup_;

    double anim_time_ = 0.0;
    float particle_anim_x_ = 0.0f;

    static constexpr double H_SI = 6.62607015e-34;
    static constexpr double C_SI = 2.998e8;

    struct ParticlePreset {
        const char* name;
        double mass_kg;
        double typical_size_m;
        bool is_quantum;
    };
    static constexpr int NUM_PRESETS = 6;
    static const ParticlePreset PRESETS[NUM_PRESETS];

    [[nodiscard]] double velocity_mps() const;
    [[nodiscard]] double momentum_si() const;
    [[nodiscard]] double lambda_si() const;
    [[nodiscard]] double lambda_display_val() const;
    [[nodiscard]] const char* lambda_display_unit() const;
    [[nodiscard]] bool is_quantum_regime() const;

    std::mt19937 rng_{42};

    void render_wave_animation(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_scale_comparison(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_diffraction(int vp_x, int vp_y, int vp_w, int vp_h);
};
