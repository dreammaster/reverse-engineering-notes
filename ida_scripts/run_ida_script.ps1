# Runs an IDA script headlessly against one of the repo's .idb databases,
# then re-exports its .asm/.idc, without opening the GUI.
#
# Usage:
#   .\run_ida_script.ps1 -Idb "fod\Fountain of Dreams KEH.idb" -ScriptName identify.py -NoExport
#   .\run_ida_script.ps1 -Idb KEH -ScriptName apply_renames_keh.py
#   .\run_ida_script.ps1 -Idb FOD -ScriptName C:\path\to\one_off.py
#
# -Idb accepts a path (absolute, or relative to the repo root) or a short
# alias: FOD / KEH (Fountain of Dreams), WL / WLLAUNCH (Wasteland 1).
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

$aliases = @{
    "FOD"      = "fod\Fountain of Dreams FOD.idb"
    "KEH"      = "fod\Fountain of Dreams KEH.idb"
    "WL"       = "wasteland1\Wasteland.idb"
    "WLLAUNCH" = "wasteland1\Wasteland Launcher.idb"
}
if ($aliases.ContainsKey($Idb.ToUpper())) {
    $Idb = $aliases[$Idb.ToUpper()]
}

if (-not (Test-Path $Idb)) {
    $Idb = Join-Path $RootDir $Idb
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
$ScriptName = (Resolve-Path $ScriptName).Path

# Pre-flight lock check: idat.exe opening an .idb the GUI already has open
# races the GUI's in-memory copy and can produce a silently partial result.
try {
    $lockCheck = [System.IO.File]::Open($Idb, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    $lockCheck.Close()
} catch {
    Write-Error "$Idb appears to be open elsewhere (IDA GUI still running?). Close it first."
    exit 1
}

# Paths contain spaces, so quote the script args inside -S.
$driverArgs = "`"$Driver`" `"$ScriptName`""
if ($NoExport) {
    $driverArgs = "$driverArgs noexport"
}

& $IdatExe -A "-S$driverArgs" $Idb

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 60
