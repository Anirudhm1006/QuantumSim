#pragma once

#include <string>
#include <vector>
#include <functional>
#include <raylib.h>

class PeriodicTable {
public:
    PeriodicTable() = default;

    void open(std::function<void(int)> on_select);
    void close();
    [[nodiscard]] bool is_open() const { return open_; }

    void render(Font font, bool has_font, int screen_w, int screen_h);
    void handle_input();

private:
    bool open_ = false;
    std::function<void(int)> on_select_;

    struct TileInfo {
        int Z;
        std::string symbol;
        std::string name;
        int grid_col;
        int grid_row;
        bool available;
    };

    [[nodiscard]] static std::vector<TileInfo> build_tiles();

    void draw_text_p(Font font, bool has_font, const char* text, float x, float y, float size, Color color) const;
};
