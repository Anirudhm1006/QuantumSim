#pragma once

#include <string>

class Scene;

class ProjectManager {
public:
    ProjectManager() = default;
    ~ProjectManager() = default;

    [[nodiscard]] bool save_project(const Scene& scene, const std::string& filepath);
    [[nodiscard]] bool load_project(Scene& scene, const std::string& filepath);

    [[nodiscard]] const std::string& get_current_path() const { return current_path_; }
    [[nodiscard]] bool has_unsaved_changes() const { return unsaved_changes_; }
    void mark_dirty() { unsaved_changes_ = true; }
    void mark_clean() { unsaved_changes_ = false; }

    static constexpr const char* FILE_EXTENSION = ".qsim";
    static constexpr const char* FILE_FILTER = "QuantumSim Project (*.qsim)";

private:
    std::string current_path_;
    bool unsaved_changes_ = false;

    static constexpr const char* FORMAT_VERSION = "1.0";
};
