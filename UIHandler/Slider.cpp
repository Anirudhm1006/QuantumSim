#include <algorithm>
#include <cstdio>
#include <raylib.h>

#include "Slider.hpp"

namespace {
    const Color PANEL_BG     = {35, 35, 42, 255};
    const Color PANEL_BORDER = {55, 55, 65, 255};
    const Color TEXT_PRIMARY = {220, 220, 225, 255};
    const Color TEXT_SEC     = {140, 140, 150, 255};
    const Color ACCENT       = {70, 130, 180, 255};
    const Color TRACK_BG     = {50, 50, 60, 255};
    const Color HANDLE_COL   = {100, 170, 220, 255};
}

Slider::Slider(const std::string& label, float min_val, float max_val, float value, const std::string& fmt)
    : label_(label), fmt_(fmt), min_(min_val), max_(max_val), value_(std::clamp(value, min_val, max_val))
{}

void Slider::draw_text_s(Font font, bool has_font, const char* text, float x, float y, float size, Color color) const {
    if (has_font) {
        DrawTextEx(font, text, {x, y}, size, 1, color);
    } else {
        DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), color);
    }
}

void Slider::set_value(float v) {
    value_ = std::clamp(v, min_, max_);
}

void Slider::set_range(float min_val, float max_val) {
    min_ = min_val;
    max_ = max_val;
    value_ = std::clamp(value_, min_, max_);
}

bool Slider::render(Font font, bool has_font, int x, int y, int w) {
    float old_value = value_;

    draw_text_s(font, has_font, label_.c_str(), static_cast<float>(x), static_cast<float>(y), 12, TEXT_SEC);

    char val_buf[32];
    std::snprintf(val_buf, sizeof(val_buf), fmt_.c_str(), value_);
    float val_w = has_font ? MeasureTextEx(font, val_buf, 12, 1).x : static_cast<float>(MeasureText(val_buf, 12));
    draw_text_s(font, has_font, val_buf, static_cast<float>(x + w) - val_w - 4, static_cast<float>(y), 12, TEXT_PRIMARY);

    int track_y = y + 18;
    int track_h = 8;
    int margin = 6;
    int track_x = x + margin;
    int track_w = w - 2 * margin;

    DrawRectangleRounded({static_cast<float>(track_x), static_cast<float>(track_y),
                          static_cast<float>(track_w), static_cast<float>(track_h)}, 0.5f, 4, TRACK_BG);

    float frac = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.0f;
    int handle_x = track_x + static_cast<int>(frac * track_w);
    int handle_y = track_y + track_h / 2;
    int handle_r = 7;

    int fill_w = handle_x - track_x;
    if (fill_w > 0) {
        DrawRectangleRounded({static_cast<float>(track_x), static_cast<float>(track_y),
                              static_cast<float>(fill_w), static_cast<float>(track_h)}, 0.5f, 4, ACCENT);
    }

    DrawCircle(handle_x, handle_y, static_cast<float>(handle_r), HANDLE_COL);

    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (mx >= track_x - handle_r && mx <= track_x + track_w + handle_r &&
            my >= track_y - handle_r && my <= track_y + track_h + handle_r) {
            dragging_ = true;
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        dragging_ = false;
    }

    if (dragging_) {
        float new_frac = static_cast<float>(mx - track_x) / static_cast<float>(track_w);
        new_frac = std::clamp(new_frac, 0.0f, 1.0f);
        value_ = min_ + new_frac * (max_ - min_);
    }

    return value_ != old_value;
}
