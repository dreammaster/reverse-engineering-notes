# Builds the IDA databases for Ultima VI's graphics and sound driver blobs.
#
# The *.DRV files are LZW-compressed raw 16-bit code (a table of 3-byte `jmp near` entries at offset 0, entered
# through a far pointer whose offset is 3 * the entry number).  They are unpacked by tools\lzw_extract.py into
# u6\game\drv\ and loaded with IDA's binary loader (8086, paragraph 0).  Existing databases are skipped unless -Force.
#
#   .\create_drivers.ps1
#   .\create_drivers.ps1 -Only u6_drv_mcga -Force

param(
    [string]$Only,
    [switch]$Force
)

$ScriptsDir = $PSScriptRoot
$RootDir = Split-Path $ScriptsDir -Parent
$Runner = Join-Path $ScriptsDir "run_ida_script.ps1"

$Targets = @(
    @("u6\u6_drv_mcga",   "U6MCGA.DRV"),
    @("u6\u6_drv_ega",    "U6EGA.DRV"),
    @("u6\u6_drv_cga",    "U6CGA.DRV"),
    @("u6\u6_drv_tandy",  "U6TANDY.DRV"),
    @("u6\u6_snd_adlib",  "U6ADLIB.DRV"),
    @("u6\u6_snd_cms",    "U6CMS.DRV"),
    @("u6\u6_snd_covox",  "U6COVOX.DRV"),
    @("u6\u6_snd_innova", "U6INNOVA.DRV"),
    @("u6\u6_snd_roland", "U6ROLAND.DRV"),
    @("u6\u6_snd_tmus",   "U6TMUS.DRV")
)

New-Item -ItemType Directory -Force (Join-Path $RootDir "u6\game\drv") | Out-Null
foreach ($t in $Targets) {
    $stem = $t[0]
    if ($Only -and ((Split-Path $stem -Leaf) -ne $Only)) { continue }
    $idb = Join-Path $RootDir "$stem.idb"
    if ((Test-Path $idb) -and -not $Force) {
        Write-Host "skip $stem (exists; use -Force to recreate)"
        continue
    }
    $src = Join-Path $RootDir ("u6\game\" + $t[1])
    if (-not (Test-Path $src)) { Write-Warning "missing $src"; continue }
    $bin = Join-Path $RootDir ("u6\game\drv\" + $t[1] + ".bin")
    python (Join-Path $RootDir "tools\lzw_extract.py") $src $bin | Out-Null
    Write-Host "creating $stem from $($t[1])"
    & $Runner -Create -Raw -Idb $stem -Exe $bin -ScriptName setup_driver.py | Out-Null
}
