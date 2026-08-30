"""
IDA Pro script: renames for gmb2.idb (GMB2.EXE) -- the **"Flip-Flop
Parlour"**, a Plinko / pachinko betting game, the second town gambling
minigame ("GMB" = gamble; GMB1 is BlackJack). Reached from a town,
chains back to TWNDR.EXE.

Recovered rules text: "FLIP-FLOP IS A GAME OF SKILL AND CHANCE. YOU DROP
A BALL [FROM THE TOP] OF THE SCREEN.  IT BOUNCES BUMPER TO BUMPER, AND
FALLS INTO A BUCKET AT THE BOTTOM ... YOU WIN GOLD BY GUESSING WHICH
BUCKET (1-6) THE BALL WILL FALL INTO.  YOU CAN ALSO WIN BY GUESSING THE
CORRECT [COLOR] ... SINCE THE BALL [IS LESS LIKELY] TO FALL INTO THE
OUTER BUCKETS, THEY PAY BACK MORE GOLD.  *** PAYOFFS *** BUCKETS 1-2:
[EVEN] / BUCKETS 3-4: DOUBLE / BUCKETS 5-6: FIVE TIMES ... THE BUMPERS
FLIP-FLOP BACK AND FORTH ... IF TIME RUNS OUT, WE'LL [DROP THE] BALL AND
PRETEND YOU CHOSE ..."  Plus a "COLOR BONUS". Bet 0 to quit; broke ->
"** YOU'VE LOST ALL YOUR GOLD **"; clean out the house -> "THE HOUSE IS
CLOSED."  Uses BIGNUM.DAT (big-digit font) and GW-BASIC DRAW macros for
the bumper / ball shapes.

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_gmb2

    .\run_ida_script.ps1 -Idb gmb2 -ScriptName apply_renames_gmb2.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# GMB2.EXE: single code seg seg000 "bmGMB2" (20 funcs), thunk table
# seg001 (467 entries), DGROUP seg003.
#
# (ea, new_name, note)
RENAMES = [
    (0x10030, "flipFlopMain",
     'the whole Flip-Flop Parlour game: loads BIGNUM.DAT, "REPEAT '
     'INSTRUCTIONS?", "WANT PRACTICE?", the bet loop ("ENTER BET (0 TO '
     'QUIT)", "WHAT BUCKET COLOR?", "BET <n> ON <x>", " AGAIN?"), '
     '"READY TO QUIT?", the outcome lines ("YOU SEEM TO HAVE THE '
     'KNACK!", "BETTER LUCK NEXT TIME.", "** YOU\'VE LOST ALL YOUR GOLD '
     '**", "THE HOUSE IS CLOSED."), then chains to TWNDR.EXE. ~3.3 KB.'),

    (0x127A4, "showInstructions",
     'renders the "** FLIP-FLOP PARLOUR **" rules screen -- a table of '
     '(indent, row, text) records ("YOU DROP A BALL ... IT BOUNCES '
     'BUMPER TO BUMPER ... *** PAYOFFS *** BUCKETS 1-2 ... 3-4 ... '
     '5-6 ..."). ~0.5 KB.'),

    (0x11DED, "playRound",
     'one wagered round: setup + win chime, dropBallAndBounce, then '
     'reads the result bucket and pays out -- "GOLD" / "BET" / "NUMBER" '
     '/ "COLOR", "EVEN MONEY." / "DOUBLE." / "FIVE TIMES.", "COLOR '
     'BONUS: <n>", "YOU WIN <n> GOLD". ~1.2 KB.'),
    (0x122C2, "playPracticeRound",
     'the "** PRACTICE TRY **" variant -- dropBallAndBounce with no '
     'wager / payout ("YOU WIN - MORE PRACTICE?" / "YOU LOST"). ~0.4 KB.'),
    (0x10DFB, "dropBallAndBounce",
     '"ENTER BUCKET NUMBER TO RELEASE BALL." then the ball-drop / '
     'bumper-bounce animation (stepBallPhysics + sound); returns the '
     'bucket it lands in. ~1.1 KB.'),
    (0x12578, "computePayout",
     'the odds / payout arithmetic (all value-stack ops rtm_FF4B / '
     'FF1F / FF20 / FF47 / FF50 over the struct at ds:1B96h) -- outer '
     'buckets pay more.'),

    (0x12B17, "drawBumpers",
     'draws the flip-flop bumper field from GW-BASIC DRAW macro strings '
     '("D4 BU4 BR1 D5 ...", colour "WHITE" etc).'),
    (0x10D0C, "playTune",
     'plays a PLAY/MML string ("MB ML L64 T75 N0" background, "MF ..." '
     'foreground).'),

    # --- tentative (no distinctive text) ---
    (0x1268F, "promptYesNo",
     'draw a prompt + read a Y/N keypress (string-compare rtm_FF0A). '
     'TENTATIVE.'),
    (0x129B6, "playBounceSound", 'short "MB ML L64 T75 N0" blip. TENTATIVE.'),
    (0x124C3, "playWinChime", 'plays "MF ML L64 T75" + a draw. TENTATIVE.'),
    (0x1131E, "drawBigNumberPanel",
     'renders a value with the BIGNUM.DAT digit font (GOLD / BET / '
     'winnings). TENTATIVE.'),
    (0x11576, "drawBallAnim",
     'draws a ball / cursor sprite frame from a DRAW macro. TENTATIVE.'),
    (0x12989, "stepBallPhysics",
     'one ball-position / bounce step inside dropBallAndBounce. TENTATIVE.'),
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
