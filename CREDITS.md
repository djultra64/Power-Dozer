# Power Dozer credits and acknowledgments

Power Dozer is a community native port of **Blast Corps USA Rev 1**. It exists
thanks to the original game developers, reverse engineering researchers,
recompilation and rendering projects, and the maintainers of the libraries and
tools below. Credit belongs to their contributors as well as the named authors.

These acknowledgments describe the dependencies identified in the pinned build
inputs, CMake targets, source headers, packaged notices and Linux package
inventory. They supplement the original copyright and license notices; they do
not replace them or certify a completed dependency or redistribution audit.

## Original game and reverse engineering

- **Rare and the original Blast Corps team**, for the game, code, art, music,
  sound and design, and **Nintendo**, its publisher and the Nintendo 64 platform
  developer. Power Dozer is unaffiliated with Rare or Nintendo. Game resources
  and translated game code are prepared locally from the player's own ROM;
  the distributed archives contain no ROM or extracted game resources.
- **[retroplastic/blastcorps](https://github.com/retroplastic/blastcorps)**,
  for the earlier Blast Corps reverse engineering work used as a reference.
- **[SlaveOfIDO/blastcorps](https://github.com/SlaveOfIDO/blastcorps)** and its
  contributors, for the continuation supplying the USA Rev 1 segment map and
  symbol addresses used by this build. The three selected mapping inputs and
  their hashes are recorded in `config/source-build.json`, at revision
  `1ab6616edb6c16b642c229ab6d55c3c8687c7945`. Acknowledging this work does not
  establish a license for the mapping or the original game.
- **[GLideN64](https://github.com/gonetz/GLideN64)**, Sergey Lipskiy and its
  contributors, for the L3D microcode reference consulted during feasibility
  work. RT64's separately supplied texture-hasher tool also acknowledges
  GLideN64 and **RiceVideo** hashing algorithms; that tool's source and GPL
  notice retain their own terms.

## Recompilation and runtime

| Project / contributors | Use in Power Dozer |
| --- | --- |
| [N64Recomp / RSPRecomp](https://github.com/N64Recomp/N64Recomp), Wiseguy and contributors | Build tools translate the game's CPU code and audio RSP microcode. Recompiler headers are also used by the host and generated code. The pinned project carries an MIT notice. |
| [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime), its maintainers and contributors | Statically linked `ultramodern` implements threads, queues, timing and hardware services; selected `librecomp` files and headers bridge recompiled game code. The project carries GPL v3 terms, with separate terms on embedded third-party portions. |
| [ares](https://github.com/ares-emulator/ares), the ares team, Near and other contributors | N64ModernRuntime's `rsp_vu.hpp` and `rsp_vu_impl.hpp` explicitly adapt the ares N64 vector unit. Their original permission notice is embedded in those headers and retained verbatim as `licenses/embedded/ares-rsp-notice.txt`, with both header hashes recorded. |
| Cameron Desrochers, **concurrentqueue / BlockingConcurrentQueue**, and Jeff Preshing's semaphore work | Header implementations used by runtime queues and synchronization. Their separate BSD and zlib notices are retained in `licenses/embedded`. |
| [JSON for Modern C++](https://github.com/nlohmann/json), Niels Lohmann and contributors | JSON headers used by the port, N64ModernRuntime and RT64. The pinned 3.12.0 header also credits Evan Nemerson's Hedley, the Abseil Authors, Björn Hoehrmann and Florian Loitsch. Exact SPDX attributions and MIT text are retained in `licenses/embedded`. |
| [miniz](https://github.com/richgel999/miniz), Rich Geldreich and contributors | N64ModernRuntime's miniz sources are compiled for bounded ROM ZIP input; RT64 includes another miniz implementation. Embedded notices also credit Tenacious Software LLC, RAD Game Tools, Valve Software, Martin Raiber and Alex Evans's PNG-writing contribution. Preserve the terms belonging to each version. |

The port changes runtime integration and scheduling through local source
preparation scripts. These modifications are Power Dozer work; they should not
be attributed to the upstream runtime as unmodified behavior.

## Rendering and libraries incorporated through RT64

**[RT64](https://github.com/rt64/rt64)** and the RT64 contributors provide the
N64 graphics renderer. Its pinned MIT notice is retained separately from the
licenses of its contributions. RT64's own documentation also acknowledges
**[Wiseguy](https://github.com/Mr-Wiseguy)** for hardware-oriented TMEM test ROMs
and **[Tharo](https://github.com/Thar0)** for RDP color-conversion research.

The Linux build links RT64, re-spirv, nativefiledialog-extended, zstd and Plume
static libraries. The following source/header contributions support those
libraries; inclusion in the renderer does not imply that the port enables every
upstream debug or texture-replacement feature.

| Project / contributors | Contribution |
| --- | --- |
| [Plume](https://github.com/renderbag/plume), renderbag and contributors | Graphics abstraction and the Vulkan backend. |
| [re-spirv](https://github.com/rt64/re-spirv), renderbag and contributors | SPIR-V processing linked into RT64. |
| [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers), Khronos Group and contributors | SPIR-V declarations used by re-spirv. |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers), Khronos Group and contributors | Vulkan API declarations used by Plume. |
| [volk](https://github.com/zeux/volk), Arseny Kapoulkine and contributors | Vulkan entry-point loading through Plume. |
| [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator), AMD and contributors | Vulkan GPU-memory allocation through Plume. |
| [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended), its authors and contributors | Desktop file chooser; the Linux build uses the portal/DBus backend. |
| [Zstandard](https://github.com/facebook/zstd), Yann Collet, Meta/Facebook and contributors | Statically linked compression library. Its bundled hashing code has its own xxHash attribution. |
| [xxHash](https://github.com/Cyan4973/xxHash), Yann Collet and contributors | Header-based hashing in RT64 and the port's graphics adapter. |
| [hlsl++](https://github.com/redorav/hlslpp), Emilio López and contributors | Shader-style CPU vector and matrix math. |
| [ddspp](https://github.com/redorav/ddspp), Emilio López and contributors | DDS texture declarations and decoding in RT64. |
| [stb](https://github.com/nothings/stb), Sean Barrett and contributors | Image decoding in RT64. The vendored stb license and per-header contributors remain applicable. |
| [Dear ImGui](https://github.com/ocornut/imgui), Omar Cornut and contributors | RT64 debug UI and Vulkan backend. ImGui also retains its embedded font and supporting-code notices. |
| [ImPlot](https://github.com/epezent/implot), Evan Pezent and contributors | Plotting compiled into RT64's debugging facilities. |
| [Im3d](https://github.com/john-chapman/im3d), John Chapman and contributors | Immediate-mode 3D debugging primitives compiled into RT64. |
| [Mupen64Plus core](https://github.com/mupen64plus/mupen64plus-core), its authors and contributors | API headers supplied in the RT64 checkout and used by the integration. This does not make the Mupen64Plus emulator core the game's runtime. |

RT64's source also contains **plainargs** for its CLI tools; **Giovanni
Dicanio's utf8conv / utf8except** Windows conversion headers; **Project64**
plugin-interface headers; the **[Mupen64Plus Windows dependency
collection](https://github.com/mupen64plus/mupen64plus-win32-deps)**; and
**[SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)**. Plume includes
**[D3D12 Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator)**
and **Apple metal-cpp** for its other backends. These contributions are
acknowledged as supplied upstream source or platform/tool support, rather than
as libraries linked into this Linux executable. Their original source notices
and platform-specific licenses remain in the vendor trees.

## Bundled Linux runtime libraries and controller data

| Project / contributors | Packaged role |
| --- | --- |
| [Simple DirectMedia Layer](https://www.libsdl.org/), Sam Lantinga and the SDL contributors | Windowing, keyboard/controller input and audio through the SDL2 API. The inspected Linux v0.8 package provides `libSDL2-2.0.so.0` with **sdl2-compat 2.32.70**, backed by **SDL3 3.4.12** (`libSDL3.so.0`). Both retain their own zlib notices. |
| [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB), its maintainers and community contributors | Controller mappings, fixed at `c1d5289a1f713b30a2c121e9fe6529d39360be0f`; database, zlib notice and recorded provenance are distributed together. |
| DBus / freedesktop.org contributors | `libdbus-1.so.3`, used by the file-dialog portal and desktop services. The inspected library is 1.16.2; `licenses/dbus` preserves its COPYING and the referenced AFL, GPL and MIT texts. The official source archive is recorded in that directory's provenance manifest. |
| [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler), Microsoft and contributors | DXC compiles RT64 shaders at build time; `libdxcompiler.so` and `libdxil.so` are also supplied with the Linux runtime package for the renderer's compiler-loading path. Binary inputs come from [rt64/dxc-bin](https://github.com/rt64/dxc-bin), with exact hashes and source-notice revision recorded in `licenses/dxc/provenance.json`. |

The exact DXC notices also acknowledge **LLVM and Clang**, the **University of
Illinois at Urbana-Champaign**, **eXtensible Systems**, **OpenBSD regex / Henry
Spencer**, **the Regents of the University of California**, **PyYAML / Kirill
Simonov**, and other portions identified there, including MD5 contributions and
compiler headers. `licenses/dxc/LICENSE.TXT` and `ThirdPartyNotices.txt` are
retained verbatim, including the upstream instruction not to translate or
localize those notices.

## Windows runtime libraries and build platform

The Windows x64 build uses the same pinned N64Recomp, N64ModernRuntime and RT64
source families, with the platform's own implementations and dependencies.
The DLL inputs below were inspected in the pinned SDK; Windows DXC version
output was measured on the native Windows build machine.

| Project / contributors | Windows role |
| --- | --- |
| [Simple DirectMedia Layer](https://www.libsdl.org/), Sam Lantinga and the SDL contributors | **SDL2 2.26.3**, supplied as `SDL2.dll` from RT64's pinned Mupen64Plus Windows dependency collection. The original `SDL2-2.26.3/COPYING.txt` notice is retained. Windows uses this SDL2 DLL, rather than the Linux sdl2-compat/SDL3 implementation. |
| [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler), Microsoft and contributors | The pinned Windows compiler reports **dxcompiler.dll 1.7.0.4147 (0dc8d9060)** and **dxil.dll 1.7 (101.7.2212.14)**. `dxc.exe` supports shader building; the compiler and validator DLLs are Windows runtime package inputs. |
| [D3D12 Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator), AMD and contributors | Included by Plume's Windows build alongside its Vulkan backend. The allocator's original LICENSE and NOTICES remain with the vendor notices. |
| nativefiledialog-extended contributors | The Windows native file-dialog implementation; the Linux portal/DBus dependency does not apply to this Windows target. |
| Giovanni Dicanio, Microsoft Windows SDK and platform contributors | RT64's Windows UTF-8 conversion support and platform/graphics API headers and services. |
| Microsoft Visual Studio / MSVC Build Tools and Windows SDK contributors | Native Windows compilation, linking, import inspection and platform support. |

The Windows DXC source identifier resolves in Microsoft's official repository
to **`0dc8d9060df41901dbfa5ad7190da03e33d09518`**. Exact notices retrieved from
that revision are retained in `licenses/dxc-windows`, with their hashes and
the exact pinned `dxcompiler.dll` / `dxil.dll` hashes in `provenance.json`.
This is an independent provenance bundle from Linux's DXC 1.8 notices in
`licenses/dxc`. The validator's complete build provenance has not been
independently attested; the recorded source revision is the compiler's
reported revision.

The Windows DXC DLL imports require Microsoft's **Visual C++ x64 runtime**
(`MSVCP140`, `VCRUNTIME140`, `VCRUNTIME140_1`) and the Windows **Universal C
Runtime**. The Microsoft redistributable installer and its runtime DLLs are
external prerequisites, rather than bundled Power Dozer files. Microsoft
provides the [official Visual C++ Redistributable documentation and
downloads](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
and the [x64 redistributable installer](https://aka.ms/vc14/vc_redist.x64.exe).
The Vulkan loader and GPU driver are supplied by the Windows installation.

## Build, reconstruction and development tools

The following support generation, reconstruction, packaging or development;
they are not all linked into the game executable.

| Project / contributors | Role |
| --- | --- |
| [spimdisasm](https://github.com/Decompollaborate/spimdisasm), Decompollaborate and contributors | MIPS disassembly/analysis used when preparing recompiler inputs. Its pinned revision is in `config/source-build.json`. |
| [Rabbitizer](https://github.com/Decompollaborate/rabbitizer), Decompollaborate and contributors | Instruction decoding in N64Recomp/RSPRecomp and spimdisasm's Python dependency; Python source input 1.16.2 and its MIT notice are recorded in `licenses/buildtools`. |
| [ELFIO](https://github.com/serge1/ELFIO), Serge Lamikhov-Center and contributors | ELF parsing in the N64Recomp tools. |
| [fmt](https://github.com/fmtlib/fmt), Victor Zverovich and contributors | Formatting used by the recompilation tools. |
| [toml++](https://github.com/marzer/tomlplusplus), Mark Gillard and contributors | Configuration parsing in the pinned N64Recomp/RSPRecomp build. The current CMake target and headers use toml++; an older upstream README reference to toml11 does not describe this pinned build. |
| PyYAML, Kirill Simonov and contributors | YAML input parsing; source input 6.0.2 and its MIT notice are recorded in `licenses/buildtools`. |
| Python / Python Software Foundation, pip and their contributors | Build and packaging scripts and isolated analysis environment. |
| CMake / Kitware, Ninja and their contributors | Build configuration, compilation orchestration, tests and packaging installation. |
| GNU GCC, GNU Binutils, GNU Make, LLVM/Clang and their contributors | Native compiler/linker toolchains supported or used by the upstream and local build workflows; Binutils `readelf` and `strip` support package inspection and installation. Actual compiler versions depend on the recorded build environment. |
| Git and pkg-config maintainers | Pinned source retrieval/revision checking and system-library discovery. |
| Homebrew and formula maintainers | Source of the inspected SDL and DBus Linux libraries and reference development environment. |
| Pillow and its contributors | Optional extraction and preview of the user-supplied control atlas. Normal builds use the already extracted header and do not require the original PNGs. |
| FFmpeg and x264 contributors, xdotool contributors, X.Org tools and Xvfb | Local gameplay input, window probing and video capture in development/QA tooling. These capture tools and private recordings are not runtime package contents. |
| RenderDoc contributors | Local graphics investigation and capture inspection. |

Additional contributions preserved in the pinned source archives include
**[SLJIT](https://github.com/zherczeg/sljit)** by Zoltan Herczeg, used by
N64Recomp's separate live-recompiler targets; **[o1heap](https://github.com/N64Recomp/o1heap)**
by Pavel Kirienko and contributors; **[SSE2NEON](https://github.com/DLTcollab/sse2neon)**
contributors for ARM SIMD support; and **[odashi/encoding](https://github.com/odashi/encoding)**
by Yusuke Oda. The runtime's encoding source also acknowledges the JIS mapping
table contributors **earthian, I'O and Project X0213**. These source components
are not all compiled by Power Dozer's selected Linux targets. Vendor test
sources retain their own embedded framework notices, including Catch/Catch2
and Google Test/Google Mock where supplied.

Local reference investigation also used **Mupen64Plus**, based on Hacktarux's
original mupen64, **[mupen64plus-rsp-cxd4](https://github.com/mupen64plus/mupen64plus-rsp-cxd4)**
and **Angrylion RDP Plus**. The latter's own credits acknowledge Angrylion,
Ville Linde, MooglyGuy, theboy181, fallaha56, loganmc10 and other contributors.
These are independent reference tools, rather than shipped Power Dozer
components. Their development prerequisites include **OpenGL**, **freeglut**,
**libpng**, **FreeType** and **NASM**, with their own project authors and terms.
The earlier **Tetrisphere** workspace supplied the inspected reference SDK and
dependency checkouts; the independent reconstruction path records its own
inputs instead of requiring that workspace.

## System services and graphics platform

The Linux executable uses the system's **GNU C Library**, **libstdc++**,
**libgcc**, math, threading and dynamic-loading facilities. The supplied DXC
libraries also require the system's **zlib**. Desktop integration relies on
**X.Org/X11 or XWayland**, desktop portal implementations and the operating
system's input/audio services. SDL selects available audio and display
backends, which may involve **ALSA**, **PulseAudio/PipeWire** and **Wayland**.

Vulkan is provided by the **Khronos Group's Vulkan ecosystem**, the Vulkan
loader and the installed GPU driver. Driver implementations and system
services belong to their respective projects and vendors; they are not
bundled as Power Dozer libraries. The installed system determines the exact
backend and transitive service/library set.

## Supplied artwork and project contributions

The project owner supplied the keyboard and Xbox/PlayStation/Nintendo Switch
control-glyph atlases. Their original artist and license have **not been
identified**. The owner explicitly confirmed permission to redistribute the
supplied artwork in the public repository and its releases on 2026-10-10;
this confirmation does not identify the author or apply the host-code GPL to
the artwork. Exact input hashes and extraction boxes are in
`config/control-atlas.json`; the extracted sprites are in
`include/control_atlas_data.h`. See `NOTICE` for the recorded scope.

**djultra64** provides Power Dozer's integration and project maintenance.
Power Dozer's contributors provide the game integration, scheduling fixes,
graphics adaptations, portable saves, controls and device-aware prompts,
packaging and documentation. Upstream source and asset authors retain their
own authorship and rights.

## Notices and provenance

- `LICENSE` covers original host code and project-specific modifications;
  `NOTICE` records the game and supplied-artwork scope.
- `config/source-build.json` and `config/dependencies.json` identify the main
  pinned source projects and reconstruction inputs. Source archives record
  recursive vendor revisions in `SOURCES.json`.
- `licenses/` and the vendor source trees preserve the original license and
  contributor notices. Linux and Windows packages place this document at
  `licenses/Power-Dozer/CREDITS.md` alongside the host notices; `BUILD.json`
  inventories the package and controller-data provenance.
- Credits are acknowledgment, not a new license grant. Existing audit flags
  remain unchanged: a complete licensing/corresponding-source review and
  exact final-binary reconstruction are not certified by this document.
