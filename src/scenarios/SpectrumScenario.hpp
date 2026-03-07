#pragma once

#include <raylib.h>

#include "../IScenario.hpp"
#include "ElementData.hpp"
#include "SpectralTransition.hpp"
#include "PeriodicTable.hpp"
#include "HelpPopup.hpp"

class SpectrumScenario : public IScenario {
public:
    SpectrumScenario();
    ~SpectrumScenario() override = default;

    void on_enter() override;
    void update(double dt) override;
    void handle_input() override;
    void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) override;
    void render_controls(int x, int y, int w, int h) override;
    void render_properties(int x, int y, int w, int h) override;

    [[nodiscard]] const char* get_name() const override { return "Spectrum"; }
    [[nodiscard]] bool uses_3d() const override { return false; }

private:
    int element_index_ = 0;
    int selected_level_from_ = -1;
    int selected_level_to_ = -1;
    bool emission_mode_ = true;

    PeriodicTable periodic_table_;
    HelpPopup help_popup_;

    static constexpr double SPECTRUM_MIN_NM = 1.0;
    static constexpr double SPECTRUM_MAX_NM = 1000.0;
    static constexpr double HC_EV_NM = 1239.84193;
    static constexpr double SPEED_OF_LIGHT = 2.99792458e8;

    void select_element(int index);
    void clamp_level_indices();

    [[nodiscard]] double selected_transition_energy_eV() const;
    [[nodiscard]] double selected_transition_wavelength_nm() const;
    [[nodiscard]] double selected_transition_frequency_hz() const;
    [[nodiscard]] bool has_valid_transition() const;

    void render_em_spectrum_bar(int x, int y, int w, int h);
    void render_energy_levels(int x, int y, int w, int h);
    void render_line_spectrum(int x, int y, int w, int h);

    bool render_button(const char* label, int x, int y, int w, int btn_h, Color bg);
    bool render_toggle_button(const char* label_on, const char* label_off, bool state,
                              int x, int y, int w);

    [[nodiscard]] float wavelength_to_x(double wl_nm, int bar_x, int bar_w) const;
};
