#pragma once

#include <string>
#include <vector>

struct SpectralLine {
    double wavelength_nm;
    double intensity;
};

struct EnergyLevel {
    double energy_eV;
    std::string label;
};

struct ElementInfo {
    int atomic_number;
    std::string symbol;
    std::string name;
    double work_function_eV;
    std::vector<int> electron_config;
    std::vector<EnergyLevel> energy_levels;
    std::vector<SpectralLine> emission_lines;
};

class ElementData {
public:
    [[nodiscard]] static const std::vector<ElementInfo>& get_elements();
    [[nodiscard]] static const ElementInfo& get_element(int atomic_number);
    [[nodiscard]] static const ElementInfo& get_element_by_index(int idx);
    [[nodiscard]] static int element_count();
    [[nodiscard]] static std::vector<int> get_electron_shell_config(int Z);
};
