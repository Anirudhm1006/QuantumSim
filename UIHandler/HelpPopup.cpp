#include <raylib.h>
#include "HelpPopup.hpp"

namespace {
    const Color OVERLAY_BG   = {0, 0, 0, 160};
    const Color POPUP_BG     = {40, 40, 50, 250};
    const Color POPUP_BORDER = {70, 130, 180, 255};
    const Color TEXT_PRI     = {220, 220, 225, 255};
    const Color TEXT_SEC     = {140, 140, 150, 255};
    const Color ACCENT       = {70, 130, 180, 255};
    const Color HELP_BTN_BG  = {55, 55, 65, 255};
    const Color HELP_BTN_TXT = {120, 120, 140, 255};
}

void HelpPopup::draw_text_h(Font font, bool has_font, const char* text, float x, float y, float size, Color color) const {
    if (has_font) DrawTextEx(font, text, {x, y}, size, 1, color);
    else DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), color);
}

void HelpPopup::show(const HelpEntry& entry) {
    entry_ = entry;
    open_ = true;
}

void HelpPopup::close() {
    open_ = false;
}

void HelpPopup::handle_input() {
    if (!open_) return;
    if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        open_ = false;
    }
}

void HelpPopup::render(Font font, bool has_font, int screen_w, int screen_h) {
    if (!open_) return;

    DrawRectangle(0, 0, screen_w, screen_h, OVERLAY_BG);

    int popup_w = 420;
    int popup_h = 240;
    int px = (screen_w - popup_w) / 2;
    int py = (screen_h - popup_h) / 2;

    DrawRectangleRounded({static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(popup_w), static_cast<float>(popup_h)}, 0.05f, 4, POPUP_BG);
    DrawRectangleRoundedLines({static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(popup_w), static_cast<float>(popup_h)}, 0.05f, 4, 2.0f, POPUP_BORDER);

    int tx = px + 20;
    int ty = py + 16;

    draw_text_h(font, has_font, entry_.symbol.c_str(), static_cast<float>(tx), static_cast<float>(ty), 22, ACCENT);
    float sym_w = has_font ? MeasureTextEx(font, entry_.symbol.c_str(), 22, 1).x : static_cast<float>(MeasureText(entry_.symbol.c_str(), 22));
    draw_text_h(font, has_font, entry_.name.c_str(), static_cast<float>(tx) + sym_w + 12, static_cast<float>(ty + 4), 16, TEXT_PRI);
    ty += 36;

    DrawLine(tx, ty, px + popup_w - 20, ty, POPUP_BORDER);
    ty += 10;

    draw_text_h(font, has_font, entry_.explanation.c_str(), static_cast<float>(tx), static_cast<float>(ty), 13, TEXT_PRI);
    ty += 40;

    if (!entry_.formula.empty()) {
        draw_text_h(font, has_font, "Formula:", static_cast<float>(tx), static_cast<float>(ty), 12, TEXT_SEC);
        ty += 18;
        draw_text_h(font, has_font, entry_.formula.c_str(), static_cast<float>(tx + 10), static_cast<float>(ty), 15, ACCENT);
        ty += 28;
    }

    if (!entry_.units.empty()) {
        draw_text_h(font, has_font, TextFormat("Units: %s", entry_.units.c_str()), static_cast<float>(tx), static_cast<float>(ty), 12, TEXT_SEC);
        ty += 18;
    }

    draw_text_h(font, has_font, "Click or press Esc to close", static_cast<float>(px + popup_w / 2 - 80),
                static_cast<float>(py + popup_h - 24), 11, TEXT_SEC);
}

bool HelpPopup::render_help_button(Font font, bool has_font, int x, int y) {
    int r = 8;
    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);
    bool hovered = (mx - x) * (mx - x) + (my - y) * (my - y) <= r * r;

    DrawCircle(x, y, static_cast<float>(r), hovered ? ACCENT : HELP_BTN_BG);
    if (has_font) {
        DrawTextEx(font, "?", {static_cast<float>(x - 3), static_cast<float>(y - 6)}, 13, 1, hovered ? TEXT_PRI : HELP_BTN_TXT);
    } else {
        DrawText("?", x - 3, y - 6, 13, hovered ? TEXT_PRI : HELP_BTN_TXT);
    }

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
