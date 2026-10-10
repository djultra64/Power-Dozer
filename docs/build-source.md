# Rebuild Power Dozer v0.82

`power-dozer-v0.82-source.tar.gz` contains the host, adaptation scripts,
configuration, tests and notices, along with pinned sources for N64Recomp,
N64ModernRuntime, RT64 and spimdisasm and the three mapping/symbol inputs.
`SOURCES.json` records repositories, recursive commits and SHA-256/size for
each file. The snapshot excludes the ROM, overlays, CPU/RSP code generated from
the ROM, private progress, captures, evidence and prebuilt binaries.
The progress fixture included in the tests is synthetic.

Rebuild requirements: Linux x86-64, Git/Python, a C/C++20 compiler,
CMake, Ninja, pkg-config, SDL2 and DBus development files, X11/Xorg protocol headers,
zlib, headers for the Python in use, venv/pip and a connection to download pinned inputs.
If SDL is outside standard prefixes, set `PKG_CONFIG_PATH` and
`CMAKE_PREFIX_PATH` to its installed prefix.

```bash
sha256sum -c power-dozer-v0.82-source.tar.gz.sha256
tar -xzf power-dozer-v0.82-source.tar.gz
cd power-dozer-v0.82-source/host

# Verify hashes and display the recipe without downloading or compiling:
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --plan

# Prepare tools, RT64 and the analysis environment in a new or empty SDK:
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --jobs 2

# Regenerate and build using your own USA Rev 1 ROM:
python3 tools/build.py '/path/Blast Corps USA Rev 1.zip' \
  --sdk-root ../sdk --mapping-source ../mapping --build-dir build
```

Exported sources and DXC/PyYAML/Rabbitizer/controller database inputs are
validated before use. Runtime and RT64 transforms produce
local copies from the pinned sources without modifying the originals.
Access to the private checkout used to prepare the snapshot is not required.
`include/control_atlas_data.h` contains the already extracted sprites: building does not
require the user's original PNGs or running the atlas extractor.
Their provenance and separate rights review are in
`config/control-atlas.json` and `NOTICE`.

```bash
ctest --test-dir build --output-on-failure
BC_SDK_ROOT=../sdk python3 -m unittest discover -s tests
```

The development executable retains the target name
`build/blastcorps-prototype`. Distribution packages rename it to
`power-dozer`. The source archive is deterministic for the same snapshot;
this does not claim bit-for-bit equality of the executable. An isolated build
using this method was checked at earlier checkpoints; the current snapshot's
manifest keeps its certification flags false.

`LICENSE` describes project code and `NOTICE` defines the scope of dependencies and assets.
Review of the upstream mapping and the scope of CPU/RSP code generated from the
ROM is still pending. Providing regeneration scripts and pinned inputs does not
by itself certify delivery of complete GPL corresponding source.
`license_review_complete`, `corresponding_source_verified`,
`clean_rebuild_verified` and `ready_for_release` remain false.


## Native Windows build

`tools/build_windows.ps1` configures a native x64 build with Visual Studio 2022,
builds RT64 and the host, copies the pinned Windows SDL/DXC DLLs and runs the
Windows save/import test. Provide CMake, Python and a prepared SDK containing
`deps/N64ModernRuntime` and `deps/rt64`. Regenerate `.local/generated` and
`.local/audio-rsp.cpp` privately from your own ROM before invoking the script.
The Linux SDK bootstrap is not a complete Windows bootstrap; the release test
used the same prepared SDK sources and privately generated inputs on both hosts.

```powershell
.\tools\build_windows.ps1 -SdkRoot 'C:\PowerDozer\sdk' `
  -CMakeExe 'C:\Tools\CMake\bin\cmake.exe' `
  -PythonExe 'C:\Tools\Python\python.exe' -Jobs 4
```

The native executable is `build-windows/Release/blastcorps-prototype.exe`.
`WINDOWS-BUILD.json` records its hash and compile/test evidence; its GPU flag
remains false until a separate interactive runtime test has been performed.
The public source archive excludes privately generated game code and the ROM.
