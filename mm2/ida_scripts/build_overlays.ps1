# (Re)builds the per-overlay databases mm2\ovl\<NAME>.idb (+ .asm/.idc) from mm2\mm2.idb.
#
#   .\mm2\ida_scripts\build_overlays.ps1                 # all 14 overlays that don't exist yet
#   .\mm2\ida_scripts\build_overlays.ps1 -Only 2PLAY     # just one
#   .\mm2\ida_scripts\build_overlays.ps1 -Force          # DESTROYS existing overlay idbs (and any
#                                                        # hand-made edits in them) and starts over
# Close the IDA GUI first.  See mm2\docs\exe-layout.md for why overlays get their own databases.

param([string[]]$Only, [switch]$Force)

$Root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$Runner = Join-Path $Root "ida_scripts\run_ida_script.ps1"
$Main = Join-Path $Root "mm2\mm2.idb"
$OvlDir = Join-Path $Root "mm2\ovl"
$Overlays = "1MENU1","1MENU2","1RETINN","2BRAIN","2CAST1","2CAST2","2CAVES","2CMDS","2COMBAT","2MISC","2MISC2","2PLAY","2SMITH","2TEMPLE"
if ($Only) { $Overlays = $Overlays | Where-Object { $Only -contains $_ } }

New-Item -ItemType Directory -Force $OvlDir | Out-Null
& $Runner -Idb $Main -ScriptName (Join-Path $Root "mm2\ida_scripts\export_names.py") -NoExport | Out-Null

foreach ($o in $Overlays) {
    $idb = Join-Path $OvlDir "$o.idb"
    if ((Test-Path $idb) -and -not $Force) { Write-Host "skip $o (exists; use -Force to rebuild)"; continue }
    Copy-Item $Main $idb -Force
    Write-Host "== $o"
    & $Runner -Idb $idb -ScriptName (Join-Path $Root "mm2\ida_scripts\load_overlay.py") | Select-String "overlay|seeded|imported|EXCEPTION|Error"
}
