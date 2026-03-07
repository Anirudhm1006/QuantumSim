#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "ProjectManager.hpp"
#include "Scene.hpp"

bool ProjectManager::save_project(const Scene& scene, const std::string& filepath) {
    try {
        nlohmann::json root;
        root["version"] = FORMAT_VERSION;
        root["scene"] = scene.serialize();

        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ProjectManager] Failed to open file for writing: "
                      << filepath << std::endl;
            return false;
        }

        file << root.dump(2);
        file.close();

        current_path_ = filepath;
        unsaved_changes_ = false;

        std::cout << "[ProjectManager] Saved project to: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ProjectManager] Save failed: " << e.what() << std::endl;
        return false;
    }
}

bool ProjectManager::load_project(Scene& scene, const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ProjectManager] Failed to open file for reading: "
                      << filepath << std::endl;
            return false;
        }

        nlohmann::json root = nlohmann::json::parse(file);
        file.close();

        if (!root.contains("version") || !root.contains("scene")) {
            std::cerr << "[ProjectManager] Invalid .qsim file format" << std::endl;
            return false;
        }

        scene.deserialize(root["scene"]);

        current_path_ = filepath;
        unsaved_changes_ = false;

        std::cout << "[ProjectManager] Loaded project from: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ProjectManager] Load failed: " << e.what() << std::endl;
        return false;
    }
}
