"""
IDA Pro script: renames for gmb1.idb (GMB1.EXE) -- the **BlackJack**
("21") table, one of the two town gambling minigames ("GMB" = gamble;
GMB2 is the other). Reached from a town, chains back to TWNDR.EXE.

Recovered rules text: "You play against the dealer. Each of you is dealt
two cards. Choose 'HIT' if you want another / 'STAY' if you don't. To
win, be closest to 21 (without going over). Dealer draws when you Stay.
HOUSE RULES: Aces count as one or eleven / You win with five cards under
21 / Dealer stops with 17 or more. Natural BlackJack pays double."
Bet "0 to quit". If you go broke the house stakes you five gold once
("Rotten luck. Here's five gold to tide you over."); clean the house out
and "You broke the bank! The house is closed." Loads BJCHR.GLB (card
graphics).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_gmb1

    .\run_ida_script.ps1 -Idb gmb1 -ScriptName apply_renames_gmb1.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# GMB1.EXE: single code seg seg000 "bmGMB1" (~21 funcs), thunk table
# seg001 (431 entries), DGROUP seg003, card graphics in seg004.
#
# (ea, new_name, note)
RENAMES = [
    (0x10030, "blackjackMain",
     'the whole BlackJack game: "Do you want instructions?", the bet '
     'loop ("Enter your bet.  Enter 0 to quit."), deal, hit/stay, and '
     'every outcome line ("Dealer has BlackJack.", "You\'re over 21 - '
     'you lose.", "It\'s a tie.", "You Win!", "Dealer busts with", '
     '"Five cards without going over 21!", "Natural BlackJack pays '
     'double."), the broke/bankrupt handling, then chains to TWNDR.EXE. '
     '~3.2 KB.'),

    (0x111CF, "showInstructions",
     'the rules screen: "You play against the dealer ... Choose \'HIT\' '
     '... \'STAY\' ... To win, be closest to 21 ... HOUSE RULES: - Aces '
     'count as one or eleven. - You win with five cards under 21. - '
     'Dealer stops with 17 or more." ~0.9 KB.'),
    (0x1158B, "showWagerRules",
     '"Natural BlackJack / pays double.", "(To quit - bet 0)" -- shown '
     'before the first wager. TENTATIVE.'),
    (0x115F9, "pressKeyToContinue", '"(Press to continue)" pager.'),
    (0x116C2, "showGoldLine", '" GOLD: <n>" status line.'),
    (0x11759, "shuffleDeck", '"Shuffling..." -- reshuffles the shoe.'),
    (0x11076, "drawFromDeck",
     'take the next card from the shoe; if the shoe pointer (ds:1F1Ah) '
     'is exhausted, call shuffleDeck first.'),

    # --- table / hand rendering + dealing (no distinctive text) ---
    (0x10F1C, "dealCardToHand",
     'deal one card into a hand and recompute its total (value-stack '
     'ops rtm_FF4B/FF27/FF44/FF22); draws the card. TENTATIVE.'),
    (0x10EAA, "dealInitialHands",
     'deal the opening two cards to player and dealer. TENTATIVE.'),
    (0x10E09, "revealDealerCard",
     'flip the dealer\'s hole card / card-reveal animation. TENTATIVE.'),
    (0x10D4C, "drawGoldAndBet",
     'redraw the money area -- formats the gold / current-bet numbers '
     'and calls showGoldLine. TENTATIVE.'),
    (0x10CED, "drawDealerArea",
     'draw the dealer\'s side of the table (sprite blit rtm_FE46). '
     'TENTATIVE.'),
    (0x10CAC, "clearPromptLine",
     'clear the prompt/message line (rect at row 2). TENTATIVE.'),
    (0x11831, "drawHandSprites",
     'blit the card sprites for a hand (struct at ds:1C7Ch). TENTATIVE.'),
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
