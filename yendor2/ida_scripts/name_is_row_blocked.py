"""
Names sub_214F4, called from ComputeDungeonCellVisibility: checks
whether every cell in the current scratch-buffer row (word_3292C,
di=0x6D60 + row*8, cx cells) is a solid-wall type (in range
_val18.._val17) and not already hidden. If so (the row is a dead-end/
closed wall), marks the next `bp` cells starting at 0x6D60 as hidden
([+6] |= 1) and returns "blocked" (ax=1); otherwise returns
"not blocked" (ax=0), letting the caller keep checking closer rows.

The per-row "is this a solid wall blocking the view beyond it"
occlusion test.

-> IsDungeonRowFullyBlocked

Run via:
    .\run_ida_script.ps1 name_is_row_blocked.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x214F4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "IsDungeonRowFullyBlocked", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'IsDungeonRowFullyBlocked': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Checks whether every cell in the row at word_3292C is a "
    "solid-wall type (_val18.._val17) -- if so, marks the next `bp` "
    "scratch cells hidden ([+6]|=1) and returns 'blocked' (ax=1); else "
    "'not blocked' (ax=0). Called from ComputeDungeonCellVisibility.",
    False,
)
