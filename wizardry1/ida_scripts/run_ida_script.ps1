<#
Run an IDA script headlessly against one of this project's .idb databases,
then re-export its .asm/.idc, without opening the GUI.

  .\run_ida_script.ps1 -Idb wiz1_interp -ScriptName some_script.py
  .\run_ida_script.ps1 -Idb wiz1_interp -ScriptName identify.py -NoExport

-Idb takes a bare stem (wiz1_interp) or a path to the .idb.
The IDA GUI must be closed first -- the .idb is locked while it is open.
See batch_run_and_export.log for the step-by-step trace.
#>
param(
    [Parameter(Mandatory = $true)][string]$Idb,
    [Parameter(Mandatory = $true)][string]$ScriptName,
    [switch]$NoExport
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

if (-not (Test-Path $Idb)) {
    $candidate = Join-Path $RootDir $Idb
    if (-not (Test-Path $candidate)) { $candidate = Join-Path $RootDir "$Idb.idb" }
    $Idb = $candidate
}
if (-not (Test-Path $Idb)) { Write-Error "IDB not found: $Idb"; exit 1 }
$Idb = (Resolve-Path $Idb).Path

if (-not (Test-Path $ScriptName)) { $ScriptName = Join-Path $ScriptsDir $ScriptName }
if (-not (Test-Path $ScriptName)) { Write-Error "Script not found: $ScriptName"; exit 1 }

try {
    $lock = [System.IO.File]::Open($Idb, 'Open', 'ReadWrite', 'None'); $lock.Close()
} catch {
    Write-Error "$Idb is open elsewhere (IDA GUI running?). Close it first."; exit 1
}

$driverArgs = "$Driver $ScriptName"
if ($NoExport) { $driverArgs = "$driverArgs noexport" }

& $IdatExe -A -S"$driverArgs" $Idb

Write-Host "`n--- batch_run_and_export.log (tail) ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 40
