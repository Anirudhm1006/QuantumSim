#include <cmath>
#include <algorithm>

#include "SpectralTransition.hpp"

double SpectralTransition::energy_level_eV(int n) {
    if (n <= 0) return 0.0;
    return -RYDBERG_EV / (static_cast<double>(n) * static_cast<double>(n));
}

double SpectralTransition::transition_energy_eV(int n_from, int n_to) {
    return energy_level_eV(n_to) - energy_level_eV(n_from);
}

double SpectralTransition::transition_wavelength_nm(int n_from, int n_to) {
    double dE = std::abs(transition_energy_eV(n_from, n_to));
    if (dE < 1e-15) return 0.0;
    return HC_EV_NM / dE;
}

double SpectralTransition::transition_frequency_hz(int n_from, int n_to) {
    double lambda_nm = transition_wavelength_nm(n_from, n_to);
    if (lambda_nm < 1e-15) return 0.0;
    return SPEED_OF_LIGHT / (lambda_nm * 1e-9);
}

std::string SpectralTransition::identify_series(int n_lower) {
    switch (n_lower) {
        case 1: return "Lyman (UV)";
        case 2: return "Balmer (Visible)";
        case 3: return "Paschen (IR)";
        case 4: return "Brackett (IR)";
        case 5: return "Pfund (Far IR)";
        default: return "Higher series";
    }
}

Color SpectralTransition::wavelength_to_rgb(double wavelength_nm) {
    double r = 0.0, g = 0.0, b = 0.0;

    if (wavelength_nm < 380.0) {
        r = 0.4; g = 0.0; b = 0.6;
    } else if (wavelength_nm < 440.0) {
        double t = (wavelength_nm - 380.0) / (440.0 - 380.0);
        r = (1.0 - t) * 0.5;
        g = 0.0;
        b = 1.0;
    } else if (wavelength_nm < 490.0) {
        double t = (wavelength_nm - 440.0) / (490.0 - 440.0);
        r = 0.0;
        g = t;
        b = 1.0;
    } else if (wavelength_nm < 510.0) {
        double t = (wavelength_nm - 490.0) / (510.0 - 490.0);
        r = 0.0;
        g = 1.0;
        b = 1.0 - t;
    } else if (wavelength_nm < 580.0) {
        double t = (wavelength_nm - 510.0) / (580.0 - 510.0);
        r = t;
        g = 1.0;
        b = 0.0;
    } else if (wavelength_nm < 645.0) {
        double t = (wavelength_nm - 580.0) / (645.0 - 580.0);
        r = 1.0;
        g = 1.0 - t;
        b = 0.0;
    } else if (wavelength_nm < 780.0) {
        r = 1.0;
        g = 0.0;
        b = 0.0;
    } else {
        r = 0.5; g = 0.0; b = 0.0;
    }

    double intensity = 1.0;
    if (wavelength_nm >= 380.0 && wavelength_nm < 420.0) {
        intensity = 0.3 + 0.7 * (wavelength_nm - 380.0) / (420.0 - 380.0);
    } else if (wavelength_nm >= 645.0 && wavelength_nm < 780.0) {
        intensity = 0.3 + 0.7 * (780.0 - wavelength_nm) / (780.0 - 645.0);
    }

    return Color{
        static_cast<unsigned char>(std::clamp(r * intensity * 255.0, 0.0, 255.0)),
        static_cast<unsigned char>(std::clamp(g * intensity * 255.0, 0.0, 255.0)),
        static_cast<unsigned char>(std::clamp(b * intensity * 255.0, 0.0, 255.0)),
        255
    };
}

std::string SpectralTransition::wavelength_region(double wavelength_nm) {
    if (wavelength_nm < 10.0) return "X-ray";
    if (wavelength_nm < 380.0) return "Ultraviolet";
    if (wavelength_nm < 780.0) return "Visible";
    if (wavelength_nm < 1000000.0) return "Infrared";
    return "Microwave";
}
