#pragma once

#include <string>
#include <vector>

#include <raylib.h>

class IQuantumObject;
class GridSolver;

class PropertiesPanel {
public:
    PropertiesPanel();
    ~PropertiesPanel() = default;

    void set_font(Font font);

    void render(int menu_bar_height, const IQuantumObject* selected, const GridSolver* solver);

    [[nodiscard]] int get_width() const { return width_; }

private:
    Font font_;
    bool has_font_;
    int width_;

    struct PropertyRow {
        std::string label;
        std::string value;
        std::string unit;
    };

    void draw_text(const char* text, float x, float y, float size, Color color);
    [[nodiscard]] float measure_text(const char* text, float size) const;

    void draw_section_header(int x, int& y, const char* title);
    void draw_property_row(int x, int& y, const char* label, const char* value, const char* unit);
    void draw_separator(int x, int& y, int w);

    void render_hydrogen(int x, int& y, const IQuantumObject* obj);
    void render_spin(int x, int& y, const IQuantumObject* obj);
    void render_wavepacket(int x, int& y, const IQuantumObject* obj);
    void render_laser(int x, int& y, const IQuantumObject* obj);
    void render_entanglement(int x, int& y, const IQuantumObject* obj);
    void render_solver_info(int x, int& y, const GridSolver* solver);
};
