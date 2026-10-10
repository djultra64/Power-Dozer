# Install Power Dozer v0.82

## Linux

Extract `power-dozer-v0.82-linux-x86_64.tar.gz` into a writable folder,
keeping the `lib` and `licenses` folders:

```bash
sha256sum -c power-dozer-v0.82-linux-x86_64.tar.gz.sha256
tar -xzf power-dozer-v0.82-linux-x86_64.tar.gz
cd power-dozer-v0.82-linux-x86_64
./'Run Power Dozer.sh'
```

## Windows

Download `power-dozer-v0.82-windows-x86_64.zip` and its SHA-256 file. Verify
the hash with `Get-FileHash` in PowerShell, extract the archive into a writable
folder, and run `Run Power Dozer.cmd` or `power-dozer.exe`. Keep all DLLs,
`gamecontrollerdb.txt` and `licenses` beside the executable.

Requires Windows x64, SSSE3/SSE4.1, Vulkan drivers and the
[Microsoft Visual C++ 2015–2022 x64 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).
The Windows package includes SDL2 2.26.3 and its pinned Windows DXC libraries.

```powershell
Get-FileHash .\power-dozer-v0.82-windows-x86_64.zip -Algorithm SHA256
Expand-Archive .\power-dozer-v0.82-windows-x86_64.zip -DestinationPath .
Set-Location .\power-dozer-v0.82-windows-x86_64
.\power-dozer.exe
```

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
The PC menu shows help, settings and per-action remapping. Prompts follow
the last active device, with Xbox, PlayStation or Switch family selectable
manually. On Switch, south/east/west/north positions are B/A/Y/X.

This is a beta. Intermittent audio loss remains under investigation.
Physical controller checks, sustained 4K/120 Hz and broad mission
coverage still require manual testing. `BUILD.json` identifies the commit and
each file's hashes; it keeps `ready_for_release` and
`license_review_complete` false. Notices are in `licenses`.
The source and its rebuild method are described in [build-source.md](build-source.md).
