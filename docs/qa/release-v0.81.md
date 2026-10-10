# Power Dozer v0.81 beta verification — 2026-10-10

This is a public testing prerelease. The checks below establish the observed
scope of the packages, rather than complete game or platform acceptance.

- Linux: native build, 45 CPU/runtime CTest checks and 134 Python checks passed.
  A relocated portable archive launched from outside the checkout for 30 seconds
  with Vulkan at 1920×1080. Menu captures were produced, exit status was 0, and
  the private seed EEPROM and Controller Pak files remained unchanged.
- Windows: native Visual Studio 2022 x64 build passed. Two CTest checks passed:
  command-line startup/help and atomic saves/transactional import with spaces,
  Latin and Japanese Unicode paths, repeated backups, and refusal to overwrite
  a nonempty destination.
- Windows interactive runtime: an isolated 15-second Vulkan run rendered at
  1280×720 with a 60 Hz target, accepted Enter/Z keyboard input, produced two
  nonblank captures and exited with status 0. The final counters were 903 VI,
  884 graphics tasks and 448 audio tasks; audio statistics recorded 326,224
  frames at 22,050 Hz with zero rejected buffers. These statistics do not
  establish audible continuity in every mission.
- The Windows startup failure was reproduced with a minimal MSVC program:
  a zero-sized line buffer triggered UCRT invalid-parameter termination. Windows
  now uses unbuffered diagnostics, and the help test covers executable startup.
- A final window-title version correction changes only displayed branding.
  It was rebuilt on both platforms without repeating the gameplay sessions.
- Runtime and source packaging validates file inventories, hashes and pinned
  notices, excludes ROMs/private saves/captures and includes `CREDITS.md`.

The reported thin black ground line is no longer visible to the reporter and
was not reproduced in recent local captures. No speculative graphics patch
was applied. Intermittent audio loss remains open. Broad mission coverage,
physical-controller prompts/actions, sustained 4K/120 fps and end-of-level
replay/fade/HUD-arrow visual checks remain manual. Source/license certification
flags remain false, as documented in `NOTICE` and the package manifests.

Commit identities and the public tagger are attributed to `djultra64`.
Authentic upstream copyright, author and license notices remain intact.
Private transfer/build/runtime evidence stays outside the public repository
and packages.
