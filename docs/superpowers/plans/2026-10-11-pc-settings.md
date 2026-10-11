# PC Settings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace paginated PC settings with complete section screens that use the game's real font, palette and shadow boxes.

**Architecture:** A PC-owned view model and SDL event adapter publish immutable snapshots to a 2D renderer using RT64's public presentation hooks. Existing guest hooks provide entry/exit, while existing settings persistence and logical input mapping remain shared.

**Tech Stack:** C++20, SDL2, RT64/Plume RHI, existing DXC, CMake, Python asset verification.

**Spec:** `docs/superpowers/specs/2026-10-11-pc-settings-design.md`

## Global Constraints

- Attribution and commit identity: djultra64.
- Four sections; all fourteen actions visible in two columns, no scrolling or MORE pages.
- Actual USA Rev1 font, palette and shadow geometry; load ROM-derived assets privately at runtime, never package them.
- Mouse, keyboard and controller navigation; prompts follow actual bindings and last active device, with Nintendo positions distinguished.
- Keep per-field save, live/restart behavior and original Game Options handoff.

## Review Focus

- Opening/closing with a held confirm key must not activate the underlying game (Task 4).
- Resize and narrow 4:3 must retain all fourteen rows and clickable targets (Tasks 1, 3).
- Failed writes must restore saved values without moving focus (Tasks 1, 4).
- Controller disconnect or ambiguous family must leave keyboard recovery usable (Task 4).
- GPU teardown and resize must not use released font or snapshot resources (Task 3).

---

### Task 1: Section model and complete layout

**Files:** Modify `include/settings_menu.h`, `src/settings_menu.cpp`, `tests/settings_menu.cpp`; create `include/pc_settings_view.h`, `src/pc_settings_view.cpp`, `tests/pc_settings_view.cpp`; update `CMakeLists.txt`.

**Interfaces:** `SettingsMenu::adjust(SettingsField field, int direction)` changes one field; `SettingsMenu::section(Page)` selects Display/Graphics/Audio/Controls. `pc_settings_layout(int width,int height)` returns row/tab/control bounds in the centered 640×480 safe area. `PcSettingsSnapshot` contains section, focus, row labels/values, prompts, modal and save status.

- [ ] Write failing tests: Display has 4 setting rows; Graphics 3; Audio 2; Controls has 14 reachable action rows; every rectangle lies in the safe area at 960×720 and 1280×720.
- [ ] Run new native tests and existing `settings_menu_test`; confirm failures describe pagination/current missing layout.
- [ ] Implement complete sections, two-column controls, direction-aware bounded volume and separate title/status.
- [ ] Verify change acknowledgement failure restores saved value and same row; test left/right adjustment and volume endpoints.
- [ ] Commit the independently tested model/layout.

### Task 2: Original font and shadow asset decoding

**Files:** Create `include/pc_settings_assets.h`, `src/pc_settings_assets.cpp`, `tests/pc_settings_assets.cpp`; update `CMakeLists.txt`.

**Interfaces:** `PcSettingsAssets load_pc_settings_assets(std::span<const uint8_t> rom,std::span<const uint8_t> code)` returns RGBA font atlas, glyph advances, original palette and shadow triangles. Guest virtual code base is 0x802447C0; asset table is ROM 0x4CE0, stride 8; font IDs 0xF4C–0xF7E are codec 6, 32×32 I4. Palette is 0x802F47B0; shadow vertices/indices are 0x802F97B0/0x802F98B0.

- [ ] Write failing tests using synthetic codec-6 blocks: exact nibble expansion, original UV orientation, 51 tile indexing, bounds/truncated-table rejection.
- [ ] Run the test and verify decoder is absent/fails expected assertions.
- [ ] Implement bounded decoding and atlas construction without copying guest texture cache state; preserve original 0.6 glyph advance.
- [ ] Compare all 26,112 decoded private bytes against the already executed original CPU decoder; test malformed sizes never read outside spans.
- [ ] Commit code/tests only, excluding private ROM pixels and proof captures.

### Task 3: Presentation overlay renderer

**Files:** Create `include/pc_settings_renderer.h`, `src/pc_settings_renderer.cpp`, `shaders/pc_settings.hlsl`; modify `src/renderer.cpp`, `CMakeLists.txt`; extend `tests/pc_settings_view.cpp`.

**Interfaces:** `initialize_pc_settings_renderer(const PcSettingsAssets&)` registers RT64 init/draw/deinit before Application setup. `publish_pc_settings_snapshot(std::shared_ptr<const PcSettingsSnapshot>)` replaces the render view atomically. Rendering uses draw-hook framebuffer extent and a Plume RHI textured-triangle/alpha pipeline.

- [ ] Add a failing layout/draw-data test proving all sections emit their visible rows, text shadows, original box shadows and family-specific glyphs, with no scissor clipping of Controls.
- [ ] Implement draw-data generation and shader compilation with existing DXC; upload assets during init/draw and release only after presentation hook deinit.
- [ ] Composite after VI and before PRESENT; isolate GPU resources from SDL state, retain snapshot for the whole frame.
- [ ] Verify Vulkan native captures at 720p in both aspects, resize repeatedly, then close/reopen and shut down without stale resources.
- [ ] Commit the renderer and native validation notes.

### Task 4: SDL input, remapping and guest handoff

**Files:** Modify `src/input.cpp`, `src/main.cpp`, `src/pc_options.cpp`, `include/pc_options.h`, `src/controls_menu.cpp`, `include/controls_menu.h`; create `src/pc_settings_input.cpp`, `include/pc_settings_input.h`, `tests/pc_settings_input.cpp`; update `CMakeLists.txt`.

**Interfaces:** `bool pc_settings_event(const SDL_Event&,const InputModel&)` consumes handled events. `bool pc_settings_open()` gates guest input; close sets an input-release latch. Reuse `take_pc_change`, acknowledgement, remap capture and `InputModel::prompt`/glyph APIs.

- [ ] Write failing tests: mouse and keys reach the same row, Tab/Shift+Tab, left/right, slider clamps, modal cancel restores focus, held confirm is suppressed until release.
- [ ] Implement keyboard/mouse/controller focus and action adjustment; keep keyboard Escape recovery independent of gameplay bindings.
- [ ] Open from existing PC OPTIONS entry without mutating the guest four-row table; return to its original window or Game Options on explicit handoff.
- [ ] Integrate existing persistence/remapping; test disk failure, disconnected controller, Switch A/B and X/Y prompt/action pairs and section navigation during saving.
- [ ] Commit tested input/handoff integration.

### Task 5: Native acceptance and documentation

**Files:** Modify `tools/probe_gameplay.py`, `tests/test_instruction_probe.py` only as relevant to changed assertions; update `docs/install.md`, `docs/qa/manual-testing.md`; create `docs/qa/pc-settings-2026-10-11.md`.

- [ ] Update native probe expectations to complete section rows and modal capture rather than paginated four-row screens.
- [ ] Run 46+ CTest checks and Python unittest discovery; all must pass, including audio regressions from the separate investigation.
- [ ] Capture every screen and remap modal on native Vulkan in 4:3/16:9; compare original typography, shadows and backdrop to approved private previews.
- [ ] Exercise settings changes with mouse, keyboard and Xbox/PlayStation/Switch virtual controllers; use real controllers when available and report any untested hardware limits.
- [ ] Verify settings survive restart, failures preserve values, held inputs do not leak and public package sources contain no ROM-derived images.
- [ ] Review complete branch before any merge/publication; document exact tested scope without claiming audio fixed solely because counters advance.
