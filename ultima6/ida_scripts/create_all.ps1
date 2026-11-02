# Builds the initial IDA databases (+ .asm/.idc) for every Ultima 6 / Martian
# Dreams / Savage Empire executable we analyse.  Databases that already exist
# are skipped, because re-creating one throws away all the naming work done in
# it; pass -Force only to deliberately start a database over.
#
#   .\create_all.ps1              # create whatever is missing
#   .\create_all.ps1 -Only u6_game
#   .\create_all.ps1 -Only u6_game -Force
#
# Original executables stay untouched in <game>\game\ (git-ignored).  Ultima 6's
# GAME.EXE is EXEPACK-compressed, so it is unpacked first (tools\unexepack.py)
# into GAME_UNP.EXE next to it and that is what IDA loads.

param(
    [string]$Only,
    [switch]$Force
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$Runner = Join-Path $ScriptsDir "run_ida_script.ps1"

# stem, original executable (relative to ultima6\)
$Targets = @(
    @("u6\u6_ultima6", "u6\game\ULTIMA6.EXE"),     # launcher, chains to U.EXE
    @("u6\u6_u",       "u6\game\U.EXE"),           # title/main menu, intro, character creation
    @("u6\u6_game",    "u6\game\GAME_UNP.EXE"),    # the game proper (unpacked from GAME.EXE)
    @("u6\u6_end",     "u6\game\END.EXE"),         # ending sequence
    @("md\md_game",    "md\game\MARTIAN\GAME.EXE"),
    @("md\md_martian", "md\game\MARTIAN\MARTIAN.EXE"),
    @("se\se_game",    "se\game\SAVAGE\GAME.EXE"),
    @("se\se_savage",  "se\game\SAVAGE\SAVAGE.EXE")
)

$packed = Join-Path $RootDir "u6\game\GAME.EXE"
$unpacked = Join-Path $RootDir "u6\game\GAME_UNP.EXE"
if ((Test-Path $packed) -and -not (Test-Path $unpacked)) {
    python (Join-Path $RootDir "tools\unexepack.py") $packed $unpacked
}

foreach ($t in $Targets) {
    $stem = $t[0]
    if ($Only -and ((Split-Path $stem -Leaf) -ne $Only)) { continue }
    $idb = Join-Path $RootDir "$stem.idb"
    if ((Test-Path $idb) -and -not $Force) {
        Write-Host "skip $stem (exists; use -Force to recreate)"
        continue
    }
    $exe = Join-Path $RootDir $t[1]
    if (-not (Test-Path $exe)) {
        Write-Warning "missing original executable $exe"
        continue
    }
    Write-Host "creating $stem from $($t[1])"
    & $Runner -Create -Idb $stem -Exe $exe -ScriptName identify.py | Out-Null
}
