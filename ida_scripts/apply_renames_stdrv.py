"""
IDA Pro script: renames for stdrv.idb (STDRV.EXE) -- the **"Stones of
Wisdom" dice game**, a museum-exhibit minigame (NOT a "story driver"
despite the STDRV name). Chained from MUS.EXE for the "STONES OF WISDOM"
exhibit (see exhibitName_stonesOfWisdom in apply_renames_mus.py).

Stones of Wisdom is a Liar's-Dice / Perudo variant: the player and the
"DEALER" each get five dice; players take turns bidding a (quantity,
value) pair, e.g. "THREE FIVES"; a bid can be raised or CHALLENGEd; the
loser of a challenge gives up one die; last player with any dice wins the
match. Winning/losing a match changes the character's INTELLIGENCE. Each
replay costs gold ("PLAY AGAIN FOR <n> GOLD", "YOU DON'T HAVE ENOUGH
GOLD TO PLAY AGAIN."). Reads STDRVSCR.DAT (the rules / instruction text).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_stdrv

    .\run_ida_script.ps1 -Idb stdrv -ScriptName apply_renames_stdrv.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# STDRV.EXE: single code segment seg000 "bmSTDRV" (39 funcs), thunk table
# seg001, DGROUP seg003.
#
# (ea, new_name, note)  -- names from the screen text (dump_strings.py /tmp/stdfn.py)
RENAMES = [
    (0x10030, "stdrv_entry",
     'STDRV.EXE entry. Builds the number-word table "NO"/"ONE"/"TWO".."NINE" '
     'used to render bids, then runs the game. ("BID".)'),

    (0x10584, "playerBidTurn",
     'the player\'s turn: "YOUR TURN:", "HOW MANY DICE?", "OF WHAT VALUE?", '
     '"CHALLENGE!!", "DEALER BIDS", "ONE  SIXES" (bid echo). ~1.9 KB.'),
    (0x10D24, "resolveChallenge",
     'challenge resolution + match bookkeeping: "YOU"/"DEALER", "CHALLENGE", '
     '"- YOU WIN."/"- YOU LOST.", "YOUR INTELLIGENCE / INCREASES BY". ~2.9 KB.'),
    (0x129A9, "formatBidText",
     'formats a (quantity,value) bid into text, e.g. "ONE  SIXES".'),
    (0x12F82, "stonesOfWisdomMain",
     'top-level game loop: loads STDRVSCR.DAT, "DEALER:", "INSTRUCTIONS?", '
     '"WOULD YOU LIKE TO SEE THESE INSTRUCTIONS AGAIN?", "ERROR", the '
     'per-match / play-again loop. ~2.5 KB.'),

    # --- no player-facing strings; roles inferred from call graph ---
    (0x11900, "dealerTurn",
     'dealer AI controller (no player-facing text): decides the dealer\'s '
     'bid or challenge; drives the dice-odds helpers (sub_12454 / sub_12C9D). '
     'TENTATIVE.'),
    (0x12454, "evalDiceOdds",
     'dice / bid probability evaluation used by dealerTurn. TENTATIVE.'),
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
