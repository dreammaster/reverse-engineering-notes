# Builds mm2\drv\<NAME>.idb (+ .asm/.idc) for every *.DRV in the game directory (16-bit raw binary).
param([string]$GameDir = "D:\GOG Games\Might and Magic 2")
$Root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$Idat = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Runner = Join-Path $Root "ida_scripts\run_ida_script.ps1"
$Out = Join-Path $Root "mm2\drv"
New-Item -ItemType Directory -Force $Out | Out-Null
foreach ($f in Get-ChildItem $GameDir -Filter *.DRV) {
    $name = $f.BaseName
    $idb = Join-Path $Out "$name.idb"
    Remove-Item $idb -ErrorAction SilentlyContinue
    $tmp = Join-Path $Out $f.Name
    Copy-Item $f.FullName $tmp -Force
    & $Idat -A -c -B "-T`"Binary file`"" "-o$idb" $tmp | Out-Null
    # -B writes <name>.asm next to the input; that raw dump is replaced by the scripted export below
    & $Runner -Idb $idb -ScriptName (Join-Path $Root "mm2\ida_scripts\load_driver.py") | Select-String "jump table|code starts|EXCEPTION|Error"
    Remove-Item $tmp -ErrorAction SilentlyContinue
}
