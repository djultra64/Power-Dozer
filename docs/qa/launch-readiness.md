# Private Linux release preparation

Historical checkpoint: the inventory and missing-license statements below describe
the private beta at that time. Public v0.81 includes the root `LICENSE`, `NOTICE`
and `CREDITS.md`; see the current release documentation for platform status.
Commit IDs in historical QA records predate the project author-identity
normalization; their source trees were preserved.

Later update: the specific HUD, ticker, map sprite, sky and fade defects reported in the videos reopen that graphics scope; the earlier terrain acceptance remains valid. The development executable already includes the corrections and an audio dispatch guard. See [the follow-up and its limits](graphics-audio-followup-2026-10-10.md). The packages cited below retain the earlier checkpoint.

Review of October 10, 2026. This document prepares a reviewable private beta;
it does not certify a candidate or authorize publication. The current scope
delivers a local beta with the current graphics accepted by the user and the
audio limitation stated below. Sustained 120 fps and broad mission coverage
are recorded in [manual-testing.md](manual-testing.md).

## Local beta artifacts

Runtime `.local/packages/blastcorps-private-linux-x86_64.tar.gz` and source
`.local/packages/blastcorps-private-source-review.tar.gz` were generated from
clean commit `99bb524bd702fa8a54350ff7a2aa18afd6b637a3`:

```text
Runtime SHA-256 0b6824931902aafcdb679540c7bc0b87b57275cf069a1308654f40b366dbdfa2
Installed ELF 7fb9d803ddb1bd3cea11766ca7f705bfb51390e6440deaeb1bc6311b95349568
91 members; 90 files in the inventory; 77 license texts
Source SHA-256 b06a3394b05ee00eb076b58fb20042af54c402dc9fc954815984559b6bc398d0
10,309 verified source files
ready_for_release=false; full license review pending
```

All hashes were verified; the runtime was extracted into a new path containing
spaces outside the checkout. They contain no ROM, private progress or captures.
Dependencies inspected with `ldd` resolve and the ELF uses `$ORIGIN/lib`.
Evidence is in `.local/qa/package-checkpoint-current.json`.

The same installed ELF passed 180- and 360-second sessions with movement,
pause and Restart (`gameplay-ghunmacy` and `gameplay-jck7jpdz`). Without code
changes, an earlier session (`gameplay-j8o4si3t`) stopped processing audio
while graphics advanced. **The intermittent audio stall remains unresolved**;
later sessions do not demonstrate its elimination. This is delivered as a
local beta with that known limitation. Brief instructions are in
[private-beta-notes.md](private-beta-notes.md).

The archives retain checkpoint 99bb524; these later notes are not inside
the snapshot. They are not regenerated and the binary is not changed for this delivery.

## Verifiable requirements of this binary

Linux x86-64 with SSSE3 and SSE4.1, a working Vulkan driver, an X11/Wayland
desktop and a writable data folder. Static inspection of the current package
ELF requires **glibc 2.38** symbols and **GLIBCXX_3.4.31** in `libstdc++.so.6`.
This is an observed symbol requirement, not certification of distributions
or every compatible CPU. A later build must repeat this inspection.

Included: `libSDL2-2.0.so.0` (sdl2-compat 2.32.70), `libSDL3.so.0`
(3.4.12), `libdbus-1.so.3` (1.16.2, bottle 1.16.2_1), `libdxcompiler.so`
and `libdxil.so`. The installed executable uses `$ORIGIN/lib`; relocated
libraries use `$ORIGIN` when they had RPATH. System dependencies remain:
libc, libm, libgcc, libstdc++, the ELF loader and **libz.so.1** for DXC.
Display/audio and Vulkan backends may load other libraries at runtime;
`DT_NEEDED` is not their complete inventory.
Windows and other distributions do not yet have native acceptance.

## Installation and beta tester notes

1. Check the `.sha256` and extract the archive into a writable folder.
2. Run `./'Run Blast Corps.sh'` from the extracted folder. The picker requests
   your own unmodified Blast Corps USA Rev 1 ROM. You can also use
   `./'Run Blast Corps.sh' --rom '/path/Blast Corps.zip'`.
3. z64/v64/n64 or a ZIP with one valid ROM are accepted. The normalized image
   must have SHA-1 `483f7161aea39de8b45c9fbc70a2c3883c4dea8c`.
4. The validated ROM remains in `data/cache/rom.z64`; overlays are regenerated
   in memory. EEPROM, Pak, preferences and remappings live in `data` next to
   the executable. `--data-dir PATH` selects another folder.
5. To preserve a previous installation, close the game and use
   `--import-data '/path/previous-installation/data'` into a new or empty folder.
   Import copies progress/preferences; it does not merge an existing
   installation. The source remains intact. Import the ROM separately.
   Retain the previous installation until the new one is verified.

