# Runs an IDA script headlessly against yendor3.idb, then re-exports
# yendor3.asm/.idc, without opening the GUI.
#
# Mirrors yendor2/ida_scripts/run_ida_script.ps1 -- see that file's
# header comment for the full -NoExport semantics (same driver script,
# batch_run_and_export.py, is shared/copied verbatim since it derives
# all paths from idc.get_idb_path() rather than hardcoding yendor2).
#
# Usage:
#   .\run_ida_script.ps1 some_script.py -NoExport
#   .\run_ida_script.ps1 some_script.py

param(
    [Parameter(Mandatory = $true)]
    [string]$ScriptName,

    [switch]$NoExport
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$IdatExe = "C:\Program Files\IDA Pro 8.2\idat.exe"
$IdbPath = Join-Path $RootDir "yendor3.idb"
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
