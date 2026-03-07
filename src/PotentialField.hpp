#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class PotentialField {
public:
    PotentialField();
    PotentialField(int nx, double x_min, double x_max);

    void resize(int nx, double x_min, double x_max);
    void clear();

    void set_barrier(double x_start, double x_end, double height);
    void set_well(double x_start, double x_end, double depth);
    void set_double_slit(double wall_x, double wall_thickness,
                         double slit_width, double slit_spacing,
                         double wall_height);

    void set_value_at(int index, double value);

    [[nodiscard]] double get_value_at(int index) const;
    [[nodiscard]] double get_value_at_x(double x) const;

    [[nodiscard]] const std::vector<double>& values() const { return values_; }
    [[nodiscard]] int get_nx() const { return nx_; }
    [[nodiscard]] double get_dx() const { return dx_; }
    [[nodiscard]] double get_x_min() const { return x_min_; }
    [[nodiscard]] double get_x_max() const { return x_max_; }

    [[nodiscard]] nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);

private:
    std::vector<double> values_;
    int nx_;
    double dx_;
    double x_min_;
    double x_max_;

    [[nodiscard]] int x_to_index(double x) const;
};
