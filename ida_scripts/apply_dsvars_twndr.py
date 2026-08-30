"""
IDA Pro script: name the TWNDR.EXE engine state variables in the DGROUP
segment (TWNDR's DGROUP is **seg004**, like DUN; seg002 = thunk table,
seg003 = RTM bootstrap).

Found with `ida_scripts/dsvars.py` (DSV_DATA_SEG=seg004). Of the 321
DGROUP words the code touches, the ~10 below are genuine engine state.

TWNDR shares LEGLIB's engine skeleton so several slots line up with
OUT / DUN: `partyGold` @ 1AD2, `hitPoints` @ 1ADA (the same offsets the
other modules use). The town-specific state (shop / guard / service id)
sits in the 0x1Fxx / 0x20xx / 0x21xx block.

Names data addresses + repeatable comments only. Run after
apply_renames_twndr.py.

    .\run_ida_script.ps1 -Idb twndr -ScriptName apply_dsvars_twndr.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

VARS = [
    (0x1AD2, "partyGold",
     "party gold (32-bit; high word at 1AD4). Every shop / bank / "
     "moneylender / fortune-teller reads it via `mov ax,1AD2 / mov "
     "dx,1AD4` and settles the transaction with rtm_EE. Same DGROUP "
     "slot as OUT.EXE / DUN.EXE."),
    (0x1AD4, "partyGold_hi", "high word of partyGold (1AD2)."),
    (0x1ADA, "hitPoints",
     "party hit points -- guardAttack subtracts damage. Same slot as "
     "OUT.EXE / DUN.EXE."),

    (0x1F22, "townServiceId",
     "id of the town building / service the player is using. "
     "townServiceDispatch is one big SELECT CASE on it (0 / 2 / 4 / 5 / "
     "9 / 0x0A / 0x0B = food shop / weapon / armor / bank / ... ); "
     "never written from seg000 (set by the bmTNCALB tile engine)."),
    (0x1F02, "tileAhead",
     "code for the tile / object in front of the player -- doWalk / "
     "walkBlocked / fightGuard branch on it (0xD7, 0xFD, 0xFE = special "
     "tiles; 0x40 = a guard, etc.)."),
    (0x216E, "guardHitPoints",
     "the guard's hit points during fightGuard / guardAttack "
     "(accumulator: `mov ax,216E / sub ax,216E / mov 216E,ax`)."),

    (0x278C, "townArrayPtr",
     "far pointer (offset @ 278C, segment @ 278E) to the main TWNDR "
     "game-data array -- pushed (seg then off) to nearly every rtm_ "
     "call (rtm_B8). Never written from seg000. Same role as OUT's "
     "overworldArrayPtr / DUN's dungeonArrayPtr."),
    (0x278E, "townArrayPtr_seg", "segment word of townArrayPtr (278C)."),

    # --- tentative ---
    (0x1E22, "menuChoice",
     "current menu / spell-menu selection -- useMagicMenu reads it as "
     "the spell index (`cmp ds:1E22h, 0x0B`), the shops as the option "
     "chosen. TENTATIVE."),
    (0x1F2A, "turnFlag",
     "1 after a turn-consuming / attention-drawing action (doWalk, "
     "fightGuard, stealGold, npcRecurringDialog set it; jailScene and "
     "the dialog code clear it). TENTATIVE."),
    (0x1F16, "shopWorkQty",
     "working quantity / haggle amount in the shop code (merchantOffer "
     "sets 0x63 = 99 max; promptSellItem / loanRepayment / buyBackShop "
     "read & update it). TENTATIVE."),
    (0x1E20, "viewMode",
     "read-only context constant (12 / 13) tested by doWalk and every "
     "shop entry -- probably 'in a building' vs 'on the street'. "
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
    s4 = ida_segment.get_segm_by_name("seg004")
    base = s4.start_ea
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
