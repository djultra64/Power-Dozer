# Install Power Dozer v0.84

Download the Linux or Windows package and its SHA-256 file from
[the v0.84 release](https://github.com/djultra64/Power-Dozer/releases/tag/v0.84).

## Linux

Extract `power-dozer-v0.84-linux-x86_64.tar.gz` into a writable folder,
keeping the `lib` and `licenses` folders:

```bash
sha256sum -c power-dozer-v0.84-linux-x86_64.tar.gz.sha256
tar -xzf power-dozer-v0.84-linux-x86_64.tar.gz
cd power-dozer-v0.84-linux-x86_64
./'Run Power Dozer.sh'
```

## Windows

Use `power-dozer-v0.84-windows-x86_64.zip`
and its SHA-256 file. Verify
the hash with `Get-FileHash` in PowerShell, extract the archive into a writable
folder, and run `Run Power Dozer.cmd` or `power-dozer.exe`. Keep all DLLs,
`gamecontrollerdb.txt` and `licenses` beside the executable.

Requires Windows x64, SSSE3/SSE4.1, Vulkan drivers and the
[Microsoft Visual C++ 2015–2022 x64 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).
The Windows packaging recipe includes SDL2 2.26.3 and its pinned Windows DXC libraries.

```powershell
Get-FileHash .\power-dozer-v0.84-windows-x86_64.zip -Algorithm SHA256
Expand-Archive .\power-dozer-v0.84-windows-x86_64.zip -DestinationPath .
Set-Location .\power-dozer-v0.84-windows-x86_64
.\power-dozer.exe
```

## Optional generic N64 USB pad profile (Windows)

The [measured profile](controller-profiles/generic-n64-windows.txt) is for one
transparent-red, three-prong Chinese N64 USB pad. It is not an automatic rule:
`0079:0006` is shared by controllers with incompatible layouts. Confirm these
raw SDL inputs, counting from zero, before importing it:

| Physical control | Input |
| --- | --- |
| A / B / Start | buttons 5 / 4 / 9 |
| Z / L / R | buttons 8 / 6 / 7 |
| C up / right / down / left | buttons 0 / 1 / 2 / 3 |
| Stick X / Y | axes 0 / 1 |
| D-pad | hat 0 |

Close the game, back up `gamecontrollerdb.txt` beside `power-dozer.exe`, and
append the three lines from the optional profile. Keep the rest of the database.
Restart and check Controls: A/B map to Confirm/Back, L/R to shoulders, Z to the
left trigger, and C buttons to signed right-stick directions. Existing remaps
in `data/controls.json` still apply. Restore the database backup if another pad
starts behaving differently.

SDL reports 4 axes, 12 buttons and 1 hat. The same unit was observed as
`USB Joystick` on SDL2 2.32.10 (CRC `8630`) and SDL2 2.26.3 (`8e4f`), and as
`Microntek USB Joystick` on SDL3 3.4.18 (`79be`). These are Windows variants;
no Linux identity is inferred. CRC is not a serial number, and older SDL can
fall back across CRCs. No Select button was identified: SDL Back/Select is unassigned,
and Z remains a gameplay input. Open PC OPTIONS from Main or the pause menu.

The owner's real capture contains 18 assignments and 206 frames over 10 seconds.
Native SDL import and SDL2 2.32.10 virtual mapping checks covered the physical
buttons, C directions, hat diagonals, intermediate stick values and release.
There has been no physical Power Dozer playtest. The capture suggests a 2% radial
inner deadzone as a starting point for this unit, but mapping text cannot apply
it: Power Dozer currently uses a fixed 20% per-axis deadzone in its input model.
This profile does not change that behavior or promise a radial calibration.

## ROM and progress

The selector asks for your own Blast Corps **USA Rev 1** ROM, in
z64/v64/n64 format or a ZIP containing a single valid ROM. Normalized SHA-1:
`483f7161aea39de8b45c9fbc70a2c3883c4dea8c`. You can also specify it with:

```bash
./'Run Power Dozer.sh' --rom '/path/Blast Corps USA Rev 1.zip'
```

No ROM or progress is included. The installed ROM, EEPROM/Pak, preferences and
remaps are stored in `data` beside the executable. `--data-dir` lets you choose
another writable folder. To keep progress from an earlier installation,
close it and copy its data into a new or empty folder:

```bash
./'Run Power Dozer.sh' --data-dir '/path/new-data' \
  --import-data '/path/previous-installation/data' --rom '/path/Blast Corps USA Rev 1.zip'
```

On Windows, the equivalent command uses `power-dozer.exe`:

```powershell
.\power-dozer.exe --data-dir 'C:\Games\Power Dozer\new-data' `
  --import-data 'C:\Games\Previous Power Dozer\data'
```

The import preserves the source and does not merge or overwrite existing data.
The ROM is provided separately. Backing up data before
changing installations makes it easier to return to an earlier version.

Requires Linux x86-64, SSSE3/SSE4.1, glibc 2.38 or later,
`GLIBCXX_3.4.31`, zlib (`libz.so.1`), a Vulkan driver, an X11/XWayland desktop
and a desktop file selector. Compatibility with other distributions
requires checking the package on them.

Default keyboard controls: arrow keys to move, Z for primary action/confirm,
X for secondary action/back, Enter for Start, Shift to brake/exit,
I/J/K/L for camera and Q/E for horn. Alt+Enter toggles fullscreen.
The original title/start flow opens Main. **START GAME** opens the original
four-file selector and Erase Game; Files Back returns Main, while Back inside
name/erase dialogs retains its local behavior. **CLOSE GAME** uses the existing
window-exit path. Main Back leaves the screen open.
Open **PC OPTIONS** from Main or the original pause menu. The PC settings screen has
Display, Graphics, Audio and Controls tabs; Controls shows all fourteen actions
in two columns. Use the mouse, Tab/Shift+Tab and arrows, or controller shoulders
and stick/D-pad. Confirm/back follow the advertised logical bindings; Escape
always provides keyboard cancellation. Remapping keys used for navigation gives
the remapped Confirm/Back action priority.

Changes save immediately. Internal resolution, aspect, frame-rate target and
MSAA are marked for the next launch; volume/mute and filters apply live. Failed
saves restore the previous value and keep focus. Drag master volume to set
0–100. Restore Defaults asks for confirmation and affects only the selected
input device. Pregame Settings conceals the logo and returns through MAIN MENU
with Options focused; paused Game Options returns to the original game menu.
All drawing and mouse targets fit the startup aspect after resize, including
letterboxing; a pending Aspect preference does not alter the current image.

Prompts follow
the last active device, with Xbox, PlayStation or Switch family selectable
manually. On Switch, south/east/west/north positions are B/A/Y/X.

This is a beta. Two bounded audio recovery/protection changes have original-code
regressions and limited native evidence; the initiating event in the observed
soundplayer hang remains untraced. See [audio.md](audio.md) for exact scope and
[main menu/graphics acceptance](qa/main-menu-2026-10-11.md) and
[prior PC settings acceptance](qa/pc-settings-2026-10-11.md) for the UI checks.
Physical controller checks, sustained 4K/120 Hz and broad mission
coverage still require manual testing. `BUILD.json` identifies the commit and
each file's hashes; it keeps `ready_for_release` and
`license_review_complete` false. Notices are in `licenses`.
The source and its rebuild method are described in [build-source.md](build-source.md).
