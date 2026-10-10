# Power Dozer v0.82 — beta for testing

Open `Run Power Dozer.sh` on this machine (the earlier launcher also works). It stays open until you close
the window; it uses scale 3 (internal 720p), a 16:9 profile, stereo audio and an
interpolation target of 120. RT64 caps that target at the available refresh rate.
The beta still needs validation of the complete menu and mission flow.
Preferences are read from `settings.json` in the data folder;
arguments change the current run. Add `--save-settings` to retain changes.
Alt+Enter preserves the window mode. The PC menu provides graphics, audio,
help and per-action remapping.

`--data-dir PATH` allows an independent folder for progress and
remaps. Earlier 512-byte progress remains compatible; saving
writes a versioned image with a checksum, retaining a recoverable backup.
Development tests use isolated folders to protect the real save.

The crash when loading the map was fixed: the frontend uses `L3D_BC`, which the
local RT64 revision identifies but leaves as `Unknown`. A
specific adapter was added, with microcode fingerprint checks, for
its F3D commands and lines. Lines are converted to thin quads; their
rasterization and clipping still need comparison with an N64. Unknown
microcodes are rejected before interpreting the graphics list.

The isolated regression completed 110 seconds: frontend entry, map and
Simian Acres loading, with 6,601 VI, 6,350 graphics tasks and 3,299 audio tasks,
without crashing. Holding Alt+Enter did not send Start or repeat the request. This
test used llvmpipe and dummy audio; it does not verify playability of the whole mission.

| Key | Action / N64 button |
|---|---|
| Alt+Enter | Toggle fullscreen; does not activate Start. |
| Arrow keys | Walk, steer vehicles and navigate menus. |
| Enter | Start: start or pause according to the game. |
| Z | Accelerate / confirm, N64 A button. |
| X | Reverse vehicle / back, N64 B button. |
| Left Shift | Stop / exit vehicle, N64 Z trigger. |
| I / J / K / L | C up / left / down / right. |
| Q / E | N64 L / R buttons. |

Controllers connect through SDL. Help and adapted instructions follow
the last active device: keyboard, Xbox, PlayStation or Switch, with names
and icons. Switch retains its actual positions (south=B, east=A, west=Y, north=X).
`--family xbox|playstation|switch` allows manual selection; the PC menu offers
per-action remapping and saves `controls.json` in the data folder. It was verified
with virtual SDL controllers; acceptance with physical controller buttons and all
tutorials/vehicles remains pending.

Interpolation generates extra frames between those of the original game.
Its correctness in cameras, vehicles, particles and demolition still needs verification.
Requesting 120 does not establish sustained 120 fps; the effective target appears
in the `rt64_metrics` log. Fullscreen does not change the desktop's
refresh mode.

The available GPU is AMD Ryzen Z1 Extreme (RADV PHOENIX). On the main
screen configured at 60 Hz, an effective target of 60 and original rate of
30 were observed in 3D scenes. This is evidence of configuration and frame generation,
not a measurement of sustained 120 fps.

```bash
./'Run Power Dozer.sh'
./'Run Power Dozer.sh' --scale 9 # 4K profile, more demanding.
./'Run Power Dozer.sh' --refresh 0 # Compare with original presentation.
./'Run Power Dozer.sh' --scale 9 --msaa 4 --save-settings
./'Run Power Dozer.sh' --volume 50 --mute # Mutes output; the RSP stays active.
```

Keyboard and Xbox/PlayStation/Switch glyphs use crops from the
supplied atlas, with transparency and size proportional to the HUD. Keys
without a specific drawing retain their name beside a blank keycap.

The reported thin black ground line is not currently reproducible: the reporter
and recent local captures no longer show it. No speculative patch was applied. The public release includes Linux and Windows
runtime archives; see [install.md](install.md) for their requirements.
