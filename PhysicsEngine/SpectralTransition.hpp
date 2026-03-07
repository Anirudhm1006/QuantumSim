#pragma once

#include <string>

#include <raylib.h>

class SpectralTransition {
public:
    SpectralTransition() = default;
    ~SpectralTransition() = default;

    [[nodiscard]] static double energy_level_eV(int n);

    [[nodiscard]] static double transition_energy_eV(int n_from, int n_to);

    [[nodiscard]] static double transition_wavelength_nm(int n_from, int n_to);

    [[nodiscard]] static double transition_frequency_hz(int n_from, int n_to);

    [[nodiscard]] static std::string identify_series(int n_lower);

    [[nodiscard]] static Color wavelength_to_rgb(double wavelength_nm);

    [[nodiscard]] static std::string wavelength_region(double wavelength_nm);

private:
    static constexpr double RYDBERG_EV = 13.605693009;
    static constexpr double HC_EV_NM   = 1239.84193;
    static constexpr double SPEED_OF_LIGHT = 2.99792458e8;
};
