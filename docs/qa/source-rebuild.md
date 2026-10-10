# Private source and independent SDK rebuild

Local preparation on October 10, 2026. The source archive supports reviewing
and rebuilding the work without Tetrisphere paths. **It is not certified as a
complete GPL corresponding-source delivery or a distribution with reviewed
licenses.** The independent host rebuild and its CPU tests are verified;
the final runtime package still needs updating and acceptance.
Graphics/stability blockers remain separate from this work.

## Contents

`tools/package_source.py` generates `.local/packages/blastcorps-private-source-review.tar.gz`
and its checksum. It includes host C/C++, headers, configuration, scripts, tests,
text documentation, a synthetic progress fixture and notices; pinned sources for
N64ModernRuntime, N64Recomp with its recursive submodules, RT64 with its contributed
sources, and spimdisasm; three map/symbol files consumed by the generator.

`SOURCES.json` identifies repositories, recursive commits, excluded files,
SHA-256/bytes for each file and the host checkout state. Contents are checked
again before writing the tar. Order, timestamps, owners and gzip header are
stable for the same snapshot. This means source archive reproducibility,
not a bit-for-bit identical executable.

Excluded: `.git`, `.local`, ROM, overlays, CPU/RSP code translated from ROM,
progress, Pak, captures, binary evidence, prebuilt libraries, tool executables,
reference emulator and QA variants. The archive does not depend on later access
to the Tetrisphere checkout used to produce it.

Runtime events/timer/threadqueue/lifecycle/mesgqueue adaptation scripts and the
RT64 observer scripts remain in the host. They regenerate local build copies
from pinned sources with fingerprint validations; they do not require changes
to the original SDK. Their provenance, flags and recipe remain reviewable.

## Input pins and provenance

