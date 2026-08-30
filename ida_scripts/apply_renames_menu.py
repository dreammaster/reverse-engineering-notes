"""
IDA Pro script: master list of symbol renames for menu.idb (MENU.EXE).

Single accumulating script, mirroring the sibling ultima1 project's
apply_renames_<stem>.py convention. Add an (ea, name, note) entry
whenever a seg000 function's role becomes clear enough to name, and
re-run. Safe to re-run: each entry is checked against the current name
and skipped if already applied.

seg000 was coerced to code by coerce_seg000_menu.py; the call graph is
menu_main -> mainMenuLoop -> per-option screen handlers. Names below are
derived from the seg003 text each function prints (see
docs/overview.md / file-formats.md) and its run-time (rtm_*) call
pattern.

    .\run_ida_script.ps1 -Idb menu -ScriptName apply_renames_menu.py
"""

import idc
import idautils
import ida_bytes
import ida_funcs
import ida_segment
import ida_auto

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x10032, "menu_main",
     "program entry (falls through into mainMenuLoop). BASIC module init: "
     "sets up the theme-music strings, YES/NO/\"empty\" literals, opens "
     "LEGACY.DAT."),

    (0x10580, "mainMenuLoop",
     "the main menu SELECT CASE dispatch loop. Draws the menu "
     "(drawMainMenuScreen), reads a key, and branches to showQuestCopyright "
     "/ showInstructions / showGameCredits / startNewGameMenu / "
     "restartGameMenu / eraseCharacterMenu / showTitleScreen / "
     "readLegacyDat. Tail-loops to itself."),

    (0x10738, "readLegacyDat",
     "reads LEGACY.DAT (menu state / last settings). Called from "
     "mainMenuLoop and the new-game / restart paths."),

    (0x108E7, "showQuestCopyright",
     "draws \"LEGACY OF THE ANCIENTS / Copyright (c) 1987 - 1989 / Quest "
     "Software, Inc.\" (the game-program copyright, distinct from "
     "showStartupSplash's EA installer notice)."),

    (0x10A76, "showGameCredits",
     "GAME CREDITS screen -- Designed by John & Charles Dougherty, IBM "
     "version Al DeYoung, Artwork Rick Tumanis / Dan Stechow / Roseann "
     "Miller, Additional programming Gregg Seelhoff / Johnny Klonaris / "
     "Bob Luzenski. \"(PICK OPTION USING KEYBOARD)\"."),

    (0x10C71, "showCharacterRoster",
     "reads CHAR.DAT and lists the saved characters. Shared by "
     "eraseCharacterMenu, startNewGameMenu and restartGameMenu. One of a "
     "family of near-identical CHAR.DAT enumeration helpers "
     "(sub_12055 / sub_12778 / sub_128A9)."),

    (0x10D97, "eraseCharacterMenu",
     "ERASE A CHARACTER screen: \"** NO CHARACTERS TO ERASE **\", "
     "\"ERASE WHICH CHARACTER? (SELECT BY NUMBER KEY)\", \"ERASING\". "
     "Reads/rewrites CHAR.DAT."),

    (0x110CD, "promptNewCharacterName",
     "\"Type in your new character's name. / It may be up to 14 letters "
     "long. / PRESS ENTER WHEN FINISHED / PRESS 'ESC' TO CANCEL\". Writes "
     "the new roster entry (CHAR.DAT / LEGACY.DAT)."),

    (0x1138D, "showInstructions",
     "SIMPLE INSTRUCTIONS screens: \"LEGACY OF THE ANCIENTS is a 'menu "
     "driven' game\", COMMANDS, CHARACTER MOVEMENT (\"Use the ARROW "
     "KEYS\"). Largest text handler (~1.5 KB)."),

    (0x11967, "playMusicTick",
     "plays/advances a pushed MML music string and polls the keyboard; "
     "returns nonzero when a key is pressed (used to abort the title "
     "sequence). Called repeatedly by showTitleScreen. NB: also touches "
     "LEGACY.DAT -- confirm."),

    (0x11AEB, "showEmptyCharacterSlots",
     "displays the \"empty\" character slots (pushes the \"!empty\" "
     "literal). Called from eraseCharacterMenu and startNewGameMenu."),

    (0x11C2F, "promptCharacterNumber",
     "\"SELECT BY NUMBER KEY\" prompt -- reads a 1-4 digit and returns "
     "the chosen character index. Shared by erase / restart."),

    (0x11D3A, "startNewGameMenu",
     "START A NEW GAME: checks for a free slot (\"YOU MUST FIRST ERASE / "
     "AN OLD CHARACTER BEFORE / STARTING A NEW ONE\"), name-collision "
     "(\"ALREADY EXISTS / PICK A DIFFERENT NAME\"), DISK FULL. On success "
     "runs playIntroAndLaunchGame."),

    (0x121E4, "restartGameMenu",
     "RESTART A GAME: \"** NO CHARACTERS TO RESTART **\", \"RESTART WHICH "
     "CHARACTER?\", \"RESTARTING <name>\". Then playIntroAndLaunchGame."),

    (0x12414, "showStartupSplash",
     "the startup splash: \"Legacy of the Ancients / Game Program "
     "Copyright (c) 1987-1989 / Installation Program Copyright (c) 1989 "
     "Electronic Arts / Program Compiler Copyright (c) 1982-1988 "
     "Microsoft Corp.\". Called once from menu_main."),

    (0x125AB, "drawMainMenuScreen",
     "renders the menu screen: \"Loading...\", the numbered items "
     "(\"1. play a game\" / \"2. simple instructions\" / \"3. game "
     "credits\" / \"4. sound is currently on/off\"), and the second-menu "
     "variant (\"1. return to first menu\" ...). Calls drawCancelOption."),

    (0x1294E, "drawCancelOption",
     "draws the \"CANCEL\" menu entry. Called only by drawMainMenuScreen."),

    (0x129A8, "playIntroAndLaunchGame",
     "the new-character intro cut-scene (\"You are only a poor peasant on "
     "the world of Tarmalon...\", the dead man / leather scroll / "
     "shimmering archway narrative) then hands off to the game "
     "(\"Mail2\", MUS.EXE / OUT.EXE / DUN.EXE). Largest seg000 function "
     "(~1.6 KB)."),

    (0x13014, "showTitleScreen",
     "loads the title image (loadTitleImage -> TITLE.GLB / TITLE.GMP into "
     "B800h) and plays the theme music (5 MML strings at seg003:3034.. "
     "via playMusicTick / rtm_CE) until a key is pressed."),

    # seg001 resident helpers pulled in by showTitleScreen
    (0x13170, "loadTitleImage",
     "resident helper (seg001): opens TITLE.GLB and TITLE.GMP, reads them "
     "and unpacks into B800h video memory. Called by showTitleScreen."),
    (0x13219, "scrollTitleImage",
     "resident helper (seg001): B800h row shuffle -- animates / scrolls "
     "the title image. Called by showTitleScreen. (name provisional)"),
    (0x131D3, "readFileWhole",
     "resident helper (seg001): DOS open (3D02) / read (3F) / close (3E) "
     "a whole file into a buffer. Used by loadTitleImage."),
    (0x131E5, "blitCharCell",
     "resident helper (seg001): copies one 8x8-ish cell into the B800h "
     "framebuffer at (row*320 + col*2)+offset. (name provisional)"),
]

