# Runs an IDA script headlessly against yendor2.idb, then re-exports
# yendor2.asm/.idc, without opening the GUI.
#
# Usage:
#   .\run_ida_script.ps1 identify.py -NoExport
#   .\run_ida_script.ps1 apply_renames.py
#   .\run_ida_script.ps1 C:\dev\yendor\yendor2\ida_scripts\some_fix_script.py
#
# IDA GUI must be closed first -- the .idb is locked while it's open.
# After it runs, check batch_run_and_export.log for a step-by-step
# trace (console output from idat.exe is not reliable).
#
# -NoExport is NOT a dry run: it only skips re-exporting yendor2.asm/.idc
# and the explicit save_database() call in batch_run_and_export.py.
# idat.exe itself still commits database writes (set_name, add_func,
# op_offset, etc.) to yendor2.idb on exit regardless -- confirmed
# 2026-09-14 by writing a comment in one -NoExport run and reading it
# back, still present, in a separate later -NoExport run. Any target
# script that mutates the database will have those mutations persist to
# the .idb even with -NoExport; only the text exports and the "did we
# save" log line are skipped. Use -NoExport for genuinely read-only
# report scripts (like identify.py) where this distinction doesn't
# matter, not as a way to try a mutating script "safely".

param(
    [Parameter(Mandatory = $true)]
    [string]$ScriptName,

    [switch]$NoExport
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$IdatExe = "C:\Program Files\IDA Pro 8.2\idat.exe"
$IdbPath = Join-Path $RootDir "yendor2.idb"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

if (-not (Test-Path $IdbPath)) {
    Write-Error "IDB not found: $IdbPath"
    exit 1
}

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
    $lockCheck = [System.IO.File]::Open($IdbPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    $lockCheck.Close()
} catch {
    Write-Error "$IdbPath appears to be open elsewhere (IDA GUI still running?). Close it first -- refusing to run idat.exe against a locked database."
    exit 1
}

$driverArgs = "$Driver $ScriptName"
if ($NoExport) {
    $driverArgs = "$driverArgs noexport"
}

& $IdatExe -A -S"$driverArgs" $IdbPath

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 40
