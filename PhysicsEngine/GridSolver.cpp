#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

#include "GridSolver.hpp"

GridSolver::GridSolver(int nx, double x_min, double x_max, double dt)
    : nx_(nx)
    , dx_((x_max - x_min) / (nx - 1))
    , x_min_(x_min)
    , x_max_(x_max)
    , dt_(dt)
    , time_(0.0)
    , mass_(1.0)
    , hbar_(1.0)
{
    if (nx < 4) {
        throw std::invalid_argument("Grid must have at least 4 points");
    }
    if (x_max <= x_min) {
        throw std::invalid_argument("x_max must be greater than x_min");
    }
    if (dt <= 0.0) {
        throw std::invalid_argument("Time step must be positive");
    }

    psi_.resize(nx_, {0.0, 0.0});
    potential_.resize(nx_, 0.0);
}

void GridSolver::set_potential(const std::vector<double>& V) {
    if (static_cast<int>(V.size()) != nx_) {
        throw std::invalid_argument("Potential vector size must match grid size");
    }
    potential_ = V;
}

void GridSolver::inject_gaussian(double x0, double sigma, double k0) {
    for (int j = 0; j < nx_; ++j) {
        double x = x_min_ + j * dx_;
        double envelope = std::exp(-(x - x0) * (x - x0) / (4.0 * sigma * sigma));
        std::complex<double> phase(0.0, k0 * x);
        psi_[j] = envelope * std::exp(phase);
    }

    double norm = get_norm();
    if (norm > 1e-15) {
        double scale = 1.0 / std::sqrt(norm);
        for (auto& val : psi_) {
            val *= scale;
        }
    }
}

void GridSolver::time_step() {
    time_step(dt_);
}

void GridSolver::time_step(double dt) {
    // alpha = i * hbar * dt / (4 * mass * dx^2)
    std::complex<double> alpha(0.0, hbar_ * dt / (4.0 * mass_ * dx_ * dx_));

    std::vector<std::complex<double>> lower(nx_);
    std::vector<std::complex<double>> diag(nx_);
    std::vector<std::complex<double>> upper(nx_);
    std::vector<std::complex<double>> rhs(nx_);

    for (int j = 0; j < nx_; ++j) {
        // beta_j = i * dt * V[j] / (2 * hbar)
        std::complex<double> beta_j(0.0, dt * potential_[j] / (2.0 * hbar_));

        // LHS tridiagonal: (I + i*dt/(2*hbar)*H)
        diag[j] = 1.0 + 2.0 * alpha + beta_j;
        lower[j] = -alpha;
        upper[j] = -alpha;
    }

    // Boundary conditions: psi = 0 at edges (Dirichlet)
    diag[0] = {1.0, 0.0};
    upper[0] = {0.0, 0.0};
    diag[nx_ - 1] = {1.0, 0.0};
    lower[nx_ - 1] = {0.0, 0.0};

    // Compute RHS: (I - i*dt/(2*hbar)*H) * psi^n
    rhs[0] = {0.0, 0.0};
    rhs[nx_ - 1] = {0.0, 0.0};

    for (int j = 1; j < nx_ - 1; ++j) {
        std::complex<double> beta_j(0.0, dt * potential_[j] / (2.0 * hbar_));

        rhs[j] = alpha * psi_[j - 1]
                + (1.0 - 2.0 * alpha - beta_j) * psi_[j]
                + alpha * psi_[j + 1];
    }

    thomas_solve(lower, diag, upper, rhs, psi_);

    apply_absorbing_boundaries();

    time_ += dt;
}

