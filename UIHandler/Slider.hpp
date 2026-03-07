#pragma once

#include <string>
#include <raylib.h>

class Slider {
public:
    Slider() = default;
    Slider(const std::string& label, float min_val, float max_val, float value, const std::string& fmt = "%.2f");

    bool render(Font font, bool has_font, int x, int y, int w);

    [[nodiscard]] float get_value() const { return value_; }
    void set_value(float v);
    void set_range(float min_val, float max_val);

    static constexpr int HEIGHT = 44;

private:
    std::string label_;
    std::string fmt_;
    float min_ = 0.0f;
    float max_ = 1.0f;
    float value_ = 0.5f;
    bool dragging_ = false;

    void draw_text_s(Font font, bool has_font, const char* text, float x, float y, float size, Color color) const;
};
