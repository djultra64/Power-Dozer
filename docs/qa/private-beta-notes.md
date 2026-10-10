# Local Blast Corps beta for Linux

Later update: the specific HUD, ticker, map sprite, sky and fade defects reported in the videos reopen that graphics scope; the earlier terrain acceptance remains valid. The development executable already includes the corrections and an audio dispatch guard. See [the follow-up and its limits](graphics-audio-followup-2026-10-10.md). The packages cited below retain the earlier checkpoint.

Checkpoint `99bb524`, October 10, 2026. The current graphics are
accepted by the user. **Audio processing may stop intermittently while
the picture continues.** Subsequent three- and six-minute sessions passed
with the same executable; the earlier failure remains open.

## Files

Available in the project's `.local/packages/`:

| File | SHA-256 |
| --- | --- |
| `blastcorps-private-linux-x86_64.tar.gz` | `0b6824931902aafcdb679540c7bc0b87b57275cf069a1308654f40b366dbdfa2` |
| `blastcorps-private-source-review.tar.gz` | `b06a3394b05ee00eb076b58fb20042af54c402dc9fc954815984559b6bc398d0` |

Each archive has its own `.sha256`. The runtime includes libraries and 77 license
texts; the source includes scripts, patches, configuration and provenance.
They include no ROM, private progress or captures. The independent SDK rebuild
has been verified; its recipe is in [source-rebuild.md](source-rebuild.md).
The source snapshot is not certified as corresponding source, and the full
license review remains pending. These are private beta artifacts.

## Launching and importing data

Requires Linux x86-64 with SSSE3/SSE4.1, glibc 2.38, `GLIBCXX_3.4.31`, zlib,
a Vulkan driver and an X11/XWayland desktop. Extract into a writable folder:

```bash
sha256sum -c blastcorps-private-linux-x86_64.tar.gz.sha256
tar -xzf blastcorps-private-linux-x86_64.tar.gz
cd blastcorps-private-linux-x86_64
./'Run Blast Corps.sh'
# You can also specify the ROM directly:
./'Run Blast Corps.sh' --rom '/path/Blast Corps.zip'
```

The picker requests your own **USA Rev 1** ROM: z64/v64/n64 or a ZIP containing
one valid ROM. Normalized SHA-1: `483f7161aea39de8b45c9fbc70a2c3883c4dea8c`.
The session continues until you close the window. The installed ROM, saves, Pak
and preferences remain in `data` next to the executable; `--data-dir` selects another folder.

Close the previous installation and copy its progress into a new or empty
folder, preserving the source:

```bash
./'Run Blast Corps.sh' --data-dir '/path/new-data' \
  --import-data '/path/previous-installation/data' --rom '/path/Blast Corps.zip'
```

Import copies saves, Pak, preferences and remappings. The ROM is imported
separately; existing destination data is neither merged nor overwritten.

## Controls

Default keyboard: arrow keys to move/steer, **Z** for the primary action and
confirm, **X** for the secondary action and back, **Enter** for pause/Start,
**Shift** to brake/exit, **I/J/K/L** for the camera and **Q/E** for the horn.
**Alt+Enter** toggles fullscreen.

On a controller, the right trigger accelerates/confirms and the left trigger
brakes/exits. Prompts follow the action and the last active device, with Xbox,
PlayStation or Switch names/glyphs; Switch south/east/west/north positions
are **B/A/Y/X**. The PC menu supports remapping by action and manual family
selection when detection is ambiguous (`--family xbox|playstation|switch`).
Physical controller, real display/audio, sustained 120 fps and broad mission
coverage checks are listed in [manual-testing.md](manual-testing.md).
