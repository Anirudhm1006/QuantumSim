// =============================================================================
// Quantum Physics Simulator - 3D Visualization with UI
// =============================================================================
// Interactive quantum physics visualization with Inspector Panel
// =============================================================================

#include <cmath>
#include <memory>
#include <raylib.h>

// PhysicsEngine headers
#include "WavePacket.hpp"
#include "SpinSystem.hpp"
#include "HydrogenModel.hpp"
#include "Laser.hpp"

// UIHandler
#include "InspectorPanel.hpp"

// =============================================================================
// Simulation State
// =============================================================================

enum class SimulationMode {
    HYDROGEN,
    SPIN,
    WAVE_PACKET,
    LASER
};

struct SimState {
    SimulationMode mode = SimulationMode::HYDROGEN;

    // Hydrogen state
    int hydrogen_n = 1;
    int hydrogen_l = 0;
    int hydrogen_m = 0;

    // Spin state
    double spin_theta = 0.0;
    double spin_phi = 0.0;

    // Wave packet state
    double wave_x0_1 = -3.0;
    double wave_sigma_1 = 0.5;
    double wave_x0_2 = 3.0;
    double wave_sigma_2 = 0.5;

    // Laser state
    double laser_wavelength = 632.8;  // HeNe laser
};

// =============================================================================
// Initialize simulation state
// =============================================================================

void init_state(SimState& state) {
    state.hydrogen_n = 1;
    state.hydrogen_l = 0;
    state.hydrogen_m = 0;

    state.spin_theta = 0.0;
    state.spin_phi = 0.0;

    state.wave_x0_1 = -3.0;
    state.wave_sigma_1 = 0.5;
    state.wave_x0_2 = 3.0;
    state.wave_sigma_2 = 0.5;

    state.laser_wavelength = 632.8;
}

// =============================================================================
// Update physics data based on mode
// =============================================================================

void update_physics_data(SimState& state, ui_handler::InspectorData& data) {
    data.current_mode = static_cast<int>(state.mode);

    switch (state.mode) {
        case SimulationMode::HYDROGEN: {
            HydrogenModel h(state.hydrogen_n, state.hydrogen_l, state.hydrogen_m);
            data.n = state.hydrogen_n;
            data.l = state.hydrogen_l;
            data.m = state.hydrogen_m;
            data.energy_eV = h.get_energy_eV();
            data.quantum_mode = true;
        } break;

        case SimulationMode::SPIN: {
            SpinSystem spin(state.spin_theta, state.spin_phi);
            data.theta = state.spin_theta;
            data.phi = state.spin_phi;
            data.spin_x = spin.get_spin_projection_x();
            data.spin_y = spin.get_spin_projection_y();
            data.spin_z = spin.get_spin_projection_z();
        } break;

        case SimulationMode::WAVE_PACKET: {
            data.x0_1 = state.wave_x0_1;
            data.sigma_1 = state.wave_sigma_1;
            data.x0_2 = state.wave_x0_2;
            data.sigma_2 = state.wave_sigma_2;
            data.superposition = true;
        } break;

        case SimulationMode::LASER: {
            data.wavelength = state.laser_wavelength;
            // Calculate frequency: f = c/lambda
            double c = 299792458.0;  // m/s
            double lambda_m = state.laser_wavelength * 1e-9;
            data.frequency = c / lambda_m;
            // Photon energy: E = hf
            double h = 4.135667696e-15;  // eV·s
            data.photon_energy = h * data.frequency;
            data.n_photons = 1000;
        } break;
    }
}

// =============================================================================
// Render 3D scene based on mode
// =============================================================================

