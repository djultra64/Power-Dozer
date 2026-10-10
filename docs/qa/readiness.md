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

Ruling: portable migration is now tested below. Bare executables use adjacent
`data`; development launchers explicitly retain `.local`, preserving the current
installation. All QA imports use private destinations; original progress is
never changed by acceptance tests.

## Work queue

- Complete: durable EEPROM storage and isolated data paths, including native reload.
- In progress: gameplay/failure/pause/progress harness and assertion detection.
- In progress: persistent graphics options and original-style settings menu.
- In progress: controls/remapping and family-correct native glyphs implemented; physical acceptance pending.
- Pending: aspect/interpolation/L3D/audio acceptance across representative missions.
- Pending: portable ROM bootstrap, clean package and platform verification.
- Pending: independent release review.

Latest controls acceptance: editor/help and original HUD/tutorial instruction
names and glyphs are integrated. Keyboard and virtual SDL families have native
evidence; physical button acceptance remains pending.

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

Audio output recovery implementation: `AudioOutput` retains the pending AI
byte budget when the sink is unavailable and consumes whole stereo frames at
the guest sample rate using a monotonic clock with fractional frame carry.
A failed initial open, failed enqueue, stopped backend or removal of the
current output device no longer terminates the game. The existing audio mutex
serializes SDL access and the AI FIFO remains intact through loss. Default
output retries once per second. Recovery pads only the remaining budget with
silence and then accepts new PCM, preventing old sound effects from replaying
or the guest's remaining-buffer count jumping. Rate changes reset both clocks
as before. Output availability changes are recorded in diagnostics.

The deterministic regression first failed because the stub discarded the
unavailable output's byte budget, then passed. Coverage includes unavailable
startup, fractional frame timing under frequent polling, mid-buffer loss,
new buffers during loss, throttled reopen, recovery with a pending budget,
failed enqueue, failed recovery padding, rate changes, long intervals and
invalid partial stereo frames. The real SDL dummy backend test verifies actual
queue consumption, stopped-device detection, reopen and new PCM delivery.
Full current suites pass: 22 native / 32 Python. The evidence checker rejects
missing injection/recovery, stalled audio and insufficient outage observations.

Native queue-failure acceptance: `.local/qa/gameplay-soy3sqy9`, 180 seconds,
10,801 VI / 10,471 graphics / 5,400 audio tasks. A private LD_PRELOAD helper
simulated three seconds of initial unavailability, a queue failure at 45
seconds and eight seconds without an available output. The four availability
transitions were observed; 150 audio tasks advanced between the pre-loss and
in-outage progress samples. Movement, pause/resume, live filters and volume
options were accepted and the source EEPROM stayed unchanged. Private helper
code is never linked into the port. The first attempted injection was invalid
because ld.so split a path containing spaces; it supplied no fault evidence.
The corrected run uses a relative preload path without spaces. New harness
runs compile helpers per isolated directory to avoid changing a library while
another test process has it loaded.

These checks use software Vulkan and SDL dummy output. They establish guest
continuity and backend recovery, not speaker audibility, a real unplug or
physical 120-Hz rendering. PulseAudio/output-device acceptance remains open.

Native SDL output-removal acceptance: `.local/qa/gameplay-vf4ptqal`, 180 seconds,
10,801 VI / 10,468 graphics / 5,400 audio tasks. Three-second startup outage,
current-device SDL removal event at 45 seconds and eight-second unavailable
period all passed the dedicated recovery checker. 301 audio tasks advanced
between the pre-loss and in-outage observations. Keyboard remap/cancel/help,
changed-key driving, original PRESS ENTER adaptation, pause/resume and Restart
were accepted. `controls-help.png` reviewed: T/X/Enter labels with matching
keycap glyphs fit the original menu. EEPROM/Pak source hashes unchanged.
This run used the new --audio-loss removal harness flag and a per-run helper;
actual hardware disconnection and speaker output remain separately pending.

