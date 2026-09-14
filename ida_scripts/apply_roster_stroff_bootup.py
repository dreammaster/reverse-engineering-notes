"""
ultima_bootup.idb counterpart to apply_roster_stroff.py -- wires the
RosterEntry struct into real instruction operands. Candidates found
via find_roster_stroff_candidates.py's read-only scan; this IDB's
result set was much cleaner than ultima_exodus.idb's (no misdecoded
data-table region turned up here), but the same false-positive
categories from that pass still apply and are excluded on the same
basis:
  - offset 0x0 (_name) matches (getMenuChoice, promptForNumberEntry)
    -- coincidental, same reasoning as the exodus pass.
  - updateLogoAnimationB/swapAnimTableRows matching 0x20/0x26 --
    boot-animation code, topically unrelated to character records.

Run once:
    .\\run_ida_script.ps1 -Idb ultima_bootup -ScriptName apply_roster_stroff_bootup.py
"""

import idc
import idautils
import ida_funcs
import ida_struct

DRY_RUN = False

STRUCT_NAME = "RosterEntry"

ALLOWED_FUNCS = {
    "showCharacterDetails", "handleCreateCharacter",
    "gatherCharacterCreationInput", "showRegister", "handleFormParty",
    "clearPartySelection", "handleTerminateCharacter",
}


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
    offsets.pop(0, None)  # _name -- see module docstring
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
        for n in range(2):
            if idc.get_operand_type(ea, n) != idc.o_displ:
                continue
            disp = idc.get_operand_value(ea, n) & 0xFFFF
            if disp in offsets:
                sites.append((ea, n))
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