void render_3d_scene(const SimState& state) {
    switch (state.mode) {
        case SimulationMode::HYDROGEN: {
            // Draw hydrogen atom representation
            // Draw orbitals as simple spheres at different distances
            for (int n = 1; n <= state.hydrogen_n; n++) {
                float radius = (float)n * 1.5f;
                DrawSphere((Vector3){radius, 0, 0}, 0.15f, Fade(BLUE, 0.5f));
                DrawSphere((Vector3){-radius, 0, 0}, 0.15f, Fade(BLUE, 0.5f));
                DrawSphere((Vector3){0, radius, 0}, 0.15f, Fade(BLUE, 0.5f));
                DrawSphere((Vector3){0, -radius, 0}, 0.15f, Fade(BLUE, 0.5f));
            }

            // Draw nucleus
            DrawSphere((Vector3){0, 0, 0}, 0.3f, MAROON);

            // Draw label
            DrawText(TextFormat("n=%d", state.hydrogen_n),
                     -30, 50, 20, WHITE);
        } break;

        case SimulationMode::SPIN: {
            // Draw Bloch sphere as simple wireframe
            float sphere_radius = 3.0f;

            // Draw sphere surface (simplified)
            DrawSphere((Vector3){0, 0, 0}, sphere_radius, Fade(DARKGRAY, 0.3f));

            // Draw axis lines
            DrawLine3D((Vector3){0, -sphere_radius, 0}, (Vector3){0, sphere_radius, 0}, RED);
            DrawLine3D((Vector3){-sphere_radius, 0, 0}, (Vector3){sphere_radius, 0, 0}, GREEN);
            DrawLine3D((Vector3){0, 0, -sphere_radius}, (Vector3){0, 0, sphere_radius}, BLUE);

            // Draw state vector
            float sx = sphere_radius * std::sin(state.spin_theta) * std::cos(state.spin_phi);
            float sy = sphere_radius * std::cos(state.spin_theta);
            float sz = sphere_radius * std::sin(state.spin_theta) * std::sin(state.spin_phi);

            DrawLine3D((Vector3){0, 0, 0}, (Vector3){sx, sy, sz}, GREEN);
            DrawSphere((Vector3){sx, sy, sz}, 0.2f, GREEN);

            // Draw axis labels
            DrawText("|+z>", -40, 60, 20, GREEN);
            DrawText("|-z>", -40, -70, 20, RED);
            DrawText("|+x>", 40, -40, 20, YELLOW);
        } break;

        case SimulationMode::WAVE_PACKET: {
            // Draw wave packets as 3D Gaussians
            int num_points = 50;

            // Wave packet 1
            for (int i = 0; i < num_points; i++) {
                float x = -8.0f + (float)i * 16.0f / (float)num_points;
                double dx = x - state.wave_x0_1;
                double gaussian = std::exp(-dx * dx / (2 * state.wave_sigma_1 * state.wave_sigma_1));
                float size = (float)(gaussian * 0.3f);

                DrawSphere((Vector3){(float)state.wave_x0_1, 0, 0}, (float)gaussian * 0.5f, Fade(BLUE, 0.5f));
            }

            // Wave packet 2
            for (int i = 0; i < num_points; i++) {
                float x = -8.0f + (float)i * 16.0f / (float)num_points;
                double dx = x - state.wave_x0_2;
                double gaussian = std::exp(-dx * dx / (2 * state.wave_sigma_2 * state.wave_sigma_2));

                DrawSphere((Vector3){(float)state.wave_x0_2, 0, 0}, (float)gaussian * 0.5f, Fade(RED, 0.5f));
            }

            // Draw superposition region
            DrawText("Superposition", -50, 50, 20, WHITE);
        } break;

        case SimulationMode::LASER: {
            // Draw laser cavity (as a cylinder)
            DrawCylinder((Vector3){-4, 0, 0}, 0.5f, 0.5f, 8.0f, 16, Fade(GRAY, 0.5f));

            // Draw laser beam (as a thin cylinder)
            Color laser_color = {255, 50, 50, 255};
            DrawCylinder((Vector3){0, 0, 0}, 0.1f, 0.1f, 6.0f, 8, laser_color);

            // Draw mirrors (as cubes)
            Color mirror_color = {192, 192, 192, 255};  // Silver-ish
            DrawCube((Vector3){-4, 0, 0}, 1.0f, 1.0f, 0.1f, mirror_color);
            DrawCube((Vector3){4, 0, 0}, 1.0f, 1.0f, 0.1f, mirror_color);

            // Draw wavelength
            DrawText(TextFormat("lambda=%.1f nm", state.laser_wavelength),
                    -60, 50, 20, WHITE);
        } break;
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    // Initialize window
    const int screen_width = 900;
    const int screen_height = 650;
    const char* window_title = "Quantum Simulator - 3D Visualization";

    InitWindow(screen_width, screen_height, window_title);

    // Set target FPS
    SetTargetFPS(60);

    // Setup camera
    Camera3D camera = {};
    camera.position = {8.0f, 6.0f, 8.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Initialize simulation state
    SimState state;
    init_state(state);

    // Initialize UI panel
    ui_handler::InspectorPanel inspector;
    ui_handler::InspectorData inspector_data;

    // Main loop
    while (!WindowShouldClose()) {
        // Handle input
        if (IsKeyPressed(KEY_ONE)) {
            state.mode = SimulationMode::HYDROGEN;
        } else if (IsKeyPressed(KEY_TWO)) {
            state.mode = SimulationMode::SPIN;
        } else if (IsKeyPressed(KEY_THREE)) {
            state.mode = SimulationMode::WAVE_PACKET;
        } else if (IsKeyPressed(KEY_FOUR)) {
            state.mode = SimulationMode::LASER;
        } else if (IsKeyPressed(KEY_R)) {
            init_state(state);
        }

        // Parameter adjustment based on mode
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT)) {
            float delta = IsKeyDown(KEY_RIGHT) ? 0.05f : -0.05f;

            switch (state.mode) {
                case SimulationMode::HYDROGEN:
                    if (IsKeyDown(KEY_UP) && state.hydrogen_n < 5) state.hydrogen_n++;
                    if (IsKeyDown(KEY_DOWN) && state.hydrogen_n > 1) state.hydrogen_n--;
                    break;
                case SimulationMode::SPIN:
                    state.spin_theta += delta;
                    state.spin_theta = std::max(0.0, std::min((double)PI, state.spin_theta));
                    break;
                case SimulationMode::WAVE_PACKET:
                    state.wave_x0_1 += delta;
                    state.wave_x0_2 -= delta;
                    break;
                case SimulationMode::LASER:
                    state.laser_wavelength += delta * 100;
                    state.laser_wavelength = std::max(400.0, std::min(700.0, state.laser_wavelength));
                    break;
            }
        }

        // Update camera
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Update physics data for UI
        update_physics_data(state, inspector_data);
        inspector.update_data(inspector_data);

        // Begin rendering
        BeginDrawing();
        {
            // Clear background
            ClearBackground({10, 10, 20, 255});

            // Begin 3D mode
            BeginMode3D(camera);
            {
                // Draw grid
                DrawGrid(20, 1.0f);

                // Draw axes
                DrawLine3D((Vector3){0, 0, 0}, (Vector3){3, 0, 0}, RED);
                DrawLine3D((Vector3){0, 0, 0}, (Vector3){0, 3, 0}, GREEN);
                DrawLine3D((Vector3){0, 0, 0}, (Vector3){0, 0, 3}, BLUE);

                // Render quantum scene
                render_3d_scene(state);
            }
            EndMode3D();

            // Draw UI panel (2D overlay)
            inspector.render();

            // Draw mode title at top
            const char* mode_names[] = {"HYDROGEN ATOM", "SPIN (BLOCH SPHERE)", "WAVE PACKET", "HE-NE LASER"};
            int mode_idx = static_cast<int>(state.mode);
            DrawText(mode_names[mode_idx], 20, 20, 24, GOLD);

            // Draw instructions
            DrawText("Press 1-4 to switch modes, arrows to adjust, R to reset",
                     20, 55, 14, GRAY);
        }
        EndDrawing();
    }

    // Cleanup
    CloseWindow();

    return 0;
}