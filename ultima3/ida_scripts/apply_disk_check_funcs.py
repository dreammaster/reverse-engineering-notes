"""Define the orphaned INT13h track-9/sector-16 disk-check routine as
a real function in whichever IDB this runs against (ultima.idb or
ultima_bootup.idb only -- both have a clean, retn-terminated, exact
byte-for-byte-matching copy with zero callers anywhere). Not applied
to ultima_exodus.idb: its copy has no retn of its own and falls
straight through into plotPixel2bpp, i.e. it's not a well-formed
function there at all -- forcing an add_func boundary on it would be
exactly the kind of guess-based flow-graph edit this project has
learned to avoid for ambiguous orphaned regions.
"""
import idc
import ida_funcs

BOUNDS = {
    "ultima.idb": (0x18793, 0x187E3),
    "ultima_bootup.idb": (0x144B3, 0x14503),
}
NAME = "checkDiskCopyProtection"

idb = idc.get_idb_path()
key = [k for k in BOUNDS if k in idb][0]
start, end = BOUNDS[key]

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
