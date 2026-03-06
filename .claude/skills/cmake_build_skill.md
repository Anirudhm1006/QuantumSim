<skill>
  <name>Modern CMake & Toolchain Master</name>
  <description>Expert in target-based CMake, FetchContent dependency management, and cross-platform compilation (Windows, Linux, Mac, WASM).</description>
  <instructions>
    <rule>Target-Based Logic: Never use global include_directories(). Use target_include_directories() and target_link_libraries() with PRIVATE/PUBLIC scope.</rule>
    <rule>Zero-Install Workflow: Use CMake's FetchContent module to pull Raylib directly from GitHub. This ensures the project builds on a fresh machine without manual installs.</rule>
    <rule>Build Presets: Mandate the use of CMakePresets.json for standardizing build configurations across the team.</rule>
    <rule>WASM Toolchain: When PLATFORM=Web is detected, switch the compiler to emcc and enforce the -s ASYNCIFY flag to prevent browser blocking.</rule>

    <concepts>
      <concept name="FetchContent Paradigm">
        Declare: FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG master)
        Populate: FetchContent_MakeAvailable(raylib)
      </concept>
      <concept name="Cross-Platform Pathing">
        Always use generic paths in CMake; let the generator handle backslashes for Windows vs forward-slashes for Unix.
      </concept>
    </concepts>

    <implementation_mandates>
      <mandate>Compile Commands: Always set CMAKE_EXPORT_COMPILE_COMMANDS=ON so Cursor and Clangd can provide perfect IntelliSense.</mandate>
      <mandate>Resource Embedding: For standalone binaries, use a custom command to bundle shaders and textures into the executable using a C-header generator.</mandate>
    </implementation_mandates>

    <example name="FetchContent Raylib Template">
      cmake_minimum_required(VERSION 3.18)
      project(QuantumSim LANGUAGES CXX)
      set(CMAKE_CXX_STANDARD 17)

      include(FetchContent)
      FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git)
      FetchContent_MakeAvailable(raylib)

      add_executable(QuantumSim src/main.cpp)
      target_link_libraries(QuantumSim PRIVATE raylib)
    </example>

    <anti_example>
      // WRONG: Hardcoding paths to a local C:/raylib folder.
      set(RAYLIB_PATH "C:/raylib/src") // Fatal Error: Not portable.
    </anti_example>
  </instructions>
</skill>