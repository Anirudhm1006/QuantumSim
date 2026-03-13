#pragma once

#include <vector>
#include <random>
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
    [[nodiscard]] int get_view_count() const override { return 4; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    GridSolver solver_;
    PotentialField potential_;

    Slider sigma_slider_;
    Slider probe_lambda_slider_;
    HelpPopup help_popup_;

    static constexpr double HBAR = 1.0;

    float electron_x_ = 0.5f, electron_y_ = 0.5f;
    float electron_vx_ = 0.0f, electron_vy_ = 0.0f;
    float photon_x_ = -0.1f;
    bool photon_active_ = false;
    float photon_timer_ = 0.0f;
    float measure_circle_r_ = 0.0f;
    float momentum_arrow_angle_ = 0.0f;
    float momentum_arrow_mag_ = 0.0f;
    bool show_measurement_ = false;
    float measure_fade_ = 0.0f;

    std::vector<float> pos_samples_;
    std::vector<float> mom_samples_;
    double sample_timer_ = 0.0;
    static constexpr int MAX_SAMPLES = 500;

    std::vector<Vector2> phase_points_;

    std::mt19937 rng_{42};

    [[nodiscard]] double sigma() const { return static_cast<double>(sigma_slider_.get_value()); }
    [[nodiscard]] double probe_lambda() const { return static_cast<double>(probe_lambda_slider_.get_value()); }
    [[nodiscard]] double delta_x_microscope() const { return probe_lambda(); }
    [[nodiscard]] double delta_p_microscope() const { return HBAR / probe_lambda(); }
    [[nodiscard]] double delta_x() const { return sigma(); }
    [[nodiscard]] double delta_p() const { return HBAR / (2.0 * sigma()); }
    [[nodiscard]] double product() const { return delta_x() * delta_p(); }

    void reset_solver();
    void reset_microscope();
    void reset_samples();

    void render_microscope_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_sampling_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_phase_space_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_wavepacket_view(int vp_x, int vp_y, int vp_w, int vp_h);

    void draw_histogram(int px, int py, int pw, int ph, const std::vector<float>& data,
                        double sig, const char* xlabel, const char* title,
                        Color bar_color, Color curve_color);
};