`config/source-build.json` pins N64Recomp, N64ModernRuntime, RT64 and spimdisasm
(`4b5d4c68eeb9f9eae14b3bb713971433ae2c3c43`, [source and MIT license](https://github.com/Decompollaborate/spimdisasm/blob/4b5d4c68eeb9f9eae14b3bb713971433ae2c3c43/LICENSE)).
PyYAML is pinned to 6.0.2 and Rabbitizer Python to 1.16.2, the version inspected
in the reference analysis venv that satisfies `spimdisasm/requirements.txt`
(>=1.14.0,<2.0.0). Both official sdists are downloaded by exact URL and verified
by SHA-256/size before installation with pip `--no-deps`; MIT notices are in
`licenses/buildtools`. Rabbitizer Python is a separate input from the Rabbitizer
C++ included in N64Recomp. Metadata comes from
[PyPI PyYAML 6.0.2](https://pypi.org/pypi/PyYAML/6.0.2/json) and
[PyPI Rabbitizer 1.16.2](https://pypi.org/pypi/rabbitizer/1.16.2/json).
Extension compilation still downloads unpinned isolated build tools
(setuptools/wheel, among others declared by the sdists); this limits bit-for-bit
reproducibility, although the two sdists are pinned.

The three DXC shader-generation tools/libraries are downloaded from the exact
dxc-bin revision and verified by SHA-256/size before use. Their exact notices
are in `licenses/dxc`. Analysis/recompilation tool sources are present;
DXC binaries are treated as verified external inputs and are not part of the
source archive. That inventory decision does not determine the legal scope
of corresponding source.

The controller database is downloaded from its pinned commit and validated
against the already accepted hash/size. The lock retains its license/provenance
URL; the runtime package retains its notice. System SDL2/SDL3 and DBus are
found by CMake/pkg-config; an exact comparison with the current binary also
requires pinning their sources/bottles, compiler, C/C++ runtime and options.

## Use outside the development checkout

Requirements: Linux x86-64, Git/Python, CMake, Ninja, C/C++20 compiler,
pkg-config, SDL2 and DBus headers/libraries, X11 headers/Xorg protocols, zlib,
Python development headers, venv/pip support, and download access for
DXC/PyYAML/controller database. For SDL outside standard prefixes, set
`PKG_CONFIG_PATH` and `CMAKE_PREFIX_PATH` to the actual installed prefix.
The Python invoking bootstrap needs its own `Python.h` headers; another
installed Python is insufficient. SDK mode has no required Homebrew prefix.

Extract the source archive into a new folder. Inside
`blastcorps-private-source/host`:

```bash
# Verify exported sources and show the recipe without downloading/compiling.
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --plan

# Build N64Recomp/RSPRecomp and RT64 libraries, and prepare the analysis environment.
# The SDK destination must be new or empty; a previous installation is not merged.
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --jobs 2

# When the game rebuild is authorized, use your own valid ROM.
python3 tools/build.py '/path/Blast Corps USA Rev 1.zip' \
  --sdk-root ../sdk --mapping-source ../mapping --build-dir build
```

The SDK contains `deps`, `toolchain/n64recomp-build`, `rt64-build`,
`analysis-venv`, `build-inputs`, `gamecontrollerdb.txt`, `SOURCE-INPUTS.json` and `SDK-BUILD.json`.
The recipe builds static RT64, SDL/Vulkan and the portal picker backend.
RT64's original CMake does not invoke DXC correctly when its path contains
spaces. Bootstrap checks the full SHA and a unique site, preserves `deps/rt64`
and generates `sdk/rt64-source` as a local copy using `cmake -E env`.
It records both hashes and the change in `RT64-SOURCE-PATCH.json`;
libraries are built from that copy. Subsequent host validation continues to
check the original sources, exact derivation of the copy and its metadata,
without ignoring modifications. Before extracting/regenerating ROM it also
validates spimdisasm, DXC and the controller database. When switching SDKs or
returning to legacy mode, it pins the library path and clears the opposite
picker selection to avoid reusing a library retained in CMake's cache.
Commands use separate arguments and tolerate spaces in directories.
The host build validates exported sources by their hashes when Git is absent.
The older `--tetrisphere` mode retains its layout and defaults.

The game rebuild revalidates the USA Rev 1 ROM, extracts overlays, generates
CPU boundaries/code and RSP microcode, creates the local hook copy and links
the host with the SDK. These outputs remain in `.local` and `build` in the
recipient's source tree; they are not included in the source archive.

## Evidence and acceptance limits

The post-lifecycle and mesgqueue snapshot of 10,309 files was extracted into
`/tmp` with spaces and all its hashes passed. N64Recomp and RSPRecomp built
from the exported sources (exit 0). Static RT64 and its four dependent
libraries (re-spirv, nfd, zstd, plume) also built (exit 0), with SDL/DBus/X11
found by pkg-config. Actual DXC shader execution confirmed the fix for paths
with spaces; the build also exposed the need for X11 headers. The independent
venv contains pinned PyYAML and Rabbitizer. This staged check is not yet an
end-to-end execution of the new bootstrap command.
The full exported host rebuild finished with exit 0: CPU/RSP generation,
configuration, all 224 targets and linking. The local ROM was a read-only input
and all writes occurred in the temporary host. CPU CTest passed 41/41 and
Python 110/110 with an owned ROM, without skips, after integrating the
mesgqueue transform that validates the original SDK source and generates a
local copy. Normal/timed fixtures prove retained critical retries do not hide
VI or overwrite full queues, and preserve jam/nonretry rules. Export configure
and build finished with exit 0; SDK, tools and original ROM were unchanged.
Private ELF for this checkpoint, SHA-256:
`27d46626bbd6b3e48a0317b4e20191bcb2a68d9c949862289470d1bdfe1c23ad`.
The earlier lifecycle rebuild had passed 39/107; the earlier binary
`537380aa960c9b52357542726d348ee5b070be06c468b085f486340413f12512`
corresponds to that earlier state. Results/checksums are in
`.local/qa/source-host-build.json`.

The ROM test allows `BC_TEST_ROM`, then `.local/rom.z64`, then a local ZIP;
if none exists, only that private check is skipped. The corrupt-ROM test still
runs without private data. The `legacy-completed.hex` fixture is synthetic
and included by its exact path, without allowing other progress hex files.

Early attempts exposed missing Rabbitizer, missing Python headers, a nonexistent
gcc-13 sysconfig compiler and an omitted synthetic fixture. The recipe addresses
those requirements and retains final results in `.local/qa`.
The CPU checkpoint recorded here does not certify its GPU execution or its
correspondence with the final package object. A CPU rebuild is not binary
equivalence, installation/runtime or corresponding-source certification.

The production binary was not modified and the game was not run in this source
work. The final checksum must be recorded after including all changes and
documentation, and the final package test must use that state.

## Pending items preventing corresponding-source/license certification

1. The `game.us.v11.yaml` map and two `symbol_addrs` come from SlaveOfIDO
   commit `1ab6616edb6c16b642c229ab6d55c3c8687c7945`. Their hashes and origin
   are retained. No license declaration was found in the inspected root;
   this does not establish ownership or redistribution permission.
2. Translated CPU/RSP code comes from instructions in the user-supplied ROM.
   An MIT generator does not automatically assign its license to the translated
   game. This archive delivers only the regeneration method; whether it
   constitutes the complete corresponding source for the delivered executable,
   and the scope of rights over generated content, must be resolved.
3. There is no license declaration for the port itself in the root. Choosing
   and applying appropriate notices and a delivery mechanism requires
   establishing the scope of covered components and deliverable sources.
4. Complete the inventory of libraries/headers/shaders actually incorporated,
   with their notices, and pin/deliver or document all applicable inputs.
   Existing commits and texts do not replace a review of that combination.
5. Run the full rebuild with an owned ROM in an isolated tree, compare binary
   identity/code and flags with the final package, verify installation/runtime,
   and define source access for each object-code recipient under the
   [runtime's GPL v3](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/COPYING).

Keep `license_review_complete`, `corresponding_source_verified` and
`clean_rebuild_verified` false until this evidence is complete. Manual 120 fps
and mission coverage tests do not resolve these pending items.


Later native acceptance: rebuilt ELF SHA-256
`537380aa960c9b52357542726d348ee5b070be06c468b085f486340413f12512`
completed 180 seconds outside the checkout in a folder containing spaces:
10,801 VI, 10,948 graphics tasks and 5,397 audio tasks, movement,
pause/Continue and Restart. It loaded ALF and completed progress from private
copies; seeds remained intact. Virtual output used radv, 1080p, 16:9 and
configured target 120, without requiring the fps threshold.
This is not physical output/audible audio acceptance or acceptance of the pending graphics defect.

The updated mesgqueue-fairness host passed a separate native regression in
`gameplay-3ar19xmo`, using ELF
`27d46626bbd6b3e48a0317b4e20191bcb2a68d9c949862289470d1bdfe1c23ad`
and cwd outside the checkout. It completed 180 seconds at VI 10801, graphics
10951 and audio 5399, with movement, pause/Continue, Restart and existing
completed-level replay verified. EEPROM/Pak seed sources stayed unchanged.
This is runtime acceptance of the rebuilt host in the checked flow; no binary
reproducibility, comprehensive platform coverage or source-license certification
is inferred. The final packaged object still needs its own acceptance.
