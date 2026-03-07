#pragma once

#include <vector>
#include <raylib.h>

#include "../IScenario.hpp"
#include "GridSolver.hpp"
#include "../PotentialField.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class HeisenbergScenario : public IScenario {
public:
    HeisenbergScenario();
    ~HeisenbergScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Heisenberg"; }
    [[nodiscard]] int get_view_count() const override { return 3; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    GridSolver solver_;
    PotentialField potential_;

    Slider sigma_slider_;
    HelpPopup help_popup_;

    static constexpr double HBAR = 1.0;

    [[nodiscard]] double sigma() const { return static_cast<double>(sigma_slider_.get_value()); }
    [[nodiscard]] double delta_x() const { return sigma(); }
    [[nodiscard]] double delta_p() const { return HBAR / (2.0 * sigma()); }
    [[nodiscard]] double product() const { return delta_x() * delta_p(); }

    void reset_solver();

    void render_x_space(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_p_space(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_split_view(int vp_x, int vp_y, int vp_w, int vp_h);

    void draw_gaussian_plot(int px, int py, int pw, int ph, double sig, const char* xlabel,
                            const char* title, Color curve_color, Color fill_color);
};
