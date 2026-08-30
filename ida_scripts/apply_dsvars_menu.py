"""
IDA Pro script: name the MENU.EXE DGROUP variables (menu's DGROUP is
**seg003**, like OUT -- seg001 is the thunk table, seg002 the RTM
bootstrap).

MENU is a launcher, not a game module, so it holds almost no state:
`ida_scripts/dsvars.py` finds only 30 DGROUP words touched, and most are
one-function scratch (mainMenuLoop stages the menu-item geometry in
ds:2124..212E; sub_10150's ds:212x cluster are a draw helper's locals).
The handful below are the real cross-function state.

Names data addresses + repeatable comments only. Run after
apply_renames_menu.py.

    .\run_ida_script.ps1 -Idb menu -ScriptName apply_dsvars_menu.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

VARS = [
    (0x1F5C, "menuHighlight",
     "the highlighted main-menu item (mainMenuLoop inits it to -1 = "
     "none; drawMainMenuScreen reads it to draw the selection bar)."),
    (0x2138, "charCount",
     "number of characters in the roster (0..8). startNewGameMenu "
     "increments it when a character is created; eraseCharacterMenu "
     "decrements it; restartGameMenu / the roster screens read it."),
    (0x1B0A, "rosterIndex",
     "the roster slot currently being drawn / edited (0..7). Used with "
     "`imul charRecordSize` to index the CHAR.DAT record; the "
     "show*CharacterSlots loops accumulate into it."),
    (0x211C, "charRecordSize",
     "CHAR.DAT record stride -- the `imul` / `mov ax,` factor the "
     "character screens use to step through the loaded roster. Set by "
     "the roster loader, read-only in seg000."),

    (0x1E22, "menuChoice",
     "current Y/N / menu answer (eraseCharacterMenu and sub_10150 read "
     "it). Same DGROUP slot the play modules use for menu answers."),

    # --- tentative ---
    (0x1F02, "menuActive",
     "1 while the main-menu loop is running (mainMenuLoop sets it, "
     "drawCancelOption checks it). TENTATIVE."),
    (0x1AEE, "introStep",
     "intro-sequence step in playIntroAndLaunchGame (0 / 3 / 0xFF). "
     "TENTATIVE."),
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
    s3 = ida_segment.get_segm_by_name("seg003")
    base = s3.start_ea
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
