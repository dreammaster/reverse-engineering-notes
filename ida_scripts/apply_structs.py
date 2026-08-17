"""
IDA Pro script: master list of struct-definition updates (new members,
renames, retypes) for Ultima II (DOS).

Companion to apply_renames.py, same rationale: one accumulating,
git-trackable file instead of a new one-off script per finding. Add an
entry to OPERATIONS below whenever a struct field's meaning becomes
clear, then re-run (DRY_RUN = True first, always). Idempotent: each
operation checks current state and skips if already applied.

Scope: operations on IDA *struct* definitions (`Savegame`, `FCB`, and
any future ones) via add_struc_member / set_member_name -- i.e. things
that show up as `player.field_2B` -> `player._something` in the .asm.
This is a different IDA API surface from apply_renames.py's
idc.set_name (which addresses one specific linear address, not a
struct-relative offset), hence the split.

Does NOT cover: struct-of-arrays-style globals. `_mapMonsters` (the
per-map monster table, loaded from MONXFF) is accessed as parallel byte
arrays -- `_mapMonsters[di]`, `(_mapMonsters+20h)[di]`,
`(_mapMonsters+40h)[di]`, ... one byte per monster slot (0-31) per
field, not one struct per monster -- so it doesn't fit an IDA struct
type the way `player` (a single Savegame instance) does. Naming those
sub-arrays is a rename-once-split problem instead: the field boundaries
(`_mapMonsters+20h/40h/60h/80h`, and further unnamed ones out past
`_mapMonsters+137h` used in the monster-AI loop and in `transact`'s
shopkeeper branch, e.g. `[di+197h]` = monster type/class byte,
`[di+1D7h]` = shopkeeper item-index flag) are real findings worth
tracking, but need a bespoke script (ida_bytes.create_data to split the
array, then set_name on each piece) similar in shape to
resolve_command_jump_table.py -- not yet written. Note this here so the
lead isn't lost; write that script once the remaining field offsets
(0x177, 0x1B7, 0x1F7, 0x217, and whatever's between 0x80 and 0x137) are
better understood.

Each OPERATIONS entry is a dict:
  {"op": "add_member",    "struct": name, "member": name, "offset": int,
   "size": 1|2|4, "note": str}
  {"op": "rename_member", "struct": name, "offset": int, "new_name": str,
   "note": str}
"""

import idc
import ida_struct

DRY_RUN = True

OPERATIONS = [
    # No pending struct findings yet. Example shape for the next one:
    #
    # {"op": "rename_member", "struct": "Savegame", "offset": 0x2B,
    #  "new_name": "_something", "note": "figured out via ..., see "
    #  "docs/overview.md#... -- replaces the placeholder field_2B"},
    #
    # {"op": "add_member", "struct": "Savegame", "member": "_newField",
    #  "offset": 0x39, "size": 1, "note": "..."},
]

_SIZE_FLAGS = {1: idc.FF_BYTE, 2: idc.FF_WORD, 4: idc.FF_DWORD}


def _get_struct_id(name):
    sid = idc.get_struc_id(name)
    if sid == idc.BADADDR:
        print(f"[!] struct {name!r} not found -- typo, or does it need "
              f"creating first? This script only edits existing structs.")
        return None
    return sid


def add_member(struct, member, offset, size, note):
    sid = _get_struct_id(struct)
    if sid is None:
        return
    existing = idc.get_member_name(sid, offset)
    if existing:
        print(f"{struct}+{offset:X}: already {existing!r} -- skipping add")
        return
    print(f"{struct}+{offset:X}: add member {member!r} (size {size})")
    print(f"    {note}")
    if DRY_RUN:
        return
    flag = _SIZE_FLAGS[size] | idc.FF_DATA
    err = idc.add_struc_member(sid, member, offset, flag, -1, size)
    if err != 0:
        print(f"    [!] add_struc_member FAILED, error code {err}")


def rename_member(struct, offset, new_name, note):
    sid = _get_struct_id(struct)
    if sid is None:
        return
    cur = idc.get_member_name(sid, offset)
    if cur == new_name:
        print(f"{struct}+{offset:X}: already {new_name!r} -- skipping")
        return
    print(f"{struct}+{offset:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_member_name(sid, offset, new_name)
    if not ok:
        print("    [!] set_member_name FAILED")


def main():
    for entry in OPERATIONS:
        op = entry["op"]
        if op == "add_member":
            add_member(entry["struct"], entry["member"], entry["offset"],
                       entry["size"], entry["note"])
        elif op == "rename_member":
            rename_member(entry["struct"], entry["offset"],
                          entry["new_name"], entry["note"])
        else:
            print(f"[!] unknown op {op!r}, skipping entry: {entry}")

    if not OPERATIONS:
        print("[no-op] OPERATIONS is empty -- nothing to apply yet.")
    elif DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
