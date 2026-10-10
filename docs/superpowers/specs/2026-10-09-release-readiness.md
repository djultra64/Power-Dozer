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

## Original prompt integration decision

Resolve the ROM's thirteen known instruction-string addresses through the same
logical action model as actual input. Match both address and original text;
leave narrative prose and modified guest strings intact. Normalize a hook at
the core font-layout entry, validating its original instructions and all text
bytes before generation. CPU-only replacement slots are consumed synchronously
by the original font code; its normal geometry remains in guest RSP memory.
Longer names reduce glyph width to preserve the original line's space. Switch
Start uses the written name PLUS because the original font omits the + sign.
This stage supplies device names; actual controller glyph drawing and native
captures for the other tutorial pages remain required.

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

## Controller Pak implementation decision

The game stores per-level data in Controller Pak files in addition to EEPROM.
Provide a host implementation of its verified high-level libultra services
(`osPfsInit`, `osPfsIsPlug`, `osPfsFreeBlocks`, `osPfsChecker`, allocation,
find/delete, read/write and file state). Keep guest menu/save logic intact.
Port 0 has one persistent Pak with 16 directory slots and 123 256-byte pages;
other ports report no Pak. File identity contains company/game codes and the
original encoded 16-byte name/4-byte extension. Preserve file indices, enforce
libultra's 32-byte read/write alignment and return its documented error codes.
Use a bounded, versioned/checksummed host image with atomic primary/backup
storage in the explicit data directory. A corrupt primary recovers only a
validated backup; two corrupt copies must not create an empty Pak. Do not claim
raw hardware-image import/export until implemented and tested separately.
The guest bridge must bound RAM pointers and O32 stack arguments and translate
metadata into the original big-endian `OSPfsState` layout. Test file capacity,
duplicates, deletion, partial writes, alignment/range errors, reload, corruption
and interrupted writes before enabling the Pak in native gameplay.

When no Pak primary or backup exists, preserve a valid legacy EEPROM player
by importing it into the first Blast Corps Pak file. Validate the original
packet checksum and footer marker; preserve the player packet, completed-level
best times and the original reconstruction sentinel for demolition levels.
Existing Pak files, including a deliberately erased empty Pak, take precedence
and must never be repopulated from the older EEPROM. The original EEPROM is
retained byte-for-byte. Stage the game's in-progress/complete save sequence
and persist only a complete transaction, retaining the previous complete image
for recovery if the process or a write is interrupted.


## Input model implementation decision

Keep the proven keyboard actions: Z confirm/accelerate, X back/reverse, Enter
Start/pause, Shift brake/exit, arrows movement, I/J/K/L camera and Q/E horns.
Default pad actions use physical south/east for confirm/back, Menu/Options/+
for Start, left trigger or north for brake/exit, shoulder buttons for horns,
left stick/D-pad for movement and right stick for the four camera actions.
Right trigger is an additional accelerator binding. This replaces the prototype
mapping that used D-pad directions for camera and right trigger for brake.
All labels and glyph identities resolve from the same actual action bindings.
Unknown controllers use physical-position labels; manual family affects pad
prompts while keyboard remains keyboard. Button release does not steal prompts
from another device. Focus loss clears guest input immediately; held controls
must release before acting again after regain/reconnection. A newly attached
pad's already-held controls are also suppressed until release. Alt+Enter
consumes its physical keys through release, independent of logical remapping.
The model and SDL adapter are tested separately; virtual SDL tests are software
acceptance and never substitute for tests with physical hardware.
