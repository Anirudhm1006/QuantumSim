# System Architecture: Quantum Physics Simulator (Sandbox)

## Overview

The system follows a **Sandbox Scene Graph** architecture. Users interact with a persistent `Scene` containing quantum objects, potential fields, emitters, and detectors. A numerical grid solver evolves the wavefunction in real-time against arbitrary, user-defined potentials. The full scene state is serializable to JSON (`.qsim`) for Save/Load workflows.

## Module Map

```
src/
  main.cpp              — Application entry point, CAD editor main loop
  Scene.hpp/cpp         — Root sandbox container (owns all objects + potential)
  PotentialField.hpp/cpp — 1D/2D potential energy grid with builder methods
  ProjectManager.hpp/cpp — JSON serialization to/from .qsim files

PhysicsEngine/
  IQuantumObject.hpp    — Abstract interface (extended with serialization)
  GridSolver.hpp/cpp    — Numerical TDSE solver (Crank-Nicolson)
  HydrogenModel.hpp/cpp — Analytical hydrogen atom (scene-placeable)
  WavePacket.hpp/cpp    — Analytical Gaussian wave packet (scene-placeable)
  SpinSystem.hpp/cpp    — Pauli matrices, Bloch sphere (scene-placeable)
  Laser.hpp/cpp         — Stimulated emission model (scene-placeable)
  Entanglement.hpp/cpp  — Bell states, multi-particle (scene-placeable)
  Constants.h           — Immutable physical constants

RenderEngine/
  (unchanged — renders scene objects via read-only interface)

UIHandler/
  MenuBar.hpp/cpp       — Top menu: File (New/Save/Load), Scenarios
  Toolbox.hpp/cpp       — Left sidebar: object placement tools
  PropertiesPanel.hpp/cpp — Right sidebar: selected object inspector
```

## 1. Scene Graph (The "World")

**Responsibility:** Own and manage all quantum objects, potential fields, and simulation state in a single container.

**Key Components:**
- `Scene`: Root container holding:
  - `std::vector<std::unique_ptr<IQuantumObject>> objects_` — all placeable quantum entities
  - `PotentialField potential_field_` — the spatial potential energy landscape
  - `GridSolver solver_` — numerical wavefunction evolution engine
  - `int selected_index_` — index of the currently selected object for the Properties Panel
- `PotentialField`: A discretized grid of potential energy values.
  - 1D: `std::vector<double> values_`, parameterized by `dx_`, `x_min_`, `x_max_`
  - 2D (future): `std::vector<std::vector<double>> values_2d_`
  - Builder methods: `set_barrier()`, `set_well()`, `set_double_slit()`, `clear()`
  - Fully serializable to/from JSON

**Scene Lifecycle:**
1. User creates a new scene (empty potential, no objects).
2. User places objects (emitters, walls, detectors) via the Toolbox.
3. `Scene::update(double dt)` calls `solver_.time_step(dt)` each frame.
4. User can Save/Load the entire scene via `ProjectManager`.

**Boundary:** The Scene does not know about Raylib. It provides read-only accessors that the `RenderEngine` and `UIHandler` query.

## 2. PhysicsEngine (The "Brain") — Extended

**Responsibility:** Pure mathematical simulation of quantum states, now including a numerical grid solver.

**Key Components (preserved):**
- `Constants.h`: Immutable $h, e, c, R_H, m_e$.
- `IQuantumObject`: Abstract interface, now extended with:
  - `virtual nlohmann::json to_json() const = 0` — serialize object state
  - `static std::unique_ptr<IQuantumObject> from_json(const nlohmann::json& j)` — factory deserializer
  - Existing methods unchanged: `get_wavefunction()`, `get_probability_density()`, `get_state_descriptor()`
- `HydrogenModel`, `SpinSystem`, `WavePacket`, `LaserSystem`, `Entanglement` — all preserved intact, each gains `to_json()` / `from_json()` implementations.

**New Component — GridSolver:**
- Implements the 1D Time-Dependent Schrodinger Equation using the **Crank-Nicolson** finite-difference method.
- Accepts a generic `std::vector<double> potential_field` so users can draw custom barriers and wells.
- Discretization: $i\hbar \frac{\psi^{n+1} - \psi^n}{\Delta t} = \frac{1}{2}(\hat{H}\psi^{n+1} + \hat{H}\psi^n)$
- Resulting tridiagonal system solved via Thomas algorithm in $O(N)$.
- Absorbing boundary conditions at grid edges to prevent unphysical reflections.
- Normalization check: $\sum |\psi_j|^2 \Delta x = 1$ verified periodically.
- All internal arithmetic in `double` / `std::complex<double>`.

**Boundary:** Strictly NO Raylib headers. Must compile as a standalone library.

## 3. RenderEngine (The "Eyes")

**Responsibility:** Translating scene state into visual geometry.

