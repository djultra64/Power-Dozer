# Blast Corps Release Readiness Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring the native port to a verified release candidate comparable to Tetrisphere.

**Architecture:** Keep translated guest code and established RT64/audio bridges.
Add host services with isolated data, durable stores and action-based input;
connect the game's menus using validated, reproducible hooks.

**Tech Stack:** C++20, SDL2, N64Recomp, N64ModernRuntime, RT64/Vulkan, Python, CMake.

**Spec:** ../specs/2026-10-09-release-readiness.md

## Global Constraints

- Read-only Tetrisphere reference; preserve user ROM and progress.
- Tests use isolated data directories and never modify the user's progress.
- ROM fingerprint: `483f7161aea39de8b45c9fbc70a2c3883c4dea8c`.
- Switch south=B/east=A/west=Y/north=X; logical actions own mappings/prompts.
- Alt+Enter remains available; no repeated approval gates for authorized local work.
- No ROM, extracted overlays, saves or captures in the clean archive.
- Requested refresh rates and virtual devices are not physical acceptance evidence.

## Review Focus

1. Interrupted progress writes and corrupt primary/backup: preserve existing data.
2. A held shortcut or focus loss: no stuck movement or leaked Start action.
3. Reconnected ambiguous controllers: preserve bindings and accurate prompts.
4. Mission/renderer transitions: keep guest scheduling and audio advancing.
5. ROM/config import failure on a clean installation: useful error, no reset of progress.

### Task 1: Durable saves and isolated testing

**Files:** `include/save_store.h`, `src/save_store.cpp`, `src/runtime.cpp`,
`src/main.cpp`, `tests/save_store.cpp`, `CMakeLists.txt`.
**Interfaces:** `bc::SaveStore(path).load()/save(array<uint8_t,512>)`;
global `bc_data_directory`; CLI `--data-dir PATH`.

- [x] Write and observe failing tests for missing/raw saves, persistence,
  damaged-primary recovery, both copies invalid, extra bytes and independent folders.
- [x] Implement a versioned/checksummed image, synced temporary writes, validated
  backup and atomic rename; legacy raw images migrate on write.
- [x] Route EEPROM and optional action mappings through the explicit data path.
- [x] Run native tests and a native launch with a copied isolated save; confirm
  the user's original save hash is unchanged.
- [x] Record acceptance and commit the coherent change.

### Task 2: Gameplay and transition harness

**Files:** `tools/probe_gameplay.py`, diagnostic hooks in `tools/recompile.py`,
`src/runtime.cpp`, `docs/qa/readiness.md`.
**Interfaces:** controlled native runs with `--data-dir`, recorded key/controller
events, progress counters, guest mode/mission telemetry and screenshots.

- [x] Add tests that reject frozen graphics/audio, fatal events and guest assertion
  faults even when a run exits zero; inspect a known bad log and observe failure.
- [ ] Automate boot, new/existing game, map, mission, movement, pause/retry,
  objective/failure transitions and save reload in isolated directories.
- [ ] Diagnose each observed fault from logs/cores before changing the bridge;
  reproduce, fix and rerun the affected flow.
- [ ] Run a long native session; record actual coverage and unresolved flows.

### Task 3: Settings and image quality

**Files:** `include/settings.h`, `src/settings.cpp`, host settings menu,
`src/main.cpp`, `src/renderer.cpp`, tests of settings and menu transitions.
**Interfaces:** validated Settings values, durable store, saved→CLI precedence,
host event queue for changes made in guest menu hooks.

- [x] Test invalid/nonfinite values, missing/old config, preservation of unrelated
  values, cancel/back and all supported graphics choices before implementation.
- [ ] Implement saved scale/aspect/refresh/fullscreen/MSAA/presentation/texture
  filters and settings menu in the original visual language.
- [x] Identify and mark which choices apply live and which need restart.
- [ ] Exercise 4:3/16:9 and 720p/1080p/4K; compare native captures of menus/HUD.

### Task 4: Complete controls and prompts

**Files:** split input action/model/SDL integration, prompt renderer/guest hooks,
virtual controller harness and mapping database provenance.
**Interfaces:** action→physical binding model; device family and last-active
source; glyph/label resolver shared with the actual guest input path.

- [x] Test all three families, remaps, unknown/manual family, trigger/D-pad,
  reconnect, focus loss and shortcut suppression before modifying behavior.
- [ ] Implement correct default controls, in-game prompts/help and optional remap.
- [x] Exercise virtual Xbox/PlayStation/Switch devices and keyboard; review native
  captures against action traces.
- [ ] Test physical devices when available.

