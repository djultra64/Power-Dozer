# Blast Corps release candidate

The user authorizes autonomous implementation and testing until this port has
the same finish as the local Tetrisphere port. The existing Tetrisphere checkout
is a read-only reference; preserve the user's ROM and progress. Work continues
without intermediate approval gates. Publishing is a separate external action.

## Scope and approach

Keep N64Recomp, N64ModernRuntime and RT64, and the proven scheduler/audio bridge.
Finish the native Linux build first. Establish reproducible Windows packaging
and acceptance where the existing toolchain and test machine permit it. Do not
describe a platform as verified solely because it cross-compiles.

Prefer the game's existing menus and graphics for options and help. Host-owned
settings, input actions, persistence and diagnostics must be independent of
guest memory. Native hooks must be tied to the supported ROM fingerprint and
regenerated reproducibly; never patch generated functions by hand.

## Reference finish and acceptance

- Portable clean package: native executable, required redistributable libraries,
  controller database, licenses, build identity, instructions and checksums.
  Import and validate the user's ROM, including ZIP and byte-order variants;
  no original ROM, extracted overlays, saves or captures in the clean archive.
- Durable progress and settings, separate from program files. Existing raw
  512-byte EEPROM progress remains loadable. Interrupted writes and corruption
  recover a validated backup or report an actionable error without erasing data.
  Tests use isolated data directories and never modify the user's progress.
- Saved resolution, aspect, refresh request, window mode, MSAA, presentation
  and texture filters; defaults, saved preferences and run overrides have clear
  precedence. Alt+Enter remains available as explicitly requested for Blast Corps.
- Logical controls with keyboard, hot-plugging, active device prompts, per-action
  remapping and manual family override. Xbox, PlayStation and Switch labels and
  glyphs match physical positions; Switch south=B/east=A/west=Y/north=X. Prompts
  inside the game must follow actions, not retain misleading N64 button labels.
- Accurate 16:9 cameras and HUD, high-resolution rendering, checked frame
  interpolation for vehicles, cameras, destruction and particles. Record actual
  presentation rates and performance; requested 120 is not measured 120.
- Continuous stereo audio during gameplay, pause, mission transitions and
  device changes. Muting output must preserve the audio task scheduler.
- Playable missions, vehicles, objective completion, failure/retry, pause,
  transitions and progress reload. No unresolved fatal services, guest assertions,
  crashes or reproducible hangs on covered flows. Coverage gaps remain explicit.
- Long native runs and fresh-install/upgrade tests. Every acceptance claim has
  a command, log and/or native capture. Physical controller and platform tests
  require the actual devices; virtual controllers prove only software behavior.

## Release gate

A candidate is ready only after the above implementation and relevant regression
checks pass, followed by an independent review and resolution of material
findings. Package privately for review. Keep a readiness ledger distinguishing
implemented, tested, physically tested, and pending; unknown coverage cannot be
converted into a passing claim.

## Settings menu implementation decision

Reuse the original pause window's fonts, animation and selection behavior.
Its More entry will open PC Options; Game Options remains reachable. Host
pages expose at most four rows so they fit the existing layout. Graphics pages
cover resolution/aspect/refresh, then fullscreen/MSAA/presentation, then texture
filter; audio and logical controls have separate pages. Back returns to the
previous page or the original pause screen. A durable-save acknowledgement
updates the displayed value; failed writes retain the saved value and report
the failure. Graphics choices that cannot yet apply live say restart required.
The selection hook uses the proven ROM-specific row indices and is regenerated,
and text scratch storage resides outside the game's 8 MiB RDRAM region in the
host allocation; it contains CPU-read strings only, never RSP data.
