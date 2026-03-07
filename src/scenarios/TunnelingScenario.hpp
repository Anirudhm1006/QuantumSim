#pragma once

#include <vector>
#include <raylib.h>

#include "../IScenario.hpp"
#include "GridSolver.hpp"
#include "../PotentialField.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class TunnelingScenario : public IScenario {
public:
    TunnelingScenario();
    ~TunnelingScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Tunneling"; }
    [[nodiscard]] int get_view_count() const override { return 3; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override;

private:
    GridSolver solver_;
    PotentialField potential_;

    Slider v0_slider_;
    Slider l_slider_;
    Slider k0_slider_;

    HelpPopup help_popup_;

    bool auto_fire_ = false;
    double auto_fire_timer_ = 0.0;

    static constexpr int GRID_NX = 512;
    static constexpr double GRID_X_MIN = -15.0;
    static constexpr double GRID_X_MAX = 15.0;
    static constexpr double GRID_DT = 0.001;
    static constexpr double AUTO_FIRE_INTERVAL = 4.0;

    [[nodiscard]] double v0() const { return static_cast<double>(v0_slider_.get_value()); }
    [[nodiscard]] double barrier_l() const { return static_cast<double>(l_slider_.get_value()); }
    [[nodiscard]] double k0() const { return static_cast<double>(k0_slider_.get_value()); }
    [[nodiscard]] double particle_energy() const { return k0() * k0() / 2.0; }

    [[nodiscard]] static double transmission_coefficient(double E, double V0, double L);
    [[nodiscard]] double numerical_transmission() const;

    void rebuild_barrier();
    void reset_simulation();
    void inject_packet();

    void render_setup_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_wavefunction_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);
    void render_te_curve_view(int vp_x, int vp_y, int vp_w, int vp_h);

    bool render_toggle_button(const char* label_on, const char* label_off, bool state, int x, int y, int w);
    bool render_button(const char* label, int x, int y, int w);
};
