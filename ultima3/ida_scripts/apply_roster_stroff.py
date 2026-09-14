"""
Wires the RosterEntry struct into real instruction operands
(idc.op_stroff), per the roadmap's "apply the struct type to actual
[bx+N] operands" item. Candidates were found via
find_roster_stroff_candidates.py's read-only scan (every [reg+N]
operand in the IDB where N matches a RosterEntry member offset,
grouped by containing function).

Restricted to an explicit ALLOWED_FUNCS list rather than applying
blindly wherever an offset happens to match -- several candidates the
raw scan turned up were false positives from unrelated code that just
happens to share a small offset value:
  - Functions matching ONLY offset 0x0 (_name) -- promptForQuantity,
    moveDungeonMonsterTowardParty, getMenuChoice,
    promptForNumberEntry -- 0 is an extremely common displacement in
    unrelated code (loop counters, [bp+0] locals, array bases).
  - swapAnimTableRows/updateLogoAnimationB (boot-animation code,
    topically unrelated to character records) matching 0x20/0x26
    coincidentally.
  - checkTerrainMovementBlocked's `test byte ptr [bx+0Eh], 10h/20h`
    -- a bitmask test shape RosterEntry's own confirmed
    _marksAndCards usage (cmdYell) doesn't match; more likely an
    unrelated structure sharing the offset by coincidence. Excluded.
  - A "???" bucket the raw scan couldn't attribute to a function
    turned out to mix a genuine spell-effect body (real RosterEntry
    hits) with an unrelated `[bp+0]` local-variable false positive in
    a different code region -- handled via EXTRA_SITES below instead
    of a blanket function-name allow, since spell effects aren't real
    IDA functions to begin with.

Run once per IDB (struct membership and function addresses both
differ, though the member names are identical):

    .\\run_ida_script.ps1 -Idb ultima_exodus -ScriptName apply_roster_stroff.py
"""

import idc
import idautils
import ida_funcs
import ida_struct

DRY_RUN = False

STRUCT_NAME = "RosterEntry"

ALLOWED_FUNCS = {
    "showZtats", "readAndDispatchCommand", "drawPartyStatusBar",
    "processPartyTurnEffects", "updateMonsterAI", "showTempleMenu",
    "showGuildMenu", "showWeaponsShopMenu", "showArmourShopMenu",
    "rollTrapEvasionChance", "healHitPoints", "castSpell", "enterShrine",
    "damageCharacterHP", "applyHungerTick", "readyWeapon", "wearArmour",
    "generateChestLoot", "showGrocerMenu", "addExperienceClamped",
    "addGoldClamped", "isCharacterAlive", "regenerateMagicPoint",
    "attemptSpecialMonsterAttack", "applyDungeonMonsterDamage",
    "deductGoldIfAffordable", "showTavernMenu",
}

# Individual (ea, opnum) sites outside a real IDA function (spell
# effects are labels, not procs) confirmed by hand from the "???"
# bucket -- spellAlcort's poison-cure body (sets _status to 'G' and
# adds to _magicPoints... actually checked: these are the cure/status
# fields, confirmed via direct disassembly read, NOT the [bp+0]
# false-positive sites from the same bucket).
EXTRA_SITES = [
    (0x16217, 0),  # cmp byte ptr [bx+11h], 50h  (_status)
    (0x16223, 0),  # mov byte ptr [bx+11h], 47h  (_status)
    (0x16232, 1),  # add al, [bx+19h]            (_magicPoints)
]

# A large data blob (very likely ASCII text) is misdecoded as code
# somewhere around 0x16700-0x16820+ inside readAndDispatchCommand's
# own function-chunk list (real, garbage 386-only instructions like
# `arpl`/`gs:`/`fs:` prefixes that can't appear in this program's real
# 8086/80186-era code, jumps into mid-instruction addresses like
# `loc_16773+1`). This produces spurious [reg+N] "hits" that happen to
# share RosterEntry's offset values purely by byte-pattern coincidence
# -- confirmed by hand-reading the actual .asm around several of them.
# A separate, genuine finding worth fixing on its own later (flagged
# in the roadmap); excluded wholesale here since none of it is real
# RosterEntry-touching code.
EXCLUDED_RANGES = [
    (0x16700, 0x16900),
]


def is_excluded(ea):
    return any(lo <= ea < hi for lo, hi in EXCLUDED_RANGES)


def get_member_offsets(sid):
    offsets = {}
    size = ida_struct.get_struc_size(sid)
    seen = set()
    m = ida_struct.get_struc(sid)
    for o in range(size):
        mem = ida_struct.get_member(m, o)
        if mem and mem.soff not in seen:
            seen.add(mem.soff)
            offsets[mem.soff] = ida_struct.get_member_name(mem.id)
    # Offset 0 (_name) excluded: a first real-run dry-check showed every
    # single [reg+0] "hit" in this codebase is a false positive --
    # misdecoded data-table garbage (`imul sp, [ebx+0], 6946h`,
    # `arpl [di+0], sp`) or ordinary stack-frame locals (`mov di,
    # [bp+0]`) that coincidentally share the same small displacement.
    # _name is a 10-byte buffer real accesses would loop over anyway,
    # not touch via a single [reg+0].
    offsets.pop(0, None)
    return offsets


def main():
    sid = idc.get_struc_id(STRUCT_NAME)
    if sid == idc.BADADDR:
        print(f"[!] struct {STRUCT_NAME!r} not found")
        return
    offsets = get_member_offsets(sid)

    sites = []
    for ea in idautils.Heads():
        f = ida_funcs.get_func(ea)
        if not f:
            continue
        fname = idc.get_func_name(f.start_ea)
        if fname not in ALLOWED_FUNCS:
            continue
        if is_excluded(ea):
            continue
        for n in range(2):
            if idc.get_operand_type(ea, n) != idc.o_displ:
                continue
            disp = idc.get_operand_value(ea, n) & 0xFFFF
            if disp in offsets:
                sites.append((ea, n))
    sites.extend(EXTRA_SITES)
    sites.sort()

    print(f"{len(sites)} site(s) to apply op_stroff to"
          f" ({'DRY RUN' if DRY_RUN else 'APPLYING'})")
    ok = 0
    fail = 0
    for ea, n in sites:
        before = idc.GetDisasm(ea)
        if DRY_RUN:
            print(f"  {ea:#06x} op{n}: {before}")
            continue
        applied = idc.op_stroff(ea, n, sid, 0)
        after = idc.GetDisasm(ea)
        status = "ok" if applied else "FAILED"
        if applied:
            ok += 1
        else:
            fail += 1
        print(f"  {ea:#06x} op{n}: {status}  {before}  ->  {after}")

    if not DRY_RUN:
        print(f"\nDone: {ok} applied, {fail} failed.")


if __name__ == "__main__":
    main()
