# Feasibility test results

## Evidence obtained

A Linux executable was built that enters at `0x802447C0` and runs the game's
original threads. 1,464 CPU function bodies were generated; the analyzed map
contains 1,594 entries, including services replaced by the
runtime. The main and frontend overlays are extracted from the validated ROM.
IPL3 startup and initial decompression are reproduced by the host;
the game body runs as recompiled code.

| Objective | Result and limitation |
|---|---|
| Native CPU code | Builds and runs intros, title, demo, map and Simian Acres loading. The complete mission is not validated. |
| Graphics | Original F3D processed by RT64/Vulkan; actual captures in `evidence/`. |
| 4K | 3840×2160 render targets were measured with scale 9 and 16:9 aspect. |
| 16:9 | Configuration applied and widescreen window; cameras, culling, HUD and all menus still need checking. |
| Interpolation | RT64 records generation/presentation of extra frames; visual correctness for vehicles and destruction still needs evaluation. |
| 120 fps | Target requested, capped by RT64 at detected 60–75 Hz. Additional test with AMD Ryzen Z1 Extreme on a 60 Hz desktop; sustained 120 fps was not measured. |
| Audio | Continuous progress through 90 s of intros, title and demo; also 35 s at 4K. Actual output through PipeWire/PulseAudio for 25 s. RSP yielding and stereo channels fixed. Perceptual comparison with original hardware and mission QA remain pending. |
| Input | Keyboard and per-action/family SDL adapter prepared; real-controller QA and replacement of in-game prompts remain pending. |
| Saving | Local 512-byte EEPROM adapter; complete persistence, slots and Controller Pak were not verified. |

## Adaptations made

1. ROM validation and extraction of compressed overlays; frontend mailboxes
   and NTSC startup information prepared in RAM.
2. Separation of tables embedded in `.text`, correction of two shared
   function boundaries and aliasing of an identical epilogue checked against the ROM.
3. Replacement of hardware services with native calls. Remaining
   services have explicit diagnostic exits instead of silently
   continuing with fabricated values.
4. Support for a second `osStartThread` on a thread already started.
5. Cooperative wait points in a controller query and two VI waits.
6. Two-buffer audio queue that reports the active buffer's length,
   avoiding treating the whole SDL queue as the AI register's length.
7. RDP pause and metadata for tasks presenting a frame. The
   DP interrupt requires actual synchronization and the final task.
8. SP completion notification after RT64 consumes the graphics list.
   Early notification caused a crash in `RDP::loadTileOperation`;
   changing the order allowed the graphics tests to complete. That
   order is retained in subsequent audio tests.
9. Virtual RSP yielding: a suspended graphics list retains its data
   while audio runs; on resume it is not resubmitted to RT64.
   Pending completion is reported when returning to that task. This avoids
   accumulating audio notifications during a slow list and blocking its thread.
10. Conversion of PCM from word-swapped RAM to the host's left/right
    stereo order, WAV capture and SDL queue metrics.
11. Controller Pak CRC routines are pure computation and are recompiled,
    avoiding the diagnostic exit observed in a transition to the frontend.
12. Frontend L3D adapter, selected by address and XXH3
    text/data fingerprints. It retains F3D commands and converts line primitives
    into quads. The pinned RT64 revision returns a non-null `Unknown` GBI
    for this microcode: previously EndDL was ignored and reads went outside
    RAM. The GBI type is also checked before processing each task.
13. Alt+Enter toggles desktop fullscreen and suppresses Start until
    Enter is released; the beta launcher keeps the process open.

Runtime modifications are generated inside `build/`; Tetrisphere
sources and its dependencies are not edited.

## Work remaining before a complete port

- Compare music, effects and stereo with original hardware; test mission
  audio, pauses, transitions and output-device reconnection.
- Navigate the menu and load/play missions, including demolition, transitions,
  vehicles and saving. The automatic demo does not establish playability.
- Compare L3D adapter lines with the original, including clipping,
  depth and thickness. Their rasterization is still an approximation.
- Separate original simulation from presentation and check interpolated movement,
  particles, shadows and destruction on a 120 Hz display.
- Adapt HUD, cameras and controller prompts without stretching 2D assets or confusing
  Nintendo positions with Xbox positions.
- Replace local static dependencies with a portable build and package
  without the ROM or files generated from it.

Startup and native rendering feasibility is demonstrated. The main
remaining cost lies in scheduler/microcode compatibility and
improvements specific to the game's visuals, rather than obtaining a complete
decompilation before starting.

## Verification

The reproduction script completed extraction, RSPRecomp, N64Recomp and linking.
Five Python tests passed (two for the ROM/overlay contract and three for the
progress checker), along with four native tests: AI queue, RSP yielding/resuming,
PCM channel order and L3D line geometry. The progress checker
detects the hang in the earlier log and passes in the corrected runs.
Initial graphics tests and the isolated regression use llvmpipe;
a desktop run with the AMD GPU was added. No benchmark was performed.
In addition to the dummy backend, sound delivery through PipeWire/PulseAudio
to the local output device was checked. Details and limitations in [audio.md](audio.md).

The beta regression completed 110 seconds with 6,601 VI, 6,350 graphics
tasks and 3,299 audio tasks, entering the map and loading Simian Acres.
It also checked that holding Alt+Enter, releasing Alt before Enter,
does not send Start or repeat the fullscreen change. The L3D adapter uses the
command structure described by the repository's SDK and the
[GLideN64 L3D implementation](https://github.com/gonetz/GLideN64/blob/master/src/uCodes/L3D.cpp).

Full logs remain in `.local/evidence/`. The public evidence
summary is in `evidence/summary.json`; it contains no ROM, extracted code or
recompiled binaries. Captures show the executable's output.
