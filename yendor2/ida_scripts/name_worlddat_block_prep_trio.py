"""
Names 3 more resource-block-setup stubs (the same family LoadMasterPalette
belongs to, per file-formats.md) -- distinct from the already-named
WorldDat_setBlock1-6 cluster (different fixed table addresses,
different record-size sources: these three use the _blockSize1/2/4
globals rather than a hardcoded size).

sub_27C96 (called from PreloadWorldDataTable): configures a FileEntry
struct (bx, ax=caller's size param) from table 0xCE5F, record size
_blockSize1. -> PrepareWorldDataTableBlockRead

sub_27CFE (called from PreloadMonsterStatsTable): same shape, table
0xCE5B, record size _blockSize2, bx hardcoded to the fixed FileEntry
0x9043. -> PrepareMonsterStatsTableBlockRead

sub_28000 (called from BuildClueLocationSuffix, alongside a separate
WorldDat_setBlock3 call earlier in the same function): same shape,
table 0xCDF7, record size _blockSize4. -> PrepareClueLocationSuffixBlockRead

Run via:
    .\run_ida_script.ps1 name_worlddat_block_prep_trio.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, table, size_src, caller in [
    (0x27C96, "PrepareWorldDataTableBlockRead", "0xCE5F", "_blockSize1", "PreloadWorldDataTable"),
    (0x27CFE, "PrepareMonsterStatsTableBlockRead", "0xCE5B", "_blockSize2", "PreloadMonsterStatsTable"),
    (0x28000, "PrepareClueLocationSuffixBlockRead", "0xCDF7", "_blockSize4", "BuildClueLocationSuffix"),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(
        ea,
        f"Resource-block-setup stub (same family as LoadMasterPalette, "
        f"distinct from the WorldDat_setBlock1-6 cluster): configures "
        f"a FileEntry struct from fixed table {table}, record size "
        f"{size_src}. Called from {caller}.",
        False,
    )
