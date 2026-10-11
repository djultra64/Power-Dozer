[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$SdkRoot,
    [Parameter(Mandatory=$true)][string]$CMakeExe,
    [Parameter(Mandatory=$true)][string]$PythonExe,
    [string]$BuildRoot = '',
    [ValidateRange(1,32)][int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$source = Split-Path $PSScriptRoot -Parent
if (-not $BuildRoot) { $BuildRoot = Join-Path $source 'build-windows' }
$sdk = (Resolve-Path -LiteralPath $SdkRoot).Path
$cmake = (Resolve-Path -LiteralPath $CMakeExe).Path
$python = (Resolve-Path -LiteralPath $PythonExe).Path
foreach ($generatedInput in @('.local\audio-rsp.cpp','.local\generated\fallbacks.cpp','.local\generated\functions\funcs.h')) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $generatedInput) -PathType Leaf)) {
        throw "Missing private generated input $generatedInput. Regenerate with tools/build.py using your own ROM before the Windows build."
    }
}
& $cmake -S $source -B $BuildRoot -G 'Visual Studio 17 2022' -A x64 "-DBC_SDK_ROOT=$sdk" "-DPython3_EXECUTABLE=$python"
if ($LASTEXITCODE) { throw 'Windows configure failed.' }
& $cmake --build $BuildRoot --config Release --target blastcorps-prototype windows_files_test --parallel $Jobs
if ($LASTEXITCODE) { throw 'Windows build failed.' }
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
& $ctest --test-dir $BuildRoot -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'Windows CPU/startup tests failed.' }
$binary = Join-Path $BuildRoot 'Release\blastcorps-prototype.exe'
$receipt = [ordered]@{
    schema=1; product='Power Dozer'; version='0.83'; platform='windows-x86_64';
    binary='Release/blastcorps-prototype.exe';
    binary_sha256=(Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToLowerInvariant();
    native_compile_verified=$true; gpu_runtime_verified=$false;
    ready_for_release=$false; license_review_complete=$false
}
$receipt | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $BuildRoot 'WINDOWS-BUILD.json') -Encoding UTF8
$receipt | ConvertTo-Json -Compress
