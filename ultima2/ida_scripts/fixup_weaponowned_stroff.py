"""
IDA Pro script: apply struct-offset typing to the known `[di+76h]`
references so they display as `[di+Savegame._weaponOwned]` instead of
the raw displacement (that's the actual rendering op_stroff produces
here -- DI is being used as a plain array index against the fixed
global `player`, not as a pointer to a Savegame instance, so this reads
a little differently than the `player._armorOwned[di]`-style renders
`_armorOwned`/`_spellCharges` get from their pre-existing operand
typing, but it's the same numeric offset and a real readability win
over bare hex either way).

Why this is needed: `apply_structs.py` created `Savegame._weaponOwned`
at offset 0x76 (a brand-new member -- see that script's OPERATIONS
history) via add_struc_member, which defines the struct member itself
but does NOT retroactively re-type existing instruction operands that
happen to reference that offset. Compare to the same run's
`_armorOwned`/`_spellCharges`, which were *resizes* of members that
already existed at those offsets -- their instructions already carried
"this operand is a struct offset" typing from whenever the original
(undersized) member was first created, so the resize's rename carried
through automatically. A genuinely new member has no such history, so
its reference sites need an explicit op_stroff call each.

Purely cosmetic/readability -- the struct member itself is already
correct either way, this only affects how the known instructions in
ready/get/steal/zstats/transact are displayed in the .asm export.
First run (2026-08-17) fixed 7 sites across the first four; missed 2
more in `transact` (a "buy weapon from shopkeeper" purchase flow,
asm ~4160-4172 as of that pass -- `try_spend_gold` then "SOLD!" then
increments the same array) because `transact` wasn't in the scan list
yet -- added below.

Idempotent: op_stroff on an already-correctly-typed operand is a no-op
success, and this script also skips any operand that no longer shows
the bare `76h` displacement (rename it, and it means someone touched it
by hand since -- don't fight that).
"""

import idc
import idaapi

DRY_RUN = False

STRUCT_NAME = "Savegame"
MEMBER_OFFSET = 0x76

# Procs known to contain a `[di+76h]`-style reference -- see
# docs/overview.md#three-per-item-ownedcharges-flag-arrays or the
# apply_structs.py OPERATIONS entry for _weaponOwned. transact added
# after the first run missed its "buy weapon" site -- if more turn up
# after a future run, add them here too rather than assuming this list
# is exhaustive (it's assembled from what's been read, not a real
# search).
PROCS_TO_SCAN = ["ready", "get", "steal", "zstats", "transact"]


def scan_proc(struct_id, proc_name):
    func_ea = idc.get_name_ea_simple(proc_name)
    if func_ea == idaapi.BADADDR:
        print(f"[!] proc {proc_name!r} not found -- renamed since this "
              f"script was written?")
        return 0

    func = idaapi.get_func(func_ea)
    if func is None:
        print(f"[!] {proc_name!r} @ {func_ea:X} has no function object")
        return 0

    fixed = 0
    ea = func.start_ea
    while ea != idaapi.BADADDR and ea < func.end_ea:
        for opnum in (0, 1):
            optype = idc.get_operand_type(ea, opnum)
            if optype != idc.o_displ:
                continue
            if idc.get_operand_value(ea, opnum) != MEMBER_OFFSET:
                continue
            text = idc.print_operand(ea, opnum)
            print(f"{ea:X} (in {proc_name}) op{opnum}: {text!r} "
                  f"-- displacement matches 0x{MEMBER_OFFSET:X}")
            if DRY_RUN:
                continue
            ok = idc.op_stroff(ea, opnum, struct_id, 0)
            if ok:
                fixed += 1
            else:
                print(f"    [!] op_stroff FAILED at {ea:X} op{opnum}")
        ea = idc.next_head(ea, func.end_ea)
    return fixed


def main():
    struct_id = idc.get_struc_id(STRUCT_NAME)
    if struct_id == idc.BADADDR:
        print(f"[!] struct {STRUCT_NAME!r} not found")
        return

    total = 0
    for proc_name in PROCS_TO_SCAN:
        total += scan_proc(struct_id, proc_name)

    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply "
              "op_stroff to the sites printed above.")
    else:
        print(f"\nDone. Fixed up {total} operand(s). Re-export the .asm "
              f"and check the [di+76h] sites now show "
              f"[di+Savegame._weaponOwned].")


if __name__ == "__main__":
    main()
