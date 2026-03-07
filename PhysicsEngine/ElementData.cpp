#include "ElementData.hpp"
#include <algorithm>
#include <cmath>

static std::vector<ElementInfo> build_elements() {
    std::vector<ElementInfo> elems;

    // Hydrogen
    elems.push_back({1, "H", "Hydrogen", 0.0, {1},
        {{-13.60, "n=1"}, {-3.40, "n=2"}, {-1.51, "n=3"}, {-0.85, "n=4"}, {-0.54, "n=5"}, {-0.38, "n=6"}},
        {{121.6, 1.0}, {102.6, 0.8}, {97.3, 0.6}, {95.0, 0.4},
         {656.3, 1.0}, {486.1, 0.8}, {434.0, 0.6}, {410.2, 0.5},
         {1875.1, 0.7}, {1282.2, 0.5}}
    });

    // Helium
    elems.push_back({2, "He", "Helium", 0.0, {2},
        {{-24.59, "1s"}, {-5.95, "2s"}, {-4.77, "2p"}, {-3.62, "3s"}, {-1.87, "3d"}},
        {{58.4, 1.0}, {388.9, 0.6}, {501.6, 0.5}, {587.6, 0.9}, {667.8, 0.7}, {706.5, 0.4}}
    });

    // Lithium
    elems.push_back({3, "Li", "Lithium", 2.93, {2, 1},
        {{-5.39, "2s"}, {-3.54, "2p"}, {-2.02, "3s"}, {-1.56, "3p"}, {-1.05, "3d"}},
        {{670.8, 1.0}, {610.4, 0.4}, {460.3, 0.3}, {413.3, 0.2}}
    });

    // Sodium
    elems.push_back({11, "Na", "Sodium", 2.28, {2, 8, 1},
        {{-5.14, "3s"}, {-3.04, "3p"}, {-1.95, "4s"}, {-1.52, "3d"}, {-1.39, "4p"}},
        {{589.0, 1.0}, {589.6, 0.95}, {330.2, 0.5}, {568.8, 0.3}, {498.3, 0.2}}
    });

    // Neon
    elems.push_back({10, "Ne", "Neon", 0.0, {2, 8},
        {{-21.56, "2p"}, {-5.09, "3s"}, {-4.05, "3p"}, {-2.73, "3d"}, {-2.49, "4s"}},
        {{585.2, 0.8}, {614.3, 1.0}, {640.2, 0.9}, {703.2, 0.6}, {724.5, 0.5}}
    });

    // Magnesium
    elems.push_back({12, "Mg", "Magnesium", 3.66, {2, 8, 2},
        {{-7.65, "3s"}, {-4.35, "3p"}, {-2.71, "4s"}, {-2.51, "3d"}},
        {{285.2, 1.0}, {383.8, 0.5}, {518.4, 0.4}, {552.8, 0.3}}
    });

    // Potassium
    elems.push_back({19, "K", "Potassium", 2.29, {2, 8, 8, 1},
        {{-4.34, "4s"}, {-2.73, "4p"}, {-1.74, "5s"}, {-1.62, "3d"}},
        {{766.5, 1.0}, {769.9, 0.9}, {404.4, 0.5}, {344.6, 0.3}}
    });

    // Calcium
    elems.push_back({20, "Ca", "Calcium", 2.87, {2, 8, 8, 2},
        {{-6.11, "4s"}, {-3.15, "4p"}, {-2.93, "3d"}, {-1.89, "5s"}},
        {{422.7, 1.0}, {396.8, 0.6}, {393.4, 0.5}, {643.9, 0.4}}
    });

    // Iron
    elems.push_back({26, "Fe", "Iron", 4.5, {2, 8, 14, 2},
        {{-7.90, "4s"}, {-5.01, "3d"}, {-3.73, "4p"}},
        {{438.4, 0.7}, {440.5, 0.6}, {526.9, 0.5}, {532.8, 1.0}, {537.1, 0.4}}
    });

    // Copper
    elems.push_back({29, "Cu", "Copper", 4.65, {2, 8, 18, 1},
        {{-7.73, "4s"}, {-5.12, "3d"}, {-3.82, "4p"}},
        {{324.8, 1.0}, {327.4, 0.8}, {510.6, 0.5}, {515.3, 0.4}, {521.8, 0.3}}
    });

    // Zinc
    elems.push_back({30, "Zn", "Zinc", 3.63, {2, 8, 18, 2},
        {{-9.39, "4s"}, {-5.80, "4p"}, {-4.01, "5s"}},
        {{213.9, 1.0}, {307.6, 0.5}, {334.5, 0.4}, {468.0, 0.3}, {636.2, 0.2}}
    });

    // Argon
    elems.push_back({18, "Ar", "Argon", 0.0, {2, 8, 8},
        {{-15.76, "3p"}, {-4.21, "4s"}, {-3.47, "4p"}, {-2.82, "3d"}},
        {{696.5, 1.0}, {706.7, 0.8}, {738.4, 0.7}, {750.4, 0.9}, {763.5, 0.6}, {811.5, 0.5}}
    });

    // Mercury
    elems.push_back({80, "Hg", "Mercury", 4.49, {2, 8, 18, 32, 18, 2},
        {{-10.44, "6s"}, {-5.77, "6p"}, {-4.67, "7s"}},
        {{253.7, 1.0}, {365.0, 0.6}, {404.7, 0.8}, {435.8, 0.9}, {546.1, 0.7}, {577.0, 0.5}, {579.1, 0.4}}
    });

    // Cesium
    elems.push_back({55, "Cs", "Cesium", 2.1, {2, 8, 18, 18, 8, 1},
        {{-3.89, "6s"}, {-2.30, "6p"}, {-1.46, "7s"}, {-1.39, "5d"}},
        {{852.1, 1.0}, {894.3, 0.8}, {455.5, 0.5}, {459.3, 0.4}}
    });

    // Platinum
    elems.push_back({78, "Pt", "Platinum", 5.65, {2, 8, 18, 32, 17, 1},
        {{-9.0, "6s"}, {-6.1, "5d"}, {-4.5, "6p"}},
        {{265.9, 1.0}, {270.2, 0.7}, {299.8, 0.5}, {306.5, 0.4}}
    });

    return elems;
}

const std::vector<ElementInfo>& ElementData::get_elements() {
    static const std::vector<ElementInfo> elements = build_elements();
    return elements;
}

const ElementInfo& ElementData::get_element(int atomic_number) {
    const auto& elems = get_elements();
    for (const auto& e : elems) {
        if (e.atomic_number == atomic_number) return e;
    }
    return elems[0];
}

const ElementInfo& ElementData::get_element_by_index(int idx) {
    const auto& elems = get_elements();
    if (idx >= 0 && idx < static_cast<int>(elems.size())) return elems[idx];
    return elems[0];
}

int ElementData::element_count() {
    return static_cast<int>(get_elements().size());
}

std::vector<int> ElementData::get_electron_shell_config(int Z) {
    std::vector<int> shells;
    const int max_per_shell[] = {2, 8, 18, 32, 32, 18, 8};
    int remaining = Z;
    for (int i = 0; i < 7 && remaining > 0; ++i) {
        int n = std::min(remaining, max_per_shell[i]);
        shells.push_back(n);
        remaining -= n;
    }
    return shells;
}
