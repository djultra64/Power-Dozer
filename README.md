# Power Dozer

**v0.81 — beta of a native Blast Corps port for Linux and Windows x86-64.**
Static recompilation of Blast Corps USA Rev 1 with N64Recomp,
N64ModernRuntime and RT64. The project is not affiliated with Rare or Nintendo.

## Download and play

Download your platform archive from
[Releases](https://github.com/djultra64/Power-Dozer/releases), check its
SHA-256 and extract it into a writable folder:

- Linux: `power-dozer-v0.81-linux-x86_64.tar.gz`; run `Run Power Dozer.sh`.
- Windows: `power-dozer-v0.81-windows-x86_64.zip`; run `Run Power Dozer.cmd`.

On first launch, select your own **Blast Corps USA Rev 1** ROM.
z64/v64/n64 and ZIP files containing a single valid ROM are supported. The ROM is not included.

Requires Linux x86-64 with SSSE3/SSE4.1, Vulkan, X11 or XWayland,
glibc 2.38 or later, a C++ runtime with GLIBCXX_3.4.31 and zlib.
Windows requires x64, SSSE3/SSE4.1, Vulkan drivers and the
[Microsoft Visual C++ 2015–2022 x64 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).
SDL and DXC libraries are included in both runtime packages.
[Installation, progress and requirements](docs/install.md).

## Options and controls

- Resolutions up to 4K, 16:9 presentation and an interpolation target of 120.
  Performance depends on the GPU and available refresh rate.
- **Alt+Enter** toggles fullscreen.
- Integrated PC menu with graphics, audio, help and per-action remapping.
- Xbox, PlayStation, Switch and keyboard: names and glyphs follow the last
  active device. Switch retains B/A/Y/X according to their physical positions.
- Full-width menu background and ticker; aligned map vehicle sprites
  and glyphs extracted from the supplied atlas.
- Persistent EEPROM and Controller Pak progress, with recoverable backups.

| Default key | Action |
| --- | --- |
| Arrow keys | Move, steer and navigate |
| Z | Accelerate / confirm |
| X | Reverse / back |
| Enter | Start / pause |
| Left Shift | Stop / exit vehicle |
| I / J / K / L | Camera / C buttons |
| Q / E | L / R buttons |

You can remap each action in the PC menu. The manual selection
`--family xbox|playstation|switch` provides a fallback for ambiguous detection.

## Beta status

Startup, file selection, map, Simian Acres, movement,
pause, resume, restart, progress and input with virtual SDL controllers were verified.
The latest graphics test confirmed Switch and keyboard glyphs in the game.
Native Windows startup, Unicode saves/import and a brief interactive Vulkan
run with keyboard input also passed. See [v0.81 verification](https://github.com/djultra64/Power-Dozer/blob/main/docs/qa/release-v0.81.md)
for the observed scope and limitations.

**There is an intermittent audio issue:** recovery for pending tasks was added,
but it has not yet been shown to eliminate all dropouts.
Full level coverage, physical controllers and sustained 120 fps
remain for manual testing. End-of-level fades/sky and the
visual stability of the HUD arrow need further checking.
v0.81 is a prerelease for testing and reports.
The reported thin black ground line is not currently reproducible; the reporter
and recent local captures no longer show it. No speculative patch was applied.

## Build and contribute

[Portable recipe and pinned dependencies](docs/build-source.md).
The release includes a source archive for the host and its dependencies.
Game code is generated locally from your ROM; packages do not
include a ROM, extracted overlays, saves or private recordings.

Project code uses GPL v3 or later; dependencies and control artwork
retain their provenance and notices. See [LICENSE](LICENSE), [NOTICE](NOTICE)
and `config/control-atlas.json`. The manifests document the limits of the
source and license review.

The function map builds on the work of
[retroplastic](https://github.com/retroplastic/blastcorps) and its continuation by
[SlaveOfIDO](https://github.com/SlaveOfIDO/blastcorps).

## Credits

See [CREDITS.md](CREDITS.md) for the projects, contributors and dependencies
used by Power Dozer, and the accompanying license notices.