Native device-driven control acceptance now uses a private LD_PRELOAD SDL
virtual-controller helper, not a family setting or guest-button injection.
SDL itself emits connection/button/axis events and the shipping input adapter
reads the virtual device through ordinary SDL controller APIs. The helper
filters physical/other host devices only inside its test process, so it cannot
select the user's HHD/Steam controller by accident. Requests use an atomic
private file; per-run libraries are not linked into the port.

Three 140-second native runs passed: PS5 `.local/qa/gameplay-qewxpyu9`,
8,401 VI / 8,064 graphics / 4,199 audio; Switch `.local/qa/gameplay-zix_q5mz`,
8,401 VI / 8,056 graphics / 4,199 audio; Xbox 360
`.local/qa/gameplay-qdhbs2yg`, 8,401 VI / 8,001 graphics / 4,199 audio.
Automatic family/generation selection, controller Help page navigation via
physical South, keyboard handover, disconnect fallback, held-South reconnect
suppression, release/repress, steering/acceleration and native pause/Continue
were accepted. Reviewed first-page captures: PS cross/circle/Options, Switch
B/A/PLUS and Xbox A/B/Start match actual SDL action traces. A separate evidence
checker verifies those captions, controller Confirm/Start bits and analog
movement under the detected source; rejects wrong Switch letters, manual-only
evidence, missing Start, no keyboard handover and held-button leakage. All
three logs passed that checker. Current suites: 22 native / 38 Python passed.

The initial versions of all three native tests failed an incorrect assertion
that a second Start directly resumes. Guest traces instead showed mode 8→2,
window 1 and paused=1. Original `hd.c` menu selection case 1 shows that Start
selects More; window 1's first row is CONTINUE. This was checked against the
extracted ROM: entry 802F58AC points to 80309A18, string CONTINUE. The revised
test verifies this exact paused window/state, then physically presses South
and waits for guest unpause. No shipping game/input behavior changed to hide
the test failure. The prior failed directories remain partial visual/action
evidence, not successful overall acceptance runs.

The primary display's listed modes peak around 60 Hz and the secondary's
around 75 Hz (`xrandr --query`, read-only, no display settings changed). Neither
currently offers a listed 120-Hz mode. Virtual device and Xvfb tests do not
verify physical controller presses or physical 120-Hz presentation. Native
controller Help glyphs/handover are accepted. The following original-instruction
acceptance extends that evidence; physical button acceptance remains open.

Original-instruction glyph bridge: the existing exact-address/exact-string
font hook now records action-resolved annotations alongside the replacement
names. Verified 80259450 resets them per frame; verified 80259824 consumes them
at text-queue flush and appends graphics commands through Gfx**, within the
FrameContext main-DL bounds and a 256-byte caller reserve. State scopes restore
RDP state; no guest texture/vertex storage is repurposed. Window icons follow
the current window coordinates only in stable phase 2. Foreground color and
alpha follow the original text; black shadows cannot keep blink-hidden icons
alive. Inline prefixes preserve tutorial right edges and avoid original
decorative sprites. Map/page hints retain both action glyphs before x=24.

RED evidence: action-span stub omitted the second map action; actual queued
glyph stub emitted no commands. Subsequent regression checks exposed shadow
passes retaining colored icons and floor-downsampling losing the small trigger
bottom border. GREEN: shared declarative action rules, consumed/reset annotations,
foreground filtering and area-based cell reduction pass the corresponding
assertions. Tests also cover exact matching, alpha, animated windows, bounds,
nonfinite positions, full-context validation and actual remapped Square raster.
An early compile failure was repaired before the valid runtime RED; it was
not accepted as evidence of expected behavior. Normalizer ROM fingerprints
and a second normalization preserve the generated CPU files byte-for-byte.

