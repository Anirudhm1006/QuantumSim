# Quantum Sim Roadmap & Task Tracker (Sandbox Pivot)

## Phase 1: Scene Graph & Serialization
- [ ] **PotentialField:** Create `src/PotentialField.hpp/cpp` with 1D grid, builder methods (`set_barrier`, `set_well`, `set_double_slit`), and JSON serialization.
- [ ] **Scene Container:** Create `src/Scene.hpp/cpp` owning `vector<unique_ptr<IQuantumObject>>`, `PotentialField`, and `GridSolver`. Implement `add_object()`, `remove_object()`, `get_selected()`, `serialize()`, `deserialize()`.
- [ ] **IQuantumObject Extension:** Add `to_json()` and `from_json()` virtual methods to `PhysicsEngine/IQuantumObject.hpp`. Implement in all existing models (HydrogenModel, SpinSystem, WavePacket, LaserSystem, Entanglement).
- [ ] **CMake: nlohmann/json:** Add `nlohmann/json` via FetchContent in `CMakeLists.txt`.
- [ ] **ProjectManager:** Create `src/ProjectManager.hpp/cpp` with `save_project()` and `load_project()` targeting `.qsim` JSON files.

## Phase 2: Numerical Engine (GridSolver)
- [ ] **GridSolver Core:** Create `PhysicsEngine/GridSolver.hpp/cpp` implementing the 1D TDSE via Crank-Nicolson.
- [ ] **Thomas Algorithm:** Implement O(N) tridiagonal solver for the implicit Crank-Nicolson step.
- [ ] **Absorbing Boundaries:** Add absorbing/damping boundary conditions at grid edges.
- [ ] **Gaussian Injection:** Implement `inject_gaussian(x0, sigma, k0)` to place normalized wave packets on the grid.
- [ ] **Normalization Verification:** Add `get_norm()` and periodic renormalization with configurable interval.
- [ ] **Observables:** Implement `get_energy()`, `get_momentum_expectation()`, `get_position_expectation()`.

## Phase 3: UI Overhaul (CAD-Style Editor)
- [ ] **MenuBar:** Create `UIHandler/MenuBar.hpp/cpp` with File (New/Save/Load) and Scenarios (Double Slit, Infinite Well, Hydrogen Atom, Free Particle) menus.
- [ ] **Toolbox:** Create `UIHandler/Toolbox.hpp/cpp` left sidebar with tool buttons and state machine (SELECT, PLACE_EMITTER, DRAW_WALL, DRAW_WELL, PLACE_DETECTOR).
- [ ] **PropertiesPanel:** Create `UIHandler/PropertiesPanel.hpp/cpp` right sidebar with dynamic object inspection in tabular format.
- [ ] **Professional Palette:** Apply mandatory color scheme (dark grays, steel blue accent, off-white text). Ban all arcade colors (GOLD, MAGENTA, YELLOW, LIME).
- [ ] **Main Loop Rewrite:** Rewrite `src/main.cpp` to initialize `Scene` + 3-panel CAD layout. Remove legacy `SimulationMode` enum and `render_3d_scene()` switch-case.

## Phase 4: Scenario Presets & Integration
- [ ] **Double Slit Preset:** Pre-built scene with two slits in a potential barrier and a Gaussian wave packet emitter.
- [ ] **Infinite Well Preset:** Pre-built scene with high walls and selectable eigenstates.
- [ ] **Hydrogen Atom Preset:** Pre-built scene wrapping the existing `HydrogenModel` with orbital visualization.
- [ ] **Free Particle Preset:** Empty potential with a single Gaussian emitter for basic wave-packet propagation.
- [ ] **Viewport Rendering:** Render `PotentialField` as barrier geometry and `GridSolver` wavefunction as animated probability density in the 3D viewport.

## Phase 5: Distribution & Polish
- [ ] **CI/CD Pipeline:** GitHub Actions for Win/Mac/Linux binaries (DevOps Agent).
- [ ] **WASM Deployment:** Verify async main loop with new Scene architecture (DevOps Agent).
- [ ] **Keyboard Shortcuts:** Ctrl+S (Save), Ctrl+O (Load), Ctrl+N (New), Delete (remove selected object).
- [ ] **Undo/Redo Stack:** Scene-level undo/redo for object placement and potential edits.
- [ ] **Performance Profiling:** Ensure GridSolver runs at 60fps for N <= 2048 grid points.
