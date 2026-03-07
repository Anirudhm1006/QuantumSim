#include <stdexcept>

#include "Scene.hpp"

Scene::Scene()
    : potential_field_(512, -10.0, 10.0)
    , solver_(512, -10.0, 10.0, 0.001)
    , selected_index_(-1)
    , simulation_running_(false)
{
}

void Scene::add_object(std::unique_ptr<IQuantumObject> obj) {
    objects_.push_back(std::move(obj));
}

void Scene::remove_object(size_t index) {
    if (index >= objects_.size()) return;

    objects_.erase(objects_.begin() + static_cast<ptrdiff_t>(index));

    if (selected_index_ == static_cast<int>(index)) {
        selected_index_ = -1;
    } else if (selected_index_ > static_cast<int>(index)) {
        --selected_index_;
    }
}

void Scene::clear() {
    objects_.clear();
    potential_field_.clear();
    solver_.reset();
    selected_index_ = -1;
    simulation_running_ = false;
}

void Scene::select(int index) {
    if (index >= 0 && index < static_cast<int>(objects_.size())) {
        selected_index_ = index;
    }
}

void Scene::deselect() {
    selected_index_ = -1;
}

IQuantumObject* Scene::get_selected() const {
    if (selected_index_ < 0 || selected_index_ >= static_cast<int>(objects_.size())) {
        return nullptr;
    }
    return objects_[selected_index_].get();
}

IQuantumObject* Scene::get_object(size_t index) const {
    if (index >= objects_.size()) return nullptr;
    return objects_[index].get();
}

void Scene::update(double dt) {
    if (!simulation_running_) return;

    solver_.time_step(dt);
}

void Scene::sync_potential_to_solver() {
    if (potential_field_.get_nx() == solver_.get_nx()) {
        solver_.set_potential(potential_field_.values());
    } else {
        PotentialField resampled(solver_.get_nx(), solver_.get_x_min(), solver_.get_x_max());
        double dx = (solver_.get_x_max() - solver_.get_x_min()) / (solver_.get_nx() - 1);
        for (int i = 0; i < solver_.get_nx(); ++i) {
            double x = solver_.get_x_min() + i * dx;
            resampled.set_value_at(i, potential_field_.get_value_at_x(x));
        }
        solver_.set_potential(resampled.values());
    }
}

nlohmann::json Scene::serialize() const {
    nlohmann::json j;

    j["potential_field"] = potential_field_.to_json();

    nlohmann::json objects_array = nlohmann::json::array();
    for (const auto& obj : objects_) {
        objects_array.push_back(obj->to_json());
    }
    j["objects"] = objects_array;

    j["solver"] = {
        {"nx", solver_.get_nx()},
        {"x_min", solver_.get_x_min()},
        {"x_max", solver_.get_x_max()},
        {"dt", 0.001}
    };

    j["selected_index"] = selected_index_;

    return j;
}

void Scene::deserialize(const nlohmann::json& j) {
    clear();

    if (j.contains("potential_field")) {
        potential_field_.from_json(j["potential_field"]);
    }

    if (j.contains("objects")) {
        for (const auto& obj_json : j["objects"]) {
            auto obj = IQuantumObject::from_json(obj_json);
            objects_.push_back(std::move(obj));
        }
    }

    if (j.contains("solver")) {
        const auto& s = j["solver"];
        int nx = s.value("nx", 512);
        double x_min = s.value("x_min", -10.0);
        double x_max = s.value("x_max", 10.0);
        double dt = s.value("dt", 0.001);
        solver_ = GridSolver(nx, x_min, x_max, dt);
        sync_potential_to_solver();
    }

    if (j.contains("selected_index")) {
        selected_index_ = j["selected_index"].get<int>();
    }
}
