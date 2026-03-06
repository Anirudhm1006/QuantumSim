<skill>
  <name>UI/UX Design for Quantum Simulator</name>
  <description>Expert in creating modern 2D Raylib overlays, Inspector Panels, and user interaction hooks for scientific visualization applications.</description>
  <instructions>
    <rule>Rendering Order: Draw all 2D UI AFTER EndMode3D() to keep panels fixed in screen space.</rule>
    <rule>Panel Placement: Inspector panels should be on the right side (200-250px wide), not obstructing the 3D view.</rule>
    <rule>Data Readout: Update UI every frame by querying the active IQuantumObject for current state.</rule>
    <rule>Input Handling: Use IsKeyPressed() for discrete toggles, IsKeyDown() for continuous adjustments.</rule>

    <concepts>
      <concept name="Inspector Panel">
        A semi-transparent dark panel displaying live physics data. Should show different metrics based on active simulation mode.
        - Hydrogen: n, l, m, energy (eV), wavelength (nm)
        - Spin: θ, φ, ⟨σx⟩, ⟨σy⟩, ⟨σz⟩
        - WavePacket: position, probability density
        - Laser: population inversion, gain, output power
      </concept>
      <concept name="Control Panel">
        Keyboard and mouse hooks for user interaction:
        - Keys 1-4: Toggle between simulation modes
        - Arrow keys: Adjust parameters
        - R: Reset to defaults
      </concept>
    </concepts>

    <implementation_mandates>
      <mandate>Use DrawRectangle() for panel background with alpha (Fade(BLACK, 0.7f))</mandate>
      <mandate>Use DrawText() with proper formatting for numerical values</mandate>
      <mandate>All coordinates in screen space (GetScreenWidth(), GetScreenHeight())</mandate>
    </implementation_mandates>

    <example name="Inspector Panel Implementation">
      void DrawInspectorPanel(IQuantumObject* obj, SimulationMode mode) {
          int panelX = GetScreenWidth() - 220;
          int panelY = 20;
          DrawRectangle(panelX, panelY, 200, GetScreenHeight() - 40,
                       Fade(BLACK, 0.7f));

          // Draw mode name
          const char* modeName = GetModeName(mode);
          DrawText(modeName, panelX + 10, panelY + 10, 20, WHITE);

          // Draw data based on mode
          if (mode == SIM_HYDROGEN) {
              HydrogenModel* h = static_cast<HydrogenModel*>(obj);
              DrawText(TextFormat("n: %d", h->get_n()), panelX + 10, panelY + 40, 16, GRAY);
              // ... more data
          }
      }
    </example>
  </instructions>
</skill>
