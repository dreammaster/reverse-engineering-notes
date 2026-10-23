# Make a whole-game reference run (for test_game) with yemu.
#   make_game_ref.ps1 -Name convo12 -Script src1\tests\refs\swap_convo.txt -GameDir <game dir> -OutRoot <where the references go>
# -GameDir is a copy of the game (yendor1\game with CURRGAME\ set to the state the run starts from, e.g. the assembled party); to run
# another program in place of PROG4 copy PROGn.EXE over PROG4.EXE in it (test_game then needs YENDOR1_SWAP=n). Run from the repository root.
param([string]$Name, [string]$Script, [string]$GameDir, [string]$OutRoot)
$run = Join-Path $OutRoot "run_$Name"
$ref = Join-Path $OutRoot $Name
Remove-Item -Recurse -Force $run, "$run.work", $ref -ErrorAction SilentlyContinue
New-Item -ItemType Directory $run | Out-Null
& src1\build\tools\yemu.exe $GameDir SW.EXE --fonts src1\fonts --script $Script --out $run --dump $run --log62 "$run\trace.txt" --work "$run.work" --clock 4660
New-Item -ItemType Directory "$ref\in" | Out-Null
Copy-Item "$run\trace.txt", "$run\trace.txt.events" $ref
Copy-Item -Recurse "$GameDir\CURRGAME" "$ref\in\CURRGAME"
Copy-Item -Recurse "$GameDir\SAVEGAME.A" "$ref\in\SAVEGAME.A"
