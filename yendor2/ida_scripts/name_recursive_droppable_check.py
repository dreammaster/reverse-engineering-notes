"""
Names sub_257BF and sub_25818, called from IsItemDroppable and each
other: a recursive "does this inventory (or a container within it)
contain a droppable item" check.

sub_257BF (-> HasDroppableItemInInventory): reads a CURGAME inventory
record (FileEntry bx=0x8FFB, errorCode=0xA) and scans its 8 slots: if
any item is directly droppable ([+0xC] bit 1), returns true (ax=
0xFFFF); if an item is a container ([+0xC] bit 0x2000), recurses one
level into it via sub_25818. Returns false (ax=0) if nothing
droppable is found anywhere.

sub_25818 (-> HasDroppableItemInContainer): the base case -- reads a
container's own CURGAME record and scans its 8 slots for a directly
droppable item (no further recursion, matching the "3 alternate bags"
fixed, non-nested structure).

Run via:
    .\run_ida_script.ps1 name_recursive_droppable_check.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x257BF: "HasDroppableItemInInventory",
    0x25818: "HasDroppableItemInContainer",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x257BF,
    "Scans an 8-slot CURGAME inventory record for a directly droppable "
    "item ([+0xC] bit 1), recursing into any container slot ([+0xC] "
    "bit 0x2000) via HasDroppableItemInContainer. Called from "
    "IsItemDroppable.",
    False,
)
ida_bytes.set_cmt(
    0x25818,
    "Base case: scans a container's own 8-slot CURGAME record for a "
    "directly droppable item, no further recursion. Called from "
    "HasDroppableItemInInventory.",
    False,
)
