# System Architecture: Quantum Physics Simulator

## 📐 Overview
The system follows a **Decoupled Engine Pattern**. This ensures that the mathematical ground truth remains pure and can be verified independently of the graphics API (Raylib).

## 🧩 1. PhysicsEngine (The "Brain")
- **Responsibility:** Pure mathematical simulation of quantum states.
- **Key Components:**
  - `Constants.h`: Centralized scientific constants ($h, e, c, R_H, m_e$).
  - `IQuantumObject.hpp`: Abstract interface for all quantum objects (wavefunctions, state vectors).
  - `WaveFunction`: Calculates radial and angular probability densities ($|\psi|^2$).
  - `AtomicState`: Manages quantum numbers ($n, l, m, s$) and energy transitions ($\Delta E$).
  - `WavePacket.cpp/h`: Gaussian wave-packet propagation for wave-particle duality labs.
  - `SpinSystem.cpp/h`: Pauli matrix operations, Bloch sphere representation, superposition states.
  - `HydrogenModel.cpp/h`: Specialized hydrogen atom with full orbital solutions.
- **Advanced Modules:**
  - **Wave-Particle Duality:** TDSE solver for double-slit interference and wave-packet propagation.
  - **Spin & Superposition:** Pauli spin matrices ($\sigma_x, \sigma_y, \sigma_z$), Bloch sphere $\theta, \phi$ representation.
  - **Entanglement:** Multi-particle state vector mapping (Bell states $|\Phi^+\rangle, |\Psi^-\rangle$).
  - **Laser Physics:** Stimulated emission, population inversion ($N_2 > N_1$), gain coefficient calculations.
- **Mathematical Foundation:**
  - Dirac notation (Bra-Ket) for state vectors: $|\psi\rangle = \sum_i c_i |i\rangle$
  - Time-Dependent Schrödinger Equation: $i\hbar\frac{\partial}{\partial t}|\psi(t)\rangle = \hat{H}|\psi(t)\rangle$
- **Boundary:** **Strictly NO Raylib headers.** Must compile as a standalone library if needed.

## 🎨 2. RenderEngine (The "Eyes")
- **Responsibility:** Translating mathematical states into visual geometry.
- **Key Components:**
  - `BohrRenderer`: Logic for discrete orbital shells and electron "jump" animations.
  - `QuantumCloudRenderer`: Manages vertex buffers for point clouds and shader parameters for volumetric-style probability clouds.
  - `Shaders/`: Directory for GLSL fragment shaders (e.g., `electron_cloud.fs`).
- **Integration:** Directly links to Raylib. Fetches data from `PhysicsEngine` via a read-only interface.

## 🛠️ 3. UIHandler (The "Hands")
- **Responsibility:** User interaction and data readout.
- **Key Components:**
  - `InspectorPanel`: Displays live numeric data (eV, $\lambda$, current $n$).
  - `ControlPanel`: Sliders for wavelength, atomic number ($Z$), and model switching.
- **Integration:** Operates on the 2D layer. Handles input event propagation to the `Camera3D`.

## 💾 4. Data Flow
1. **User Input:** (e.g., slider changes $n$ from 1 to 2).
2. **PhysicsEngine:** Calculates transition energy $\Delta E$ and calculates the new probability grid.
3. **RenderEngine:** Updates the GPU buffers to reflect the new state.
4. **UIHandler:** Refreshes the Inspector Panel with the calculated values.

## 🌐 5. WebAssembly (WASM) Bridge
- **Paradigm:** Asynchronous Main Loop.
- **Implementation:** The standard C++ `while` loop is wrapped in `#if defined(PLATFORM_WEB)` blocks to allow the browser's `requestAnimationFrame` to handle the frame stepping via `emscripten_set_main_loop`.

## 🛡️ 6. Memory & Safety
- **Resource Lifecycle:** - Shaders and Meshes are owned by `RenderEngine` using `std::unique_ptr` with custom Raylib deleters.
  - No raw pointers passed between modules; use `const Reference` or `SharedPointer`.