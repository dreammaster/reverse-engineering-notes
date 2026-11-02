# Runs an IDA script headlessly against one of the ultima6 databases, then
# re-exports its .asm/.idc, without opening the GUI.
#
# Usage (from anywhere):
#   .\run_ida_script.ps1 -Idb u6\u6_game -ScriptName identify.py -NoExport
#   .\run_ida_script.ps1 -Idb u6_game    -ScriptName apply_renames_u6_game.py
#   .\run_ida_script.ps1 -Idb md\md_game -ScriptName C:\path\to\one_off.py
#
# -Idb is a path to the .idb (extension optional) relative to the ultima6
# folder, or a bare stem (u6_game) which is searched for in u6\, md\ and se\.
#
# -Create builds a NEW database from the original executable instead of
# opening an existing one; -Idb is then the stem to create and -Exe is the
# original executable (normally under <game>\game\).
#   .\run_ida_script.ps1 -Create -Idb u6\u6_game -Exe u6\game\GAME.EXE -ScriptName identify.py
#
# -Raw (with -Create) loads a headerless 16-bit code blob instead of a DOS executable.
#
# The IDA GUI must be closed first -- the .idb is locked while it's open.
# After the run, batch_run_and_export.log has a step-by-step trace (console
# output from idat.exe is not reliable).

param(
    [Parameter(Mandatory = $true)]
    [string]$Idb,

    [Parameter(Mandatory = $true)]
    [string]$ScriptName,

    [switch]$NoExport,
    [switch]$Create,
    [switch]$Raw,
    [string]$Exe
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$IdatExe = "C:\Program Files\IDA Pro 8.3\idat.exe"
$Driver = Join-Path $ScriptsDir "batch_run_and_export.py"

function Resolve-Idb([string]$name) {
    $cands = @($name, "$name.idb")
    foreach ($base in @($RootDir, (Join-Path $RootDir "u6"), (Join-Path $RootDir "md"), (Join-Path $RootDir "se"))) {
        foreach ($c in $cands) {
            $p = Join-Path $base $c
            if (Test-Path $p -PathType Leaf) { return (Resolve-Path $p).Path }
        }
    }
    return $null
}

if (-not (Test-Path $ScriptName)) {
    $ScriptName = Join-Path $ScriptsDir $ScriptName
}
if (-not (Test-Path $ScriptName)) {
    Write-Error "Script not found: $ScriptName"
    exit 1
}
$ScriptName = (Resolve-Path $ScriptName).Path

$driverArgs = "$Driver $ScriptName"
if ($NoExport) {
    $driverArgs = "$driverArgs noexport"
}

if ($Create) {
    if (-not $Exe) { Write-Error "-Create needs -Exe"; exit 1 }
    if (-not (Test-Path $Exe)) { $Exe = Join-Path $RootDir $Exe }
    $Exe = (Resolve-Path $Exe).Path
    $Out = if ([System.IO.Path]::IsPathRooted($Idb)) { $Idb } else { Join-Path $RootDir $Idb }
    $Out = [System.IO.Path]::ChangeExtension($Out, $null).TrimEnd('.')
    # -c: new database, discarding any existing one; -o: where to put it
    # (the original executable stays untouched in game\).
    $loader = @()
    if ($Raw) {
        # a raw 16-bit code blob (the *.DRV drivers): binary loader, 8086, loaded at paragraph 0
        $loader = @("-TBinary file", "-pmetapc", "-b0")
    }
    & $IdatExe -A -c @loader "-o$Out.idb" -S"$driverArgs" $Exe | Out-Null
} else {
    $IdbPath = Resolve-Idb $Idb
    if (-not $IdbPath) {
        Write-Error "IDB not found: $Idb"
        exit 1
    }

    # Pre-flight lock check: idat.exe opening an .idb the GUI already has open
    # races the GUI's in-memory copy and can produce a silently partial result.
    try {
        $lockCheck = [System.IO.File]::Open($IdbPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
        $lockCheck.Close()
    } catch {
        Write-Error "$IdbPath appears to be open elsewhere (IDA GUI still running?). Close it first."
        exit 1
    }

    # Piping the output makes PowerShell wait for idat's self-relaunch to finish.
    & $IdatExe -A -S"$driverArgs" $IdbPath | Out-Null
}

Write-Host "`n--- batch_run_and_export.log ---"
Get-Content (Join-Path $ScriptsDir "batch_run_and_export.log") -Tail 40