Polished native keyboard run `.local/qa/gameplay-yyyyrgt_`, 120 seconds:
7,201 VI / 6,865 graphics / 3,599 audio tasks, movement/pause/Continue and
original PRESS ENTER accepted. PS5 `.local/qa/gameplay-2922n01c`, 140 seconds:
8,401 VI / 8,049 graphics / 4,199 audio. Switch `.local/qa/gameplay-q_e3uqv8`,
140 seconds: 8,401 VI / 8,056 graphics / 4,198 audio. Both controller runs
accepted automatic help, keyboard handover, held-button reconnect suppression,
steering/acceleration and native pause/Continue. Original-instruction command
checker passed all three logs (three keyboard sites plus family-resolved map
controls for each controller). Source EEPROM/Pak hashes remained unchanged.
Reviewed PS5 mission-start and active-help captures: tutorial keycap no longer
touches the original question mark; the small L2/R1 map cells retain borders
and matching labels. Pre-polish captures are diagnostic evidence only.

The permanent probe now has --instruction-glyphs to verify emitted original
HUD/tutorial commands together with mapped text. Its checker explicitly keeps
visual capture review separate, rejects missing/invalid/out-of-bounds commands
and mismatched labels; virtual-controller checks do not claim physical devices.
Current suites: 23 native and 44 Python tests passed. Software Vulkan and dummy
audio evidence does not establish 4K performance, physical 120-Hz output or
speaker audibility. Mission objective completion and portable bootstrap remain
open release gates.

Xbox 360 polished original-instruction acceptance:
`.local/qa/gameplay-yej0967t`, 140 seconds, 8,401 VI / 8,066 graphics /
4,198 audio tasks. This run used --instruction-glyphs; the integrated checker
accepted four original sites including actual LT/RB map glyph commands. Native
controller help/handover/reconnect, movement and pause/Continue also passed.
Reviewed active-help capture: actual A/B/Start controls and closed LT/RB map
cells match labels. Source EEPROM/Pak remained unchanged. All three prioritized
controller families now have native original-map glyph evidence; physical
presses and wider vehicle/tutorial coverage remain separately pending.

Task 6 isolated native ROM service (not yet connected to shipping main):
include/rom_input.h and src/rom_input.cpp bound reads to 32 MiB, ZIP directory
work to 128 entries and extraction to a single exactly 8-MiB ROM. ZIP paths
are never written to disk; encrypted/unsupported/ambiguous candidates fail.
Canonical z64 and swapped v64/n64 normalize before full SHA-1 revision
validation. The two executable overlays inflate into exact-size buffers from
verified ROM slices, with size/CRC checks. Pinned runtime miniz is compiled
locally; its export header is generated in our build directory, leaving the
Tetrisphere reference untouched. No new runtime dependency or save write.

Actual RED: the API stub failed the empty SHA-1 vector. An earlier missing
miniz_export.h compile error was repaired and was not treated as behavioral
RED. GREEN covers empty/abc/million-a and double-block-padding SHA-1 vectors,
raw/swapped normalization, sizes/header/revision rejection, nested uppercase
ZIP member with magic discovery, ambiguity, invalid archive/expanded size,
missing files, input limit and no filesystem extraction.

Private ZIP and raw acceptance both match the existing preparation's code
831,136 bytes and frontend 171,664 bytes exactly. Real private swapped v64 and
n64 copies also pass full validation and exact-overlay comparisons, stored in
.local/qa/rom-input-* only. Evidence: .local/evidence/rom-input-private-{zip,raw,
v64,n64}.log. EEPROM SHA256 remains
1bb5b014be69e0a38a76d24d78c953ca51f492f93aff40d4d5fda62ab4bae26b.
Full native suite passed 24 tests; after adding the padding-boundary assertion,
the affected native loader test passed again. Previous 44 Python tests remain
unchanged. Cache recovery, progress migration, GUI selection and clean-package
launch are not yet implemented/accepted; main still uses the legacy asset paths.

Task 6 native cache connected to main: --rom accepts the validated native
input service; cache/rom.z64 lives under --data-dir. Each restart validates
the complete cache and regenerates overlays in memory. Main no longer loads
source-root hd_code.bin/hd_front_end.bin or source-root ROM assets. A missing
cache can bootstrap from data-relative legacy rom.z64; corrupt cache requires
a valid explicit import. Import/extraction finishes before atomic cache
replacement and before --save-settings writes. Matching canonical imports
avoid rewriting the cache. Default data directory and controller database
lookup remain legacy source paths deliberately until migration acceptance.

