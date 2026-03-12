#pragma once

#include <vector>
#include <raylib.h>

#include "../IScenario.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class DoubleSlitScenario : public IScenario {
public:
    DoubleSlitScenario();
    ~DoubleSlitScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Double Slit"; }
    [[nodiscard]] int get_view_count() const override { return 4; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override;

private:
    Slider slit_width_slider_;
    Slider slit_sep_slider_;
    Slider wavelength_slider_;
    Slider particle_rate_slider_;

    HelpPopup help_popup_;

    bool wave_mode_ = true;
    bool detector_on_ = false;

    static constexpr int HISTOGRAM_BINS = 200;

    std::vector<float> histogram_;
    std::vector<Vector2> particles_;
    int particles_detected_ = 0;
    double sim_time_ = 0.0;
    double particle_spawn_accum_ = 0.0;

    [[nodiscard]] double slit_width() const { return static_cast<double>(slit_width_slider_.get_value()); }
    [[nodiscard]] double slit_sep() const { return static_cast<double>(slit_sep_slider_.get_value()); }
    [[nodiscard]] double wavelength() const { return static_cast<double>(wavelength_slider_.get_value()); }

    [[nodiscard]] static double sinc(double x);
    [[nodiscard]] double intensity_at_y(double y_pos) const;
    [[nodiscard]] double intensity_no_interference(double y_pos) const;
    [[nodiscard]] double current_intensity(double y_pos) const;
    [[nodiscard]] double fringe_spacing() const;

    void reset_simulation();
    void spawn_particle();
    void advance_particles(double dt);

    void render_setup_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_wave3d_view(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);
    void render_pattern_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_heatmap_view(int vp_x, int vp_y, int vp_w, int vp_h);

    bool render_toggle_button(const char* label_on, const char* label_off, bool state, int x, int y, int w);
};
