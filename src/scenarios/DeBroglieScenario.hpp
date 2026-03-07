#pragma once

#include <raylib.h>

#include "../IScenario.hpp"
#include "GridSolver.hpp"
#include "../PotentialField.hpp"
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
    [[nodiscard]] int get_view_count() const override { return 2; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return current_view_ == 0; }

private:
    GridSolver solver_;
    PotentialField potential_;

    Slider velocity_slider_;
    int particle_type_ = 0;

    HelpPopup help_popup_;

    static constexpr double HBAR = 1.0;
    static constexpr double H_SI = 6.62607015e-34;

    struct ParticlePreset {
        const char* name;
        double mass_kg;
    };
    static constexpr ParticlePreset PRESETS[] = {
        {"Electron", 9.109e-31},
        {"Proton", 1.673e-27},
        {"Baseball (0.145 kg)", 0.145}
    };
    static constexpr int NUM_PRESETS = 3;

    [[nodiscard]] double k0() const;
    [[nodiscard]] double momentum_au() const;
    [[nodiscard]] double lambda_au() const;
    [[nodiscard]] double energy_au() const;
    [[nodiscard]] double lambda_si() const;

    void reset_solver();

    void render_wavepacket_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);
    void render_comparison_view(int vp_x, int vp_y, int vp_w, int vp_h);
};
