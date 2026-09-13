"""
IDA Pro script: master list of symbol renames (functions + globals) for
BOOTUP.BIN (DOS) -- Ultima III's actual game executable, chained into
from ULTIMA.COM's title screen (see docs/overview.md's "ULTIMA.COM's
real role" section). Companion to apply_renames_ultima.py, same
accumulating-list rationale -- see that file's docstring for the full
convention writeup (naming casing, DRY_RUN policy, scope vs.
apply_structs_bootup.py). Run against ultima_bootup.idb:

    .\\run_ida_script.ps1 -Idb ultima_bootup -ScriptName apply_renames_bootup.py

Before adding a new entry here, check whether the same address in
ultima.idb already has a confirmed name -- BOOTUP.BIN is loaded into
the exact same memory range ULTIMA.COM occupied (both are tiny-model
COM-style images based at paragraph 1000h, offset 100h), and shares its
low-level runtime (drawTileGrid, playSoundEffect, etc. -- see
docs/overview.md). A shared-runtime function at the same address in
both IDBs should get the same name in both, not be re-derived from
scratch.
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
