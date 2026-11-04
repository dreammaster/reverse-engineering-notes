# Creates a fresh .idb from an unpacked executable/driver under unpacked\,
# runs the initial auto-analysis, and exports the first .asm/.idc.
# The .idb (gitignored) and its exports land in disasm\<game>\.
#
# Usage:
#   .\create_idb.ps1 -Game cadaver2 -Exe GAME.EXE -Stem game
#   .\create_idb.ps1 -Game cadaver2 -Exe ADLIB.BIN -Stem adlib_bin -Binary
#
# -Binary loads the file with IDA's "Binary file" loader as raw 16-bit
# 8086 code at segment 1000h, offset 0 (the sound drivers start with
# "call near; retf" entry stubs, i.e. they are position-independent from
# offset 0 of whatever segment the game loads them to).  The loader
# makes the segment 32-bit headlessly, so init_binary16.py fixes that up.
#
# Refuses to overwrite an existing .idb: delete it by hand to start over.

param(
    [Parameter(Mandatory = $true)] [ValidateSet("cadaver1", "cadaver2")] [string]$Game,
    [Parameter(Mandatory = $true)] [string]$Exe,
    [Parameter(Mandatory = $true)] [string]$Stem,
    [switch]$Binary
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"
$Identify = Join-Path $ScriptsDir "identify.py"
$InitBinary = Join-Path $ScriptsDir "init_binary16.py"

$InFile = Join-Path $RootDir "unpacked\$Game\$Exe"
$OutDir = Join-Path $RootDir "disasm\$Game"
$Idb = Join-Path $OutDir "$Stem.idb"

if (-not (Test-Path $InFile)) { Write-Error "Not found: $InFile (run tools\unpack_exes.ps1)"; exit 1 }
if (Test-Path $Idb) { Write-Error "$Idb already exists"; exit 1 }
New-Item -ItemType Directory -Force $OutDir | Out-Null

$idaArgs = @("-A", "-o$Idb", "-S$Driver $Identify")
if ($Binary) { $idaArgs = @("-A", "-T`"Binary file`"", "-pmetapc", "-b1000", "-o$Idb", "-S$Driver $InitBinary") }

& $IdatExe @idaArgs $InFile

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 40
