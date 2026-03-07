#pragma once

#include <string>
#include <raylib.h>

namespace ui_colors {
    constexpr Color BG_DARK       = {25, 25, 30, 255};
    constexpr Color PANEL_BG      = {35, 35, 42, 255};
    constexpr Color PANEL_BORDER  = {55, 55, 65, 255};
    constexpr Color PANEL_HOVER   = {45, 45, 55, 255};
    constexpr Color TEXT_PRIMARY  = {220, 220, 225, 255};
    constexpr Color TEXT_SECONDARY = {140, 140, 150, 255};
    constexpr Color TEXT_DISABLED = {80, 80, 90, 255};
    constexpr Color ACCENT        = {70, 130, 180, 255};
    constexpr Color SUCCESS       = {60, 160, 80, 255};
    constexpr Color DANGER        = {180, 60, 60, 255};
    constexpr Color WAVEFUNCTION  = {90, 160, 220, 200};
    constexpr Color POTENTIAL     = {180, 60, 60, 180};
    constexpr Color GRID_LINE    = {40, 40, 50, 255};
    constexpr Color AXIS_X       = {160, 60, 60, 255};
    constexpr Color AXIS_Y       = {60, 160, 60, 255};
    constexpr Color AXIS_Z       = {60, 60, 160, 255};
}

class IScenario {
public:
    virtual ~IScenario() = default;

    void set_font(Font font) { font_ = font; has_font_ = true; }

    virtual void on_enter() = 0;
    virtual void update(double dt) = 0;
    virtual void handle_input() = 0;
    virtual void render_viewport(Camera3D& cam, int vp_x, int vp_y, int vp_w, int vp_h) = 0;
    virtual void render_controls(int x, int y, int w, int h) = 0;
    virtual void render_properties(int x, int y, int w, int h) = 0;

    [[nodiscard]] virtual const char* get_name() const = 0;
    [[nodiscard]] virtual int get_view_count() const { return 1; }
    [[nodiscard]] virtual const char* get_view_name(int idx) const { (void)idx; return "Default"; }
    [[nodiscard]] virtual bool uses_3d() const { return true; }

    [[nodiscard]] int get_current_view() const { return current_view_; }
    void set_view(int idx) {
        if (idx >= 0 && idx < get_view_count()) current_view_ = idx;
    }

protected:
    Font font_{};
    bool has_font_ = false;
    int current_view_ = 0;

    void draw_text(const char* text, float x, float y, float size, Color color) const {
        if (has_font_) {
            DrawTextEx(font_, text, {x, y}, size, 1, color);
        } else {
            DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), color);
        }
    }

    [[nodiscard]] float measure_text(const char* text, float size) const {
        if (has_font_) return MeasureTextEx(font_, text, size, 1).x;
        return static_cast<float>(MeasureText(text, static_cast<int>(size)));
    }

    void draw_panel_bg(int x, int y, int w, int h) const {
        DrawRectangle(x, y, w, h, ui_colors::PANEL_BG);
        DrawLine(x + w - 1, y, x + w - 1, y + h, ui_colors::PANEL_BORDER);
    }

    void draw_section(const char* title, int x, int& y) const {
        draw_text(title, static_cast<float>(x), static_cast<float>(y), 14, ui_colors::ACCENT);
        y += 20;
    }

    void draw_prop(const char* label, const char* value, int x, int& y, Color val_color = ui_colors::TEXT_PRIMARY) const {
        draw_text(label, static_cast<float>(x), static_cast<float>(y), 13, ui_colors::TEXT_SECONDARY);
        draw_text(value, static_cast<float>(x + 110), static_cast<float>(y), 13, val_color);
        y += 18;
    }

    void draw_separator(int x, int& y, int w) const {
        DrawLine(x, y, x + w, y, ui_colors::PANEL_BORDER);
        y += 8;
    }
};
