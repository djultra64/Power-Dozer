# PC settings acceptance — October 11, 2026

This record covers the v0.83 Linux native candidate. Captures use the real RT64
Vulkan presentation on AMD Ryzen Z1 Extreme (RADV PHOENIX), through a private
headless Cage/Xwayland display. A compositor screencopy supplies the reviewed
image; X11 window readback is retained only as a diagnostic because it can show
an older Vulkan frame. The executable, ROM, saves, logs and screenshots remain
private. This settings change adds no ROM-derived images to public sources. The host
source-package inventory excludes images and private generated assets; final
archive verification remains a separate release gate.

The approved design uses original uppercase lettering, original palette colors
and translucent menu shadows over the current paused game. The native screen
therefore shows the actual mission/map beneath it; the design preview's title
background is not substituted into gameplay.

## Procedure and evidence

`tools/probe_gameplay.py --exercise --options --controls --controller FAMILY`
uses isolated copies of EEPROM/Pak, drives actual SDL input, captures the
presented image and records changed immutable settings snapshots. The optional
`BC_TRACE_PC_SETTINGS=1` diagnostics are off by default and emit only changes.
Snapshot assertions alone do not certify visual output.

The probe checks every Display (4), Graphics (3), Audio (2) and Controls (14)
row, capture and defaults dialogs at 960×720 and 1280×720, resizing and reopening.
It changes each field in both directions, drags volume to both bounds, forces a
real save failure using a disposable data path, remaps/cancels/restores bindings,
cycles the manual family fallback, checks held-input suppression, returns to the
original paused Game Options and exercises native movement/Start afterward.
A second process loads the saved preferences without command-line overrides and
activates Confirm with the persisted remapped key.

The native run initially found an original map instruction overlapping the
host menu. The correction suppresses only verified original action instructions
and their queued glyphs while PC settings is open. Original narrative and the
closed-menu path are retained. The failed capture remains part of the private
record; it is not accepted as a passing visual result.

## Completed native scope

The Xbox, PS5 and Switch virtual-controller runs each completed 240 seconds,
then a separate 12-second reload without settings overrides. All section/modal,
input/remap, save rollback, held-input, live-filter application and persistence
assertions passed. Source save/Pak hashes stayed unchanged. Actual compositor
images of all four sections, both modals and controller handovers were reviewed
at both extents against the approved original-style previews; first-open
captures have no original instruction overlapping the host menu.

Xbox used executable SHA-256
`c833b063c5dcd4403beb8e0377195c507ba3ad127acf94b1b4123597ef8bc546`.
PS5/Switch used
`a0c08f507f8720d14e204f209e2994c9a7abd3e25ae58fa7aba26fc5b85f5557`,
which additionally preserves Alt+Enter globally and removes ordinary action
prompts during binding capture. Final VI/GFX/audio counts were respectively
14,401/14,089/7,197; 14,402/14,092/7,199; and 14,401/14,088/7,199.
These audio counts are progress evidence only.

An earlier PS5 probe timed out while trying to reopen during the original pause
menu animation. Its counters continued advancing. The failed result is retained
privately; the passing repeat allows that transition to settle and still checks
held-close suppression and an actual reopen.

A later rendering-only change adds small white arrows to physical stick
bindings. A separate 120-second native image check used SHA-256
`a26945ca0ece1e76d7ca9cfafd64825f491c2790c857952a487c90da28052362`.
Its actual Switch Controls compositor image shows all eight LS/RS directions
clearly and correctly, with complete sections/modals/reopening still passing.
This follow-up uses manual family selection; actual virtual-controller behavior
is covered by the three earlier runs. The arrows additionally have draw-data
regressions across all three families, remaps and non-directional glyphs.

## Limits

Virtual SDL Xbox, PlayStation and Switch layouts exercise physical-position
mapping and matching names/glyphs. They do not validate actual controllers,
USB/Bluetooth behavior, a physical monitor, a speaker or headphone output.
These hardware checks remain in [manual testing](manual-testing.md).

Windows compilation, native presentation and controller testing for this
candidate remain pending. Linux acceptance does not certify a Windows archive
or imply that either platform's release archive has been published.

UI probes use SDL dummy audio. Audio counters in these runs only show progress;
separate Pulse/PCM evidence and its causality limits are described in
[audio diagnostics](../audio.md). No universal freeze or audible-output claim
follows from this settings acceptance.