RED: composed-cache stub failed the actual-ROM assertion "explicit import
returns validated ROM". GREEN private native acceptance covers import/restart,
invalid import preserving cache, corrupt-cache rejection/explicit repair,
legacy data-relative bootstrap, and unchanged sentinel progress. A real
non-root read-only cache repair fails without replacing bytes. The invalid
shipping CLI exits 66 with a ROM error before creating cache/settings or
changing sentinel progress, including --save-settings. Native suite 24 passed
and Python suite 44 passed.

Relocated executable acceptance: .local/qa/relocated ROM bj6mjd_d, arbitrary
working directory and private data path both containing spaces. First boot
imports the user's ZIP, 721 VI / 680 graphics / 359 audio tasks over 12 seconds;
second boot supplies no --rom and reads only its validated private cache,
721 VI / 680 graphics / 358 audio. Both exit normally and report expected
overlay byte counts. This establishes the asset loader's relocated operation,
not clean redistributable libraries or portable default data.

Full mission regression with cache-import startup: .local/qa/gameplay-mse0ob2l,
120 seconds, 7,201 VI / 6,875 graphics / 3,599 audio, map/Simian Acres, movement,
pause/Continue and three original-instruction glyph sites accepted. Source
EEPROM/Pak unchanged. This run preceded the final move of the optional settings
write after validation; the changed invalid-ROM/settings path was then checked
separately. No new renderer/audio/input behavior was changed by that move.
All graphics in these runs remain software Vulkan/dummy audio, not physical
4K/120-Hz or speaker acceptance.

Next portable-layout evidence: read-only Tetrisphere data_directory.h resolves
SDL_GetBasePath()/data, with explicit environment override for QA. Match that
portable behavior after a transactional migration/import service is tested;
remove BC_ROOT from runtime/controller mappings and provide packaged database
lookup. The beta still defaults to an eight-second prototype run and needs
normal indefinite operation before an RC. GUI ROM selection, cache/migration
interruption checks and dependency packaging remain required.

Portable data layout/import acceptance: application_data_directory resolves
SDL_GetBasePath()/data (matching Tetrisphere), with BLASTCORPS_DATA_DIR and
--data-dir overrides. The normal run is indefinite; --seconds remains a QA
limit. Runtime/input no longer reference BC_ROOT: remaps live in the selected
data directory, and optional controller database is beside the executable.
Development launchers explicitly use .local to retain existing progress.
The import service recognizes EEPROM/Pak/settings/controls plus backups and
legacy parent config/controls.json only where applicable. It bounds each file
to 64 KiB, refuses symlinks and nonempty destinations, writes a unique sibling
staging directory, validates its complete copied snapshot with the real store
recovery/committed-Pak rules, atomically renames it and syncs the parent. Original
bytes and unknown metadata are retained. It never imports ROM/overlays/captures
as progress and never deletes unrelated interrupted staging directories.

RED: missing import behavior failed the initial native migration. GREEN native
fixtures cover preservation of progress/Pak/settings/remaps, unchanged source,
no unrelated files/assets, refusal to merge, invalid snapshot with no partial
destination, valid backup recovery, empty destination, legacy remap, missing/
empty source, interrupted-staging retry and symlink rejection. Current suites
25 native / 44 Python passed.

Relocated default-layout evidence: .local/qa/portable data 8d8tskcj. A copied
binary run from /tmp imports the real .local snapshot into its private adjacent
data, then restarts with neither --data-dir nor --rom. Both 12-second sessions
exit normally. A third session supplies no --seconds, remains alive at 12
seconds and reports 601 VI / 558 graphics / 298 audio tasks at the ten-second
sample; the QA supervisor then terminates it deliberately. This proves the
prototype timeout was removed, not orderly user-close acceptance. Original
EEPROM hash unchanged.

