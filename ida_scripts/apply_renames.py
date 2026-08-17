"""
IDA Pro script: master list of symbol renames (functions + globals) for
Ultima II (DOS).

Single accumulating script instead of one standalone name_*.py file per
finding. Whenever a function's or global's role becomes clear enough to
name confidently, add an entry to RENAMES below and re-run. Safe to
re-run repeatedly -- each entry is checked against the address's
*current* name and skipped if already applied, so old entries are
harmless to leave in place. Keep them rather than deleting: this file
doubles as a dated changelog of "what have we named and roughly why"
that's easy to diff in git, which is the whole point of consolidating
into one file instead of many.

Convention: DRY_RUN is left False (Paul's call, 2026-08-17) -- new
entries take effect the moment they're added and the script is re-run,
no separate dry-run confirmation pass. Sanity-check a new entry's
address and name before adding it, since there's no dry-run safety net
here anymore -- get it right in the list, not via a preview step.
(apply_structs.py hasn't had this same call made yet; treat it as
dry-run-first until told otherwise.)

Scope: plain renames only (idc.set_name on an address that IDA can
already address -- a function start, or an existing named/auto-named
data item). If a finding requires *creating* new data where there was
previously nothing (splitting an array, building a table, adding xrefs)
that's structural surgery, not a rename -- write a dedicated one-off
script for it instead (see resolve_command_jump_table.py and
fix_access_file_calls.py for that pattern). Struct member renames/
additions go in apply_structs.py, not here, since they use a different
IDA API (add_struc_member / set_member_name) and address a struct
definition rather than a single linear address.

For fuller justification of each rename, see the matching section of
docs/overview.md or docs/file-formats.md -- the `note` field here is
just enough to read the list top-to-bottom without cross-referencing.
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x10730, "command_jump_table",
     "26-entry A-Z command dispatch table (formerly off_10730). See "
     "docs/overview.md#resolved-a-z-command-jump-table-and-the-tlkxff-loader."),

    (0x122D5, "load_talk_file",
     "loads TLKXFF for the current map; formerly sub_122D5. NOT a "
     "Talk-key handler -- there is no Talk key, called from `enter` on "
     "VILLAGE/TOWN/CASTLE only. See docs/overview.md as above."),

    (0x1154E, "print_indexed_shop_string",
     "given an index in AL, walks the load_talk_file buffer "
     "(word_17886) to the index-th null-terminated string and prints "
     "it via write_character. Called only from `transact`'s shopkeeper "
     "branch -- TLKXFF data turns out to be shop response text, not "
     "walk-up NPC dialogue. See "
     "docs/file-formats.md#consumer-traced-read-out-by-transact-not-a-walk-up-talk. "
     "Proposed name, not yet confirmed with Paul."),

    (0x153F4, "print_char",
     "thin wrapper, literally `call write_character; retn`. Called from "
     "sub_10A30 and from print_indexed_shop_string; why the wrapper "
     "exists (vs. calling write_character directly) isn't understood "
     "yet. Proposed name, not yet confirmed with Paul."),
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
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
