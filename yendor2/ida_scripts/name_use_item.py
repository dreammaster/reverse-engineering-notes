"""
Traced sub_17B92 (called directly from `start`) and its first callee,
sub_1C890 -- the top-level "use an item" system.

sub_1C890(ax=item id) -> LoadItemData: frees any previously-loaded
item data buffers (word_2E54C, word_328F4), reads a fixed catalog
record at WORLD.DAT-backed address 0xBCE (FileEntry bx=0x9043,
errorCode=0xF) to find this item's data block size/location, allocates
a buffer sized to fit it, and reads the item's actual data from
WORLD.DAT into that buffer. This is WORLD.DAT's item-data catalog --
structurally similar in spirit to g_pictureDir cataloging
PICTURES.VGA, just for item definitions instead of pictures.

sub_17B92 -> UseItem: calls LoadItemData first (bailing out if it
signals nothing to do), then dispatches on word_2E410 (the loaded
item's own type-flags word) to one of several type-specific effect
handlers (sub_1BF94/sub_1C123/sub_1C589/sub_1BEA1/sub_1BBED/sub_1BB48
and a further fallback branch keyed on a secondary type field at
es:[si+0xE]) -- each presumably implementing a different item
category's effect (the one type-2 branch of sub_1BBED, already traced
two rounds ago, turned out to consume a BCD material counter and set a
per-character event flag). None of the type-specific handlers are
named this round -- a good next-round target now that the top-level
dispatcher and its data source are understood.

Run via:
    .\run_ida_script.ps1 name_use_item.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x17B92: "UseItem",
    0x1C890: "LoadItemData",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x17B92,
    "UseItem, reached from a normal keyboard command slot (called "
    "directly from `start`). Calls LoadItemData first; if that "
    "signals nothing to do, bails. Otherwise dispatches on "
    "word_2E410 (the loaded item's type-flags word) to one of "
    "several type-specific effect handlers (sub_1BF94/sub_1C123/"
    "sub_1C589/sub_1BEA1/sub_1BBED/sub_1BB48, plus a fallback keyed "
    "on a secondary type field) -- none named yet.",
    False,
)
ida_bytes.set_cmt(
    0x1C890,
    "LoadItemData(ax=item id): frees any previously-loaded item data "
    "buffers, reads WORLD.DAT's item catalog record (fixed address "
    "0xBCE, FileEntry bx=0x9043) to find this item's data block, "
    "allocates a buffer sized to fit, and reads the item's data from "
    "WORLD.DAT into it. WORLD.DAT's item-data catalog, structurally "
    "similar to g_pictureDir's role for PICTURES.VGA.",
    False,
)
