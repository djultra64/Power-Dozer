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

Latest controls acceptance: editor and help are integrated into the original
pause window. Task 4 remains in progress: original tutorial/HUD prompts and
drawn device glyphs are not yet integrated, and physical acceptance is pending.

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

Completed live-filter regression: `.local/qa/gameplay-lquhyjws`, 180 seconds,
exit 0, 10,801 VI / 10,480 graphics / 5,398 audio tasks. Saved scale 4.5,
presentation pixel, texture linear and volume 0; the renderer acknowledged
both filter changes. Original pause return and mission resumption passed.
Captures reviewed, source EEPROM SHA-256 unchanged. Software Vulkan/dummy
audio only; physical output performance remains pending.

Controller Pak foundation: a bounded 16-slot/123-page file model and atomic
checked primary/backup store now pass their tests. Coverage includes duplicate
identity, page rounding, directory/data capacity, allocation overflow,
unwritten-file Bad Data, aligned partial writes, index/range errors, metadata,
reload, backup recovery, checksum damage, oversized input and refusal to replace
two invalid copies. Initial RED was the missing Pak implementation; GREEN is
`controller_pak_files_and_recovery`, with 10/10 native tests passing.
The guest services still report no Pak; native allocation/progress acceptance
must follow the high-level service bridge implementation. Structural malformed
directory fixtures and interrupted-write injection also remain to be added.

Controller Pak bridge now supplies initialization/presence, free blocks,
checker, allocation/find/delete, aligned read/write and file state. Controller
query/init retain the existing runtime and add memory-card presence on port 0.
The bindings are generated from the supported ROM, not edited manually.
Tests cover O32 stack arguments, endian metadata, RAM/stack bounds, absent
ports, empty-directory lookup and failed atomic commits retaining both RAM and
disk contents. A valid-CRC malformed directory fixture and an independent
Python/zlib inspector were added. Twelve native / 25 Python tests pass.

Native Pak creation: `.local/qa/gameplay-d3_576k7`, 180 seconds, exit 0,
10,801 VI / 10,491 graphics / 5,399 audio tasks. The original game created its
four 3,584-byte files, accepted player AAA, loaded Simian Acres and passed
movement/pause/resume. An independent inspector verified the checked image and
player header. Native reload: `.local/qa/gameplay-7n7m9q9t`, 150 seconds,
9,001 VI / 8,675 graphics / 4,498 audio tasks, existing AAA recognized without
name entry; the same mission and controls passed. The source Pak snapshot was
unchanged. These are software Vulkan/dummy-audio tests.

Upgrade protection: enabling the Pak hides the original EEPROM-only selector.
Only when no Pak primary or backup exists, import the validated legacy player
into slot 1 without modifying EEPROM. A deliberately emptied existing Pak must
not be repopulated. Import validates the game's player checksum and completion
marker; a bad source stops before writing. The first native attempt omitted
the footer marker, and the game rejected the isolated converted file. A RED
regression reproduced that omission; the corrected import retains the marker
at the Pak location. `.local/qa/gameplay-k70mc8w6` passed 180 seconds: 10,801 VI /
10,466 graphics / 5,399 audio tasks, original ALF recognized without name entry,
map/mission, movement, pause/resume and Restart. Original EEPROM hash unchanged.

Remaining progress hardening: the supplied ALF save has no completed-level
records. Convert the EEPROM best-time pairs and the original demolition-state
sentinels for completed saves before claiming general upgrade acceptance.
Also group the game's in-progress→complete marker save sequence into a durable
transaction: atomic storage of each individual Pfs write alone cannot guarantee
that an interrupted multi-write game save restores its last complete version.
These are required before the release gate, alongside objective completion.


Progress transaction hardening: `PakSession` now stages the ROM's explicit
in-progress marker and following writes, committing the complete player file
only on the original completion marker. Other players' pending data must not
leak into that commit. Tests cover interruption/reload, write failure with
retry, concurrent player saves, unrelated writes and deletion. The guest ABI
suite repeats interruption/failure/retry through actual O32 Pfs calls. Older
checked images carrying the explicit in-progress marker recover a complete
backup; without one, startup refuses the image and preserves it. The backup
written after recovery must remain complete. RED→GREEN observed for transaction
and old-image recovery tests; 13/13 native tests pass.

Native transaction flow: `.local/qa/gameplay-0cs2gidt` passed 180 seconds,
10,801 VI / 10,492 graphics / 5,398 audio tasks. Four original files created,
new AAA player named, map and mission reached, movement/pause/resume verified.
The name-save sequence logged two staged writes followed by one durable
complete write. Checked image inspected; software Vulkan/dummy audio only.

Completed legacy conversion now reconstructs original per-vehicle time packets
with their game checksum, validates EEPROM time/XOR pairs, and supplies the
original demolition reconstruction sentinel with its checksum. Preserve the
entire player packet and exclude original special levels 38/47/49. The level
and vehicle slot tables are checked against ROM bytes during normalization.
An independently encoded synthetic fixture covers early/later demolition and
a race time in different vehicle slots; invalid time pairs/completion marker
must refuse import without mutating the candidate. RED→GREEN observed.

