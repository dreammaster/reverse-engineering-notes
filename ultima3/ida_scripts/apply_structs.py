"""
IDA Pro script: master list of struct-definition updates (new members,
renames, retypes) for Ultima III (DOS).

Companion to apply_renames.py, same rationale: one accumulating,
git-trackable file instead of a new one-off script per finding. Add an
entry to OPERATIONS below whenever a struct field's meaning becomes
clear, then re-run. Idempotent: each operation checks current state and
skips if already applied.

Convention: DRY_RUN starts True, same as apply_renames.py, until a first
batch of struct edits has been verified end-to-end -- flip to False only
as a deliberate, logged decision (see docs/roadmap.md).

Scope: operations on IDA *struct* definitions via add_struc_member /
set_member_name -- i.e. things that show up as `player.field_2B` ->
`player._something` in the .asm. This is a different IDA API surface
from apply_renames.py's idc.set_name (which addresses one specific
linear address, not a struct-relative offset), hence the split.

Supports array-typed members: a struct member can be an N-element byte/
word/dword array, not just a scalar (ported from ultima2's precedent,
where a single indexed access was enough for IDA to render `name[reg]`
syntax even when the "official" declared size was too small -- this kind
of undersizing is easy to miss until the index range has actually been
traced). Two ops cover this:
  - "add_member": for an offset with NO existing member yet. Supports
    element_size (1/2/4, byte/word/dword) and count (defaults to 1 for
    a plain scalar); total member size is element_size * count.
  - "resize_member": for an offset that already has a member, but at
    the wrong size (or a default IDA-auto-generated name) -- deletes and
    recreates it via del_struc_member + add_struc_member.

Each OPERATIONS entry is a dict:
  {"op": "add_member", "struct": name, "member": name, "offset": int,
   "element_size": 1|2|4, "count": int (default 1), "note": str}
  {"op": "rename_member", "struct": name, "offset": int, "new_name": str,
   "note": str}
  {"op": "resize_member", "struct": name, "offset": int, "new_name": str,
   "element_size": 1|2|4, "count": int, "note": str}
"""

import idc
import ida_struct

DRY_RUN = True

OPERATIONS = [
]

_SIZE_FLAGS = {1: idc.FF_BYTE, 2: idc.FF_WORD, 4: idc.FF_DWORD}


def _get_struct_id(name):
    sid = idc.get_struc_id(name)
    if sid == idc.BADADDR:
        print(f"[!] struct {name!r} not found -- typo, or does it need "
              f"creating first? This script only edits existing structs.")
        return None
    return sid


def add_member(struct, member, offset, element_size, count, note):
    sid = _get_struct_id(struct)
    if sid is None:
        return
    existing = idc.get_member_name(sid, offset)
    if existing:
        print(f"{struct}+{offset:X}: already {existing!r} -- skipping add "
              f"(use \"resize_member\" if it needs to change size/name)")
        return
    total = element_size * count
    label = f"{count} x {element_size}-byte elements = {total} bytes" \
        if count > 1 else f"{total} byte(s)"
    print(f"{struct}+{offset:X}: add member {member!r} ({label})")
    print(f"    {note}")
    if DRY_RUN:
        return
    flag = _SIZE_FLAGS[element_size] | idc.FF_DATA
    err = idc.add_struc_member(sid, member, offset, flag, -1, total)
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


def resize_member(struct, offset, new_name, element_size, count, note):
    sid = _get_struct_id(struct)
    if sid is None:
        return
    cur_name = idc.get_member_name(sid, offset)
    if cur_name == new_name:
        print(f"{struct}+{offset:X}: already {new_name!r} -- skipping "
              f"(not re-verifying size; delete the member by hand in IDA "
              f"first if it needs correcting again)")
        return
    total = element_size * count
    print(f"{struct}+{offset:X}: {cur_name!r} -> {new_name!r} "
          f"({count} x {element_size}-byte elements = {total} bytes)")
    print(f"    {note}")
    if DRY_RUN:
        return
    if cur_name:
        if not idc.del_struc_member(sid, offset):
            print("    [!] del_struc_member FAILED -- not attempting the add")
            return
    flag = _SIZE_FLAGS[element_size] | idc.FF_DATA
    err = idc.add_struc_member(sid, new_name, offset, flag, -1, total)
    if err != 0:
        print(f"    [!] add_struc_member FAILED, error code {err}")


def main():
    for entry in OPERATIONS:
        op = entry["op"]
        if op == "add_member":
            add_member(entry["struct"], entry["member"], entry["offset"],
                       entry["element_size"], entry.get("count", 1),
                       entry["note"])
        elif op == "rename_member":
            rename_member(entry["struct"], entry["offset"],
                          entry["new_name"], entry["note"])
        elif op == "resize_member":
            resize_member(entry["struct"], entry["offset"],
                          entry["new_name"], entry["element_size"],
                          entry["count"], entry["note"])
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
