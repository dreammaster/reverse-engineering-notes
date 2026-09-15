"""
Read-only discovery script: for a set of already-named "anchor" functions,
lists their still-unnamed direct callees. Callees of a known subsystem
initializer (InitGraphics, InitMemory, ...) are much easier to confidently
identify than a random sub_XXXXX, since we already know the calling
context -- this is meant to surface good next candidates for manual
reading rather than naming anything itself.

    .\run_ida_script.ps1 callees_of_named.py -NoExport
"""
import idautils
import idc
import ida_funcs

ANCHORS = [
    "start", "InitGame", "InitGlobals", "InitMemory", "InitGraphics",
    "InitMouse", "MapUnmapPages", "ErrorCheck", "findSavegame",
    "loadWorldDat1", "loadWorldDat2", "loadWorldDat3", "loadWorldDat4",
    "loadWorldDat5", "WorldDat_setBlock1", "WorldDat_setBlock2",
    "WorldDat_setBlock3", "WorldDat_setBlock4", "WorldDat_setBlock5",
    "WorldDat_setBlock6", "Struc1_Allocate", "calcOffset", "allocMem",
    "writeString", "writeChar", "getTextPos",
    "GameDialog_drawButtons", "GameDialog_drawAnimation",
]

for name in ANCHORS:
    ea = idc.get_name_ea_simple(name)
    if ea == idc.BADADDR:
        print(f"{name}: NOT FOUND")
        continue
    f = ida_funcs.get_func(ea)
    if not f:
        print(f"{name}: not a function")
        continue
    callees = set()
    cur = f.start_ea
    while cur < f.end_ea:
        mnem = idc.print_insn_mnem(cur)
        if mnem == "call":
            optype = idc.get_operand_type(cur, 0)
            if optype in (idc.o_near, idc.o_far):
                # get_operand_value() for o_near/o_far returns a segment-
                # relative offset, not the absolute ea -- read the target
                # off the instruction's own code xref instead.
                for x in idautils.CodeRefsFrom(cur, 0):
                    if ida_funcs.get_func(x) and ida_funcs.get_func(x).start_ea == x:
                        callees.add(x)
        cur = idc.next_head(cur, f.end_ea)
    unnamed = sorted(t for t in callees if idc.get_func_name(t).startswith("sub_"))
    named = sorted(idc.get_func_name(t) for t in callees if not idc.get_func_name(t).startswith("sub_"))
    print(f"{name} ({ea:#x}, size={f.end_ea-f.start_ea}): "
          f"{len(callees)} callees, {len(unnamed)} unnamed")
    if unnamed:
        print("  unnamed:", ", ".join(f"{t:#x}" for t in unnamed))
    if named:
        print("  named:", ", ".join(named))
