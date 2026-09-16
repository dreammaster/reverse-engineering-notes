"""
Names sub_12449, called from InitGame right before
PreloadMonsterStatsTable: the same shape (allocate a large one-time
buffer -- here 0x677 paragraphs, ~25.9KB -- wire it into a FileEntry,
read with errorCode=9), but via a different resource-setup stub
(sub_27C96, using a WORLD.DAT block-descriptor table at 0xCE5F, one
entry before the 0xCE63 table WorldDat_setBlock5 uses) -- so a
different WORLD.DAT block's data, not confirmed which one.
-> PreloadWorldDataTable

Run via:
    .\run_ida_script.ps1 name_preload_world_data.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12449
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PreloadWorldDataTable", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PreloadWorldDataTable': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Same shape as PreloadMonsterStatsTable (allocate a large one-time "
    "buffer, read with errorCode=9) but via a different resource-setup "
    "stub (sub_27C96, table at 0xCE5F) -- a different WORLD.DAT block, "
    "not confirmed which. Called from InitGame.",
    False,
)
