# Smoke test for the packaged Windows app: slice a cube with prusa-slicer-console.exe.
# On failure it collects what is needed to find out why the app does not start
# (missing DLL imports, CRT versions, a retry without the app-local CRT) and emits it
# as a warning annotation, because job logs are not readable without admin rights.
#
#   pwsh .github/scripts/win_smoke.ps1 -Dir dist\<AppName>
param([Parameter(Mandatory = $true)][string]$Dir)
$ErrorActionPreference = 'Continue'
$out = New-Object System.Collections.Generic.List[string]

python -c "import struct;s=10.0;v=[(-s,-s,0),(s,-s,0),(s,s,0),(-s,s,0),(-s,-s,20),(s,-s,20),(s,s,20),(-s,s,20)];t=[(0,2,1),(0,3,2),(4,5,6),(4,6,7),(0,1,5),(0,5,4),(2,3,7),(2,7,6),(1,2,6),(1,6,5),(0,4,7),(0,7,3)];open('cube.stl','wb').write(bytes(80)+struct.pack('<I',len(t))+b''.join(struct.pack('<fff',0,0,0)+b''.join(struct.pack('<fff',*v[i]) for i in x)+struct.pack('<H',0) for x in t))"

function Invoke-Slice([string]$d, [string]$tag) {
    Remove-Item -ErrorAction SilentlyContinue cube.gcode
    $res = & (Join-Path $d 'prusa-slicer-console.exe') --export-gcode cube.stl --output cube.gcode 2>&1
    $code = $LASTEXITCODE
    $ok = ($code -eq 0) -and (Test-Path cube.gcode)
    $out.Add("[$tag] exit code: $code, gcode written: $(Test-Path cube.gcode)")
    $res | Select-Object -Last 15 | ForEach-Object { $out.Add("  $_") }
    return $ok
}

$ok = Invoke-Slice $Dir 'as packaged'
if ($ok) {
    Write-Host ($out -join "`n")
    Write-Host ("first line: " + (Get-Content cube.gcode -TotalCount 1))
    exit 0
}

# --- Diagnostics -------------------------------------------------------------------
$sys = Join-Path $env:WINDIR 'System32'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$dumpbin = & $vswhere -latest -find 'VC\Tools\MSVC\**\bin\Hostx64\x64\dumpbin.exe' | Select-Object -First 1
function Get-Imports([string]$file) {
    & $dumpbin /nologo /dependents $file | ForEach-Object { $_.Trim() } |
        Where-Object { $_ -match '\.dll$' -and $_ -notmatch '^(Dump of|Image has)' }
}
if ($dumpbin) {
    # Walk imports starting at PrusaSlicer.dll; follow DLLs that live in the package.
    $seen = @{}; $queue = New-Object System.Collections.Generic.Queue[string]
    $queue.Enqueue((Join-Path $Dir 'PrusaSlicer.dll'))
    $missing = New-Object System.Collections.Generic.List[string]
    while ($queue.Count -gt 0) {
        $f = $queue.Dequeue()
        foreach ($imp in (Get-Imports $f)) {
            $k = $imp.ToLower(); if ($seen.ContainsKey($k)) { continue }; $seen[$k] = 1
            if ($k -match '^(api|ext)-ms-') { continue }
            $local = Join-Path $Dir $imp
            if (Test-Path $local) { $queue.Enqueue($local) }
            elseif (-not (Test-Path (Join-Path $sys $imp))) { $missing.Add("$imp (needed by $(Split-Path $f -Leaf))") }
        }
    }
    $out.Add("imports not found in package or System32: " + ($(if ($missing.Count) { $missing -join ', ' } else { 'none' })))
    $out.Add("PrusaSlicer.dll imports: " + ((Get-Imports (Join-Path $Dir 'PrusaSlicer.dll')) -join ', '))
} else { $out.Add('dumpbin not found') }

foreach ($n in 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll') {
    $p = Join-Path $Dir $n; $s = Join-Path $sys $n
    $pv = if (Test-Path $p) { (Get-Item $p).VersionInfo.FileVersion } else { '-' }
    $sv = if (Test-Path $s) { (Get-Item $s).VersionInfo.FileVersion } else { '-' }
    $out.Add("$n package: $pv, System32: $sv")
}

# Retry with the app-local VC++ runtime removed (falls back to the system copy).
$alt = "$Dir-nocrt"
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $alt
Copy-Item -Recurse $Dir $alt
Get-ChildItem $alt -File | Where-Object { $_.Name -match '^(msvcp140|vcruntime140|concrt140|vccorlib140)' } | Remove-Item
$null = Invoke-Slice $alt 'without app-local CRT'

$text = ($out -join "`n")
Write-Host $text
$esc = $text.Replace('%', '%25').Replace("`r", '%0D').Replace("`n", '%0A')
Write-Host "::warning title=Windows smoke test failed::$esc"
exit 1
