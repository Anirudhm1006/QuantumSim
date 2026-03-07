#pragma once

#include <vector>
#include <string>

#include <raylib.h>

#include "../IScenario.hpp"
#include "ElementData.hpp"
#include "HydrogenModel.hpp"
#include "Slider.hpp"
#include "PeriodicTable.hpp"
#include "HelpPopup.hpp"

class AtomViewerScenario : public IScenario {
public:
    AtomViewerScenario();
    ~AtomViewerScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Atom Viewer"; }
    [[nodiscard]] int get_view_count() const override { return 3; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return current_view_ != 0; }

private:
    int element_index_ = 0;
    HydrogenModel model_;

    Slider n_slider_;
    Slider l_slider_;
    Slider m_slider_;
    Slider sample_count_slider_;

    std::vector<Vector3> samples_;
    int sample_count_ = 2000;

    PeriodicTable periodic_table_;
    HelpPopup help_popup_;

    float orbit_angle_ = 0.0f;

    bool measuring_ = false;
    Vector2 measure_start_{};
    Vector2 measure_end_{};
    int dots_in_circle_ = 0;
    float measure_radius_ = 0.0f;

    void select_element(int index);
    void validate_quantum_numbers();
    void regenerate_model();
    void regenerate_samples();

    [[nodiscard]] int current_n() const { return static_cast<int>(n_slider_.get_value()); }
    [[nodiscard]] int current_l() const { return static_cast<int>(l_slider_.get_value()); }
    [[nodiscard]] int current_m() const { return static_cast<int>(m_slider_.get_value()); }
    [[nodiscard]] int current_Z() const;
    [[nodiscard]] double energy_eV() const;
    [[nodiscard]] std::string orbital_name_from_nl(int n, int l) const;

    void render_bohr_model(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_orbital_cloud(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);
    void render_dot_density(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);

    void count_dots_in_measurement_circle(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h);

    bool render_button(const char* label, int x, int y, int w, int btn_h, Color bg);

    static constexpr const char* SHELL_LABELS[] = {"K", "L", "M", "N", "O", "P", "Q"};
};
