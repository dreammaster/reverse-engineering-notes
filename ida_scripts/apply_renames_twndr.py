"""
IDA Pro script: renames for twndr.idb (TWNDR.EXE) -- the town driver
(entered from OUT.EXE's board at a town; chains back to OUT).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_twndr

Add (ea, name, note) entries as functions become clear -- from the
screen text a function prints (dump_strings.py) and the call graph.

    .\run_ida_script.ps1 -Idb twndr -ScriptName apply_renames_twndr.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# TWNDR.EXE links two compiled-BASIC code segments: seg000 "bmTWNDR"
# (the town driver) + seg001 "bmTNCALB" (town/castle animation helpers,
# 26 funcs, mostly no screen text). Thunk table is seg002 (only 431
# entries -- bare ordinals 0x00..0xD9, TWNDR uses fewer runtime routines).
#
# (ea, new_name, note)  -- names from the screen text (dump_strings.py)
RENAMES = [
    (0x10037, "twndr_entry", "TWNDR.EXE entry -> the init chain ('twnnum')."),

    (0x101A2, "doWalk", '"WALK ", "DISCOVERED!!" -- town movement.'),
    (0x10637, "chooseAbove", '"-CHOOSE ABOVE" menu helper.'),
    (0x106C0, "passTurn", '"PASS".'),
    (0x106F5, "changeGameSpeed",
     '"** CHANGE GAMESPEED ** / (1 IS FASTEST) / GAMESPEED IS: "'),
    (0x107A4, "walkBlocked", '"WALK OUT YOURSELF.", "MOVE NOWHERE".'),
    (0x1089C, "fightGuard",
     '"ENTER DIRECTION: ", "THE JAIL BARS HOLD.", "ATTACK ON GUARD '
     'MISSED", "GUARD STRUCK ", "GUARD KILLED". (IDA: j_rt_FE2C_3.) '
     '~1.4 KB.'),
    (0x10E29, "useMagicMenu", '"USE WHICH MAGIC?", "YOU HAVE NO ", "ATTACK WITH "'),
    (0x10F9F, "guardAttack",
     '"ATTACKED BY GUARD!", " -- MISSED", " -- BLOW ", "YOU DIED."'),
    (0x1119E, "speakCommand", '"SPEAK".'),
    (0x113C2, "foodShop",
     'the provisioner: "FOOD & WATER", "WE SELL FOOD FOR TRAVEL", "COST '
     'IS  GOLD PER \'DAY\'", "MAXIMUM PURCHASE:  DAYS", "THANKS FOR THE '
     'LETTER DELIVERY". ~1 KB.'),
    (0x1177F, "mailDeliveryJob",
     'NPC mail-carrying quest: "WOULD YOU LIKE TO EARN SOME", "HERE\'S '
     'SOME MAIL TO / DELIVER TO ", " DAYS OF FOOD".'),
    (0x11A62, "weaponShopEntry", '"WEAPONS".'),
    (0x11B0E, "armorShopEntry", '"ARMOR".'),
    (0x11CAC, "robberyEvent", '" ROBBERY IN PROGRESS ".'),
    (0x120A6, "townServiceDispatch",
     'the town shop/service dispatcher (~6 KB, called from sub_11ED0): '
     'the CONVIENENCE BANK ("1. DEPOSIT FUNDS / 2. WITHDRAW FUNDS / 3. '
     'BALANCE INQUIRY", "CURRENT BALANCE: "), item grab ("YOU COULDN\'T '
     'GRAB THE ", "YOU GET  GOLD."), and the shop counters.'),
    (0x13965, "notEnoughGold", '"YOU\'RE SHORT ON GOLD.".'),
    (0x13996, "promptQuantity",
     '"PURCHASE HOW MANY ", "Enter number and press <return> key: ".'),
    (0x13AAA, "pressKeyToContinue", '"PRESS KEY TO CONTINUE".'),
    (0x13AF8, "itemTakenOrBought", '"NOTHING ", "TAKEN.", "PURCHASED.".'),
    (0x13B90, "inventoryFull",
     '"NO PURCHASE.  YOU\'RE / CARRYING TOO MUCH.".'),
    (0x13BDA, "purchaseOrSteal",
     '" PURCHASED.", " STOLEN.". (IDA: j_rt_FE5B.)'),
    (0x13CB3, "merchantOffer",
     '"I\'LL PAY EXACTLY  GOLD / FOR YOUR ". (IDA: j_rt_FE45_1.)'),
    (0x13D7D, "buyTooMany", '"YOU CAN\'T BUY THIS MANY".'),
    (0x13E0B, "merchantRefuses", '"THE MERCHANT WON\'T LET YOU ".'),
    (0x13E65, "nothingInReach", '"NO ITEMS WITHIN REACH HERE.".'),
    (0x13EB3, "buyBackShop",
     '"BUY-BACK SHOP", "1. WEAPONS / 2. ARMOR", "SELECT (0 TO CANCEL)", '
     '"TOO HIGH!", "I\'LL GIVE ". (IDA: j_rt_FE2C_4.) ~2.4 KB.'),
    (0x14828, "declineDeal",
     '"MAYBE WE CAN DEAL LATER", "COME BACK WHEN YOU\'RE SERIOUS". '
     '(IDA: j_rt_FE5B_5.)'),
    (0x14900, "sellShopIntro",
     '"I WILL HAPPILY PURCHASE / YOUR USED ARMS AND ARMOR", "CHOOSE '
     'ITEMS TO SELL".'),
    (0x14997, "promptSellItem", '"WHAT  WILL YOU SELL ME?".'),
    (0x14ADD, "loanRepayment",
     'the debt collector: "LENDING ASSOCIATION", "YOU OWE:  GOLD!", '
     '"DUE DATE: ", "PAY HOW MUCH? (AT LEAST  GOLD)", "LOAN REPAID.". '
     '~1.5 KB.'),
    (0x150C4, "borrowMoney",
     'the loan office: "MONEY AT \'FRIENDLY\' RATES", "YOU MAY BORROW UP '
     'TO  GOLD", "BORROW HOW MUCH?", " GOLD BORROWED.", "YOU\'LL OWE ".'),
    (0x152C5, "fortuneTeller",
     '"I KNOW NO MORE.", "READ YOUR FORTUNE FOR  GOLD?".'),
    (0x154B0, "arrestedByGuards", '"THE GUARDS OVERWHELM YOU!".'),
    (0x15510, "jailScene", '"YOU FIND YOURSELF IN JAIL".'),
    (0x1560B, "jailRelease",
     '"I\'LL LET YOU OUT FOR A PRICE", "IT HAS COST  GOLD TO GET OUT.", '
     '"NOW - GET OUT OF TOWN!".'),
    (0x15974, "museumCoinOffer", '"WANT A MUSEUM COIN FOR  GOLD?".'),
    (0x15B77, "robCommand", '"ROB".'),
    (0x15CF0, "stealGold", '" BAGS OF GOLD!".'),
    (0x15EC4, "npcRecurringDialog",
     '"WE REMEMBER YOU - SLIME!", "THIS IS YOUR FRIENDLY LENDER" -- '
     'recurring-NPC lines keyed on prior encounters.'),
    (0x1624E, "loadTownData", '"TOWN", loads TCASOBJ.BSV.'),
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
