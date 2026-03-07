#pragma once

#include <raylib.h>

#include "../IScenario.hpp"
#include "GridSolver.hpp"
#include "../PotentialField.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class FreeParticleScenario : public IScenario {
public:
    FreeParticleScenario();
    ~FreeParticleScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Free Particle"; }
    [[nodiscard]] int get_view_count() const override { return 1; }
    [[nodiscard]] const char* get_view_name(int idx) const override { (void)idx; return "3D View"; }
    [[nodiscard]] bool uses_3d() const override { return true; }

private:
    GridSolver solver_;
    PotentialField potential_;

    Slider x0_slider_;
    Slider sigma_slider_;
    Slider k0_slider_;

    HelpPopup help_popup_;

    bool paused_ = false;
    int speed_mult_ = 5;

    [[nodiscard]] double x0() const { return static_cast<double>(x0_slider_.get_value()); }
    [[nodiscard]] double sigma() const { return static_cast<double>(sigma_slider_.get_value()); }
    [[nodiscard]] double k0() const { return static_cast<double>(k0_slider_.get_value()); }

    void reset_solver();
};
