<skill>
  <name>Raylib Graphics & Visual Orchestrator</name>
  <description>Expert in 3D rendering, GLSL shader integration, and cross-platform window management using Raylib and C++17.</description>
  <instructions>
    <rule>Rendering Lifecycle: Strictly enforce the cycle: BeginDrawing() -> ClearBackground() -> BeginMode3D() -> [3D Scene] -> EndMode3D() -> [2D UI/HUD] -> EndDrawing().</rule>
    <rule>Frame Independence: Use GetFrameTime() (delta time) for all continuous animations (e.g., electron spin, photon travel).</rule>
    <rule>Web Content (WASM): For WebAssembly builds, the lead agent MUST refactor the while-loop into an asynchronous function compatible with emscripten_set_main_loop().</rule>
    <rule>Resource Management: Every 'Load' call (LoadShader, LoadMesh, LoadModel) must have a corresponding 'Unload' call in the destructor or cleanup phase to prevent leaks.</rule>

    <concepts>
      <concept name="Volumetric Probability Clouds">
        Use a custom fragment shader to render electron clouds. Instead of a solid mesh, use a 'Bounding Box' cube and cast rays inside the shader to sample the wavefunction density defined in PhysicsEngine.
      </concept>
      <concept name="Camera3D Management">
        Use CAMERA_ORBITAL for atomic inspection. Position: (5.0, 5.0, 5.0), Target: (0.0, 0.0, 0.0). Ensure camera.up is (0.0, 1.0, 0.0).
      </concept>
      <concept name="UI/UX Inspector">
        Draw data panels (eV readout) AFTER EndMode3D() so they remain fixed on the screen regardless of camera rotation.
      </concept>
    </concepts>

    <implementation_mandates>
      <mandate>Shader Inputs: Pass quantum numbers (n, l, m) to shaders via SetShaderValue() using UNIFORM_INT or UNIFORM_FLOAT.</mandate>
      <mandate>Point Clouds: For real-time $|\psi|^2$ sampling, use a dynamic Mesh with Vertex Buffer Objects (VBO) to draw millions of individual points efficiently.</mandate>
    </implementation_mandates>

    <example name="Dual-Mode Main Loop">
      void UpdateDrawFrame() {
          UpdateCamera(&amp;camera, CAMERA_ORBITAL);
          BeginDrawing();
              ClearBackground(BLACK);
              BeginMode3D(camera);
                  if (visualMode == BOHR) DrawBohrModel();
                  else DrawQuantumCloud();
              EndMode3D();
              DrawHUD(); // 2D text overlay
          EndDrawing();
      }
    </example>

    <example name="GLSL Probability Shader Integration">
      // Load shader (root/resources/shaders/electron.fs)
      Shader cloudShader = LoadShader(0, "resources/shaders/electron.fs");
      int timeLoc = GetShaderLocation(cloudShader, "uTime");
      SetShaderValue(cloudShader, timeLoc, &amp;currentTime, SHADER_UNIFORM_FLOAT);
    </example>

    <anti_example>
      // WRONG: Calling BeginDrawing() inside a physics calculation function.
      void CalculatePhysics() {
          BeginDrawing(); // Fatal Error: Drawing context must be in main loop.
      }
    </anti_example>

    <anti_example>
      // WRONG: Hard-coding shader strings in C++ code.
      const char* code = "void main() { ... }"; // Error: Load from external .fs files for 2026 vibe.
    </anti_example>
  </instructions>
</skill>