The beta offers PC options in the original style, fullscreen toggling with
Alt+Enter, remapping by action, help and instructions following the last active
device, and manual family selection when detection is ambiguous. Automated
evidence covers Simian Acres, pause, Continue, Restart, failure, objective
completion and progress reload, as well as virtual SDL controllers.
These results do not cover every mission or physical controllers.

The interpolation target of 120 was measured at 720p/1080p on virtual 120 Hz
output. Sustained 4K measurement remained around 116.5 fps. Physical display,
audible continuity, broad coverage and sustained performance remain pending
in the manual checklist. The user accepts the current graphics for this local
beta. The external-producer stall already has a correction and native
regressions; intermittent audio and source/license review remain open.
Their status is in [readiness.md](readiness.md).

## Notice and provenance inventory

Licenses are established from each source's inspected text, not from missing
metadata. Packages found in the recursive scan may contain tools, examples
or Windows components that are not linked on Linux; the notice count does not
demonstrate a complete review.

| Actual component or category | Inspected primary evidence | Status and next step |
| --- | --- | --- |
| Statically linked N64ModernRuntime and compiled librecomp files | [Pinned COPYING](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/COPYING), GPL v3; this port's `CMakeLists.txt` links `ultramodern` and compiles librecomp | Text included. Prepare corresponding source and change notices before distributing the executable. |
| N64Recomp / RSPRecomp | [Pinned LICENSE](https://github.com/N64Recomp/N64Recomp/blob/ffb39cdad1da5de07eaaa48bd1db4a89a7986771/LICENSE), MIT | Notice included. Distinguish code generation tools from incorporated code/headers. |
| RT64 | [Pinned LICENSE](https://github.com/rt64/rt64/blob/43373749dac9bbc1b653e6a02aed40a9e1783bed/LICENSE), MIT | Notice included. RT64's license does not replace those of its contributions. |
| re-spirv, nativefiledialog, zstd, plume, ImGui, im3d, implot, hlslpp, ddspp, xxHash, stb and runtime miniz | Sources pinned by the preceding trees/submodules and static libraries enumerated by CMake | Notices named LICENSE/COPYING already included; each used library/header/shader must be linked to its texts and revisions. |
| Redistributed DXC and DXIL | [Pinned dxc-bin](https://github.com/rt64/dxc-bin/tree/cc15e715ee378a4f675b335bd1071ff105873fc8), local version `1.8(dev;4461-11e1318c)`; [exact LICENSE.TXT](https://github.com/microsoft/DirectXShaderCompiler/blob/11e1318c3ed77aa08326e16eaf52598ad6430546/LICENSE.TXT) and [exact ThirdPartyNotices](https://github.com/microsoft/DirectXShaderCompiler/blob/11e1318c3ed77aa08326e16eaf52598ad6430546/ThirdPartyNotices.txt) | Both texts are added with SHA/bytes and verified binary inputs in `licenses/dxc`. The version identifies the compiler; DXIL accompanies the same binary revision, without independent build attestation. |
| JSON 3.12.0 incorporated by runtime/RT64 | Exact headers with SPDX; [official pinned MIT](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/LICENSE.MIT) | MIT text and SPDX attribution for Niels Lohmann, Hedley, Abseil, Hoehrmann and Loitsch are added, with hashes for both headers. |
| ConcurrentQueue / BlockingConcurrentQueue and semaphore | BSD and zlib notices inside the three headers of pinned N64ModernRuntime | Exact notices for Cameron Desrochers and Jeff Preshing are added in `licenses/embedded`, without choosing an absent alternative license. |
| miniz compiled inside RT64 | `src/contrib/miniz/miniz.c` enumerated in RT64 CMake | The three distinct notice blocks are added, extracted without editing the MIT/Unlicense texts. The runtime miniz license does not replace these notices. |
| Vulkan-Headers / SPIRV-Cross | Their `LICENSES/*.txt`, previously omitted by the name filter | The scan now includes Apache/MIT/other texts inside LICENSES and NOTICE/ThirdPartyNotices variants. Attribution from the pinned tree is preserved. |
| SDL2-compat and SDL3 | Installed Zlib texts, Homebrew SBOM/receipts and preceding versions | Texts included. Retain source URLs/checksums and bottle origins in the final inventory; they are incomplete in BUILD.json. |
| DBus 1.16.2 | [Official source archive](https://dbus.freedesktop.org/releases/dbus/dbus-1.16.2.tar.xz), COPYING and LICENSES verified in `licenses/dbus/provenance.json` | AFL 2.1, GPL 2 or later and MIT texts included. Document the chosen distribution route and comply with it; including both texts does not resolve that review. |
| SDL_GameControllerDB | [Pinned commit](https://github.com/mdqinc/SDL_GameControllerDB/tree/c1d5289a1f713b30a2c121e9fe6529d39360be0f), Zlib license, database/text hashes | Notice and provenance included; do not update the database during final acceptance. |
| Own source, decompilation metadata and ROM-generated code | This checkout's source and [pinned SlaveOfIDO map](https://github.com/SlaveOfIDO/blastcorps/tree/1ab6616edb6c16b642c229ab6d55c3c8687c7945) | There is no port license declaration in the root or map license in the inspected root. That does not determine ownership or game rights; resolve scope and provenance before publishing or offering source. |

New manifests retain `complete_dependency_license_review=false`.
Original SDK and library bytes are verified before copying notices: a changed
dependency requires reviewing/updating its provenance, rather than automatically
reusing a text obtained from another version.

## Pending corresponding source and rebuild

[GPL v3, sections 1, 4–6](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/COPYING)
defines corresponding source and conditions for delivering object code.
The runtime package includes the GPL text and there is a separate snapshot
of reviewable sources. That snapshot is not yet certified as corresponding
source and has no documented public delivery route.
Concrete preparation must retain linked sources/headers, generation/installation
scripts, interfaces and runtime/RT64 changes, identify modified files and
establish the applicable source delivery mechanism. Repository pins alone
do not complete that work.

The runtime binary was built with the private Tetrisphere layout and prebuilt
libraries. Now `tools/package_source.py` exports a reviewable private snapshot
with recursive sources, map, notices, patches/scripts and hashes;
`tools/bootstrap_sdk.py` builds a separate SDK, and `build.py --sdk-root`
supports rebuilding from the exported host with an owned ROM, without depending
on the Tetrisphere checkout. The recipe and limits are in
[source-rebuild.md](source-rebuild.md). N64Recomp/RSPRecomp and RT64/plume/zstd/
re-spirv/nfd have already built from that export in paths containing spaces.
Checks include spimdisasm, DXC and the controller database, along with validation
of the prepared RT64 copy and clearing library selection when switching SDKs.
The isolated host has completed generation, compilation and linking (224 targets),
with CPU CTest 41/41 and Python 110/110 after adding the mesgqueue transform
and its normal/timed fixtures. SDK/export configure/build finished with exit 0;
the checkpoint is recorded in source-host-build.json. Evidence is recorded
separately; this does not by itself certify corresponding source or licenses,
equivalence to the delivered object or GPU runtime. Transitive Python build
tools, bottles/C/C++ runtime and the source access mechanism applicable to
the final binary still need pinning.

The exported host with the mesgqueue correction, ELF
`27d46626bbd6b3e48a0317b4e20191bcb2a68d9c949862289470d1bdfe1c23ad`,
passed its CPU suites and 180 native seconds outside the checkout. Recipe and
inputs are in [source-rebuild.md](source-rebuild.md). The snapshot includes
SDK transformation scripts, sources, map, configuration and notices;
it requires an owned ROM to regenerate CPU/RSP. These technical results do
not certify license review or the package's corresponding source.

Before distribution:

- Review the source/script snapshot and complete the inventory of all inputs
  (recursive commits, local patches, build versions/options, checksums of
  prebuilt libraries and tools, Python packages).
- Test rebuilding RT64/plume/zstd/re-spirv/nfd, N64Recomp/RSPRecomp and the port
  from a clean directory. Do not depend on the private Tetrisphere checkout
  or generated files that cannot be rebuilt.
- Define how the recipient generates parts using their valid ROM without
  packaging ROM, overlays, progress or captures. Do not consider generated
  code licensing/redistribution resolved merely because the ROM was omitted.
- Establish port notices and map/incorporated-code provenance from evidence.
  Avoid assigning ownership or licenses by inference.
- Associate the source package and access mechanism with the exact final binary
  checksum. Full dependency review remains pending.

## Packaging recipe for a later change

The current delivery is already packaged; opening it locally does not require
a rebuild or further tests. If code changes, close GPU tests using the executable,
build the new state and check the updated package:

```bash
python3 tools/package_linux.py --build build \
  --tetrisphere '/var/home/alfredo/Documents/ChatGPT/Tetrisphere/.worktrees/options-beta' \
  --output '.local/packages/blastcorps-private-linux-x86_64.tar.gz'
sha256sum -c .local/packages/blastcorps-private-linux-x86_64.tar.gz.sha256
```

The packager installs a stripped copy; it does not alter the development ELF.
Inspect BUILD.json, hashes, RPATH, required symbols and tar members.
Extract into a new folder containing spaces outside the checkout, use isolated
data and accept cold startup, ROM selection/import, mission, save/exit/reload,
pause/Restart and package options/controls. Retain private logs and original
progress hashes before/after.

Exclude ROM, extracted overlays, user data, captures, private logs, reference
emulator and QA variants. The inventory must not include `data`, `.local` or
files with private extensions. Notice counts and passing suites do not resolve
intermittent audio or certify sources/licenses. Keep both release flags false
until their evidence is completed separately.
