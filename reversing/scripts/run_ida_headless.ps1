<#
.SYNOPSIS
    Runs an IDAPython script against the Rob Blanc 1 IDB headlessly (no IDA
    GUI), via idat.exe -A -S, then re-exports .asm/.idc and prints the tail
    of the run's log file.

.DESCRIPTION
    This is a WRAPPER around ida_headless_driver.py, not a replacement for
    it. All it does is:
      1. Resolve the target script (bare name -> looked up in ScriptsDir).
      2. SAFETY CHECK (see below) -- refuse to proceed if the .idb is
         already open elsewhere.
      3. Pick idat.exe vs idat64.exe based on the .idb/.i64 file extension.
      4. Build the idat.exe command line and run it, capturing its exit code.
      5. Print the tail of the log file the driver wrote, plus a summary.

    CRITICAL SAFETY CHECK -- read this before removing it:
    Before idat.exe is ever launched, this script attempts to open the
    target .idb with FileShare.None (i.e. it demands the SAME exclusive
    access IDA itself takes when it opens a database). If that fails, it
    means something else -- almost certainly the IDA GUI -- already has the
    database open, and this script aborts loudly instead of proceeding.

    This check exists because running idat.exe against a database the GUI
    already has open does NOT fail cleanly. idat.exe and the GUI race each
    other's in-memory copy of the database, and idat.exe's changes can be
    silently dropped -- including, in the case this was built to prevent,
    the LAST entry of a multi-operation rename batch vanishing with zero
    error output while every earlier operation in the same run applied
    fine. There is no reliable way to detect that failure mode after the
    fact from outside IDA, so the only real fix is to make it impossible to
    trigger: check exclusive access before idat.exe is invoked at all.

.PARAMETER Script
    The target IDAPython script to run. Either a bare filename (resolved
    against -ScriptsDir) or a full/relative path.

.PARAMETER IdaDir
    IDA Pro installation directory (contains idat.exe / idat64.exe).
    Defaults to "C:\Program Files\IDA Pro 8.3".

.PARAMETER IdbPath
    Path to the .idb/.i64 database. Defaults to the single .idb/.i64 found
    in the repo root (rob_blanc_1.idb).

.PARAMETER ScriptsDir
    Directory bare script names are resolved against. Defaults to this
    wrapper script's own directory (reversing/scripts).

.PARAMETER LogDir
    Directory log files are written to. Defaults to reversing/scripts/logs
    (created if missing).

.PARAMETER TailLines
    How many lines of the log file to print after the run. Default 60.

.EXAMPLE
    .\run_ida_headless.ps1 apply_structs.py

.EXAMPLE
    .\run_ida_headless.ps1 -Script apply_matches.py -TailLines 100
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Script,

    [string]$IdaDir = "C:\Program Files\IDA Pro 8.3",

    [string]$IdbPath,

    [string]$ScriptsDir = $PSScriptRoot,

    [string]$LogDir = (Join-Path $PSScriptRoot "logs"),

    [int]$TailLines = 60
)

$ErrorActionPreference = "Stop"

