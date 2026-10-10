# Power Dozer v0.82 beta verification — 2026-10-10

The shared vehicle renderer preserves each animated part's bind pivot before
RT64 interpolates its transform. Vertices are rebased around that pivot without
changing endpoint geometry. Vehicle animation channels supply the pivots for
both skin buffers; metadata survives inactive channels and is cleared when a
model slot is reused. This covers Backlash, Sideswipe and other wheeled vehicles,
including gameplay as well as attract mode.

- Native Linux build passed; 138 Python checks and 46 CTest checks passed.
- Regression tests use actual RT64 rigid-body interpolation with small, large
  and reversed rotations, moving bodies and fractional weights 0–1. They check
  endpoint geometry, axle motion, tire radius, rotated bind recovery, both
  animation buffers, inactive channels, model reloads and snapshot lifetime.
- Generated hooks target the vehicle animator and model loader only. Tests
  check hook placement and idempotence and leave building animation untouched.
- A final 200-second Linux attract-mode run exited with status 0: 12,002 VI,
  12,281 graphics tasks and 5,999 audio tasks. Pivot telemetry identifies actual
  Backlash and Sideswipe parts; private captures of both vehicles were reviewed.
- Presentation telemetry confirmed fractional frames at 60 Hz over the game's
  original 30 Hz rate. This does not establish sustained 120 fps.
- A relocated Linux runtime launched from `/tmp` with Vulkan at 1280×720 for
  60 seconds through Backlash, using isolated progress data, and exited with
  status 0: 3,601 VI, 3,603 graphics tasks and 1,798 audio tasks.
- Independent review of the final vehicle fix found no remaining blocker.
- Native Windows Visual Studio 2022 x64 build and both startup/Unicode-save
  CTest checks passed.
- A final 200-second Windows Vulkan attract-mode run in the interactive desktop
  session exited with status 0: 12,003 VI, 12,413 graphics tasks and 5,999 audio
  tasks. Five private captures cover Backlash and Sideswipe. The tested executable
  SHA-256 is `7a82c765af083731b71ad33a116ca36ad5576306ec4c2f3bcd666f4314833c47`;
  the Windows package uses that executable.

Intermittent audio loss remains open: the final Windows run recorded one
rejected audio buffer and does not establish audible continuity. Broad level
coverage, physical controllers and existing end-of-level/HUD checks retain
previous limits. Package auditing checks archive and per-file hashes, clean
commit provenance and absence of ROMs, generated guest code, private captures
and saved progress. Source/license certification flags retain their documented
status. Project commit and release attribution is `djultra64`; authentic
upstream notices remain intact.
