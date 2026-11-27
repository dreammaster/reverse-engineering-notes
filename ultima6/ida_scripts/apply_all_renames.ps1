# Applies every apply_renames_<stem>.py to its database (each one re-exports the .asm/.idc and saves),
# so that all listings carry the names the port has found.  Idempotent.  Close the IDA GUI first.
#
#   .\apply_all_renames.ps1               # every database that has an apply_renames script
#   .\apply_all_renames.ps1 -Only md_game
#
# The databases are found by run_ida_script.ps1 from the stem (u6, md, se folders); the script name is apply_renames_<stem>.py.

param(
    [string]$Only
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent

foreach ($script in Get-ChildItem (Join-Path $ScriptsDir "apply_renames_*.py")) {
    $stem = $script.BaseName.Substring("apply_renames_".Length)
    if ($Only -and $stem -ne $Only) { continue }
    Write-Host "== $stem"
    & (Join-Path $ScriptsDir "run_ida_script.ps1") -Idb $stem -ScriptName $script.FullName | Select-String "renamed|FAILED"
}
