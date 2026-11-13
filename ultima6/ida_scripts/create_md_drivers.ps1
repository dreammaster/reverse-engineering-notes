# Builds the IDA databases for Martian Dreams' video drivers (the VGA blocks of TM.DRV and SUPERTM.DRV).
#
# tools\extract_md_drivers.py unpacks the blocks into md\game\drv\; IDA's binary loader reads them as 8086 code at paragraph 0 (setup_md_driver.py makes the segment 16-bit and names the
# entries).  Existing databases are skipped unless -Force.
#
#   .\create_md_drivers.ps1
#   .\create_md_drivers.ps1 -Force

param(
    [switch]$Force
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$Runner = Join-Path $ScriptsDir "run_ida_script.ps1"

python (Join-Path $RootDir "tools\extract_md_drivers.py") (Join-Path $RootDir "md\game\MARTIAN") (Join-Path $RootDir "md\game\drv") | Out-Null
$Targets = @(
    @("md\md_drv_tm_vga",      "TM_0.bin"),
    @("md\md_drv_supertm_vga", "SUPERTM_0.bin")
)
foreach ($t in $Targets) {
    $idb = Join-Path $RootDir ($t[0] + ".idb")
    if ((Test-Path $idb) -and -not $Force) {
        Write-Host "skip $($t[0]) (exists; use -Force to recreate)"
        continue
    }
    $bin = Join-Path $RootDir ("md\game\drv\" + $t[1])
    Write-Host "creating $($t[0]) from $($t[1])"
    & $Runner -Create -Raw -Idb $t[0] -Exe $bin -ScriptName setup_md_driver.py | Out-Null
}