Task 4 next acceptance step: the action model/editor/help and thirteen original
instruction addresses are implemented and have native keyboard regressions.
Draw actual device glyphs through the game's graphics path, then obtain native
virtual-family help/tutorial captures against matched action traces. Do not
substitute window-title labels or glyph enum identities for rendered glyphs.
Original `8026BCE0` returns the active display-list pointer; the frame's main
list begins at `FrameContext+0x48B0`, with `0xB5E` eight-byte entries. Any appended
UI commands must verify space within that list and preserve the original text
animation and RDP state. Avoid reserving arbitrary guest vertex/texture memory:
the existing host-only text slots must never become RSP texture addresses.

### Task 5: Graphics/interpolation and audio acceptance

**Files:** game-specific renderer hooks, `src/l3d.cpp`, audio device bridge,
timing/PCM probes and regression tests.

- [ ] Compare original presentation against interpolated vehicles, cameras,
  destruction, particles and menus, including discontinuities and L3D clipping.
- [ ] Diagnose culling/HUD/aspect problems; use targeted game hooks with regression
  evidence rather than stretching the entire framebuffer.
- [ ] Measure effective rates, frame times, audio continuity and stereo ordering.
- [ ] Test mute/volume without disabling RSP tasks and audio device loss/recovery.
- [ ] Run sustained representative missions under the final graphics profiles.

### Task 6: Portable ROM bootstrap and package

**Files:** native ROM/cache import and overlay extraction, package scripts,
licenses, build manifest, checksums and distribution instructions.

- [ ] Test invalid revision, swapped formats, bounded ZIP candidates, missing cache,
  read-only data and clean/upgrade layouts before integrating the loader.
- [ ] Remove source-root runtime paths; extract validated overlays from the user's
  ROM on launch. Cache and settings/progress live in the portable data directory.
- [ ] Package required redistributables and controller database; verify no private
  ROM/assets/saves/captures are included and paths work outside the source tree.
- [ ] Build Windows using the existing toolchain if viable; test natively when
  the existing machine is available. Record unverified platforms explicitly.

### Task 7: Release review

- [ ] Run relevant suites, fresh-install/upgrade and long-session regression.
- [ ] Obtain an independent whole-branch review with spec, plan, ledger and evidence.
- [ ] Resolve material findings with regression tests and produce a private RC
  archive plus checksums, release notes, limitations and acceptance report.
- [ ] Mark ready only when the release gate is supported by evidence.

Audio recovery implementation detail (authorized Task 5): the current bridge
exits on open/queue failure and ignores SDL output-device removal. Extract a
small audio-output state machine with a backend and monotonic clock. When an
output disappears, retain the remaining byte budget and consume it at the
N64 sample rate in silence; do not reset the guest's two-buffer AI accounting.
Retry default output at one-second intervals. On recovery, pad only the
remaining budget with silence before accepting new PCM, preventing replay of
old effects or a jump in AI remaining length. SDL stays behind the existing
audio mutex. Verify unavailable startup, mid-buffer loss, failed enqueue,
retry throttling, queued-budget continuity, fractional frame timing and sample
rate changes using a deterministic fake backend, then the real SDL bridge and
native gameplay. Actual speaker/device changes require separate acceptance.

Task 4 original-instruction glyph design finding: `80259EC4` lays out text into
CPU character/vertex queues; it does not return a display-list pointer. Do not
append RDP commands at that entry. `80259824` flushes the queued characters to
`Gfx**` (arg0), with FrameContext in arg1; `80259BD4`/`80259C24` call it for
window/HUD text. A safe approach is to record exact known instruction sites
at the existing text hook, then append bounded glyph commands after that
flush using the verified main-DL range and existing state-preserving backend.
Reset pending annotations at original `80259450` (per-frame font reset), and
consume them on the relevant flush, so blink/hidden lines cannot retain icons.
The three direct HUD addresses 8030821C/80308228/80308240 use screen coordinates
in entry SP+14 (float x), +18 (int y), +20 (height); justification is SP+10.
Window text uses local coordinates offset by half the current window size;
when original phase=2, screen x/y are local x/y + window x/y + half size.
Opening/closing windows rotate/scale in yoshi.c, so suppress new icons during
those phases as the existing PC glyph bridge does. Alpha comes from the same
font arguments. Multi-action instructions need both resolved glyphs, without
changing guest vertex/texture allocation. Verify ROM entry/return instructions
and layout before inserting any new normalizer hooks; test bounds, queue
reset/consumption, exact-string matching, blinking, alpha and two-action sites
before native keyboard/controller captures. This design has not yet been
implemented or accepted.
