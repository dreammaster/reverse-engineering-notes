"""Define and name ultima_exodus.idb's orphaned checkDebugModeFlag
equivalent (0x15000-0x15003, 'mov al,0FFh; retn'), confirmed by its
position -- right after drawCharGlyph (0x14F90-0x15000), exactly
matching the layout in ultima.idb and ultima_bootup.idb. Unlike those
two (called once from their title-screen sequences), this copy has
zero callers anywhere in EXODUS.BIN, consistent with EXODUS.BIN having
no title/boot animation sequence to gate."""
import idc
import ida_funcs

start, end = 0x15000, 0x15003
NAME = "checkDebugModeFlag"

existing = ida_funcs.get_func(start)
if existing:
    print(f"{start:#x} already in func {idc.get_func_name(existing.start_ea)!r} -- skipping")
else:
    ok = ida_funcs.add_func(start, end)
    print(f"add_func({start:#x}, {end:#x}) -> {ok}")
    if ok:
        renamed = idc.set_name(start, NAME, idc.SN_NOWARN)
        print(f"set_name -> {renamed}")

f = ida_funcs.get_func(start)
print(f"post-state: func {f.start_ea:#x}-{f.end_ea:#x} name={idc.get_func_name(f.start_ea)!r}" if f else "post-state: NO FUNC")
