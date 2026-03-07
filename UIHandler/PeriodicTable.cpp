#include <raylib.h>
#include "PeriodicTable.hpp"
#include "ElementData.hpp"

namespace {
    const Color OVERLAY_BG   = {0, 0, 0, 180};
    const Color POPUP_BG     = {35, 35, 45, 250};
    const Color TILE_BG      = {50, 50, 62, 255};
    const Color TILE_AVAIL   = {60, 70, 85, 255};
    const Color TILE_HOVER   = {70, 130, 180, 255};
    const Color TILE_UNAVAIL = {40, 40, 48, 255};
    const Color TEXT_PRI     = {220, 220, 225, 255};
    const Color TEXT_SEC     = {140, 140, 150, 255};
    const Color TEXT_DIM     = {80, 80, 90, 255};
    const Color ACCENT       = {70, 130, 180, 255};
    const Color BORDER       = {55, 55, 65, 255};
}

void PeriodicTable::draw_text_p(Font font, bool has_font, const char* text, float x, float y, float size, Color color) const {
    if (has_font) DrawTextEx(font, text, {x, y}, size, 1, color);
    else DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), color);
}

void PeriodicTable::open(std::function<void(int)> on_select) {
    on_select_ = on_select;
    open_ = true;
}

void PeriodicTable::close() {
    open_ = false;
}

std::vector<PeriodicTable::TileInfo> PeriodicTable::build_tiles() {
    const auto& elems = ElementData::get_elements();
    std::vector<int> available_z;
    for (const auto& e : elems) available_z.push_back(e.atomic_number);

    struct Pos { int Z; const char* sym; const char* name; int col; int row; };
    std::vector<Pos> positions = {
        {1, "H", "Hydrogen", 0, 0}, {2, "He", "Helium", 17, 0},
        {3, "Li", "Lithium", 0, 1}, {4, "Be", "Beryllium", 1, 1},
        {5, "B", "Boron", 12, 1}, {6, "C", "Carbon", 13, 1},
        {7, "N", "Nitrogen", 14, 1}, {8, "O", "Oxygen", 15, 1},
        {9, "F", "Fluorine", 16, 1}, {10, "Ne", "Neon", 17, 1},
        {11, "Na", "Sodium", 0, 2}, {12, "Mg", "Magnesium", 1, 2},
        {13, "Al", "Aluminium", 12, 2}, {14, "Si", "Silicon", 13, 2},
        {15, "P", "Phosphorus", 14, 2}, {16, "S", "Sulfur", 15, 2},
        {17, "Cl", "Chlorine", 16, 2}, {18, "Ar", "Argon", 17, 2},
        {19, "K", "Potassium", 0, 3}, {20, "Ca", "Calcium", 1, 3},
        {26, "Fe", "Iron", 7, 3}, {29, "Cu", "Copper", 10, 3},
        {30, "Zn", "Zinc", 11, 3},
        {55, "Cs", "Cesium", 0, 5}, {78, "Pt", "Platinum", 9, 5},
        {79, "Au", "Gold", 10, 5}, {80, "Hg", "Mercury", 11, 5},
    };

    std::vector<TileInfo> tiles;
    for (const auto& p : positions) {
        bool avail = false;
        for (int z : available_z) { if (z == p.Z) { avail = true; break; } }
        tiles.push_back({p.Z, p.sym, p.name, p.col, p.row, avail});
    }
    return tiles;
}

void PeriodicTable::handle_input() {
    if (!open_) return;
    if (IsKeyPressed(KEY_ESCAPE)) {
        close();
    }
}

void PeriodicTable::render(Font font, bool has_font, int screen_w, int screen_h) {
    if (!open_) return;

    DrawRectangle(0, 0, screen_w, screen_h, OVERLAY_BG);

    int popup_w = 740;
    int popup_h = 340;
    int px = (screen_w - popup_w) / 2;
    int py = (screen_h - popup_h) / 2;

    DrawRectangleRounded({static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(popup_w), static_cast<float>(popup_h)}, 0.03f, 4, POPUP_BG);
    DrawRectangleRoundedLines({static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(popup_w), static_cast<float>(popup_h)}, 0.03f, 4, 1.5f, BORDER);

    draw_text_p(font, has_font, "Select Element", static_cast<float>(px + 20), static_cast<float>(py + 12), 16, ACCENT);
    draw_text_p(font, has_font, "Highlighted elements have spectral data. Press Esc to cancel.",
                static_cast<float>(px + 20), static_cast<float>(py + 32), 11, TEXT_SEC);

    auto tiles = build_tiles();
    int tile_w = 38;
    int tile_h = 42;
    int gap = 2;
    int grid_x = px + 14;
    int grid_y = py + 52;

    Vector2 mouse = GetMousePosition();
    int mx = static_cast<int>(mouse.x);
    int my = static_cast<int>(mouse.y);

    for (const auto& t : tiles) {
        int tx = grid_x + t.grid_col * (tile_w + gap);
        int ty = grid_y + t.grid_row * (tile_h + gap);

        bool hovered = mx >= tx && mx < tx + tile_w && my >= ty && my < ty + tile_h;

        Color bg = t.available ? (hovered ? TILE_HOVER : TILE_AVAIL) : TILE_UNAVAIL;
        Color text_col = t.available ? TEXT_PRI : TEXT_DIM;

        DrawRectangle(tx, ty, tile_w, tile_h, bg);
        DrawRectangleLinesEx({static_cast<float>(tx), static_cast<float>(ty),
                              static_cast<float>(tile_w), static_cast<float>(tile_h)}, 1.0f, BORDER);

        draw_text_p(font, has_font, TextFormat("%d", t.Z), static_cast<float>(tx + 2), static_cast<float>(ty + 2), 9, TEXT_SEC);
        draw_text_p(font, has_font, t.symbol.c_str(), static_cast<float>(tx + tile_w / 2 - 6), static_cast<float>(ty + 14), 15, text_col);

        if (hovered && t.available && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (on_select_) on_select_(t.Z);
            close();
        }
    }

    draw_text_p(font, has_font, "Esc: cancel", static_cast<float>(px + popup_w - 80), static_cast<float>(py + popup_h - 20), 11, TEXT_SEC);
}
