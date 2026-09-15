"""
Names the "drop held item" cluster: sub_1A37E, sub_25740, sub_1A294,
called from `start`/HandleDungeonInput.

sub_1A37E (-> TryDropHeldItem): only proceeds while carrying an item
(word_31946). Checks IsItemDroppable; if not droppable,
FlashStatusWarning and bail. Otherwise shows a confirm prompt (msg 4,
"drop it?"); if confirmed (ax==5), calls PlaceItemOnGround with the
staged item id/quantity and clears the held-item state; if declined,
restores the held item unchanged.

sub_25740 (-> IsItemDroppable): loads the held item's catalog record
and checks its [+0xC] flags (bit 1 or bit 0x2000) -- if word_31948
(item id) is 0, defaults to "not droppable" (ax=0); bit 1 set ->
"droppable" (ax=0xFFFF); bit 0x2000 (container flag, matching the
already-documented container-contents system) -> a third outcome.

sub_1A294 (-> PlaceItemOnGround): the actual placement. If the item is
a container (catalog [+0xC] bit 0x2000), recursively processes its
8-slot contents the same way (each sub-item checked for being a
container too) via sub_1A34C -- persisting a dropped container's full
contents, not just the container itself.

Run via:
    .\run_ida_script.ps1 name_drop_item_cluster.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1A37E: "TryDropHeldItem",
    0x25740: "IsItemDroppable",
    0x1A294: "PlaceItemOnGround",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1A37E,
    "The 'drop held item' action: checks IsItemDroppable (warns/bails "
    "if not), shows a confirm prompt, then calls PlaceItemOnGround on "
    "confirmation or restores the held item on decline. Called from "
    "`start`/HandleDungeonInput.",
    False,
)
ida_bytes.set_cmt(
    0x25740,
    "Checks the held item's catalog [+0xC] flags (bit 1 / bit 0x2000, "
    "the container flag) to determine droppability. Called from "
    "TryDropHeldItem.",
    False,
)
ida_bytes.set_cmt(
    0x1A294,
    "Places the held item on the ground; if it's a container "
    "([+0xC] bit 0x2000), recursively processes its 8-slot contents "
    "the same way via sub_1A34C -- persists a dropped container's "
    "full contents. Called from TryDropHeldItem.",
    False,
)
