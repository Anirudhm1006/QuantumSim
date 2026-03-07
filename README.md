# QuantumSim - Quantum Physics Simulator

An educational quantum physics simulator for the Norwegian VGS Fysikk 2 curriculum. Features interactive simulations covering double slit interference, quantum tunneling, infinite potential well, hydrogen atom viewer, photoelectric effect, atomic spectra, de Broglie wavelength, Heisenberg uncertainty principle, and free particle wave packet propagation.

## Download

Pre-built binaries are available on the [GitHub Releases](https://github.com/anirudh-projects/quantum-simulator/releases) page.

## Build from Source

### Linux

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev cmake build-essential

# Clone and build
git clone https://github.com/anirudh-projects/quantum-simulator.git
cd quantum-simulator
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows

```powershell
# Install CMake and MSVC
# Clone and build
git clone https://github.com/anirudh-projects/quantum-simulator.git
cd quantum-simulator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
```

### macOS

```bash
# Install CMake via Homebrew
brew install cmake

# Clone and build
git clone https://github.com/anirudh-projects/quantum-simulator.git
cd quantum-simulator
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Controls

- **Left-click drag**: Orbit camera around the scene
- **Mouse wheel**: Zoom in/out
- **Middle-click drag**: Pan the view
- **Number keys 1-9**: Switch between views within a scenario
- **Scenarios menu**: Switch between different quantum physics scenarios

## Requirements

The application requires a `resources/` folder containing:
- `fonts/inter.ttf` - Font file for UI text rendering
