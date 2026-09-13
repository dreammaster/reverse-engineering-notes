# Runs an IDA script headlessly against ultima.idb, then re-exports
# ultima.asm/.idc, without opening the GUI.
#
# Usage:
#   .\run_ida_script.ps1 apply_structs.py
#   .\run_ida_script.ps1 apply_renames.py
#   .\run_ida_script.ps1 identify.py -NoExport
#   .\run_ida_script.ps1 C:\dev\ultima3\ida_scripts\some_fix_script.py
#
# IDA GUI must be closed first -- the .idb is locked while it's open.
# After it runs, check batch_run_and_export.log for a step-by-step
# trace (console output from idat.exe is not reliable).

param(
    [Parameter(Mandatory = $true)]
    [string]$ScriptName,

    [switch]$NoExport
)

$ScriptsDir = $PSScriptRoot
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$IdbPath = "C:\dev\ultima3\ultima.idb"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

if (-not (Test-Path $ScriptName)) {
    $ScriptName = Join-Path $ScriptsDir $ScriptName
}
if (-not (Test-Path $ScriptName)) {
    Write-Error "Script not found: $ScriptName"
    exit 1
}

# Pre-flight lock check: idat.exe opening an .idb the GUI already has open
# doesn't fail cleanly -- it races the GUI's in-memory copy and can produce
# a silently partial/inconsistent result (this bit ultima2 once: the last
# entry in an apply_structs.py run vanished with zero error output). Refuse
# to even launch idat.exe if something else already has the .idb open
# exclusively.
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
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 30
