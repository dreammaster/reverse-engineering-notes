# Runs an IDA script headlessly against one of the ultima1*.idb databases,
# then re-exports its .asm/.idc, without opening the GUI.
#
# Usage:
#   .\run_ida_script.ps1 -Idb ultima1_space -ScriptName identify.py
#   .\run_ida_script.ps1 -Idb ultima1_space -ScriptName apply_renames.py
#   .\run_ida_script.ps1 -Idb ultima1_mondain -ScriptName C:\dev\ultima1\ida_scripts\some_fix_script.py
#   .\run_ida_script.ps1 -Idb ultima1_space -ScriptName identify.py -NoExport
#
# -Idb accepts either a bare stem (ultima1_space) or a full/relative path
# to the .idb; the five known stems are: ultima1, ultima1_gen,
# ultima1_mondain, ultima1_out, ultima1_space.
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
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

if (-not (Test-Path $Idb)) {
    $candidate = Join-Path $RootDir $Idb
    if (-not (Test-Path $candidate)) {
        $candidate = Join-Path $RootDir "$Idb.idb"
    }
    $Idb = $candidate
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