**Key Components (preserved):**
- `BohrRenderer`, `QuantumCloudRenderer`, `BlochSphereRenderer`, `WavePacketRenderer`
- `Shaders/`: GLSL fragment shaders for volumetric probability clouds

**New Responsibilities:**
- Render the `PotentialField` as a 2D height-mapped surface or barrier rectangles in 3D.
- Render the `GridSolver` wavefunction as a real-time animated probability density curve.
- Render emitter and detector icons at their scene positions.

**Integration:** Reads from `Scene` via const reference. Fetches object list and solver state.

## 4. UIHandler (The "Hands") — Overhauled

**Responsibility:** Professional CAD-style editor layout with 3 panels + central viewport.

**Layout:**
```
+------------------------------------------------------+
|  MenuBar (30px) — File | Scenarios | Help             |
+--------+------------------------------+---------------+
| Toolbox|      Central 3D Viewport     | Properties    |
| (200px)|                              | Panel (250px) |
|        |                              |               |
| [Emit] |                              | n = 3         |
| [Wall] |                              | E = -1.51 eV  |
| [Well] |                              | |psi|^2 norm  |
| [Detct]|                              | = 0.9998      |
|        |                              |               |
+--------+------------------------------+---------------+
```

**Components:**
- `MenuBar`: File (New / Save / Load), Scenarios (Double Slit, Infinite Well, Hydrogen Atom, Free Particle).
- `Toolbox`: Left sidebar with tool buttons. State machine: SELECT, PLACE_EMITTER, DRAW_WALL, DRAW_WELL, PLACE_DETECTOR.
- `PropertiesPanel`: Right sidebar displaying the selected `IQuantumObject`'s properties in a tabular format (parameter | value | unit). For `GridSolver`: shows norm, total energy, momentum expectation values.

**Visual Design Mandates:**
- Background: `{25, 25, 30, 255}` (near-black)
- Panels: `{35, 35, 42, 255}` (dark charcoal)
- Text: `{220, 220, 225, 255}` (off-white)
- Accent: `{70, 130, 180, 255}` (steel blue)
- BANNED colors: GOLD, MAGENTA, YELLOW, LIME, ORANGE, PINK, PURPLE, or any neon/arcade palette.

**Integration:** Operates on the 2D layer after `EndMode3D()`. Sends commands to `Scene` (add object, select object, save/load).

## 5. ProjectManager (The "Disk")

**Responsibility:** Serialize and deserialize the full `Scene` to/from JSON files.

**Format:** `.qsim` extension, JSON content:
```json
{
  "version": "1.0",
  "scene": {
    "potential_field": {
      "x_min": -10.0,
      "x_max": 10.0,
      "dx": 0.05,
      "values": [0.0, 0.0, 5.0, ...]
    },
    "objects": [
      {
        "type": "HydrogenModel",
        "n": 3, "l": 1, "m": 0, "Z": 1
      },
      {
        "type": "WavePacket",
        "x0": -3.0, "v": 1.0, "sigma": 0.5, "k0": 5.0, "omega": 12.5
      }
    ],
    "solver": {
      "nx": 512,
      "dt": 0.001
    }
  }
}
```

**Dependency:** `nlohmann/json` (fetched via CMake FetchContent).

**API:**
- `bool save_project(const Scene& scene, const std::string& filepath)`
- `bool load_project(Scene& scene, const std::string& filepath)`

## 6. Data Flow

1. **User Input:** User clicks "Add Wall" in Toolbox, then clicks in the viewport.
2. **UIHandler:** Sends `scene.potential_field().set_barrier(x1, x2, height)` command.
3. **Scene:** Updates `PotentialField`, calls `solver_.set_potential(potential_field_.values())`.
4. **GridSolver:** On next `time_step(dt)`, evolves $\psi$ through the updated potential via Crank-Nicolson.
5. **RenderEngine:** Reads `solver_.get_probability_density()` and renders the animated wavefunction curve + barrier geometry.
6. **PropertiesPanel:** Reads `solver_.get_norm()`, `solver_.get_energy()` and displays them.

## 7. WASM Bridge

Unchanged from prior architecture:
- Asynchronous main loop via `emscripten_set_main_loop`.
- `#if defined(PLATFORM_WEB)` guards around the main loop.
- `-s ASYNCIFY` flag in CMake for non-blocking browser execution.

## 8. Memory and Safety

- **Scene Ownership:** `Scene` exclusively owns all `IQuantumObject` instances via `std::unique_ptr`.
- **No raw new/delete.** Smart pointers throughout.
- **Shader/Mesh lifecycle:** `RenderEngine` owns GPU resources with Raylib `Unload*` calls in destructors.
- **Solver buffers:** `GridSolver` manages its own `std::vector` allocations — no external raw pointer access.
- **JSON library:** Header-only `nlohmann/json` — zero additional runtime dependencies.
