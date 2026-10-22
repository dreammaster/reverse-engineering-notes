# Creates a fresh IDA database for one of the original Yendorian Tales Book I
# executables in yendor1\game, runs the auto-analysis, and exports the
# disassembly listing (<name>.asm) and IDC dump (<name>.idc) next to it.
#
# Usage (from yendor1\ida_scripts):
#   .\create_idb.ps1 SW.EXE            # -> yendor1\sw.idb / sw.asm / sw.idc
#   .\create_idb.ps1 PROG4.EXE
#   .\create_idb.ps1 -All              # every *.EXE / *.DRV in yendor1\game
#   .\create_idb.ps1 SW.EXE -Force     # rebuild over an existing database
#
# Refuses to overwrite an existing .idb unless -Force is given: a database
# that has had renames/comments applied is the project's real work product
# (the .idb itself is gitignored; the .asm/.idc exports are what's tracked).
#
# Single idat.exe pass: -c creates the database from the input file, -o puts
# it under yendor1\ rather than next to the original, and the usual
# batch_run_and_export.py driver (with the no-op export_only.py target) waits
# for auto-analysis, exports the .asm/.idc and saves.

param(
    [Parameter(Position = 0)]
    [string]$Exe,

    [switch]$All,
    [switch]$Force
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$GameDir = Join-Path $RootDir "game"
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"
$Target = Join-Path $ScriptsDir "export_only.py"
$Log = Join-Path $ScriptsDir "batch_run_and_export.log"

if ($All) {
    $inputs = Get-ChildItem $GameDir -File | Where-Object { $_.Extension -in ".exe", ".drv" } | Sort-Object Name
} elseif ($Exe) {
    $path = if (Test-Path $Exe) { $Exe } else { Join-Path $GameDir $Exe }
    if (-not (Test-Path $path)) { Write-Error "Input not found: $path"; exit 1 }
    $inputs = @(Get-Item $path)
} else {
    Write-Error "Give an executable name (e.g. SW.EXE) or -All"
    exit 1
}

foreach ($in in $inputs) {
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($in.Name).ToLower()
    $idb = Join-Path $RootDir ($stem + ".idb")
    if ((Test-Path $idb) -and -not $Force) {
        Write-Host "[skip] $idb already exists (use -Force to rebuild)"
        continue
    }
    Write-Host "[*] $($in.Name) -> $idb"
    # -c: discard any existing db and start fresh; -o: output database path.
    # Anything that isn't an MZ executable (VIDP16.DRV is a flat code blob
    # that IDA would otherwise mislabel as a DOS device driver) is loaded
    # with the Binary loader as 16-bit x86 at offset 0; it has no entry point
    # of its own, so seed it afterwards (see seed_vidp16.py).
    $head = [System.IO.File]::ReadAllBytes($in.FullName)[0..1]
    $loaderArgs = @()
    if (-not ($head[0] -eq 0x4D -and $head[1] -eq 0x5A)) {
        $loaderArgs = @("-TBinary file", "-pmetapc")
    }
    & $IdatExe -A -c @loaderArgs "-o$idb" -S"$Driver $Target" $in.FullName
    foreach ($ext in ".idb", ".asm", ".idc") {
        $f = Join-Path $RootDir ($stem + $ext)
        if (Test-Path $f) { Write-Host ("    {0,-12} {1,10:N0} bytes" -f ($stem + $ext), (Get-Item $f).Length) }
        else { Write-Host "    $stem$ext MISSING" }
    }
}

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content $Log -Tail 15
