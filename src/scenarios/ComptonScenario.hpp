#pragma once

#include <raylib.h>

#include "../IScenario.hpp"
#include "Slider.hpp"
#include "HelpPopup.hpp"

class ComptonScenario : public IScenario {
public:
    ComptonScenario();
    ~ComptonScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Compton"; }
    [[nodiscard]] int get_view_count() const override { return 4; }
    [[nodiscard]] const char* get_view_name(int idx) const override;
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    Slider theta_slider_;
    Slider lambda_in_slider_;
    int target_type_ = 0;
    HelpPopup help_popup_;

    double anim_time_ = 0.0;
    float anim_phase_ = 0.0f;

    static constexpr double H_SI = 6.62607015e-34;
    static constexpr double C_SI = 2.998e8;
    static constexpr double ME_SI = 9.109e-31;
    static constexpr double MP_SI = 1.673e-27;
    static constexpr double EV_TO_J = 1.602e-19;

    struct TargetPreset {
        const char* name;
        double mass_kg;
        double compton_wavelength_pm;
    };
    static const TargetPreset TARGETS[2];

    [[nodiscard]] double theta_rad() const;
    [[nodiscard]] double lambda_in_m() const;
    [[nodiscard]] double compton_wavelength() const;
    [[nodiscard]] double delta_lambda() const;
    [[nodiscard]] double lambda_out_m() const;
    [[nodiscard]] double photon_energy_in_eV() const;
    [[nodiscard]] double photon_energy_out_eV() const;
    [[nodiscard]] double electron_ke_eV() const;
    [[nodiscard]] double electron_recoil_angle() const;

    void render_scattering_anim(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_wavelength_compare(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_shift_graph(int vp_x, int vp_y, int vp_w, int vp_h);
    void render_classical_vs_quantum(int vp_x, int vp_y, int vp_w, int vp_h);
};
