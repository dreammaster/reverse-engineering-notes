# Runs an IDA script headlessly against ultima2.idb, then re-exports
# ultima2.asm/.idc, without opening the GUI.
#
# Usage:
#   .\run_ida_script.ps1 apply_structs.py
#   .\run_ida_script.ps1 apply_renames.py
#   .\run_ida_script.ps1 C:\dev\ultima2\ida_scripts\some_fix_script.py
#
# IDA GUI must be closed first -- the .idb is locked while it's open.
# After it runs, check batch_run_and_export.log for a step-by-step
# trace (console output from idat.exe is not reliable).

param(
    [Parameter(Mandatory = $true)]
    [string]$ScriptName
)

$ScriptsDir = $PSScriptRoot
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$IdbPath = "C:\dev\ultima2\ultima2.idb"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

if (-not (Test-Path $ScriptName)) {
    $ScriptName = Join-Path $ScriptsDir $ScriptName
}
if (-not (Test-Path $ScriptName)) {
    Write-Error "Script not found: $ScriptName"
    exit 1
}

& $IdatExe -A -S"$Driver $ScriptName" $IdbPath

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 30
