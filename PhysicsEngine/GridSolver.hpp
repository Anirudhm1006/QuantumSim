#pragma once

#include <complex>
#include <vector>

class GridSolver {
public:
    GridSolver(int nx, double x_min, double x_max, double dt);
    ~GridSolver() = default;

    void set_potential(const std::vector<double>& V);
    void inject_gaussian(double x0, double sigma, double k0);

    void time_step();
    void time_step(double dt);

    [[nodiscard]] double get_norm() const;
    [[nodiscard]] double get_energy() const;
    [[nodiscard]] double get_position_expectation() const;
    [[nodiscard]] double get_momentum_expectation() const;

    [[nodiscard]] const std::vector<std::complex<double>>& get_psi() const { return psi_; }
    [[nodiscard]] std::vector<double> get_probability_density() const;
    [[nodiscard]] const std::vector<double>& get_potential() const { return potential_; }

    [[nodiscard]] int get_nx() const { return nx_; }
    [[nodiscard]] double get_dx() const { return dx_; }
    [[nodiscard]] double get_x_min() const { return x_min_; }
    [[nodiscard]] double get_x_max() const { return x_max_; }
    [[nodiscard]] double get_time() const { return time_; }

    void reset();
    void renormalize();

private:
    std::vector<std::complex<double>> psi_;
    std::vector<double> potential_;

    int nx_;
    double dx_;
    double x_min_;
    double x_max_;
    double dt_;
    double time_;
    double mass_;
    double hbar_;

    void apply_absorbing_boundaries();

    void thomas_solve(
        const std::vector<std::complex<double>>& lower,
        const std::vector<std::complex<double>>& diag,
        const std::vector<std::complex<double>>& upper,
        const std::vector<std::complex<double>>& rhs,
        std::vector<std::complex<double>>& solution
    );
};
