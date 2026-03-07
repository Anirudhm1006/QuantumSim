#pragma once

#include <string>
#include <raylib.h>

struct HelpEntry {
    std::string symbol;
    std::string name;
    std::string explanation;
    std::string formula;
    std::string units;
};

class HelpPopup {
public:
    HelpPopup() = default;

    void show(const HelpEntry& entry);
    void close();
    [[nodiscard]] bool is_open() const { return open_; }

    void render(Font font, bool has_font, int screen_w, int screen_h);
    void handle_input();

    static bool render_help_button(Font font, bool has_font, int x, int y);

private:
    bool open_ = false;
    HelpEntry entry_;

    void draw_text_h(Font font, bool has_font, const char* text, float x, float y, float size, Color color) const;
};
