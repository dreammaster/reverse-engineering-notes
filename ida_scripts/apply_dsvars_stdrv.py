"""
IDA Pro script: name the STDRV.EXE ("Stones of Wisdom" dice game) DGROUP
variables (STDRV's DGROUP is **seg003**, like OUT / MENU).

Found with `ida_scripts/dsvars.py` (DSV_DATA_SEG=seg003). STDRV is a
small minigame -- only 47 DGROUP words touched, and most are one-function
`mov ds:28xx, <const>` writes that stage the drawString text-field
layout for playerBidTurn / resolveChallenge. The handful of real state:

Shared LEGLIB slots line up: `partyGold` @ 1AD2, `menuChoice` @ 1E22.

Names data addresses + repeatable comments only. Run after
apply_renames_stdrv.py.

    .\run_ida_script.ps1 -Idb stdrv -ScriptName apply_dsvars_stdrv.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

VARS = [
    (0x1AD2, "partyGold",
     "party gold (32-bit; high word at 1AD4). resolveChallenge settles "
     "the wager against it. Same DGROUP slot as the other modules."),
    (0x1AD4, "partyGold_hi", "high word of partyGold (1AD2)."),
    (0x1E22, "menuChoice",
     "current Y/N / menu answer -- read by playerBidTurn, "
     "resolveChallenge, stonesOfWisdomMain (\"INSTRUCTIONS?\", \"PLAY "
     "AGAIN?\"). Same DGROUP slot as the other modules."),

    (0x1AF0, "intelligenceStat",
     "the character's INTELLIGENCE -- resolveChallenge computes the "
     "delta and stores it back here (\"YOUR INTELLIGENCE INCREASES / "
     "DECREASES BY\"). Read once at stdrv_entry (the starting value)."),

    (0x29CA, "stdrvArrayPtr",
     "far pointer (offset @ 29CA, segment @ 29CC) to STDRV's game-data "
     "array (dice / bid tables) -- pushed (seg then off) to the "
     "dice-evaluation helpers. Never written from seg000."),
    (0x29CC, "stdrvArrayPtr_seg", "segment word of stdrvArrayPtr (29CA)."),

    # --- tentative ---
    (0x28FE, "diceCount",
     "a dice tally / round counter -- reset to 0, inc'd and compared "
     "across dealerTurn / resolveChallenge / evalDiceOdds / the "
     "dice-eval helpers. TENTATIVE."),
    (0x2108, "playerBid",
     "the player's current bid (reset to 0 then -1 = 'none' by "
     "playerBidTurn). TENTATIVE."),
    (0x2106, "dealerBid",
     "the dealer's current bid (same -1 = none convention, "
     "resolveChallenge). TENTATIVE."),
    (0x1EF2, "gameScore",
     "a 32-bit accumulator (high word at 1EF4) stdrv_entry seeds and "
     "resolveChallenge updates. Role unclear -- possibly a running "
     "score or the match pot. TENTATIVE."),
    (0x1EF4, "gameScore_hi", "high word of gameScore (1EF2). TENTATIVE."),
    (0x28B8, "instructionsSeenFlag",
     "set to 0 at stdrv_entry; checked by sub_101FE / sub_12C9D -- "
     "probably 'instructions already shown'. TENTATIVE."),
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
