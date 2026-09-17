<#
    build.ps1 - configure + build AutoMaster on Windows (MSVC).

    Usage:
        pwsh scripts/build.ps1                     # Release, fetch JUCE
        pwsh scripts/build.ps1 -Config Debug
        pwsh scripts/build.ps1 -JucePath C:\JUCE   # use a local JUCE checkout

    Requires: CMake >= 3.22 and Visual Studio 2022 (Desktop C++ workload).
#>
param(
    [string]$Config   = "Release",
    [string]$JucePath = "",
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

$args = @(
    "-S", $root,
    "-B", (Join-Path $root $BuildDir),
    "-G", "Visual Studio 17 2022",
    "-A", "x64"
)
if ($JucePath -ne "") {
    $args += "-DAUTOMASTER_JUCE_PATH=$JucePath"
}

Write-Host "==> cmake $($args -join ' ')" -ForegroundColor Cyan
& cmake @args

Write-Host "==> cmake --build $BuildDir --config $Config" -ForegroundColor Cyan
& cmake --build (Join-Path $root $BuildDir) --config $Config --parallel

Write-Host ""
Write-Host "VST3 output:" -ForegroundColor Green
Get-ChildItem -Recurse -Filter "AutoMaster.vst3" (Join-Path $root $BuildDir) |
    ForEach-Object { Write-Host "  $($_.FullName)" }
