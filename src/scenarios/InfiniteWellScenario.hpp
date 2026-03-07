#pragma once

#include <vector>
#include <raylib.h>

#include "../IScenario.hpp"
#include "GridSolver.hpp"
#include "../PotentialField.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class InfiniteWellScenario : public IScenario {
public:
    InfiniteWellScenario();
    ~InfiniteWellScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Infinite Well"; }
    [[nodiscard]] int get_view_count() const override { return 3; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override;

private:
    GridSolver solver_;
    PotentialField potential_;

    Slider well_width_slider_;
    Slider n_slider_;
    Slider superpos_n1_;
    Slider superpos_n2_;
    Slider superpos_n3_;

    HelpPopup help_popup_;

    static constexpr int GRID_NX = 512;
    static constexpr double GRID_X_MIN = -10.0;
    static constexpr double GRID_X_MAX = 10.0;
    static constexpr double GRID_DT = 0.001;

    [[nodiscard]] double well_width() const { return static_cast<double>(well_width_slider_.get_value()); }
    [[nodiscard]] int quantum_n() const { return static_cast<int>(n_slider_.get_value()); }
    [[nodiscard]] double energy_n(int n) const;
    [[nodiscard]] double psi_n(int n, double x) const;

    void rebuild_potential();
    void reset_simulation();

    void render_standing_wave_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_wavefunction_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);
    void render_superposition_view(int vp_x, int vp_y, int vp_w, int vp_h);
};