Completed-player native harness diagnosis: `.local/qa/gameplay-1746cgpc`
recognized the converted player, read time 1234 and reconstructed its status,
but timed out because the test expected a new-level load instead of mode 7
statistics. `.local/qa/gameplay-zd58mlnm` additionally replayed that level and
passed movement/pause/resume; its restart attempt selected View Stats, a row
that is disabled for new levels. Neither is accepted as a complete regression.
The harness now follows the original statistics/GO path, skips the enabled
View Stats row before Restart, and accepts the actual pause transition for
both mission states. Acceptance requires map mode 14 and mission mode 2;
mode 0 is only a new-player introduction, not a mission. A checker regression
explicitly rejects missing mission mode 2 and accepts existing-player flows
without mode 0. Current Python suite: 26/26 passed. Full rerun pending.

Completed-player regression: `.local/qa/gameplay-1wvhvmie` passed 180 seconds,
10,801 VI / 10,919 graphics / 5,399 audio tasks. QA DONE recognized without
name entry, original time 1234 read successfully, demolition status rebuilt,
statistics/GO (14→7→13→2), movement/pause/resume and Restart (2→13→2) verified.
Captures reviewed. The synthetic EEPROM seed and original user EEPROM stayed
unchanged. This establishes migrated completed-progress acceptance; it does
not establish completing a new objective during gameplay, still pending.


Task 4 input foundation: logical action bindings now own keyboard, physical
buttons and signed axes, including D-pad movement/right-stick camera, optional
multiple bindings and validated remaps. Label/glyph resolution shares those
bindings: Switch south B/east A/west Y/north X, PlayStation shapes, Xbox labels,
unknown physical positions and manual-family fallback. Last keyboard/pad press
changes prompts; releases do not steal them. The source-independent model
covers invalid remaps, dead zone/nonfinite axes, focus loss, held regain,
shortcut suppression and disconnection. This is not yet the in-game prompt or
remapping-editor implementation; window title is still its current UI consumer.
Legacy controls JSON is bounded and errors preserve the source; the complete
versioned remapping store remains pending.

Virtual SDL regression: all Xbox/PlayStation/Switch descriptor fixtures plus an
ambiguous controller pass with matched physical south action and prompt.
Manual fallback, D-pad/right-stick/trigger actions, focus loss/held regain,
disconnection and reconnect are exercised through actual SDL polling. It found
two RED regressions: generic SDL virtual Xbox type needed name fallback, and
already-held buttons fired on connection. Both are corrected and rerun GREEN.
Physical controllers and native in-game family glyph captures remain pending.
Current native suite: 15/15; Python: 26/26 passed.

Native keyboard regression after model integration:
`.local/qa/gameplay-zot6vvgq`, 180 seconds, exit 0, 10,802 VI / 10,471 graphics /
5,399 audio tasks. ALF legacy upgrade recognized, map/mission entered,
movement/pause/resume, graphics/audio menu changes and Restart verified.
Source save and original EEPROM hash unchanged. Software Vulkan/dummy audio.
The later SDL family-name/hotplug corrections are covered by virtual SDL tests;
they do not alter the keyboard path, but the final release regression must
still run against the eventual complete build.

Controls persistence/editor: versioned, bounded action JSON with primary/backup
recovery, strict source/type validation, preserved unknown metadata and legacy
flat-controller import. Capture waits for opening controls to release, cancels
on Escape, focus loss or disconnection, rejects an input assigned to another
action, and applies a remap only after its durable save succeeds. Defaults
require explicit selection; reset also waits for durable acknowledgement.
Tests cover signed-axis/integer overflow, corrupt files, write failure, held
inputs, cancellation, conflicts and live/reloaded mappings. Native suite 18/18
and Python 26/26 passed. These are software tests, not physical acceptance.

Native editor acceptance: `.local/qa/gameplay-wg2epib2` passed 180 seconds,
10,801 VI / 10,471 graphics / 5,399 audio tasks. Changed Confirm/acceleration
Z→T, checked durable JSON, cancelled a second change with byte-identical config,
reviewed two help pages, drove using T, paused/resumed and restarted. A prior
harness timeout came from assuming JSON key order; corrected parsing uses the
event fields and the full run was repeated.

Capture review found the cloned View Stats row retained its original disabled
grey palette. An unconditional guest-table regression reproduced this; custom
rows now use ROM-verified white/red colours. `.local/qa/gameplay-el8i5pp9`
repeated the full 180-second flow after the fix: 10,801 VI / 10,462 graphics /
5,399 audio tasks, accepted with remap/cancel/help/movement/pause/Restart and no
unclassified assertions. Keyboard screenshot confirms Back is white/available.
The selected row follows the original blink animation. User EEPROM SHA-256
remains `1bb5b014be69e0a38a76d24d78c953ca51f492f93aff40d4d5fda62ab4bae26b`.
Both runs use software Vulkan and dummy audio; physical output remains pending.

