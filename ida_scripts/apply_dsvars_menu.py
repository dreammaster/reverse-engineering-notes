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

    (0x1F02, "menuRunning",
     "1 while the main-menu loop runs, 0 to exit -- mainMenuLoop inits "
     "it to 1, passes its address (with menuLevel / menuHighlight) to "
     "the key-dispatch routine, and `cmp ds:1F02h, 0` breaks the loop; "
     "drawCancelOption checks it too."),
    (0x1F0C, "menuLevel",
     "which menu screen is active -- mainMenuLoop cycles it 1 (main "
     "menu) -> 2 (a submenu / dialog) -> 1. Passed by reference "
     "alongside menuRunning."),
    (0x0101, "dgroupSeg",
     "the DGROUP self-segment -- `mov es, ds:101h` in "
     "playIntroAndLaunchGame."),

    # --- tentative ---
    (0x1AEE, "introStep",
     "intro-sequence step in playIntroAndLaunchGame (0 / 3 / 0xFF). "
     "TENTATIVE."),

    # --- seg001 resident title-screen helpers (hand-written asm) ---
    (0x317C, "titleGlbName", "\"TITLE.GLB\" -- the title tile-graphics file."),
    (0x3186, "titleGmpName", "\"TITLE.GMP\" -- the title cell-map file."),
    (0x3190, "titleGlbSize",
     "byte count loadTitleImage passes to readFileWhole for TITLE.GLB."),
    (0x3192, "titleGmpSize", "byte count for TITLE.GMP."),
    (0x3194, "titleGlbBuf",
     "load buffer for TITLE.GLB (the 8x8 tile bitmaps). ~8 KB region "
     "(0x3194..0x5194)."),
    (0x5194, "titleGmpBuf",
     "load buffer for TITLE.GMP (the per-cell tile-index map the "
     "blitter walks)."),
    (0x6194, "titleTilePtr",
     "pointer into titleGlbBuf at the tile-bitmap data (buf + 0x11, "
     "skipping the BSAVE header). blitCharCell adds a tile index*2 to "
     "it."),
    (0x6196, "titleScrollX",
     "horizontal scroll offset of the title image -- scrollTitleImage "
     "advances it by 0x28 (40) each music tick and wraps at 0xA0 (160)."),
    (0x6198, "titleColOfsTable",
     "per-column screen-offset table scrollTitleImage indexes "
     "(`[di+6198h]`)."),
    (0x61C0, "titleColTileTable",
     "per-column tile-index table scrollTitleImage indexes "
     "(`[di+61C0h]`)."),
]


def name_data(ea, name, cmt):
    if name.endswith("Name"):
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 12)
        idc.create_strlit(ea, ea + 10)
    else:
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
