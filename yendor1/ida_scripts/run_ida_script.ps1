# Runs an IDA script headlessly against one of the yendor1 databases, then
# re-exports its .asm/.idc, without opening the GUI.
#
# Usage (from yendor1\ida_scripts):
#   .\run_ida_script.ps1 sw identify.py -NoExport
#   .\run_ida_script.ps1 prog4 some_rename_script.py
#   .\run_ida_script.ps1 sw C:\dev\yendor\yendor1\ida_scripts\some_fix_script.py
#
# The first argument is the database name: the lower-case stem of the
# executable, i.e. sw, prog0..prog18, install, vid16 -> yendor1\<name>.idb.
# The IDA GUI must be closed first -- the .idb is locked while it's open.
# After it runs, check batch_run_and_export.log for a step-by-step trace
# (console output from idat.exe is not reliable).
#
# -NoExport is NOT a dry run: it only skips re-exporting <name>.asm/.idc and
# the explicit save_database() call in batch_run_and_export.py. idat.exe
# itself still commits database writes (set_name, add_func, op_offset, etc.)
# to the .idb on exit regardless. Use -NoExport only for genuinely read-only
# report scripts (like identify.py).

param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [Parameter(Mandatory = $true, Position = 1)]
    [string]$ScriptName,

    [switch]$NoExport
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$IdbPath = Join-Path $RootDir ($Name.ToLower() + ".idb")
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

if (-not (Test-Path $IdbPath)) {
    Write-Error "IDB not found: $IdbPath (create it first with create_idb.ps1)"
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
# doesn't fail cleanly -- it races the GUI's in-memory copy and can produce a
# silently partial/inconsistent result. Refuse to launch if it's locked.
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
