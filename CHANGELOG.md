# Changelog

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
