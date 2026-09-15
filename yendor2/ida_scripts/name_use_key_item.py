"""
Traced sub_1AEF8, another of UseItem's dispatch branches (called from
UseItem+0x1B1, a spot beyond the 4 main type-handler branches already
named). It passes the item's own type-flags field (es:[si+0x10] -- the
same field UseItem's outer dispatch reads) directly as the lock/object
id argument to the newly-named LoadLockState. This is a key item: its
own catalog "type" value doubles as the numbered door/lock it opens.

-> UseKeyItem

Run via:
    .\run_ida_script.ps1 name_use_key_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AEF8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseKeyItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseKeyItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "UseItem dispatch branch for key items: passes the item's own "
    "type-flags field (es:[si+0x10]) directly as the lock/object id "
    "to LoadLockState -- the item's catalog 'type' value doubles as "
    "which numbered door/lock it opens.",
    False,
)
