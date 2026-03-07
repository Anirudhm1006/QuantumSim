#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "PotentialField.hpp"

PotentialField::PotentialField()
    : nx_(512)
    , dx_(0.0)
    , x_min_(-10.0)
    , x_max_(10.0)
{
    dx_ = (x_max_ - x_min_) / (nx_ - 1);
    values_.resize(nx_, 0.0);
}

PotentialField::PotentialField(int nx, double x_min, double x_max)
    : nx_(nx)
    , dx_((x_max - x_min) / (nx - 1))
    , x_min_(x_min)
    , x_max_(x_max)
{
    if (nx < 2) {
        throw std::invalid_argument("PotentialField requires at least 2 grid points");
    }
    if (x_max <= x_min) {
        throw std::invalid_argument("x_max must be greater than x_min");
    }
    values_.resize(nx_, 0.0);
}

void PotentialField::resize(int nx, double x_min, double x_max) {
    nx_ = nx;
    x_min_ = x_min;
    x_max_ = x_max;
    dx_ = (x_max_ - x_min_) / (nx_ - 1);
    values_.assign(nx_, 0.0);
}

void PotentialField::clear() {
    std::fill(values_.begin(), values_.end(), 0.0);
}

void PotentialField::set_barrier(double x_start, double x_end, double height) {
    int i_start = std::max(0, x_to_index(x_start));
    int i_end = std::min(nx_ - 1, x_to_index(x_end));

    for (int i = i_start; i <= i_end; ++i) {
        values_[i] = height;
    }
}

void PotentialField::set_well(double x_start, double x_end, double depth) {
    int i_start = std::max(0, x_to_index(x_start));
    int i_end = std::min(nx_ - 1, x_to_index(x_end));

    for (int i = i_start; i <= i_end; ++i) {
        values_[i] = -std::abs(depth);
    }
}

void PotentialField::set_double_slit(double wall_x, double wall_thickness,
                                      double slit_width, double slit_spacing,
                                      double wall_height) {
    double wall_start = wall_x - wall_thickness / 2.0;
    double wall_end = wall_x + wall_thickness / 2.0;

    set_barrier(wall_start, wall_end, wall_height);

    double slit_center_1 = wall_x - slit_spacing / 2.0;
    double slit_center_2 = wall_x + slit_spacing / 2.0;

    int i_start = std::max(0, x_to_index(wall_start));
    int i_end = std::min(nx_ - 1, x_to_index(wall_end));

    for (int i = i_start; i <= i_end; ++i) {
        double x = x_min_ + i * dx_;
        double dist_to_slit1 = std::abs(x - slit_center_1);
        double dist_to_slit2 = std::abs(x - slit_center_2);

        bool in_slit = (dist_to_slit1 < slit_width / 2.0) ||
                       (dist_to_slit2 < slit_width / 2.0);

        if (in_slit) {
            values_[i] = 0.0;
        }
    }
}

void PotentialField::set_value_at(int index, double value) {
    if (index >= 0 && index < nx_) {
        values_[index] = value;
    }
}

double PotentialField::get_value_at(int index) const {
    if (index < 0 || index >= nx_) return 0.0;
    return values_[index];
}

double PotentialField::get_value_at_x(double x) const {
    int idx = x_to_index(x);
    if (idx < 0 || idx >= nx_) return 0.0;
    return values_[idx];
}

int PotentialField::x_to_index(double x) const {
    return static_cast<int>(std::round((x - x_min_) / dx_));
}

nlohmann::json PotentialField::to_json() const {
    nlohmann::json j;
    j["nx"] = nx_;
    j["x_min"] = x_min_;
    j["x_max"] = x_max_;
    j["values"] = values_;
    return j;
}

void PotentialField::from_json(const nlohmann::json& j) {
    nx_ = j.at("nx").get<int>();
    x_min_ = j.at("x_min").get<double>();
    x_max_ = j.at("x_max").get<double>();
    dx_ = (x_max_ - x_min_) / (nx_ - 1);
    values_ = j.at("values").get<std::vector<double>>();
}
