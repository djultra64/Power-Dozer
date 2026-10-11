# Private beta manual testing

Updated October 11, 2026. The [PC settings native record](pc-settings-2026-10-11.md)
separates completed Linux automation from the physical checks below. Virtual GPU and virtual SDL
controller results retain their value as automated evidence; they do not count
as physical display, button or audio tests. Do not check boxes merely because
120 Hz was requested or a mission opened.

## Records and data protection

Record the date, package checksum and BUILD.json commit, distribution/kernel,
CPU/GPU/Vulkan driver, display/resolution/actual refresh rate, connection,
audio device, controller/model/connection and game options.
Use an imported copy of progress in a new folder with `--data-dir`.
Save source hashes before/after and verify they did not change. Do not use
original progress for corruption, forced shutdown or failure tests.
Logs/captures/results remain private and outside the distribution archive.

For each row, record **pending / passed / failed / unavailable**, duration,
steps, expected/observed result and evidence. A failure report includes mission,
vehicle, display mode, time and reproduction steps; do not replace it with
"looks fine." Known freezes and graphics defects remain priorities for
accepting a candidate.

## Real installation and stability

- [ ] Extract the package with its final checksum into a folder containing
  spaces, outside the checkout, without development environment variables or SDK.
  Start with the native picker: cancellation does not alter data; an invalid ROM
  allows retry and a valid ROM starts correctly.
- [ ] Test a clean installation and an update with `--import-data`;
  check player, EEPROM, Pak, preferences and remappings after exiting and
  restarting. The source retains its hashes.
- [ ] Repeat cold starts, player creation/selection, map, mission entry,
  failure, Continue, Restart and return to the map. Record repetition count
  and session time; frozen graphics/audio is a failure.
- [ ] Switch focus, minimize/restore, toggle fullscreen with Alt+Enter
  and close on different screens; actions must not remain held,
  the shortcut must not activate Start, and previously saved progress must not be lost.
- [ ] Run a long session with several missions and option changes. Record
  crashes, freezes, stopped audio, unexpected assertions and regressions.
- [ ] Test another distribution compatible with the ELF requirements. Check
  library loading as well as the picker, video, audio and controllers.

## Display, graphics and sustained 120 fps

The earlier virtual output result passed 720p/1080p120; sustained 4K was around
116.5 fps. This checklist requires new physical display evidence. The original
simulation rate, RT64 presentations and display refresh are different
measurements: record all three when tools are available.

| Profile | Minimum scenes | Measurements and criteria | Status |
| --- | --- | --- | --- |
| 4:3, 720p/1080p, original presentation | Title, map, tutorial, HUD, movement and demolition | Correct aspect, legible text, no spurious transparent geometry or incorrect clipping | Pending |
| 16:9, 720p/1080p, interpolation 120 | Cameras, quick turns, particles, explosion, pause/Restart | HUD without stretching; transitions without persistent trails/jumps; stable pacing on a physical display | Pending |
| 4K, interpolation 120 | Same steps and a scene with more destruction | Presentations and completed frames for at least 10 min; averages/p95/p99, windows below 120, actual refresh; 8.33 ms budget | Pending |
| Every available filter/MSAA option | Edges, sprites/textures, text and the same route | No corruption; options requiring restart persist and apply afterward | Pending |

Also record vsync/VRR and load profiles. Do not hide drops in an overall
average; requesting 120 does not demonstrate sustained 120. Compare original
and interpolated presentation for vehicles, camera, destruction, particles and
discontinuities. Review L3D lines, shadows, culling and overlays during the
reproducible Simian Acres route.

## Actions and prompts with physical controllers

Test **the action and its prompt together** in the tutorial, HUD, help and editor.
The prompt must follow the last device used without reconfiguring controls.
Retain remapping by action and test manual family selection as a fallback
for an ambiguous controller.

| Physical position | Xbox | PlayStation | Nintendo Switch |
| --- | --- | --- | --- |
| South | A | Cross | B |
| East | B | Circle | A |
| West | X | Square | Y |
| North | Y | Triangle | X |

- [ ] Xbox: confirm/accelerate, back/secondary action, Start, stick,
  D-pad, triggers, camera and exit vehicle; the prompt matches the button.