Original prompts: implemented a ROM-validated core font hook for thirteen
original string addresses (eleven distinct instructions), resolving pause,
thrust, dive, skid, launch, vehicle exit and map/page controls through the live
action model. Exact address+text matching preserves ordinary prose and original
ROM strings. Tests exercise keyboard/Xbox/PlayStation/Switch, remapping,
unbound actions, every known address, unrelated/modified guest strings,
CPU-only text bounds and longer-name width. Switch PLUS naming fixes an observed
font limitation: + has no original glyph. 19 native / 26 Python tests pass.

Native regression after regeneration: `.local/qa/gameplay-i50dfc9f` passed
180 seconds, 10,801 VI / 10,469 graphics / 5,399 audio tasks, covering the editor,
cancel/help, changed-key driving, pause/resume and Restart. The guest font hook
executed PRESS START→PRESS ENTER during the mission introduction.
`.local/qa/gameplay-0mn243uj` passed 100 seconds, 6,001 VI / 5,673 graphics /
2,999 audio tasks, explicitly capturing that instruction plus movement and
pause/resume. `mission-action-prompt-next.png` reviewed: PRESS ENTER is visible
and fits the original location. Both sources remained unchanged; software
Vulkan/dummy audio only. Other tutorial lines have guest-memory test coverage,
but their native screenshots and actual device glyph rendering are pending.

Device glyph implementation: actual vector cells are now integrated into the
original control editor/help rows. The guest-table test observes RED for the
missing post-draw bridge, then GREEN for bounded commands, no write when there
is insufficient display-list space, closed-window suppression and opacity.
The raster/command suite covers four distinct face buttons in all three
families plus unknown positions, remapping, all default control types,
unbound actions, cell/screen bounds and paired graphics state restoration.
SDK macros were compiled independently to verify the RDP encodings. Normalizer
validates the original return instructions before inserting the post-draw hook.
Current suites: 20 native / 26 Python passed.

Native keyboard glyph regression: `.local/qa/gameplay-l9rj1ddt`, 180 seconds,
10,801 VI / 10,467 graphics / 5,398 audio tasks. Remap/cancel/help, changed-key
movement, pause/resume and Restart accepted. `controls-saved.png` reviewed:
keycap cells align beside the actual labels; the original map/page instruction
also reflects the configured keyboard bindings.

Manual-family editor rendering captures: PlayStation `.local/qa/gameplay-k2bx7xpg`
130 seconds, 7,801 VI / 7,470 graphics / 3,899 audio; Xbox
`.local/qa/gameplay-x54q6gt1` 150 seconds, 9,001 VI / 8,655 graphics / 4,499 audio;
Switch `.local/qa/gameplay-6slspflu` 150 seconds, 9,001 VI / 8,663 graphics /
4,498 audio. All five editor pages, movement and pause/resume accepted.
Reviewed first pages: PS cross/circle, Xbox green A/red B and Switch B/A/PLUS
match their captions. PS camera/stick direction and shoulder page plus final
movement page also reviewed. These software Vulkan/dummy audio runs use manual
family selection and keyboard navigation, not actual controller actions.

Final polish following these captures: read the original 16-bit opacity field
as a halfword (regression explicitly checks alpha 127 and alpha 0) and rename
HORN L/R to SPECIAL L/R because the same actions skid/launch in other vehicles.
Current-build PlayStation rerun `.local/qa/gameplay-aei22ros` passed 130 seconds,
7,801 VI / 7,474 graphics / 3,899 audio tasks. All five pages, movement and
pause/resume accepted; reviewed the new SPECIAL L caption with its L1 glyph.
Source progress stays unchanged. Full suites pass.

Remaining Task 4 work: native device-driven help/tutorial glyph captures,
glyphs beside original HUD/tutorial instructions (currently adapted names),
and physical button/prompt acceptance.
Kernel inventory announces a physical Xbox-compatible controller and HHD/Steam
virtual devices; enumerating those does not count as pressing physical buttons.

Task 5 source finding: pinned RT64 `rt64_workload_queue.cpp` lines 221–226
clamps manual refresh to the detected swap-chain rate. Xvfb's 60-Hz surface
therefore gives effective target 60 despite --refresh 120. Do not bypass this
with a fake physical acceptance claim; validate actual supported display modes
and geometry interpolation separately before the graphics release gate.

Controller generation refinement: SDL-reported Xbox 360/One and PS3/4/5 types
now select the actual Start/Back, Menu/View, Start/Select, Options/Share or
Options/Create names. Normalized device-name fallback handles Linux X-Box and
PLAYSTATION(R) names when SDL only identifies a virtual/unknown controller.
Reliable SDL type takes priority over names; single Joy-Cons stay unknown
because their physical orientation is not inferred. Same-family manual
selection preserves a known generation; a different-family override uses
that family's generic labels. Reserved capture cancel follows the actual
physical Back control even while keyboard is active. Stick clicks use a
distinct glyph from stick directions, with L3/R3 names on PlayStation.

Current verification: build completed, 20/20 native and 26/26 Python tests
passed. The SDL integration test uses seven virtual device identities, sends
actual South/Start events, checks resulting guest action bits and captions,
and exercises keyboard handover, focus release, disconnect/reconnect and
durable remapping. This verifies the SDL adapter, not physical hardware
presses or a rendered native controller handover; those acceptance gates
remain open.
