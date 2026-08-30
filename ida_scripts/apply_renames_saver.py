"""
IDA Pro script: renames for saver.idb (SAVER.EXE) -- the **save-game
handler**. A small chained module the play modules (OUT / DUN / ...) hand
off to when the player asks to save or quit:

  * "DO YOU WANT TO SAVE / THE GAME NOW IN PROGRESS?"
  * validates the character disk ("<name> is not on this / character
    disk", "empty" slot), writes the roster to CHAR.DAT ("SAVING TO
    DISK")
  * then "{TO QUIT} - Hit the ESC key." / "{TO CONTINUE PLAYING}- Hit
    any other key." -- ESC exits to DOS, anything else re-execs OUT.EXE
    or DUN.EXE (whichever the player came from).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_saver

    .\run_ida_script.ps1 -Idb saver -ScriptName apply_renames_saver.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# SAVER.EXE: tiny -- single code seg seg000 "bmSAVER" (5 real funcs,
# 1.5 KB), thunk table seg001 (373 entries), DGROUP seg003.
#
# (ea, new_name, note)
RENAMES = [
    (0x10030, "saver_entry",
     'SAVER.EXE entry / save-or-quit flow: "DO YOU WANT TO SAVE / THE '
     'GAME NOW IN PROGRESS?", character-disk validation ("is not on this '
     '/ character disk", "empty"), "SAVING TO DISK" -> saveRosterToDisk, '
     'then the quit/continue prompt; falls into chainBackOrQuit. ~1 KB.'),

    (0x10504, "saveRosterToDisk",
     'writes the character roster to CHAR.DAT (opens "char.dat", builds '
     'the record with rt_73/basStrBuild, file I/O via rt_FE35 / rt_FE39). '
     'Handles the wrong-disk / empty-slot cases.'),

    (0x10412, "chainBackOrQuit",
     'post-save dispatch: "{TO QUIT} - Hit the ESC key." -> exit to DOS; '
     'otherwise re-exec OUT.EXE or DUN.EXE (whichever module invoked the '
     'save). Self-looping key wait.'),
]


def main():
    seg = ida_segment.get_segm_by_name("seg000")
    S0, S0E = seg.start_ea, seg.end_ea

    done = skip = 0
    for ea, name, note in RENAMES:
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  {ea:#x}: {cur!r} -> {name!r}")
            continue
        if cur != name and not idc.set_name(ea, name, idc.SN_NOWARN):
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
            continue
        idc.set_func_cmt(ea, note, 1)
        done += 1 if cur != name else 0
        skip += 1 if cur == name else 0

    total = sum(1 for _ in idautils.Functions(S0, S0E))
    named = sum(1 for f in idautils.Functions(S0, S0E)
                if not idc.get_func_name(f).startswith(("sub_", "j_", "nullsub")))
    print(f"\napplied {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))
    print(f"seg000: {named}/{total} functions named")


main()