- [ ] PlayStation: repeat with actual names/glyphs for its layout.
- [ ] Switch: explicitly verify A/B and X/Y swapped relative to
  Xbox; confirm on the south B button displays B, and the east A button displays A.
- [ ] Remap an action: tutorial/HUD/help show its new button and the actual
  action activates there; save/exit/reload preserves the remapping.
- [ ] Alternate keyboard/controller and two controllers; disconnect/reconnect
  and switch USB/Bluetooth connection when available. Family and bindings
  persist; no input sticks after focus loss.
- [ ] Force manual family selection with ambiguous detection and return to automatic.
  Verify labels, glyphs and actual actions again.
- [ ] Keyboard: help, Enter/Start, Z/X, camera and Alt+Enter; check the binding
  editor, cancellation without changes and shortcut without an additional action.

## Complete PC settings screen

- [ ] Open PC OPTIONS from a paused mission at 960×720 and 1280×720;
  inspect Display (4 rows), Graphics (3), Audio (2), and Controls (14 actions,
  two columns). Compare original uppercase lettering, colors and translucent
  shadows over the paused game. No original map instructions overlap the menu.
- [ ] Resize with a row focused, close/reopen, and inspect first presentation;
  text, action glyphs, mouse targets and volume slider remain aligned and legible.
- [ ] Change every field in both directions; verify live volume/mute and filters,
  and the next-launch marker for resolution/aspect/frame rate/MSAA. Drag the
  volume past both endpoints and verify 0/100 bounds.
- [ ] Navigate with mouse, Tab/Shift+Tab, arrows, controller shoulders, stick
  and D-pad. Match each displayed Confirm/Back glyph with its actual action,
  including remaps that use navigation keys. Escape always recovers.
- [ ] Capture a binding, cancel without changes, and commit a remap. Inspect the
  capture and Restore Defaults modals at both aspect ratios: no underlying row
  text shows through, and prompts follow the last device and current bindings.
- [ ] Cancel Restore Defaults, then confirm for each selected device. Confirm
  only that device's bindings reset; the other device's remaps remain intact.
- [ ] Select each manual family and return to Auto; confirm actual button
  positions, names and glyphs together, including Nintendo's swapped A/B and X/Y.
- [ ] In a disposable data directory, force a write failure. SAVE FAILED stays
  visible with the previous value/focus; restoring write access allows saving.
- [ ] Close while Confirm is held: the original game remains paused and no held
  action leaks. Reopen, choose GAME OPTIONS and use the original paused menu.
- [ ] Exit and launch again without command-line settings overrides. Previously
  saved fields load, and the remapped key/button still activates its action.

## Physical audio

- [ ] Listen to music/effects on the title, map and missions; test volume/mute
  from options during playback. Mute does not stop the game or tasks.
- [ ] Verify left/right channels with a localized route/effect and known
  headphones or speakers; a PCM capture alone is insufficient.
- [ ] Remove/change the output device during audio; continue playing,
  reconnect and listen for recovery without replaying old effects,
  bursts, freezes or perceptible drift. Record backend and connection.
- [ ] Start without output, add the device later, change frequency/device
  and run a session lasting at least 10 minutes.

## Missions and vehicles

Simian Acres already has automated objective and reload evidence. Record
additional coverage using only missions/vehicles available in the progress
copy; do not declare the whole game complete from that single mission.

| Actual mission and vehicle | Enter and move | Unique mechanics and demolition | Failure/retry | Objective and return to map | Save/reload | Graphics/audio |
| --- | --- | --- | --- | --- | --- | --- |
| Simian Acres; observed vehicle | Manual pending | Manual pending | Manual pending | Manual pending | Manual pending | Manual pending |
| Second available mission: enter name | Pending | Pending | Pending | Pending | Pending | Pending |
| Mission with another vehicle: enter name | Pending | Pending | Pending | Pending | Pending | Pending |
| Dense demolition scene: enter name | Pending | Pending | Pending | Pending | Pending | Pending |

Add rows for every vehicle and mission type tested. Include tutorials,
timer/convoy, vehicle changes, ramps/jumps, large destruction, menus between
missions and unlocked challenges when available in the progress. Record
medals/state before and after reload. Keep an explicit list of missions and
vehicles that remain untested.
