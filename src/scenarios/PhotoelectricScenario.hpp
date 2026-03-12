#pragma once

#include <vector>
#include <raylib.h>

#include "../IScenario.hpp"
#include "PhotoelectricEffect.hpp"
#include "SpectralTransition.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class PhotoelectricScenario : public IScenario {
public:
    PhotoelectricScenario();
    ~PhotoelectricScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Photoelectric Effect"; }
    [[nodiscard]] int get_view_count() const override { return 3; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    Slider frequency_slider_;
    Slider intensity_slider_;
    int metal_idx_ = 0;

    HelpPopup help_popup_;

    struct Photon { float x, y; bool active; };
    struct Electron { float x, y; float vx, vy; float life; };
    std::vector<Photon> photons_;
    std::vector<Electron> electrons_;
    double spawn_timer_ = 0.0;
    int photon_count_ = 0;
    int electron_count_ = 0;

    double anim_pulse_ = 0.0;

    [[nodiscard]] double frequency_THz() const { return static_cast<double>(frequency_slider_.get_value()); }
    [[nodiscard]] double frequency_Hz() const { return frequency_THz() * 1e12; }
    [[nodiscard]] double wavelength_nm() const;
    [[nodiscard]] double photon_energy_eV() const;
    [[nodiscard]] double intensity() const { return static_cast<double>(intensity_slider_.get_value()); }

    void render_experiment_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_energy_diagram_view(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_graph_view(int vp_x, int vp_y, int vp_w, int vp_h);

    void render_em_bar(int x, int y, int w);
};
