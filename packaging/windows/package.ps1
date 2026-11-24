# Build a portable Windows .zip from a finished Visual Studio (multi-config) build.
#
#   pwsh packaging/windows/package.ps1 -BuildDir <dir> -OutDir <dir> -Label <label>
#
# The app name comes from version.inc (SLIC3R_APP_NAME), so the same script packages
# PrusAngleSlicer and PrusSLASlicer. The result is unsigned (see INSTALL.md).
param(
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [Parameter(Mandatory = $true)][string]$Label
)
$ErrorActionPreference = 'Stop'

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$m = Select-String -Path (Join-Path $Root 'version.inc') -Pattern '^set\(SLIC3R_APP_NAME "(.*)"\)$'
if (-not $m) { throw 'SLIC3R_APP_NAME not found in version.inc' }
$AppName = $m.Matches[0].Groups[1].Value

$Rel = Join-Path $BuildDir 'src\Release'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$Stage = Join-Path $OutDir $AppName
if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
New-Item -ItemType Directory -Path $Stage | Out-Null

# Executables + DLLs produced next to the app (launchers, PrusaSlicer.dll, OCCTWrapper.dll
# for STEP import, GMP/MPFR, WebView2Loader).
Copy-Item (Join-Path $Rel '*.exe') $Stage
Copy-Item (Join-Path $Rel '*.dll') $Stage

# A launcher named after the app for people to double-click. prusa-slicer.exe stays too:
# the app starts new instances of itself by that name.
Copy-Item (Join-Path $Stage 'prusa-slicer.exe') (Join-Path $Stage "$AppName.exe")

# In the build tree "resources" is a link back to the source tree; ship a real copy.
Copy-Item -Recurse (Join-Path $Root 'resources') (Join-Path $Stage 'resources')

# Ship the Visual C++ runtime next to the app (app-local deployment), so it runs on
# machines without the "VC++ Redistributable" installed. Harmless if linked statically.
$vsRoot = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
$crt = Get-ChildItem -Path (Join-Path $vsRoot 'VC\Redist\MSVC') -Recurse -Directory -Filter 'Microsoft.VC14*.CRT' |
       Where-Object { $_.FullName -match '\\x64\\' -and $_.FullName -notmatch 'onecore|debug' } |
       Sort-Object { [version](($_.FullName -replace '.*\\MSVC\\([0-9.]+)\\.*', '$1') -replace '^[^0-9].*', '0.0') } -Descending |
       Select-Object -First 1
# Use the newest redist: an app-local CRT older than the compiler breaks loading PrusaSlicer.dll.
if ($crt) { Write-Host "VC++ runtime from: $($crt.FullName)"; Copy-Item (Join-Path $crt.FullName '*.dll') $Stage }
else { Write-Warning 'VC++ runtime DLLs not found; the zip relies on the installed redistributable.' }

# Sanity checks.
$required = @('prusa-slicer.exe', "$AppName.exe", 'PrusaSlicer.dll', 'OCCTWrapper.dll',
              'resources\profiles', 'resources\icons', 'resources\shaders')
foreach ($r in $required) {
    if (-not (Test-Path (Join-Path $Stage $r))) { throw "package is missing $r" }
}

$Zip = Join-Path $OutDir "$AppName-$Label-Windows-x64.zip"
if (Test-Path $Zip) { Remove-Item -Force $Zip }
Compress-Archive -Path $Stage -DestinationPath $Zip
Write-Host "zip: $Zip"
Get-ChildItem $Stage | Select-Object Name, Length | Format-Table -AutoSize | Out-String | Write-Host