Abrupt-interruption native evidence: .local/qa/import-interrupted-xc_t4bp0.
A private LD_PRELOAD rename wrapper calls _exit(99) after the first staged
EEPROM file rename. The exact exit code and partial sibling staging directory
were observed, with live data still absent. A clean retry completes import/
cache/bootstrap and exits normally. The original source hash stays unchanged.
The helper is private and not linked into the port; this is process-loss
acceptance, not physical power-cut evidence. Existing stage is preserved.
Clean library packaging, GUI ROM selection and Windows acceptance remain open.

Native regression after portable paths/default timeout changes:
.local/qa/gameplay-ric9_1xo, 140 seconds, 8,401 VI / 8,062 graphics / 4,199
audio. Automatic Switch Help, keyboard handover, held-button reconnect
suppression, steering/acceleration and native pause/Continue passed. Integrated
original-instruction checker accepted four sites including ZL/R map controls.
Source EEPROM/Pak unchanged. Active-help capture reviewed: Switch B/A/PLUS
face/Start prompts and ZL/R map labels/glyphs agree. This remains virtual SDL,
software Vulkan and dummy audio evidence.

Task 6 native ROM selection implemented: missing/invalid default cache opens
NFD's UTF-8 file chooser with z64/v64/n64/zip filters. Existing valid cache
boots without a chooser. Explicit --rom remains deterministic CLI;
--no-rom-dialog reports failures without UI. The host-only selection controller
returns on cancel, retains loader errors for retry/quit, and never calls the
loader on fresh-install cancellation. SDL video/NFD are scoped before the
game runtime; cancel does not initialize guest threads or write progress.
Unavailable native chooser shows a visible error with manual --rom fallback.

RED: selection stub failed "valid cache boots without opening chooser". GREEN
unit cases cover valid-cache bypass, fresh selection, cancellation without
load, invalid ROM/retry, quit on error and corrupt-cache repair. Full native
suite passed 26 and unchanged Python suite passed 44.

Native SDK/SDL acceptance uses tools/qa_rom_portal.py on a private
dbus-run-session plus Xvfb. It refuses desktop DISPLAY and non-private bus
addresses, owns only that private portal name, and returns controlled responses
through actual NFD D-Bus parsing. It is not linked into the port. This verifies
SDK integration, not the user's installed portal chooser or physical clicks.
The original .local EEPROM remains unchanged.

Initial cancel and valid selection passed in .local/qa/rom-dialog-uap9xhod;
cancel leaves its data folder empty, valid ZIP boot completes 481 VI /
446 graphics / 239 audio over eight seconds. Initial retry attempt failed
overall. A subsequent visible-dialog run showed actual Enter chose Quit despite
RETURNKEY_DEFAULT on the second button; the screenshot confirms the original
Quit-first order. The button array now puts retry first with its return flag,
retaining Quit/Escape. No loader/game behavior was changed to bypass failure.

Final native retry/quit: .local/qa/rom-dialog-final-oklvuvsy. Actual SDL error
dialog renders useful ROM-size/revision instructions and the two buttons.
Enter after visible mapping triggers a second real NFD request, imports the
valid ROM and boots 481 VI / 448 graphics / 238 audio. Escape exits normally
after invalid selection, leaves data empty and makes no second request.
Reviewed retry screenshot: readable content and primary retry precedes Quit.
The first black screenshot preceded mapping and is not visual acceptance.

Native chooser service-error path: .local/qa/rom-dialog-error-5wbyxh_d. Private
portal supplies an explicit error; shipping native unavailable-chooser message
becomes visible, Return acknowledges it, main exits 66 and data stays empty.
The final unavailable-message code was rebuilt and this affected flow checked.
GTK accessibility/portal teardown warnings belong to the private bus fixture,
not guest assertions. Full mission/control regressions remain established by
the prior unchanged normal boot path; these short chooser runs prove bootstrap
only. Clean dependency packaging, actual installed portal/UI acceptance,
mission objectives, 4K/interpolation/audio performance and final review remain
open release work.
