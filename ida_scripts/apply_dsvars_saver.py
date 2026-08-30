"""
IDA Pro script: name the SAVER.EXE (save-game handler) DGROUP variables
(SAVER's DGROUP is **seg003**).

SAVER has 3 real functions and `ida_scripts/dsvars.py` finds only 25
DGROUP words touched -- all but three are `saver_entry` single-writes
staging the drawString prompt layout (row/col constants). The three that
matter:

Shared LEGLIB slots: `rosterIndex` @ 1B0A (same as MENU), `menuChoice`
@ 1E22.

Names data addresses + repeatable comments only. Run after
apply_renames_saver.py.

    .\run_ida_script.ps1 -Idb saver -ScriptName apply_dsvars_saver.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

VARS = [
    (0x1B0A, "rosterIndex",
     "which CHAR.DAT roster slot to write -- saver_entry sets it, "
     "saveRosterToDisk uses it to index the record. Same DGROUP slot as "
     "MENU.EXE's rosterIndex."),
    (0x1E22, "menuChoice",
     "the \"DO YOU WANT TO SAVE / THE GAME NOW IN PROGRESS?\" Y/N answer "
     "(saver_entry: `cmp ds:1E22h, 1`). Same DGROUP slot as the other "
     "modules."),
    (0x1ACA, "returnTarget",
     "carries the OUT-vs-DUN re-exec decision from saver_entry to "
     "chainBackOrQuit: after the ESC check, chainBackOrQuit tests "
     "\"OUT\" then \"DUN\" via rtm_FF08 and folds the result through "
     "`sub ax, ds:1ACAh` before rtm_FE05 execs the chosen module."),

    # --- tentative ---
    (0x20AA, "diskRetryState",
     "saver_entry scratch -- the disk-check / retry loop counter "
     "(reset to 0, read repeatedly). TENTATIVE."),
]


def name_data(ea, name, cmt):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 2)
    ida_bytes.create_data(ea, ida_bytes.FF_WORD, 2, idc.BADADDR)
    if idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK):
        if cmt:
            idc.set_cmt(ea, cmt, 1)
        return True
    return False


def main():
    base = ida_segment.get_segm_by_name("seg003").start_ea
    done = skip = 0
    for off, name, cmt in VARS:
        ea = base + off
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  ds:{off:04X}: {cur!r} -> {name!r}")
            continue
        if cur == name:
            skip += 1
            continue
        if name_data(ea, name, cmt):
            done += 1
        else:
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
