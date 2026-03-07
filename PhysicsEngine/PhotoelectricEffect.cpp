#include "PhotoelectricEffect.hpp"

std::optional<double> PhotoelectricEffect::compute_kinetic_energy(double wavelength_nm, double work_function_eV) {
    double E_photon = photon_energy_eV(wavelength_nm);
    double Ek = E_photon - work_function_eV;
    if (Ek <= 0.0) return std::nullopt;
    return Ek;
}

double PhotoelectricEffect::photon_energy_eV(double wavelength_nm) {
    if (wavelength_nm <= 0.0) return 0.0;
    return HC_EV_NM / wavelength_nm;
}

double PhotoelectricEffect::threshold_wavelength_nm(double work_function_eV) {
    if (work_function_eV <= 0.0) return 0.0;
    return HC_EV_NM / work_function_eV;
}

double PhotoelectricEffect::frequency_hz(double wavelength_nm) {
    if (wavelength_nm <= 0.0) return 0.0;
    return SPEED_OF_LIGHT / (wavelength_nm * 1e-9);
}

const std::vector<MetalPreset>& PhotoelectricEffect::get_metal_presets() {
    static const std::vector<MetalPreset> presets = {
        {"Cesium (Cs)",    2.1},
        {"Sodium (Na)",    2.28},
        {"Zinc (Zn)",      3.63},
        {"Copper (Cu)",    4.7},
        {"Platinum (Pt)",  5.65}
    };
    return presets;
}
