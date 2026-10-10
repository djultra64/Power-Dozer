# Release readiness ledger

Reference inspected: Tetrisphere options-beta checkout, Blocksphere 1.0.1 Linux
package instructions and in-game PC options design. Its Windows and physical
controller acceptance are separate from compilation.

Current Blast Corps evidence: boot/demo, RT64 high-resolution target, stereo
audio, Alt+Enter, L3D map and Simian Acres load. The latest isolated graphical
run completed 110 seconds; it did not complete a mission.

Ruling: execute local design/plan stages without renewed approvals — the user
explicitly authorizes autonomous work while away — scope decisions must remain
documented and publishing remains external.

Ruling: preserve the existing `.local/eeprom.bin` default until portable bootstrap
migration is tested; add explicit isolated data paths first — tests must not
change the user's progress — incorrect migration could otherwise lose saves.

## Work queue

- Complete: durable EEPROM storage and isolated data paths, including native reload.
- In progress: gameplay/failure/pause/progress harness and assertion detection.
- Pending: persistent graphics options and original-style settings menu.
- Pending: complete controls, in-game family-correct glyphs and remapping.
- Pending: aspect/interpolation/L3D/audio acceptance across representative missions.
- Pending: portable ROM bootstrap, clean package and platform verification.
- Pending: independent release review.

Task 1 tests: missing save, raw import, persistence, damaged-primary recovery,
both copies invalid (no reset), oversized image, checksum corruption, interrupted
temporary file and independent data folders. RED: missing SaveStore sources;
GREEN: native save_recovery_and_isolation test. Full native suite 5/5 and existing
Python suite 5/5 passed after integration.

Task 1: complete. A fresh native startup wrote primary/backup in the checked
format; another native launch loaded that image and completed 8 seconds
(481 VI / 445 graphics / 239 audio tasks), exit 0. Added and observed RED→GREEN
for a checked file truncated to exactly 512 bytes: it must not be mistaken for
legacy raw data. Current full suites: 5 native / 10 Python passed.

Native isolation probe: 110 seconds loading the existing raw save and reaching
Simian Acres, no graphics/audio failure. Original SHA-256 stayed
`1bb5b014be69e0a38a76d24d78c953ca51f492f93aff40d4d5fda62ab4bae26b`.
No save write occurred in this seeded flow, so no migration is expected yet.
Fresh-start probe produced versioned primary and backup (524 bytes each).

Task 2 tests: assertion faults, crash text, missing mission transition and frozen
graphics with advancing audio are rejected. The previous successful 110-second
map/mission-load log passes; the earlier live log is correctly rejected for
guest assertions. A long AMD desktop run completed 25,938 VI / 26,612 graphics /
12,966 audio tasks with only one empty-queue observation, but is NOT accepted
for release: yoshi pending-demand and mb render-mode assertions occurred.

Task 2 reproduction: a fresh native installation consistently reports pending
yoshiDemand assertions on transition 53→18. The original game function reports
these when an unconsumed window demand already exists. Investigate calls and
frame consumption before selecting a fix; do not suppress the diagnostic.

Ruling: use branch `codex/release-readiness` in the existing unborn repository
instead of duplicating ROM/private build state in a worktree — this workspace
contains only the prototype, and no prior branch commit exists — reproducible
commits must now preserve checkpoints; native test data remains isolated.
