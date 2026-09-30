# Runs an IDA script headlessly against one of the .idb databases in this
# repo, then re-exports its .asm/.idc, without opening the GUI.
#
# Usage (from C:\dev\might_and_magic):
#   .\ida_scripts\run_ida_script.ps1 -Idb mm2\mm2.idb -ScriptName mm2\ida_scripts\identify.py -NoExport
#   .\ida_scripts\run_ida_script.ps1 -Idb mm2\mm2.idb -ScriptName C:\path\to\script.py
#
# -Idb        path to the .idb (relative to the repo root, or absolute)
# -ScriptName path to the script (relative to the repo root, or absolute)
# -NoExport   read-only run: skip the .asm/.idc export and the save
#
# The IDA GUI must be closed first -- the .idb is locked while it is open.
# After it runs, check ida_scripts\batch_run_and_export.log for a
# step-by-step trace (console output from idat.exe is not reliable).

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

foreach ($name in 'Idb', 'ScriptName') {
    $p = Get-Variable $name -ValueOnly
    if (-not (Test-Path $p)) { $p = Join-Path $RootDir $p }
    if (-not (Test-Path $p)) { Write-Error "$name not found: $p"; exit 1 }
    Set-Variable $name (Resolve-Path $p).Path
}

# idat.exe on an .idb the GUI already has open doesn't fail cleanly -- it can
# produce a silently partial result. Refuse to launch if it is locked.
try {
    $lockCheck = [System.IO.File]::Open($Idb, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    $lockCheck.Close()
} catch {
    Write-Error "$Idb appears to be open elsewhere (IDA GUI still running?). Close it first."
    exit 1
}

$driverArgs = "$Driver $ScriptName"
if ($NoExport) { $driverArgs = "$driverArgs noexport" }

& $IdatExe -A -S"$driverArgs" $Idb

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 60
