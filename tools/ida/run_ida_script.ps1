# Runs an IDA script headlessly against a database under ida\, then re-exports its .asm/.idc.
#   .\run_ida_script.ps1 -Create -Idb tww\TWW -Exe D:\Games\...\TWW.exe -ScriptName analyse_init.py
#   .\run_ida_script.ps1 -Idb tww\TWW -ScriptName some_script.py [-NoExport]
# The IDA GUI must be closed first. The step trace is in batch_run_and_export.log.
param(
    [Parameter(Mandatory = $true)][string]$Idb,
    [Parameter(Mandatory = $true)][string]$ScriptName,
    [switch]$NoExport,
    [switch]$Create,
    [string]$Exe
)
$ScriptsDir = $PSScriptRoot
$IdaDir = Join-Path (Split-Path (Split-Path $ScriptsDir -Parent) -Parent) "ida"
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"
if (-not (Test-Path $ScriptName)) { $ScriptName = Join-Path $ScriptsDir $ScriptName }
$ScriptName = (Resolve-Path $ScriptName).Path
$driverArgs = "$Driver $ScriptName"
if ($NoExport) { $driverArgs = "$driverArgs noexport" }
$Out = Join-Path $IdaDir ([System.IO.Path]::ChangeExtension($Idb, $null).TrimEnd('.'))
if ($Create) {
    if (-not $Exe) { Write-Error "-Create needs -Exe"; exit 1 }
    & $IdatExe -A -c "-o$Out.idb" -S"$driverArgs" $Exe | Out-Null
} else {
    & $IdatExe -A -S"$driverArgs" "$Out.idb" | Out-Null
}
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 30
