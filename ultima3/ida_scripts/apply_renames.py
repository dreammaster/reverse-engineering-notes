"""
IDA Pro script: master list of symbol renames (functions + globals) for
Ultima III (DOS).

Single accumulating script instead of one standalone name_*.py file per
finding. Whenever a function's or global's role becomes clear enough to
name confidently, add an entry to RENAMES below and re-run. Safe to
re-run repeatedly -- each entry is checked against the address's
*current* name and skipped if already applied, so old entries are
harmless to leave in place. Keep them rather than deleting: this file
doubles as a dated changelog of "what have we named and roughly why"
that's easy to diff in git.

Naming convention (Paul's call, 2026-09-13): **camelCase** for function
names, matching ultima1 rather than ultima2 (which uses snake_case) --
see docs/overview.md. Globals/statics keep the shared cross-project
convention regardless: leading underscore + camelCase (`_savegame`,
`_playerX`). Structs: PascalCase. Struct fields: underscore-prefixed
camelCase. Constant/lookup tables: ALL_CAPS. CRT-internal runtime
functions get a leading underscore to mark them as non-game-logic
regardless of the function-casing convention (`_fopen`, `_toupper`).

Convention: DRY_RUN starts True until a first batch of renames has been
verified end-to-end (export, re-open, spot check) -- flip to False only
as a deliberate, logged decision (see roadmap.md), same as ultima1/
ultima2's precedent.

Scope: plain renames only (idc.set_name on an address that IDA can
already address -- a function start, or an existing named/auto-named
data item). If a finding requires *creating* new data where there was
previously nothing (splitting an array, building a table, adding xrefs)
that's structural surgery, not a rename -- write a dedicated one-off
script for it instead. Struct member renames/additions go in
apply_structs.py, not here, since they use a different IDA API
(add_struc_member / set_member_name) and address a struct definition
rather than a single linear address.

For fuller justification of each rename, see the matching section of
docs/overview.md or docs/file-formats.md -- the `note` field here is
just enough to read the list top-to-bottom without cross-referencing.
"""

import idc

DRY_RUN = True

# (ea, new_name, note)
RENAMES = [
]


def apply_rename(ea, new_name, note):
    cur = idc.get_name(ea)
    if cur == new_name:
        print(f"{ea:X}: already {new_name!r} -- skipping")
        return
    print(f"{ea:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    if not ok:
        print("    [!] rename FAILED")


def main():
    for ea, new_name, note in RENAMES:
        apply_rename(ea, new_name, note)
    if not RENAMES:
        print("[no-op] RENAMES is empty -- nothing to apply yet.")
    elif DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
