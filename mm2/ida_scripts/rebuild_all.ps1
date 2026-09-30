# Full refresh: apply names to mm2.idb, rename the thunks after them, then rebuild every overlay
# database from it (overlay databases are regenerated from mm2/names/*.tsv, so keep manual work
# in the .tsv files or in names/ -- a rebuild discards GUI-only edits in ovl\*.idb).
$Root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$Run = Join-Path $Root "ida_scripts\run_ida_script.ps1"
$Main = Join-Path $Root "mm2\mm2.idb"
foreach ($s in "apply_names.py", "define_thunk_targets.py", "resolve_strings.py", "apply_names.py") {
    & $Run -Idb $Main -ScriptName (Join-Path $Root "mm2\ida_scripts\$s") | Out-Null
}
& (Join-Path $PSScriptRoot "build_overlays.ps1") -Force
