# Main menu before file selection

Project attribution: djultra64.

## Intent

Add a main menu immediately before the original file-selection screen. Reuse
that screen's actual animated background and original bottom scrolling band
(ticker), original font, colors and shadows. Replace the four file entries and
Erase Game on this new screen with exactly three entries: Start Game, PC Options
and Close Game. The original file selector and erase flow remain behind Start
Game. This feature does not replace the title screen or attract mode.

## Screens and navigation

After confirming the original title/start flow, show the main menu where file
selection currently appears. Start Game is initially selected. Draw three
clearly separated, centered vertical entries, with the original game's focused
color and shadow style. The heading is POWER DOZER, using the original font.
Keep the actual backdrop and ticker running; do not substitute a screenshot.
Do not show file names, file-row medals or Erase Game on the main menu. Keep
icons belonging to the ticker itself.

Start Game opens the original four-file selector and original Erase Game entry.
Selecting a file, entering a name, cancelling an erase confirmation and loading
existing progress continue through the original game logic. Back from the file
selector returns to the main menu. Back inside erase/name dialogs retains its
original meaning; it must not unexpectedly jump to the main menu.

PC Options opens the complete existing Display, Graphics, Audio and Controls
screens. In this entry context, replace the pause-only Game Options control with
MAIN MENU. Back returns to the main menu with PC Options still selected. The
pause entry inside gameplay retains its original Game Options control and
return behavior. Settings persistence, failures, remapping and manual controller
family selection retain their existing behavior.

Close Game requests the same exit path as closing the game window. Back on the
main menu does not close the game or trigger another hidden screen. No additional
exit-confirmation dialog is added. Closing while settings save synchronously
must follow the existing main-loop ordering rather than terminating inside an
input callback.

## Input and appearance

Support mouse selection/click, keyboard navigation and mapped logical controller
actions. Prompts follow the last active input device and the selected physical
binding. Xbox, PlayStation and Switch names/glyphs must agree with the actual
button positions; retain optional action remaps and manual family fallback.
Retain the centered white directional arrows inside stick icons.

Consume held inputs across main-to-files, main-to-options, options-to-main and
files-to-main transitions. A held Confirm must not select a file immediately
when Start Game opens the selector. Preserve fixed Escape recovery inside PC
Options. Resizing/fullscreen and 4:3/16:9 must preserve legibility and reachability.

## Implementation boundaries

Reuse the existing host-owned menu renderer and ROM-decoded font/palette/shadow
assets. Add explicit MainMenu/Pause entry and return context for PC settings;
do not manufacture a guest pause or infer context from a hidden selected file.
Guest frontend setup/save loading remains intact. Entry and return hooks are
fingerprinted to the original USA Rev 1 function/instruction sites. Suppress
only file content rendering while the host owns the new screen, and prevent
hidden file-selection changes from changing the ticker's selected player.
Keep original backdrop and ticker producers running.

The guest publishes frontend ownership/transition information; SDL owns host
input and exit requests; rendering reads immutable snapshots. Do not let the
render hook mutate guest RAM or access live mutable input state. Implement in
normal project sources and verified generation hooks, not by committing private
generated game functions or extracted ROM assets.

Native integration adjustment (2026-10-11): the retained backdrop includes a
baked Blast Corps logo. Main keeps it, with POWER DOZER below its lower edge;
shared reference row tops are 195, 251 and 307, and the Select footer is at 357.
The actual ticker starts around reference Y388, so all host content clears it.
The user's subsequent explicit request removes the visible logo in pregame
PC Settings: that origin uses the original shadow mesh with an opaque center
and narrow faded edges covering the logo. Its canonical vertical layout is
80% of the pause layout while font and icon sizes stay unchanged. All settings,
fourteen action bindings, modal targets and footer remain available above the
ticker, with shared mouse/render coordinates. Pause retains its existing layout
and translucent shadow. All host screens fit the actual letterboxed startup
aspect using immutable snapshot metadata and shared mouse/render safe areas;
pending Aspect changes take effect only at next launch. The original backdrop
and ticker continue running.

The existing window-exit path ends the process without a complete coordinated
join of every guest/render worker. Close Game shares that established path;
this menu does not claim a new graceful-shutdown guarantee. A full lifecycle
rewrite is outside this feature.

## Related bug acceptance

The reported medal clipping is a separate repair: medals/rank icons must remain
visible across the same full-width ticker region as its text in 16:9, with proper
edge clipping and unchanged 4:3 behavior. Hiding file content on the main menu
does not count as fixing medal behavior after Start Game.

The vehicle-attached black line is also a separate repair. Diagnose and verify
the initialized shadow texture extent and fullscreen/window transitions without
removing the vehicle shadow or changing its silhouette clipping. Current
original-function/GPU evidence is recorded privately; the new main menu must
not obscure or waive this graphics acceptance.

Texture smoothing already exists as Graphics > Texture Filter > LINEAR. Retain
that working option and its separate relationship to anti-aliasing and screen
filtering; this menu feature does not add an unsupported anisotropic setting.

## Verification

Check screen/context transitions, held-input suppression, remapped actions,
selected-family prompts, exactly-once exit routing and untouched file/erase/name
flows with focused CPU/SDL tests. Check exact generated hook occurrences and
unchanged unrelated paths. Run the existing required suites after integration.

Capture main menu, original file selector and pre-game PC Options with real
Vulkan presentation at 4:3/16:9, including resize and fullscreen. Confirm the
same animated background/ticker, exactly three main-menu entries, complete
settings sections, correct return behavior and original typography/shadows.
Exercise Xbox/PlayStation/Switch virtual controllers and state hardware limits
honestly; verify real controllers when available. Keep ROMs, generated guest
code, saves, captures and GPU recordings private. Before any eventual merge,
push or release, verify project author/committer identity is djultra64 and audit
runtime/source packages and native acceptance independently.
