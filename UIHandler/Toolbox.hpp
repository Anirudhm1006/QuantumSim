#pragma once

#include <string>
#include <vector>

#include <raylib.h>

enum class ToolType {
    SELECT,
    PLACE_EMITTER,
    DRAW_WALL,
    DRAW_WELL,
    PLACE_DETECTOR,
    DRAW_POTENTIAL
};

class Toolbox {
public:
    Toolbox();
    ~Toolbox() = default;

    void set_font(Font font);

    void update(int menu_bar_height);
    void render(int menu_bar_height);

    [[nodiscard]] ToolType get_active_tool() const { return active_tool_; }
    void set_active_tool(ToolType tool) { active_tool_ = tool; }

    [[nodiscard]] int get_width() const { return width_; }

    [[nodiscard]] bool consumes_click(int mx, int my, int menu_bar_height) const;

private:
    struct ToolButton {
        std::string label;
        ToolType tool;
    };

    Font font_;
    bool has_font_;
    std::vector<ToolButton> buttons_;
    ToolType active_tool_;
    int width_;
    int button_height_;
    int button_margin_;
    int button_spacing_;

    [[nodiscard]] bool is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) const;
};
