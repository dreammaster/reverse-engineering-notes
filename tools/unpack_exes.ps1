# Produces unpacked/ from the retail game folders (cadaver1/, cadaver2/).
#
# Both games ship their executables compressed: Cadaver 1 with PKLITE
# 1.12, Cadaver 2 with Rob Northen's Pro-Pack (RNC).  IDA only sees the
# decompression stub in the packed files, so everything is unpacked first
# and the IDBs are built from unpacked/.
#
#   - .EXE/.COM  -> UNP 4.11 (C:\dev\unpackers\UNP.EXE) run under DOSBox
#   - Cadaver 2 *.BIN sound drivers are bare RNC1 blobs -> tools\rnc1.py
#   - Cadaver 1 *.BIN are stored uncompressed and are just copied
#   - files UNP reports as not packed (cadaver2 CADAVER.EXE, SETUP.EXE)
#     are copied as is
#
# Usage:  .\tools\unpack_exes.ps1

$Root = Split-Path $PSScriptRoot -Parent
$Unp = "C:\dev\unpackers\UNP.EXE"
$DosBox = "C:\dev\dosbox-0.74-3\visualc_net\Debug\dosbox.exe"
$Work = Join-Path $env:TEMP "cadaver_unpack"

$Jobs = @{
    cadaver1 = @("CAD.EXE", "INTRO.EXE", "CADAVER.COM", "SETUP.COM")
    cadaver2 = @("GAME.EXE", "INTRO.EXE", "CADAVER.EXE", "SETUP.EXE")
}

$env:SDL_VIDEODRIVER = "dummy"
$env:SDL_AUDIODRIVER = "dummy"

foreach ($game in $Jobs.Keys) {
    $src = Join-Path $Root $game
    $dst = Join-Path $Root "unpacked\$game"
    $tmp = Join-Path $Work $game
    New-Item -ItemType Directory -Force $dst, $tmp | Out-Null
    Copy-Item $Unp $tmp -Force

    $lines = @("@echo off", "mount c $tmp", "c:")
    $i = 0
    foreach ($f in $Jobs[$game]) {
        Copy-Item (Join-Path $src $f) $tmp -Force
        # UNP output names are limited to 8.3, so use a numbered stem
        $ext = [IO.Path]::GetExtension($f)
        $lines += "unp $f U$i$ext >> LOG.TXT"
        $i++
    }
    $lines += "exit"
    $conf = Join-Path $tmp "run.conf"
    ("[sdl]", "fullscreen=false", "[autoexec]") + $lines | Set-Content $conf -Encoding ascii
    Remove-Item (Join-Path $tmp "LOG.TXT") -ErrorAction SilentlyContinue
    Push-Location $tmp    # DOSBox drops stdout.txt/stderr.txt into its cwd
    & $DosBox -noconsole -conf $conf | Out-Null   # DOSBox often segfaults on exit; harmless
    Pop-Location

    $i = 0
    foreach ($f in $Jobs[$game]) {
        $ext = [IO.Path]::GetExtension($f)
        $out = Join-Path $tmp "U$i$ext"
        if (Test-Path $out) {
            Copy-Item $out (Join-Path $dst $f) -Force      # was packed
        } else {
            Copy-Item (Join-Path $src $f) (Join-Path $dst $f) -Force   # UNP: not packed
        }
        $i++
    }
    Get-Content (Join-Path $tmp "LOG.TXT") | Select-String "processing file|processed with|new file size"

    foreach ($bin in Get-ChildItem $src -Filter *.BIN) {
        $head = [IO.File]::ReadAllBytes($bin.FullName)[0..2]
        if ([Text.Encoding]::ASCII.GetString($head) -eq "RNC") {
            python (Join-Path $Root "tools\rnc1.py") $bin.FullName (Join-Path $dst $bin.Name)
        } else {
            Copy-Item $bin.FullName (Join-Path $dst $bin.Name) -Force
        }
    }
}