void GridSolver::thomas_solve(
    const std::vector<std::complex<double>>& lower,
    const std::vector<std::complex<double>>& diag,
    const std::vector<std::complex<double>>& upper,
    const std::vector<std::complex<double>>& rhs,
    std::vector<std::complex<double>>& solution)
{
    int n = static_cast<int>(diag.size());

    std::vector<std::complex<double>> c_prime(n);
    std::vector<std::complex<double>> d_prime(n);

    // Forward sweep
    c_prime[0] = upper[0] / diag[0];
    d_prime[0] = rhs[0] / diag[0];

    for (int i = 1; i < n; ++i) {
        std::complex<double> m = lower[i] / (diag[i] - lower[i] * c_prime[i - 1]);
        c_prime[i] = upper[i] / (diag[i] - lower[i] * c_prime[i - 1]);
        d_prime[i] = (rhs[i] - lower[i] * d_prime[i - 1]) /
                     (diag[i] - lower[i] * c_prime[i - 1]);
    }

    // Back substitution
    solution[n - 1] = d_prime[n - 1];
    for (int i = n - 2; i >= 0; --i) {
        solution[i] = d_prime[i] - c_prime[i] * solution[i + 1];
    }
}

void GridSolver::apply_absorbing_boundaries() {
    int absorb_width = std::max(1, nx_ / 10);

    for (int j = 0; j < absorb_width; ++j) {
        double ratio = static_cast<double>(j) / absorb_width;
        double mask = std::sin(ratio * M_PI * 0.5);
        mask *= mask;  // sin^2 ramp: 0 at edge, 1 at interior
        psi_[j] *= mask;
    }

    for (int j = 0; j < absorb_width; ++j) {
        int idx = nx_ - 1 - j;
        double ratio = static_cast<double>(j) / absorb_width;
        double mask = std::sin(ratio * M_PI * 0.5);
        mask *= mask;
        psi_[idx] *= mask;
    }
}

double GridSolver::get_norm() const {
    double norm = 0.0;
    for (int j = 0; j < nx_; ++j) {
        norm += std::norm(psi_[j]);
    }
    return norm * dx_;
}

double GridSolver::get_energy() const {
    std::complex<double> energy(0.0, 0.0);
    double kinetic_coeff = -hbar_ * hbar_ / (2.0 * mass_ * dx_ * dx_);

    for (int j = 1; j < nx_ - 1; ++j) {
        // Kinetic: -hbar^2/(2m) * (psi[j+1] - 2*psi[j] + psi[j-1]) / dx^2
        std::complex<double> kinetic = kinetic_coeff *
            (psi_[j + 1] - 2.0 * psi_[j] + psi_[j - 1]);

        // Potential: V[j] * psi[j]
        std::complex<double> pot = potential_[j] * psi_[j];

        energy += std::conj(psi_[j]) * (kinetic + pot) * dx_;
    }

    return std::real(energy);
}

double GridSolver::get_position_expectation() const {
    double result = 0.0;
    for (int j = 0; j < nx_; ++j) {
        double x = x_min_ + j * dx_;
        result += std::norm(psi_[j]) * x;
    }
    return result * dx_;
}

double GridSolver::get_momentum_expectation() const {
    // <p> = sum conj(psi[j]) * (-i*hbar) * (psi[j+1] - psi[j-1]) / (2*dx) * dx
    std::complex<double> result(0.0, 0.0);
    std::complex<double> neg_i_hbar(0.0, -hbar_);

    for (int j = 1; j < nx_ - 1; ++j) {
        std::complex<double> dpsi = (psi_[j + 1] - psi_[j - 1]) / (2.0 * dx_);
        result += std::conj(psi_[j]) * neg_i_hbar * dpsi * dx_;
    }

    return std::real(result);
}

std::vector<double> GridSolver::get_probability_density() const {
    std::vector<double> density(nx_);
    for (int j = 0; j < nx_; ++j) {
        density[j] = std::norm(psi_[j]);
    }
    return density;
}

void GridSolver::reset() {
    std::fill(psi_.begin(), psi_.end(), std::complex<double>(0.0, 0.0));
    time_ = 0.0;
}

void GridSolver::renormalize() {
    double norm = get_norm();
    if (norm > 1e-15) {
        double scale = 1.0 / std::sqrt(norm);
        for (auto& val : psi_) {
            val *= scale;
        }
    }
}
