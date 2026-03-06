# Quantum Sim Roadmap & Task Tracker

## 🟢 Phase 1: Infrastructure & Scaffolding
- [x] **Setup Project Structure:** Create directories (`src`, `include`, `PhysicsEngine`, `RenderEngine`, `resources/shaders`).
- [x] **CMake Integration:** Implement `CMakeLists.txt` with Raylib FetchContent (DevOps Agent).
- [x] **Sanity Check Build:** Compile a basic Raylib "Hello World" window (Visualizer Agent).
- [ ] **WASM Setup:** Verify cross-compilation toolchain for browser support (DevOps Agent).

## 🟡 Phase 2: Scientific Core (PhysicsEngine)
- [x] **Physical Constants:** Define $h, c, R_H$ in `Constants.h` (Physicist Agent).
- [x] **Bohr Math:** Implement $E_n = -13.6/n^2$ calculations (Physicist Agent).
- [x] **Wavefunction Logic:** Implement Associated Laguerre Polynomials for Hydrogen radial distribution (Physicist Agent).
- [x] **Unit Tests:** Verify spectral line output for Hydrogen Balmer series (Lead/Physicist).
- [x] **Wave-Particle Duality:** Gaussian wave-packet superposition with interference (Physicist Agent).
- [x] **Spin & Superposition:** Pauli matrices, Bloch sphere representation (Physicist Agent).
- [x] **Entanglement:** Multi-particle state vectors (Bell states) (Physicist Agent).
- [x] **Laser Physics:** Stimulated emission, population inversion (Physicist Agent).

## 🔵 Phase 3: Visual Implementation (RenderEngine) - IN PROGRESS
- [x] **3D Visualizers:** Bloch sphere and wave packet rendering (Visualizer Agent).
- [x] **Model Toggle:** Keys 1-4 to switch between simulation modes (Visualizer Agent).
- [x] **Camera Controls:** Setup Orbital Camera for atomic inspection (Visualizer Agent).
- [x] **UIHandler:** Inspector panel with real-time data readout (UI/UX Designer).
- [ ] **Bohr View:** Render concentric shells and an orbiting electron "sprite" (Visualizer Agent).
- [ ] **Probability Cloud Shader:** Write GLSL fragment shader for volumetric $|\psi|^2$ rendering (Visualizer Agent).

## 🟠 Phase 4: UI & Labs
- [x] **Inspector Panel:** Real-time eV and $\lambda$ readout using Raylib 2D overlay (UI/UX Designer).
- [ ] **Transition Lab:** User-defined photon λ to trigger electron jumps (Lead Agent).
- [ ] **Photoelectric Lab:** Interactive metal plate simulation (Lead Agent).

## 🏁 Phase 5: Distribution
- [ ] **CI/CD Pipeline:** Create GitHub Actions for Win/Mac/Linux binaries (DevOps Agent).
- [ ] **WASM Deployment:** Optimize build for web hosting (DevOps Agent).