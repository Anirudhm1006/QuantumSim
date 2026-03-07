<skill>
  <name>UI/UX Design for Quantum Simulator (Professional CAD Style)</name>
  <description>Authority on professional scientific UI design using Raylib 2D overlays. Enforces a CAD-style 3-panel editor layout with a strict engineering color palette. Governs Save/Load workflows via File menu.</description>
  <instructions>
    <rule>Rendering Order: Draw all 2D UI AFTER EndMode3D() to keep panels fixed in screen space.</rule>
    <rule>Layout: The application uses a 3-panel CAD editor layout: Top MenuBar (30px), Left Toolbox (200px), Right PropertiesPanel (250px), Central 3D Viewport (fills remaining space).</rule>
    <rule>Data Readout: The PropertiesPanel updates every frame by querying the selected IQuantumObject from the Scene for its current state.</rule>
    <rule>Input Handling: Use IsKeyPressed() for discrete toggles, IsKeyDown() for continuous adjustments. Mouse clicks in Toolbox/MenuBar trigger tool/menu actions. Mouse clicks in the viewport trigger scene placement when a tool is active.</rule>
    <rule>Save/Load: File menu provides New (Ctrl+N), Save (Ctrl+S), Load (Ctrl+O). Serialization goes through ProjectManager to .qsim JSON files.</rule>

    <color_palette>
      <description>MANDATORY professional engineering palette. All UI elements MUST use only these colors. Arcade/neon colors are permanently banned.</description>

      <color name="BG_DARK" value="{25, 25, 30, 255}" usage="Application background, viewport clear color" />
      <color name="PANEL_BG" value="{35, 35, 42, 255}" usage="Panel backgrounds (MenuBar, Toolbox, PropertiesPanel)" />
      <color name="PANEL_BORDER" value="{55, 55, 65, 255}" usage="Panel edges, separator lines" />
      <color name="PANEL_HOVER" value="{45, 45, 55, 255}" usage="Button/item hover state background" />
      <color name="TEXT_PRIMARY" value="{220, 220, 225, 255}" usage="Primary text, labels, headings" />
      <color name="TEXT_SECONDARY" value="{140, 140, 150, 255}" usage="Secondary text, units, hints" />
      <color name="TEXT_DISABLED" value="{80, 80, 90, 255}" usage="Disabled or inactive text" />
      <color name="ACCENT" value="{70, 130, 180, 255}" usage="Selection highlight, active tool indicator, focused input borders" />
      <color name="ACCENT_HOVER" value="{90, 160, 220, 255}" usage="Hovered accent elements" />
      <color name="DANGER" value="{180, 60, 60, 255}" usage="Delete buttons, error indicators" />
      <color name="SUCCESS" value="{60, 160, 80, 255}" usage="Confirmation, normalization OK indicator" />
      <color name="GRID_LINE" value="{40, 40, 50, 255}" usage="3D viewport grid lines" />
      <color name="AXIS_X" value="{160, 60, 60, 255}" usage="X-axis indicator (muted red)" />
      <color name="AXIS_Y" value="{60, 160, 60, 255}" usage="Y-axis indicator (muted green)" />
      <color name="AXIS_Z" value="{60, 60, 160, 255}" usage="Z-axis indicator (muted blue)" />

      <banned_colors>
        GOLD, MAGENTA, YELLOW, LIME, ORANGE, PINK, PURPLE, VIOLET, SKYBLUE, BEIGE, MAROON.
        Any color with a single channel above 200 while others are below 80 (i.e., neon/saturated arcade colors).
        Raylib built-in color constants that violate this palette (e.g., GOLD, MAGENTA) must NOT appear anywhere in UI code.
      </banned_colors>
    </color_palette>

    <concepts>
      <concept name="MenuBar">
        A 30px-tall horizontal bar across the full window width at the top of the screen.
        Contains dropdown menus: File (New, Save, Load), Scenarios (Double Slit, Infinite Well, Hydrogen Atom, Free Particle), Help.
        Background: PANEL_BG. Text: TEXT_PRIMARY. Hover: PANEL_HOVER.
        Dropdown menus appear below the menu item on click, with PANEL_BG background and PANEL_BORDER outline.
      </concept>
      <concept name="Toolbox">
        A 200px-wide vertical panel on the left side, from below the MenuBar to the bottom of the screen.
        Contains tool buttons stacked vertically: Select, Add Emitter, Add Wall, Add Well, Add Detector, Draw Potential.
        Active tool is highlighted with ACCENT background.
        Each button is 180px wide, 36px tall, with 10px horizontal margin and 4px vertical spacing.
        Background: PANEL_BG. Button text: TEXT_PRIMARY. Inactive button bg: PANEL_BG. Hover: PANEL_HOVER.
      </concept>
      <concept name="PropertiesPanel">
        A 250px-wide vertical panel on the right side, from below the MenuBar to the bottom of the screen.
        Displays properties of the currently selected scene object in a tabular format:
        | Parameter | Value   | Unit |
        |-----------|---------|------|
        | n         | 3       |      |
        | Energy    | -1.5111 | eV   |
        | Norm      | 0.9998  |      |
        Section headers use TEXT_PRIMARY at 16px. Data rows use TEXT_SECONDARY at 14px for labels, TEXT_PRIMARY at 14px for values.
        When no object is selected, display "No Selection" in TEXT_DISABLED.
      </concept>
      <concept name="Central Viewport">
        The 3D rendering area occupies all remaining space between MenuBar (top), Toolbox (left), and PropertiesPanel (right).
        Clear color: BG_DARK. Grid: GRID_LINE. Axes: AXIS_X, AXIS_Y, AXIS_Z (muted, not saturated).
        Mouse interaction in the viewport is context-dependent on the active Toolbox tool.
      </concept>
    </concepts>

    <implementation_mandates>
      <mandate>Use DrawRectangle() for panel backgrounds with the exact RGBA values from the palette.</mandate>
      <mandate>Use DrawText() with TEXT_PRIMARY for headings, TEXT_SECONDARY for labels, and fixed-width formatting for numerical values (e.g., TextFormat("%.4f", value)).</mandate>
      <mandate>All coordinates are in screen space using GetScreenWidth() and GetScreenHeight(). Panels must reflow on window resize.</mandate>
      <mandate>Font size: 14px for data rows, 16px for section headers, 12px for hints/shortcuts.</mandate>
      <mandate>Panel separators: 1px horizontal lines using PANEL_BORDER color.</mandate>
      <mandate>Buttons: DrawRectangle for background, DrawRectangleLines for border (1px PANEL_BORDER), DrawText centered.</mandate>
      <mandate>File dialogs: For Save/Load, write the path to stdout and use a simple text input overlay within the application (no OS-native dialog dependency for portability).</mandate>
    </implementation_mandates>

    <example name="Professional Panel Background">
      const Color PANEL_BG = {35, 35, 42, 255};
      const Color PANEL_BORDER = {55, 55, 65, 255};
      const Color TEXT_PRIMARY = {220, 220, 225, 255};

      void draw_panel(int x, int y, int w, int h) {
          DrawRectangle(x, y, w, h, PANEL_BG);
          DrawRectangleLinesEx({(float)x, (float)y, (float)w, (float)h}, 1.0f, PANEL_BORDER);
      }
    </example>

    <example name="Properties Row">
      const Color TEXT_SECONDARY = {140, 140, 150, 255};
      const Color TEXT_PRIMARY = {220, 220, 225, 255};

      void draw_property_row(int x, int y, const char* label, const char* value, const char* unit) {
          DrawText(label, x, y, 14, TEXT_SECONDARY);
          DrawText(value, x + 120, y, 14, TEXT_PRIMARY);
          DrawText(unit, x + 200, y, 14, TEXT_SECONDARY);
      }
    </example>

    <example name="Toolbox Button">
      const Color PANEL_HOVER = {45, 45, 55, 255};
      const Color ACCENT = {70, 130, 180, 255};

      void draw_tool_button(int x, int y, const char* label, bool active, bool hovered) {
          Color bg = active ? ACCENT : (hovered ? PANEL_HOVER : PANEL_BG);
          DrawRectangle(x, y, 180, 36, bg);
          DrawRectangleLinesEx({(float)x, (float)y, 180.0f, 36.0f}, 1.0f, PANEL_BORDER);
          int text_x = x + (180 - MeasureText(label, 14)) / 2;
          DrawText(label, text_x, y + 11, 14, TEXT_PRIMARY);
      }
    </example>

    <anti_example>
      // BANNED: Arcade/neon color usage
      DrawText("QUANTUM SIMULATOR", x, y, 18, GOLD);          // GOLD is banned
      DrawText("QUANTUM NUMBERS", x, y, 14, YELLOW);           // YELLOW is banned
      DrawText("SUPERPOSITION", x, y, 14, MAGENTA);            // MAGENTA is banned
      DrawRectangle(x, y, w, h, Fade(BLACK, 0.7f));            // Use PANEL_BG instead
    </anti_example>

    <anti_example>
      // BANNED: Single right-panel "museum exhibit" layout
      // The application MUST use the 3-panel CAD layout (MenuBar + Toolbox + PropertiesPanel)
      int panelX = GetScreenWidth() - 220;  // Old single-panel approach
    </anti_example>
  </instructions>
</skill>
