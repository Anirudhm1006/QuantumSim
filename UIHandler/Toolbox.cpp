#include <raylib.h>

#include "Toolbox.hpp"

namespace {
    const Color PANEL_BG      = {35, 35, 42, 255};
    const Color PANEL_BORDER  = {55, 55, 65, 255};
    const Color PANEL_HOVER   = {45, 45, 55, 255};
    const Color TEXT_PRIMARY  = {220, 220, 225, 255};
    const Color TEXT_SECONDARY = {140, 140, 150, 255};
    const Color ACCENT        = {70, 130, 180, 255};
}

Toolbox::Toolbox()
    : font_{}
    , has_font_(false)
    , active_tool_(ToolType::SELECT)
    , width_(200)
    , button_height_(36)
    , button_margin_(10)
    , button_spacing_(4)
{
    buttons_ = {
        {"Select",         ToolType::SELECT},
        {"Add Emitter",    ToolType::PLACE_EMITTER},
        {"Add Wall",       ToolType::DRAW_WALL},
        {"Add Well",       ToolType::DRAW_WELL},
        {"Add Detector",   ToolType::PLACE_DETECTOR},
        {"Draw Potential",  ToolType::DRAW_POTENTIAL}
    };
}

void Toolbox::set_font(Font font) {
    font_ = font;
    has_font_ = true;
}

void Toolbox::update(int menu_bar_height) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);

    if (mx >= width_) return;

    int y_offset = menu_bar_height + 20;

    for (const auto& btn : buttons_) {
        int btn_x = button_margin_;
        int btn_w = width_ - 2 * button_margin_;
        int btn_y = y_offset;

        if (is_point_in_rect(mx, my, btn_x, btn_y, btn_w, button_height_)) {
            active_tool_ = btn.tool;
            return;
        }

        y_offset += button_height_ + button_spacing_;
    }
}

void Toolbox::render(int menu_bar_height) {
    int screen_h = GetScreenHeight();
    int panel_h = screen_h - menu_bar_height;

    DrawRectangle(0, menu_bar_height, width_, panel_h, PANEL_BG);
    DrawLine(width_ - 1, menu_bar_height, width_ - 1, screen_h, PANEL_BORDER);

    float header_x = static_cast<float>(button_margin_);
    float header_y = static_cast<float>(menu_bar_height + 6);
    if (has_font_) {
        DrawTextEx(font_, "TOOLS", {header_x, header_y}, 12, 1, TEXT_SECONDARY);
    } else {
        DrawText("TOOLS", static_cast<int>(header_x), static_cast<int>(header_y), 12, TEXT_SECONDARY);
    }

    int y_offset = menu_bar_height + 20;

    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);

    for (const auto& btn : buttons_) {
        int btn_x = button_margin_;
        int btn_w = width_ - 2 * button_margin_;
        int btn_y = y_offset;

        bool active = (btn.tool == active_tool_);
        bool hovered = is_point_in_rect(mx, my, btn_x, btn_y, btn_w, button_height_);

        Color bg;
        if (active) {
            bg = ACCENT;
        } else if (hovered) {
            bg = PANEL_HOVER;
        } else {
            bg = PANEL_BG;
        }

        DrawRectangle(btn_x, btn_y, btn_w, button_height_, bg);
        DrawRectangleLinesEx(
            {(float)btn_x, (float)btn_y, (float)btn_w, (float)button_height_},
            1.0f, PANEL_BORDER);

        if (has_font_) {
            float label_w = MeasureTextEx(font_, btn.label.c_str(), 14, 1).x;
            float label_x = static_cast<float>(btn_x) + (static_cast<float>(btn_w) - label_w) * 0.5f;
            float label_y = static_cast<float>(btn_y) + (static_cast<float>(button_height_) - 14.0f) * 0.5f;
            DrawTextEx(font_, btn.label.c_str(), {label_x, label_y}, 14, 1, TEXT_PRIMARY);
        } else {
            int label_w = MeasureText(btn.label.c_str(), 14);
            int label_x = btn_x + (btn_w - label_w) / 2;
            int label_y = btn_y + (button_height_ - 14) / 2;
            DrawText(btn.label.c_str(), label_x, label_y, 14, TEXT_PRIMARY);
        }

        y_offset += button_height_ + button_spacing_;
    }

    int footer_y = screen_h - 60;
    DrawLine(button_margin_, footer_y, width_ - button_margin_, footer_y, PANEL_BORDER);

    if (has_font_) {
        DrawTextEx(font_, "Space: Run/Pause", {header_x, static_cast<float>(footer_y + 8)}, 12, 1, TEXT_SECONDARY);
        DrawTextEx(font_, "Del: Remove selected", {header_x, static_cast<float>(footer_y + 24)}, 12, 1, TEXT_SECONDARY);
    } else {
        DrawText("Space: Run/Pause", button_margin_, footer_y + 8, 12, TEXT_SECONDARY);
        DrawText("Del: Remove selected", button_margin_, footer_y + 24, 12, TEXT_SECONDARY);
    }
}

bool Toolbox::consumes_click(int mx, int my, int menu_bar_height) const {
    return mx >= 0 && mx < width_ && my >= menu_bar_height;
}

bool Toolbox::is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) const {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}
