# Graphics and audio follow-up — October 10, 2026

**Status:** integrated 90 s run completed with exit0: **5401 VI / 2699 audio tasks**, in `.local/qa/gameplay-abhmxopg`. **45/45 CTest** (`user-video-20261010/ctest-final.log`) and **124/124 Python** (`widescreen-python-suite.log`) pass, under `.local/qa/`. Visual and audible validation limits remain; no version is certified for distribution.

| Request | Evidence and current scope | Pending check |
| --- | --- | --- |
| Vertical RDUs | Original `hd_code/2B3F0.c`, `802701A8`: billboards rotated around Y. Vertices `802F99C0`, heights 0/12 and diagonal ±6: vertical orientation is intentional. | Compare position, texture and orientation; do not treat vertical orientation as a defect. |
| Original menu background | `attract2`, USA Rev1 ROM `0x6AD3F0–0x6BF2F0`, RGB5551 **320×240**. The 153600 bytes exactly match running the original decoder on CPU. Four tests pass. | Verified extraction, without generation or cropping captures. |
| HUD arrow | Only the ten-vertex F3D mesh `0x002FD9B8`; IDs `0xBC000001/0xBC000002`, without interpolation and with aspect adjustment. | `arrow-idle-*` shows the tutorial rather than the HUD arrow; it does not validate stability/placement. |
| Ticker, sky and white screen in 16:9 | Helpers expand ticker/FIFO, sky geometry/UV and white coverage. `completed-level-replay.png` actually shows statistics, with a full-width ticker. | `player-select-ticker-*` shows the title rather than player selection. `pak-player-selection-compositor.png` does confirm the full ticker in player selection. Sky/white screen and aspect changes remain pending. |
| Line on map vehicles | `801F9258 → 80272ED8`: two RGBA16 64×32 strips; asset without a line. Scoped reflected T-domain change 64→32. **`map.png` shows the bulldozer without the stripe** on GPU, scale 4.5. | This capture confirms that case; it does not cover all vehicles/scales. |
| Pending audio without active RSP | Guard requests `0x29E` with a real task prepared. Independent review and original fixture: dispatch, RSP NOP and **one original ACK only**; healthy control unchanged. The native trial retained activity through the end. | There were **0 `audio_dispatch_request`** events: it did not exercise intermittent recovery. Backlog origin remains unresolved; audible output unverified. |

PNG: `.local/evidence/assets/blast-corps-main-menu-background.png`; [provenance](</var/home/alfredo/Documents/ChatGPT/Blast Corps/.local/evidence/assets/blast-corps-main-menu-background.json>). ROM assets remain private and **are excluded from source code and packages**. Extractor/tests contain no ROM content.

PCM `audio.wav`: stereo 22050 Hz/16 bits, **88.345 s**. All 18 windows of up to 5 s contain signal; maximum exact silence is 0.627 s at the start, with none at the end. Final segment: RMS −16.8 dBFS. Log: 0 rejected buffers and 2613 empty-queue observations; these do not prove audible dropouts or uninterrupted output. Private detail: `audio-signal-summary.json`, alongside the WAV.

Implementation: [arrow](</var/home/alfredo/Documents/ChatGPT/Blast Corps/include/carrier_arrow.h>), [16:9](</var/home/alfredo/Documents/ChatGPT/Blast Corps/include/widescreen_guest.h>), [map](</var/home/alfredo/Documents/ChatGPT/Blast Corps/include/map_vehicle_sprite.h>), [audio](</var/home/alfredo/Documents/ChatGPT/Blast Corps/src/audio_dispatch.cpp>). CPU fixture: `.local/qa/stability-launch/audio-dispatch-green-result.json`.

**SHA256 of the tested binary:** `c89f5edcf9484f27acc697c8095b6b5a4052893d2f929e8f82671aadf9de9fb5`.

Requested background stretched to 16:9: `.local/evidence/assets/blast-corps-main-menu-background-16x9.png`, **1920×1080**, nonuniform resize of the original extraction, without cropping or added content. Preview generated separately; the delivered file preserves the original texels using nearest-neighbor sampling. Not installed as a replacement.

Two additional private 60 s attempts are not accepted as regressions: `gameplay-o75qz565` timed out during the capture sequence and lost the window; `gameplay-imriqpop` finished at 3601 VI / 1799 audio tasks, but its screencopy was black and the harness did not confirm pause. They do not prove the arrow or invalidate the integrated 90 s trial. Further automatic tests are shortened; targeted visual confirmation remains pending.

New beta snapshot from commit `450d67b`: runtime `.local/packages/blastcorps-private-linux-x86_64-450d67b.tar.gz` (SHA256 `f82958eaafdf1a39a72d727e318a8a413962e14acb6401d8998b180a5765e94b`) and source `.local/packages/blastcorps-private-source-review-450d67b.tar.gz` (SHA256 `55cf09202bb8b07b01450a650e92ac1fc418c5ba35e8e04105dabf1b7afc084a`). Inventories and `.sha256` files were checked after packaging, without ROM, PNG or WAV. This new snapshot has no additional GPU acceptance session or independent rebuild; both manifests still do not certify release. The development launcher opens the updated, already tested binary.


In-game background update: the earlier stretched PNG was a separate file. The original producer `80200BE0` still uses 75 rectangles of 64×16, too narrow for RT64's automatic background detection. Its two paths (copy/blended) now emit a D4 marker, restricted to that producer by ROM instruction checks; the F3D handler requests `G_EX_ASPECT_STRETCH` only during those calls and restores state. Original texels, logo and text are preserved; in 4:3 the handler retains original behavior.

**Change validation:** independent review without findings; 45/45 CTest and 125/125 Python; actual compositor capture `gameplay-nv8pir_u/pak-player-selection-compositor.png` confirms the background fills edge to edge, with proportionate logo/text. Menu session of 45 s, exit0, 2701 VI / 1349 audio tasks and unchanged source saves. Updated binary: SHA256 `1513826776d2229e190fbc9681f3fd877944c1c355a5148dd4f44488d9a20214`. Launcher `Run Blast Corps Beta.sh` opens this binary; the 450d67b snapshot archives retain their earlier version. The menu session does not replace the pending gameplay/audio checks described above.


Glyph update for Power Dozer v0.8: generic strokes replaced
with 169 RGBA32×32 crops from the two supplied atlases, covering Xbox/PlayStation/
Switch/Generic and the full keyboard. The host expands a D5 descriptor per icon
into triangles without touching TMEM; it combines SHADE and alpha and restores graphics state.
RT64 combiner word order was corrected (w1 over w0). The giant diagnostic marker
in file selection was removed; normal icons measure 13–16 pixels and follow
HUD size. Literal names are retained for keys without a specific illustration.

Validation: 45/45 CTest and 127/127 Python. `gameplay-dug66oia`, private Radeon/compositor
1920×1080, 60 s, exit0, retained audio through the end (1798 PCM buffers,
0 rejections). Captures `switch-active-help-next-compositor.png` and
`atlas-keyboard-help-compositor.png` were inspected: they show B/A/+ and Z/X/Enter,
respectively, aligned with their actions. They do not certify physical controllers
or recovery from the intermittent audio failure. The early trials had valid CPU
commands but invisible sprites and do not count as acceptance.
