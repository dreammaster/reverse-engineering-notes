# Runs an IDA script headlessly against one of the disasm\*\*.idb databases,
# then re-exports its .asm/.idc, without opening the GUI.
#
# Usage:
#   .\run_ida_script.ps1 -Idb game -ScriptName identify.py
#   .\run_ida_script.ps1 -Idb cadaver1\cad -ScriptName apply_renames_cad.py
#   .\run_ida_script.ps1 -Idb game -ScriptName C:\path\to\some_fix_script.py
#   .\run_ida_script.ps1 -Idb game -ScriptName identify.py -NoExport
#
# -Idb accepts a bare stem (game), a game-qualified stem (cadaver2\game --
# needed because intro/setup/cadaver exist in both games), or a full/relative
# path to the .idb.  See disasm\ for the available databases.
#
# IDA GUI must be closed first -- the .idb is locked while it's open.
# After it runs, check batch_run_and_export.log for a step-by-step
# trace (console output from idat.exe is not reliable).

param(
    [Parameter(Mandatory = $true)]
    [string]$Idb,

    [Parameter(Mandatory = $true)]
    [string]$ScriptName,

    [switch]$NoExport
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$DisasmDir = Join-Path $RootDir "disasm"
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

if (-not (Test-Path $Idb)) {
    $stem = $Idb -replace '\.idb$', ''
    $matches = @()
    $direct = Join-Path $DisasmDir "$stem.idb"
    if (Test-Path $direct) {
        $matches = @($direct)
    } else {
        $matches = @(Get-ChildItem $DisasmDir -Recurse -Filter "$stem.idb" | ForEach-Object { $_.FullName })
    }
    if ($matches.Count -gt 1) {
        Write-Error "Ambiguous -Idb '$Idb', matches: $($matches -join ', '). Qualify with the game folder, e.g. cadaver2\$stem"
        exit 1
    }
    if ($matches.Count -eq 1) { $Idb = $matches[0] }
}
if (-not (Test-Path $Idb)) {
    Write-Error "IDB not found: $Idb"
    exit 1
}
$Idb = (Resolve-Path $Idb).Path

if (-not (Test-Path $ScriptName)) {
    $ScriptName = Join-Path $ScriptsDir $ScriptName
}
if (-not (Test-Path $ScriptName)) {
    Write-Error "Script not found: $ScriptName"
    exit 1
}

# Pre-flight lock check: idat.exe opening an .idb the GUI already has open
# doesn't fail cleanly -- it races the GUI's in-memory copy and can produce
# a silently partial/inconsistent result. Refuse to even launch idat.exe if
# something else already has the .idb open exclusively.
try {
    $lockCheck = [System.IO.File]::Open($Idb, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    $lockCheck.Close()
} catch {
    Write-Error "$Idb appears to be open elsewhere (IDA GUI still running?). Close it first -- refusing to run idat.exe against a locked database."
    exit 1
}

$driverArgs = "$Driver $ScriptName"
if ($NoExport) {
    $driverArgs = "$driverArgs noexport"
}

& $IdatExe -A -S"$driverArgs" $Idb

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 40
