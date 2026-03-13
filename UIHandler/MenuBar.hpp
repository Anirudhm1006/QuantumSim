#pragma once

#include <string>
#include <vector>

#include <raylib.h>

struct MenuAction {
    enum class Type {
        NONE,
        SCENARIO_DOUBLE_SLIT,
        SCENARIO_TUNNELING,
        SCENARIO_INFINITE_WELL,
        SCENARIO_HYDROGEN,
        SCENARIO_FREE_PARTICLE,
        SCENARIO_PHOTOELECTRIC,
        SCENARIO_SPECTRUM,
        SCENARIO_DE_BROGLIE,
        SCENARIO_HEISENBERG,
        SCENARIO_SPIN,
        SCENARIO_SUPERPOSITION,
        SCENARIO_COMPTON,
        SCENARIO_ENTANGLEMENT
    };

    Type type = Type::NONE;
};

class MenuBar {
public:
    MenuBar();
    ~MenuBar() = default;

    void set_font(Font font);

    MenuAction update();
    void render_bar();
    void render_dropdowns();

    [[nodiscard]] int get_height() const { return height_; }

private:
    struct MenuItem {
        std::string label;
        std::vector<std::string> items;
        std::vector<MenuAction::Type> actions;
        bool open = false;
        int x_pos = 0;
        int width = 0;
    };

    Font font_;
    bool has_font_;
    std::vector<MenuItem> menus_;
    int height_;
    int active_menu_;

    void close_all_menus();
    void recalculate_positions();
    [[nodiscard]] bool is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) const;
};
