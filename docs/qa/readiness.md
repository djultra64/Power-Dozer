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
- In progress: persistent graphics options and original-style settings menu.
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

Task 2 diagnosis: the ROM itself deliberately saves the pending window and
calls `yoshiShow(0)` at `80274CB8` (encoded instructions validated), before
restoring it at `802750F0`. This necessarily trips its generic pending-demand
guards during the fade. Keep guest behavior and diagnostics intact. The
acceptance checker recognizes only the exact pair of yoshi.c 1312/1331 inside
instrumented cancellation, with matching saved demand, state and restoration
within 180 VI. Missing/late/wrong-state restoration and all other assertions
remain failures. Tests observed RED→GREEN for the original fade classification.

Native fresh-install gameplay: 120 seconds, exit 0, 7,202 VI / 6,893 graphics /
3,596 audio tasks; new name, map, Simian Acres, acceleration, pause mode 2→8,
resume 8→2. Native captures reviewed. Source progress was untouched.
The next telemetry run independently confirmed changed vehicle position and
paused/resumed state. Its attempted restart selected Quit instead; therefore
restart/failure acceptance remains pending and its timeout is a harness failure.

Task 2 graphics reproduction: a 360-second attract run logged four mb.c 157
assertions during level 15 load, all mode `0C192078`, lookup index 3. The original
three-row table then reads outside its bounds. This is the G_RM_PASS cycle-1
variant of `00552078`, already covered by the ROM's six-row ghost table.
Extend only the three verified PASS variants while preserving cycle 1 and
applying the original cycle-2/shared-flag translation. Validate the original
table bytes before generation; unknown modes now stop before an out-of-bounds
read. Six native tests and 17 Python tests pass; the corrected native attract
regression is running. Do not claim its acceptance until it finishes.

Progress follow-up: Controller Pak services are currently absent. The original
EEPROM saves player info; the 3,552-byte Pak file stores additional per-level
data. Complete and verify Pak persistence before the release gate.

Attract regression after the render-mode correction: 360 seconds, exit 0,
21,602 VI / 22,870 graphics / 10,799 audio tasks. Exact former level-15 failure
sites were reached four times and used `0C192078`→`0C192230`; no mb assertions.
The acceptance checker passed, classifying five verified original fade pairs.
This used software Vulkan and dummy audio; output-device continuity remains
a separate desktop test.

Gameplay restart and failure: telemetry proves mode 8→13→2 (Restart), changed
vehicle position, pause/resume and failed=1 followed by modes 9/6. The full log
was rejected for a second original yoshi diagnostic: the failure renderer at
`8026BF7C` repeats the same OFF demand (`NEW:4000 OLD:4000`). It assigns the
same value and has no other side effect for that demand. Instrument this exact
ROM-validated site, preserving the diagnostics; classify only the matching pair
with failed=1 in mode 9 and equal pending OFF demands. A different pending
window still fails. RED→GREEN tests passed; native reproduction is running.

Settings foundation: atomic primary/backup JSON storage, strict types/ranges,
nonfinite and integer-overflow rejection, bounded files and unknown-field
preservation. Preferences now reach graphics/audio and CLI overrides remain
temporary unless explicitly saved. Alt+Enter saves only window preference.
Mute queues zero samples while executing RSP tasks. Native 8-second probes
produced 239 audio tasks and 724,608 zero PCM bytes muted; reload with temporary
unmute/50% volume produced nonzero PCM while persistent mute/75% stayed intact.
In-game settings menu remains pending. Seven native and 19 Python tests pass.

Test audit: the two new tests initially used `assert`, disabled by the release
build's NDEBUG. Converted them to unconditional checks like the existing suite
and reran 7/7 successfully. An isolated build with the earlier unchecked integer
conversion now aborts against the settings regression; the corrected build
passes. No progress or audio tests had disabled checks.

Failure regression with exact OFF-site instrumentation: 240 seconds, exit 0,
14,401 VI / 14,014 graphics / 7,199 audio tasks. Copied save, mission entry,
changed position, pause/resume, Restart (8→13→2), tutorial pages and failed=1
were reached; acceptance passed, classifying one exact original duplicate OFF
pair. The source save remained unchanged. No objective-completion acceptance
has yet been recorded. Host telemetry was subsequently routed through one
stdio stream after observing partial JSON interleaving between stdout/stderr.

Settings menu model now covers original-sized pages, save acknowledgement,
failed-save rollback, preservation of unrelated settings, restart status,
Back and Controls/Game Options requests. The runtime hook is still pending;
this does not constitute an in-game menu implementation.

Ruling: use branch `codex/release-readiness` in the existing unborn repository
instead of duplicating ROM/private build state in a worktree — this workspace
contains only the prototype, and no prior branch commit exists — reproducible
commits must now preserve checkpoints; native test data remains isolated.

PC Options runtime: the original pause font, four-row layout and animations
now display graphics/audio/controls pages. Resolution/aspect/rate/MSAA are
persisted with explicit restart status; volume/mute/window/family and
presentation/texture changes reach their host subsystems. Controller help and
remapping requests remain pending Task 4; these rows are not finished features.
The hooks validate the ROM pause layout and function entries before generation.
Normalization was rerun twice without changing generated code.

Native menu regression: `.local/qa/gameplay-4m4dczgz`, 150 seconds, exit 0,
9,001 VI / 8,703 graphics / 4,499 audio tasks. The test navigated graphics and
audio, saved scale 4.5 and volume 0, returned to the original pause and resumed
with changed vehicle position. Captures reviewed. Source save unchanged.
The first run exposed Start reopening PC Options instead of resuming. A guest
bridge test reproduced this failure; the hook now preserves the original Start
path, and the corrected native run passes. Nine native and 19 Python tests pass.
Guest bridge coverage includes the host save queue, original window restoration,
Back retaining pause, and transitions out of the pause state. Shortcut updates
refresh the displayed saved settings without replacing an outstanding edit.
Native live-filter acceptance is running and has not yet passed.

Live-filter probe follow-up: both `pixel` presentation and `linear` texture
were persisted and acknowledged by the renderer in a native mission. The
expanded harness sent consecutive Back edges during the original animation,
so it remained on Graphics and then failed its Audio assertion. Its log has
no guest fault, but this is not an accepted complete menu regression. The
harness now waits for each page animation; a full rerun is required.
