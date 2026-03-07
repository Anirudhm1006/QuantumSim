#pragma once

#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "IQuantumObject.hpp"
#include "GridSolver.hpp"
#include "PotentialField.hpp"

class Scene {
public:
    Scene();
    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = default;

    void add_object(std::unique_ptr<IQuantumObject> obj);
    void remove_object(size_t index);
    void clear();

    void select(int index);
    void deselect();

    [[nodiscard]] IQuantumObject* get_selected() const;
    [[nodiscard]] int get_selected_index() const { return selected_index_; }
    [[nodiscard]] size_t object_count() const { return objects_.size(); }
    [[nodiscard]] IQuantumObject* get_object(size_t index) const;

    PotentialField& potential_field() { return potential_field_; }
    [[nodiscard]] const PotentialField& potential_field() const { return potential_field_; }

    GridSolver& solver() { return solver_; }
    [[nodiscard]] const GridSolver& solver() const { return solver_; }

    void update(double dt);
    void sync_potential_to_solver();

    [[nodiscard]] bool is_simulation_running() const { return simulation_running_; }
    void set_simulation_running(bool running) { simulation_running_ = running; }

    [[nodiscard]] nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& j);

private:
    std::vector<std::unique_ptr<IQuantumObject>> objects_;
    PotentialField potential_field_;
    GridSolver solver_;
    int selected_index_;
    bool simulation_running_;
};
