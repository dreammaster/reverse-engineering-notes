"""
Traced sub_26415's container-open/close branches (the item-interaction
handler already partially explored a few rounds ago) into two small
CURGAME-backed persistence functions, confirming the "3 alternate bags"
inventory-group hypothesis from GetInventorySlotPtr: they're literally
container items the character can open, each with its own contents
saved separately in the savegame.

When the player clicks an unopened container item, sub_26415 assigns
it to the first free bag slot (+0x17C/+0x1A2/+0x1C8 in priority order,
matching GetInventorySlotPtr's group markers), stores the container's
own id/data there, and calls sub_26846(ax=?, bx=word_328D4+group-base)
-> LoadContainerContents: reads the container's saved inventory
contents from CURGAME (FileEntry bx=0x8FFB, errorCode=0xB) into that
bag's slot area.

When the player closes an open container (clicking it again),
sub_26415 calls sub_268F4(di=marker offset) -> SaveAndCloseContainer:
if the bag slot area has contents ([di]!=0), writes them back to
CURGAME (same FileEntry/errorCode), then clears the marker fields
(zeroes [di]/[di+2]) -- unloading the bag from the character.

Run via:
    .\run_ida_script.ps1 name_container_persistence.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x26846: "LoadContainerContents",
    0x268F4: "SaveAndCloseContainer",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x26846,
    "LoadContainerContents(ax=?, bx=word_328D4+group-base): reads a "
    "container item's saved inventory contents from CURGAME "
    "(FileEntry bx=0x8FFB, errorCode=0xB) into the character's bag "
    "slot area. Called when opening a container item into one of the "
    "3 alternate-bag inventory groups (see GetInventorySlotPtr).",
    False,
)
ida_bytes.set_cmt(
    0x268F4,
    "SaveAndCloseContainer(di=bag marker offset, si=word_328D4): if "
    "the bag slot area has contents ([di]!=0), writes them back to "
    "CURGAME (FileEntry bx=0x8FFB, errorCode=0xB), then zeroes the "
    "marker fields ([di]/[di+2]) -- unloads the container.",
    False,
)
