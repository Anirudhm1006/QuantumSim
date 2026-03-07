#include <raylib.h>
#include "MenuBar.hpp"

namespace {
    const Color PANEL_BG      = {35, 35, 42, 255};
    const Color PANEL_BORDER  = {55, 55, 65, 255};
    const Color PANEL_HOVER   = {45, 45, 55, 255};
    const Color TEXT_PRIMARY  = {220, 220, 225, 255};
    const Color TEXT_SECONDARY = {140, 140, 150, 255};
    const Color ACCENT        = {70, 130, 180, 255};
}

MenuBar::MenuBar()
    : font_{}
    , has_font_(false)
    , height_(30)
    , active_menu_(-1)
{
    MenuItem scenario_menu;
    scenario_menu.label = "Scenarios";
    scenario_menu.items = {
        "Double Slit",
        "Tunneling",
        "Infinite Well",
        "Atom Viewer",
        "Free Particle",
        "Photoelectric Effect",
        "Spectra",
        "De Broglie",
        "Heisenberg"
    };
    scenario_menu.actions = {
        MenuAction::Type::SCENARIO_DOUBLE_SLIT,
        MenuAction::Type::SCENARIO_TUNNELING,
        MenuAction::Type::SCENARIO_INFINITE_WELL,
        MenuAction::Type::SCENARIO_HYDROGEN,
        MenuAction::Type::SCENARIO_FREE_PARTICLE,
        MenuAction::Type::SCENARIO_PHOTOELECTRIC,
        MenuAction::Type::SCENARIO_SPECTRUM,
        MenuAction::Type::SCENARIO_DE_BROGLIE,
        MenuAction::Type::SCENARIO_HEISENBERG
    };
    menus_.push_back(scenario_menu);

    recalculate_positions();
}

void MenuBar::set_font(Font font) {
    font_ = font;
    has_font_ = true;
    recalculate_positions();
}

void MenuBar::recalculate_positions() {
    int x_offset = 10;
    for (auto& menu : menus_) {
        menu.x_pos = x_offset;
        if (has_font_) {
            menu.width = static_cast<int>(MeasureTextEx(font_, menu.label.c_str(), 14, 1).x) + 20;
        } else {
            menu.width = MeasureText(menu.label.c_str(), 14) + 20;
        }
        x_offset += menu.width + 4;
    }
}

MenuAction MenuBar::update() {
    MenuAction action;
    action.type = MenuAction::Type::NONE;

    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < static_cast<int>(menus_.size()); ++i) {
            if (is_point_in_rect(mx, my, menus_[i].x_pos, 0, menus_[i].width, height_)) {
                if (active_menu_ == i) {
                    close_all_menus();
                } else {
                    close_all_menus();
                    menus_[i].open = true;
                    active_menu_ = i;
                }
                return action;
            }
        }

        if (active_menu_ >= 0) {
            auto& menu = menus_[active_menu_];
            if (menu.open) {
                int dropdown_y = height_;
                int dropdown_w = 240;
                int item_h = 28;

                for (int j = 0; j < static_cast<int>(menu.items.size()); ++j) {
                    int item_y = dropdown_y + j * item_h;
                    if (is_point_in_rect(mx, my, menu.x_pos, item_y, dropdown_w, item_h)) {
                        action.type = menu.actions[j];
                        close_all_menus();
                        return action;
                    }
                }
                close_all_menus();
            }
        }
    }

    return action;
}

void MenuBar::render_bar() {
    int screen_w = GetScreenWidth();

    DrawRectangle(0, 0, screen_w, height_, PANEL_BG);
    DrawLine(0, height_ - 1, screen_w, height_ - 1, PANEL_BORDER);

    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);

    for (int i = 0; i < static_cast<int>(menus_.size()); ++i) {
        auto& menu = menus_[i];
        bool hovered = is_point_in_rect(mx, my, menu.x_pos, 0, menu.width, height_);

        if (menu.open) {
            DrawRectangle(menu.x_pos, 0, menu.width, height_, ACCENT);
        } else if (hovered) {
            DrawRectangle(menu.x_pos, 0, menu.width, height_, PANEL_HOVER);
        }

        float text_x = static_cast<float>(menu.x_pos + 10);
        float text_y = static_cast<float>((height_ - 14) / 2);

        if (has_font_) {
            DrawTextEx(font_, menu.label.c_str(), {text_x, text_y}, 14, 1, TEXT_PRIMARY);
        } else {
            DrawText(menu.label.c_str(), static_cast<int>(text_x), static_cast<int>(text_y), 14, TEXT_PRIMARY);
        }
    }

    const char* title = "QuantumSim";
    if (has_font_) {
        float tw = MeasureTextEx(font_, title, 14, 1).x;
        DrawTextEx(font_, title, {static_cast<float>(screen_w) - tw - 10.0f,
                   static_cast<float>((height_ - 14) / 2)}, 14, 1, TEXT_SECONDARY);
    } else {
        DrawText(title, screen_w - MeasureText(title, 14) - 10, (height_ - 14) / 2, 14, TEXT_SECONDARY);
    }
}

void MenuBar::render_dropdowns() {
    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);

    for (int i = 0; i < static_cast<int>(menus_.size()); ++i) {
        auto& menu = menus_[i];
        if (!menu.open) continue;

        int dropdown_y = height_;
        int dropdown_w = 240;
        int item_h = 28;
        int dropdown_h = static_cast<int>(menu.items.size()) * item_h;

        DrawRectangle(menu.x_pos, dropdown_y, dropdown_w, dropdown_h, PANEL_BG);
        DrawRectangleLinesEx({(float)menu.x_pos, (float)dropdown_y, (float)dropdown_w, (float)dropdown_h}, 1.0f, PANEL_BORDER);

        for (int j = 0; j < static_cast<int>(menu.items.size()); ++j) {
            int item_y = dropdown_y + j * item_h;
            bool item_hovered = is_point_in_rect(mx, my, menu.x_pos, item_y, dropdown_w, item_h);

            if (item_hovered) {
                DrawRectangle(menu.x_pos + 1, item_y, dropdown_w - 2, item_h, PANEL_HOVER);
            }

            float tx = static_cast<float>(menu.x_pos + 12);
            float ty = static_cast<float>(item_y + 7);

            if (has_font_) {
                DrawTextEx(font_, menu.items[j].c_str(), {tx, ty}, 13, 1, TEXT_PRIMARY);
            } else {
                DrawText(menu.items[j].c_str(), static_cast<int>(tx), static_cast<int>(ty), 13, TEXT_PRIMARY);
            }
        }
    }
}

void MenuBar::close_all_menus() {
    for (auto& menu : menus_) menu.open = false;
    active_menu_ = -1;
}

bool MenuBar::is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) const {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}
