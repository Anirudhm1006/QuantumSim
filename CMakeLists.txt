cmake_minimum_required(VERSION 3.18)
project(QuantumSim LANGUAGES CXX)

# ============================================================================
# Build Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE IntelliSense (Cursor/Clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Determine build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# ============================================================================
# Options
# ============================================================================
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer in Debug builds" OFF)

# ============================================================================
# FetchContent: Raylib (Zero-Install Workflow)
# ============================================================================
include(FetchContent)

# Raylib - modern graphics library
# Using STABLE branch for production, change to "master" for latest features
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE
)

# nlohmann/json - modern JSON library for C++
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

# Platform-specific Raylib options
if(WASM)
    # WebAssembly specific settings
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(raylib json)

# ============================================================================
# Application Sources
# ============================================================================
# Collect all source files
file(GLOB_RECURSE APP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/PhysicsEngine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PhysicsEngine/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/RenderEngine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RenderEngine/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIHandler/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/UIHandler/*.c"
)

# Add main executable
add_executable(QuantumSim ${APP_SOURCES})

# ============================================================================
# Target Configuration
# ============================================================================

# Include directories (target-based, not global)
target_include_directories(QuantumSim
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/PhysicsEngine
        ${CMAKE_CURRENT_SOURCE_DIR}/RenderEngine
        ${CMAKE_CURRENT_SOURCE_DIR}/UIHandler
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
)

# Platform-specific include directories
if(WIN32)
    target_include_directories(QuantumSim PRIVATE "${CMAKE_SOURCE_DIR}/resources/shaders")
else()
    target_include_directories(QuantumSim PRIVATE "${CMAKE_SOURCE_DIR}/resources/shaders")
endif()

# Link Raylib
target_link_libraries(QuantumSim PRIVATE raylib nlohmann_json::nlohmann_json)

# ============================================================================
# Build Type Specific Settings
# ============================================================================
target_compile_definitions(QuantumSim
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG=1>
)

# Debug sanitizers (Address Sanitizer + Leak Sanitizer)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(QuantumSim PRIVATE
            -fsanitize=address
            -fsanitize=leak
            -fno-omit-frame-pointer
        )
        target_link_options(QuantumSim PRIVATE
            -fsanitize=address
            -fsanitize=leak
        )
    elseif(MSVC)
        target_compile_options(QuantumSim PRIVATE
            /fsanitize=address
            /Zi
        )
        target_link_options(QuantumSim PRIVATE
            /fsanitize=address
        )
    endif()
    message(STATUS "Address Sanitizer enabled for Debug build")
endif()

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(QuantumSim PRIVATE -O3)
    elseif(MSVC)
        target_compile_options(QuantumSim PRIVATE /O2 /Oi)
    endif()
endif()

# ============================================================================
# Resource Copying (for standalone binaries)
# ============================================================================
# Copy shader files to build directory
file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*")
foreach(shader ${SHADER_FILES})
    get_filename_component(shader_name ${shader} NAME)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/resources/shaders/${shader_name}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${shader}
            ${CMAKE_BINARY_DIR}/resources/shaders/${shader_name}
        DEPENDS ${shader}
        COMMENT "Copying shader: ${shader_name}"
    )
    target_sources(QuantumSim PRIVATE ${CMAKE_BINARY_DIR}/resources/shaders/${shader_name})
endforeach()

# Create resources directories in build
add_custom_command(TARGET QuantumSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/resources/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/resources/fonts
)

# Copy font files to build directory
file(GLOB FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/*.ttf")
foreach(font ${FONT_FILES})
    get_filename_component(font_name ${font} NAME)
    add_custom_command(TARGET QuantumSim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${font}
            ${CMAKE_BINARY_DIR}/resources/fonts/${font_name}
        COMMENT "Copying font: ${font_name}"
    )
endforeach()

# ============================================================================
# WASM-specific configuration
# ============================================================================
if(WASM)
    # Ensure ASYNCIFY is enabled for non-blocking main loop
    set_target_properties(QuantumSim PROPERTIES
        LINK_FLAGS "-s ASYNCIFY -s WASM=1 -O3"
    )
    message(STATUS "WASM build: ASYNCIFY enabled for browser UI responsiveness")
endif()

# ============================================================================
# Installation Rules
# ============================================================================
install(TARGETS QuantumSim
    RUNTIME DESTINATION bin
)

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "=== QuantumSim Build Configuration ===")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Raylib: ${raylib_VERSION}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "")
