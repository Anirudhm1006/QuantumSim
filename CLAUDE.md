# Quantum Physics Simulator (Fysikk 2) - Lead Architect Brief

## ⚛️ Project Context
High-performance C++17 educational simulator for the Norwegian VGS "Fysikk 2" curriculum. 
Uses Raylib for 3D visualization and dual-mode (Deterministic Bohr vs. Probabilistic Wavefunction) rendering.
Target: Standalone binaries for Windows, Linux, and Mac. Secondary: WebAssembly (WASM).

## 🏛 Team & Context Routing
- **Lead Agent (You):** Orchestrate logic and enforce global architecture.
- **Specialists:** See `@.claude/agents/*.md` for specific role instructions.
- **Skills:** Refer to `@skills/*.md` for domain-specific logic (Physics math, GLSL, CMake).
- **Auto-Memory:** Update `@TODO.md` after every successful milestone. Update `@ARCH.md` for architectural shifts.

## 🛠 Critical Commands
- **Build:** `mkdir -p build && cd build && cmake .. && make -j$(nproc)`
- **Run:** `./build/QuantumSim`
- **Clean:** `rm -rf build/`
- **WASM Build:** `./scripts/build_wasm.sh` (Requires emscripten)
- **Compact Context:** Use `/compact` every 30 minutes to manage API token costs.
- **Verification:** Always run `grep -r "TODO" .` before declaring a task complete.

## 📜 Coding Standards (C++17)
- **Style:** `snake_case` for variables, `PascalCase` for Classes/Structs.
- **Memory:** Zero raw `new/delete`. Use `std::unique_ptr` and `std::shared_ptr`.
- **Headers:** `#include` order: Standard Lib -> Raylib -> Local Project Headers.
- **Math:** Use `double` for all physics calculations; cast to `float` only for Raylib drawing.
- **Safety:** Every mathematical function must be marked `[[nodiscard]]`.

## 🧪 Physics Ground Truth (Non-Negotiable)
- All simulation logic **must** align with formulas in `@skills/physics_engine_skill.md`.
- Rydberg constant ($R_H$) and Planck's constant ($h$) must be treated as immutable constants.
- Bohr orbits are discrete integers ($n=1, 2, 3...$). Transition energy must match $\Delta E = hf$.

## 🔄 Vibe Coding Workflow
1. **Plan-First:** You must provide a structured plan and receive explicit user approval before writing any code.
2. **Zero-Trust:** Treat your own generated code as untrusted. Verify all Raylib memory deallocations (`UnloadShader`, `UnloadMesh`) explicitly.
3. **Subagent Delegation:** When the user asks for a shader or a complex build script, acknowledge the task, then prompt: "I am spawning the `gl_expert` / `devops` agent to handle this."
4. **Verification Loop:** After implementing a feature, cross-reference the output against the "Mini-Lab" requirements in the Project Brief.

## 💾 Agent Compact Instructions
When the `/compact` command is run, prioritize preserving:
- The current $n$-level and wavefunction state logic.
- The specific Raylib camera configuration used in the last stable build.
- The path to the active GLSL fragment shader.