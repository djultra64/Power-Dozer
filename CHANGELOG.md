# Changelog

## v0.84 — 2026-10-11 (candidate; Linux and source preparation)

- Add Main before original file selection, with Start Game, PC Options and
  Close Game over the actual animated backdrop and live ticker. Retain the
  original title/attract flow and file, name and erase logic behind Start.
- Use the original font, palette and shadows. Keep the logo on Main with
  POWER DOZER below it; conceal the logo in pregame Settings while preserving
  all four sections, fourteen bindings and the ticker. Paused Game Options
  remains available, and pending Aspect preferences apply at next launch.
- Fit drawing and mouse targets to the active startup image after resize,
  including letterboxing. Consume held input across ownership transitions;
  route Close Game through the existing main loop.
- Preserve logical actions and physical Xbox/PlayStation/Switch prompts,
  including south/east B/A and west Y on Switch and optional action remaps.
- Clear the entire sampled vehicle-shadow texture while retaining the original
  silhouette clip. Native GPU evidence isolates the formerly undefined final
  border row; it does not reproduce every reported live display symptom.
- Extend original ticker medal/rank sprites across the same full-width 16:9
  region as text, preserving their UV span, edge clipping and 4:3 behavior.
- Update the isolated gameplay probe for explicit Main→Files ownership and
  active-aspect mouse geometry; retain settings, remap, filter and reload checks.
- Validate the final Linux build with 60 CTest checks and 172 Python tests.
- Prepare v0.84 runtime/build/package identities and source inventory including
  all new menu and graphics modules. No Windows executable is relabeled.

Linux native acceptance includes original erase-confirm cancellation, name
entry/deletion/cancellation, existing-file load, all three virtual controller
families with actual actions and matching glyphs, failed-save rollback, remaps,
live filters, reload, resizing/fullscreen and clean timer/Close exits. Integrated
shadow and medal captures cover 1080p/4K fullscreen transitions. See
[main menu acceptance](docs/qa/main-menu-2026-10-11.md).

Windows v0.84 awaits a fresh build/native run; the build host refused SSH.
Physical controllers, broad mission coverage and sustained physical 4K/120 Hz
remain manual. Shared GPU acceptance makes no performance claim. Source and
license review flags remain false. Final archive audits and merged validation
precede publication; check Releases for actual availability.

## v0.83 — 2026-10-11 (public beta; Linux and source)

- Add complete Display, Graphics, Audio and Controls sections with all fourteen
  logical actions visible, optional remapping, device/family selection and
  persistence feedback. Keyboard, mouse and controller navigate the host menu.
- Render the PC overlay using the original font, palette and shadow geometry
  decoded locally from the user's ROM; no extracted game pixels are packaged.
  Return to the original pause menu and Game Options remains available.
- Let the existing pending-audio recovery request yield a valid running graphics
  task through the original scheduler. Regression coverage executes the actual
  yield query, generated audio RSP and original reply waking the audio thread.
- Protect the live soundplayer's periodic heartbeat from ordinary-event queue
  saturation and stale event state. Original callback regressions demonstrate
  both structural mechanisms; the native initiating event remains unconfirmed.
- Include the required CMake hook and shader source in host source archives,
  while excluding compiled shaders and private generated guest functions.
- Update Linux runtime/source package identities and archive roots to v0.83.
- Validate the Linux production build with 55 CTest checks and 150 Python tests,
  plus a ten-minute real GPU/PulseAudio attract-mode run with sustained
  audio-task activity. This validates the recorded scenario only.
- Release scope is Linux and source. Windows v0.83 awaits a fresh build and
  native validation; Windows v0.82 remains part of the prior release.

Full mission coverage, real physical controllers, native Windows validation of
these new changes and sustained 4K/120 Hz remain pending. No general gameplay
or audio stability claim is made. Source and license review flags remain false;
the beta release does not certify a complete source/license audit.

## v0.82 — 2026-10-10

- Correct wheel interpolation for animated vehicle parts, including Backlash
  and other wheeled vehicles during attract mode and gameplay. Wheel geometry
  and transforms now interpolate around the original axle pivot, preserving
  the tire radius and keeping the wheel attached at high speeds.
- Preserve each bind pivot in both vehicle skin buffers and after an
  animation stops; invalidate it when a new model loads.
- Regression coverage exercises actual RT64 fractional-frame interpolation,
  bind recovery, buffer reuse and the shared vehicle animation producer.


## v0.81 — 2026-10-10

- English repository documentation, including installation, rebuild, audio and
  feasibility guides, with historical evidence and outstanding limits preserved.
- Comprehensive project and dependency acknowledgments in `CREDITS.md`, included
  with the Linux and Windows package notices and in the source archive.
- Native Windows x64 build and portable Windows packaging with pinned SDL/DXC DLLs.
- Windows atomic saves and progress import, including backups in Unicode paths.
- Correct Windows startup diagnostics to avoid an invalid UCRT buffer size.
- Updated runtime/source package identities and archive roots to v0.81.
- Project author and committer identities standardized on `djultra64`.

The reported thin black ground line is no longer visible to the reporter and
was not reproduced in recent local captures; no speculative patch was applied.
Intermittent audio loss, broad mission coverage,
physical controllers and sustained 120 fps retain their existing testing limits.
Source and license review flags remain false.

## v0.8 — 2026-10-10

First Power Dozer prerelease for Linux x86-64, based on Blast Corps USA Rev 1.

- Native static recompilation with N64Recomp, N64ModernRuntime and RT64.
- Widescreen presentation, resolutions up to 4K and configurable interpolation
  targeting 120 Hz, subject to the display and hardware.
- Alt+Enter fullscreen, in-game PC options, audio controls and action remapping.
- Last-device prompts for keyboard, Xbox, PlayStation and Switch, using the
  supplied pixel atlas. Switch face labels follow the real physical layout.
- Full-width menu background/ticker and corrected map vehicle sprite seams.
- Persistent EEPROM/Controller Pak data, backup recovery and migration.
- Runtime event-delivery fixes and recovery for pending audio tasks.

Known limits: intermittent audio loss is still under investigation; full-game
coverage, physical controllers, sustained 120 fps, completion-replay sky/fades
and HUD-arrow visual stability need manual testing. This is a testing beta.

Source and runtime manifests retain the outstanding source/license audit flags;
see NOTICE and the bundled dependency notices.