# 3-byte `jmp rt_ED` procedure-epilogue stubs that IDA split off between
# functions and daisy-chain-named `j_j_..._rt_ED`; fold each into the
# function that precedes it.
MERGE_EPILOGUE_STUBS = True


def main():
    seg = ida_segment.get_segm_by_name("seg000")
    S0, S0E = seg.start_ea, seg.end_ea

    done = skip = 0
    for ea, name, note in RENAMES:
        cur = idc.get_name(ea)
        if DRY_RUN:
            if cur != name:
                print(f"  would name {ea:#x}: {cur!r} -> {name!r}")
                done += 1
            else:
                skip += 1
            continue
        if not ida_funcs.get_func(ea):
            ida_funcs.add_func(ea)
        if cur != name and not idc.set_name(ea, name, idc.SN_NOWARN):
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
            continue
        idc.set_func_cmt(ea, note, 1)
        done += 1 if cur != name else 0
        skip += 1 if cur == name else 0

    merged = 0
    crefs = 0
    if not DRY_RUN and MERGE_EPILOGUE_STUBS:
        for f in list(idautils.Functions(S0, S0E)):
            fn = ida_funcs.get_func(f)
            if not fn or (fn.end_ea - fn.start_ea) > 5:
                continue
            last = idc.prev_head(fn.end_ea, fn.start_ea)
            if idc.print_insn_mnem(last) != "jmp":
                continue
            prev = ida_funcs.get_func(fn.start_ea - 1)
            if not prev:
                continue
            new_end = fn.end_ea
            ida_funcs.del_func(fn.start_ea)
            ida_funcs.set_func_end(prev.start_ea, new_end)
            merged += 1
        ida_auto.auto_wait()

        # Cosmetic, and the LAST thing to touch seg000: IDA's `int 3Fh`
        # overlay special-casing renders a "; ---" block break + fresh
        # label after every `call far <thunk>` (the 3-byte stub can't be
        # proven to return). Add an explicit fall-through cref past each so
        # the listing reads as continuous flow. No auto_wait after this --
        # a reanalysis reconsiders the crefs away.
        import ida_xref
        for a in idautils.Heads(S0, S0E):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(a)):
                continue
            if idc.print_insn_mnem(a) == "call" and idc.get_operand_type(a, 0) == idc.o_far:
                nxt = idc.get_item_end(a)
                if nxt < S0E and ida_bytes.is_code(ida_bytes.get_full_flags(nxt)):
                    ida_xref.add_cref(a, nxt, ida_xref.fl_F)
                    crefs += 1

    named_funcs = sum(1 for f in idautils.Functions(S0, S0E)
                      if not idc.get_func_name(f).startswith(("sub_", "j_", "nullsub")))
    total = sum(1 for _ in idautils.Functions(S0, S0E))
    print(f"\napplied {done}, already-named {skip}, epilogue stubs merged {merged}, "
          f"fall-through crefs {crefs}" + ("   [DRY_RUN]" if DRY_RUN else ""))
    print(f"seg000: {named_funcs}/{total} functions named")


main()
