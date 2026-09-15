"""
Traced sub_1CECB, FindItemInInventoryRange's container-recursion
callee. Reads a container's contents from CURGAME (FileEntry
bx=0x8FFB, same block-read pattern as LoadContainerContents) into a
scratch buffer, then searches its 8 item slots for one whose id falls
in [word_3293E, word_32940] -- recursing into nested containers the
same way (bag-within-a-bag support) via sub_1CF50 (not traced).

-> FindItemInsideContainer

Run via:
    .\run_ida_script.ps1 name_find_item_in_container.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1CECB
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FindItemInsideContainer", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FindItemInsideContainer': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Reads a container's contents from CURGAME (FileEntry bx=0x8FFB, "
    "same pattern as LoadContainerContents) into a scratch buffer, "
    "searches its 8 slots for an item id in "
    "[word_3293E, word_32940], and recurses into nested container "
    "items the same way via sub_1CF50 -- FindItemInInventoryRange's "
    "container-search step.",
    False,
)
