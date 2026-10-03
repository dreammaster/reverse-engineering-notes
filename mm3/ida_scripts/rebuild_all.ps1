# Rebuilds mm3\mm3.idb from the installed game:
#   1. unpack MM3.EXE (NWC LZW + EXEPACK) with tools\unpack_mm3.py,
#   2. let IDA load it (its MZ loader maps the Borland overlays),
#   3. run ida_scripts\build_idb.py (names, ds, function seeds) and export mm3.asm / mm3.idc.
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File mm3\ida_scripts\rebuild_all.ps1 [-Exe "D:\GOG Games\Might and Magic 3\MM3.EXE"]
param([string]$Exe = "D:\GOG Games\Might and Magic 3\MM3.EXE")
$Root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$Mm3 = Join-Path $Root "mm3"
$Build = Join-Path $Mm3 "build"
$Idat = "C:\Program Files\IDA Pro 8.3\idat.exe"
New-Item -ItemType Directory -Force $Build | Out-Null
python (Join-Path $Mm3 "tools\unpack_mm3.py") $Exe (Join-Path $Build "stage1.bin") (Join-Path $Build "mm3.exe")
Remove-Item (Join-Path $Build "mm3.exe.idb"), (Join-Path $Build "mm3.exe.asm") -ErrorAction SilentlyContinue
& $Idat -B -A -c (Join-Path $Build "mm3.exe") | Out-Null
Copy-Item (Join-Path $Build "mm3.exe.idb") (Join-Path $Mm3 "mm3.idb") -Force
& (Join-Path $Root "ida_scripts\run_ida_script.ps1") -Idb (Join-Path $Mm3 "mm3.idb") -ScriptName (Join-Path $Mm3 "ida_scripts\build_idb.py")
& (Join-Path $Root "ida_scriptsun_ida_script.ps1") -Idb (Join-Path $Mm3 "mm3.idb") -ScriptName (Join-Path $Mm3 "ida_scriptspply_names.py")
& (Join-Path $Root "ida_scripts\run_ida_script.ps1") -Idb (Join-Path $Mm3 "mm3.idb") -ScriptName (Join-Path $Mm3 "ida_scripts\resolve_strings.py")
& (Join-Path $Root "ida_scripts\run_ida_script.ps1") -Idb (Join-Path $Mm3 "mm3.idb") -ScriptName (Join-Path $Mm3 "ida_scripts\apply_structs.py")
$env:MM3_CC = Join-Path (Split-Path $Exe -Parent) "MM3.CC"
& (Join-Path $Root "ida_scripts\run_ida_script.ps1") -Idb (Join-Path $Mm3 "mm3.idb") -ScriptName (Join-Path $Mm3 "ida_scripts\load_driver.py")
