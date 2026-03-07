#pragma once

#include <optional>
#include <string>
#include <vector>

struct MetalPreset {
    std::string name;
    double work_function_eV;
};

class PhotoelectricEffect {
public:
    PhotoelectricEffect() = default;
    ~PhotoelectricEffect() = default;

    [[nodiscard]] static std::optional<double> compute_kinetic_energy(double wavelength_nm, double work_function_eV);

    [[nodiscard]] static double photon_energy_eV(double wavelength_nm);

    [[nodiscard]] static double threshold_wavelength_nm(double work_function_eV);

    [[nodiscard]] static double frequency_hz(double wavelength_nm);

    [[nodiscard]] static const std::vector<MetalPreset>& get_metal_presets();

private:
    static constexpr double PLANCK_CONSTANT = 6.62607015e-34;
    static constexpr double SPEED_OF_LIGHT  = 2.99792458e8;
    static constexpr double ELECTRON_CHARGE = 1.602176634e-19;
    static constexpr double HC_EV_NM        = 1239.84193;
};