function Fail($msg) {
    Write-Host ""
    Write-Host "FAILED: $msg" -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Resolve the project root (two levels up from reversing/scripts) so IdbPath
# has a sensible default without hardcoding an absolute path outside of a
# parameter default.
# ---------------------------------------------------------------------------
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")

if (-not $IdbPath) {
    $candidates = @(Get-ChildItem -Path $RepoRoot -Filter "*.idb" -File) +
                  @(Get-ChildItem -Path $RepoRoot -Filter "*.i64" -File)
    if ($candidates.Count -eq 0) {
        Fail "No .idb/.i64 found in $RepoRoot and -IdbPath was not given."
    } elseif ($candidates.Count -gt 1) {
        Fail ("Multiple .idb/.i64 files found in {0}: {1}. Pass -IdbPath explicitly." -f `
              $RepoRoot, (($candidates | ForEach-Object { $_.Name }) -join ", "))
    }
    $IdbPath = $candidates[0].FullName
}

if (-not (Test-Path -LiteralPath $IdbPath -PathType Leaf)) {
    Fail "IDB not found: $IdbPath"
}
$IdbPath = (Resolve-Path -LiteralPath $IdbPath).Path

# ---------------------------------------------------------------------------
# Resolve the target script: bare name -> ScriptsDir; otherwise use as given.
# ---------------------------------------------------------------------------
if (Test-Path -LiteralPath $Script -PathType Leaf) {
    $TargetScript = (Resolve-Path -LiteralPath $Script).Path
} else {
    $candidate = Join-Path $ScriptsDir $Script
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
        $TargetScript = (Resolve-Path -LiteralPath $candidate).Path
    } else {
        Fail "Could not resolve target script '$Script' (tried it as-is and under $ScriptsDir)."
    }
}

$DriverScript = Join-Path $PSScriptRoot "ida_headless_driver.py"
if (-not (Test-Path -LiteralPath $DriverScript -PathType Leaf)) {
    Fail "Driver script not found: $DriverScript"
}
$DriverScript = (Resolve-Path -LiteralPath $DriverScript).Path

# ---------------------------------------------------------------------------
# Pick idat.exe vs idat64.exe based on the ACTUAL extension of the database
# in use -- never assume. Classic 32-bit-address-space databases are .idb
# and require idat.exe; 64-bit-address-space databases are .i64 and require
# idat64.exe. Using the wrong one refuses to open the database.
# ---------------------------------------------------------------------------
$idbExt = [System.IO.Path]::GetExtension($IdbPath).ToLowerInvariant()
switch ($idbExt) {
    ".idb" { $idatExeName = "idat.exe" }
    ".i64" { $idatExeName = "idat64.exe" }
    default { Fail "Unrecognized database extension '$idbExt' on $IdbPath (expected .idb or .i64)." }
}
$IdatExe = Join-Path $IdaDir $idatExeName
if (-not (Test-Path -LiteralPath $IdatExe -PathType Leaf)) {
    Fail "$idatExeName not found under IDA install dir: $IdaDir"
}

Write-Host "IDA install dir : $IdaDir"
Write-Host "idat executable : $IdatExe  (chosen for '$idbExt' database)"
Write-Host "IDB             : $IdbPath"
Write-Host "Driver script   : $DriverScript"
Write-Host "Target script   : $TargetScript"

# ---------------------------------------------------------------------------
# *** THE SAFETY CHECK ***
# Attempt to open the .idb with FileShare.None -- i.e. demand the same
# exclusive lock IDA itself would take. If the IDA GUI (or any other idat
# process) already has this database open, this throws and we abort BEFORE
# idat.exe is ever launched. See the .DESCRIPTION block above for why this
# is not optional.
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "Checking exclusive access to the IDB before launching idat.exe..."
try {
    $fs = [System.IO.File]::Open(
        $IdbPath,
        [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None
    )
    $fs.Close()
    $fs.Dispose()
    Write-Host "OK: no other process has $IdbPath open." -ForegroundColor Green
} catch {
    Fail (
        "Could not open $IdbPath with exclusive access -- it is almost " +
        "certainly already open in the IDA GUI (or another idat process). " +
        "Close it there first. Refusing to launch idat.exe: running it " +
        "against a database the GUI already has open does NOT fail " +
        "cleanly -- it races the GUI's in-memory copy and can silently " +
        "drop operations with zero error output.`nUnderlying error: $($_.Exception.Message)"
    )
}

# ---------------------------------------------------------------------------
# Build paths and run.
# ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$scriptStem = [System.IO.Path]::GetFileNameWithoutExtension($TargetScript)
$LogPath = Join-Path $LogDir "$scriptStem`_$timestamp.log"
$IdaOwnLogPath = Join-Path $LogDir "$scriptStem`_$timestamp.idalog"

# One single argument for -S: IDA splits this internally on whitespace to
# populate idc.ARGV, so it must be ONE token as far as idat.exe's own argv
# is concerned. None of our paths contain spaces (repo root has none), so
# passing it as one PowerShell string element is sufficient -- PowerShell's
# `&` call operator delivers each array element as one native-process
# argument regardless of embedded spaces, with no extra quoting needed here.
$sArg = "-S$DriverScript $TargetScript $LogPath"

Write-Host ""
Write-Host "Log file        : $LogPath"
Write-Host "IDA's own log   : $IdaOwnLogPath"
Write-Host ""
Write-Host "Launching: `"$IdatExe`" -A $sArg -L`"$IdaOwnLogPath`" `"$IdbPath`""
Write-Host ""

& $IdatExe "-A" $sArg "-L$IdaOwnLogPath" $IdbPath
$idatExitCode = $LASTEXITCODE

Write-Host ""
Write-Host "idat.exe exited with code $idatExitCode"

if (Test-Path -LiteralPath $LogPath) {
    Write-Host ""
    Write-Host "----- tail of $LogPath (last $TailLines lines) -----"
    Get-Content -LiteralPath $LogPath -Tail $TailLines
    Write-Host "----- end of log tail -----"
} else {
    Write-Host ""
    Write-Host "WARNING: expected log file was not created: $LogPath" -ForegroundColor Yellow
    Write-Host "This means idat.exe likely failed before the driver script even started" -ForegroundColor Yellow
    Write-Host "(e.g. a bad -S argument, or idat.exe itself failed to launch)." -ForegroundColor Yellow
    if (Test-Path -LiteralPath $IdaOwnLogPath) {
        Write-Host ""
        Write-Host "----- tail of IDA's own log $IdaOwnLogPath -----"
        Get-Content -LiteralPath $IdaOwnLogPath -Tail $TailLines
        Write-Host "----- end -----"
    }
}

if ($idatExitCode -ne 0) {
    Write-Host ""
    Write-Host "Run FAILED (exit code $idatExitCode). See log above." -ForegroundColor Red
    exit $idatExitCode
} else {
    Write-Host ""
    Write-Host "Run completed successfully." -ForegroundColor Green
}
