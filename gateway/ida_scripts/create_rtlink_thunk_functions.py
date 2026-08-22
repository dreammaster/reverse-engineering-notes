"""
One-off structural script (post-regeneration, 2026-08-23): finds EVERY
call site to rtlink_thunk via real code cross-references
(idautils.CodeRefsTo), not by scanning idautils.Functions() the way the
older find_rtlink_thunks.py/apply_rtlink_thunks_gatemain.py did --
that approach silently misses any "call rtlink_thunk; jmp <target>"
shape sitting in code the fresh IDB's auto-analysis never wrapped in a
function boundary (see survey_rtlink_thunk_sites.py's findings and
docs/overview.md's "orphan code" note: 1119 of 1272 call sites, and
1129 of their jump targets, were NOT recognized as functions after the
2026-08-23 regeneration).

For every call site found:
  1. Verify the exact shape: `call near ptr rtlink_thunk` (E8, 3 bytes)
     immediately followed by a jmp (E9 near or EA far) to the real
     target, in a DIFFERENT overlay segment (that's the whole reason
     RTLink needed a thunk here -- a same-segment far call/jmp needs no
     thunk at all).
  2. If the jmp target has no function defined starting exactly there,
     create one (idc.add_func(target_ea), letting IDA's own analyzer
     determine the extent -- it's real subroutine code, not boilerplate).
  3. If the call site itself has no function defined starting exactly
     there, create one spanning exactly the 2-instruction thunk (its
     size is fully determined: 3 bytes for the call + the jmp's own
     instruction size, either 3 (near) or 5 (far)).
  4. Rename the call-site function to thunk_<target-name>, exactly the
     convention find_rtlink_thunks.py/apply_rtlink_thunks_gatemain.py
     established originally (thunk_sub_XXXXX for still-unnamed targets
     is expected and fine -- re-running this script after a later pass
     names more targets will refresh the stale ones, same maintenance
     note as before).

Skips the one legitimate edge case from the original scripts: a jmp
landing in a tail chunk IDA attributes back to the SAME function
starting at the call site -- a relocated continuation, not a genuine
cross-overlay thunk (shouldn't occur here since every real
`call rtlink_thunk` site is by construction a cross-segment call, but
kept as a defensive check).

Convention: DRY_RUN starts True. Run once with DRY_RUN True, check the
output, then flip and re-run to actually create functions and rename.

    .\\run_ida_script.ps1 -Idb gatemain -ScriptName create_rtlink_thunk_functions.py -NoExport
"""

import idc
import idautils
import ida_bytes
import ida_funcs

DRY_RUN = False

rtlink_thunk_ea = idc.get_name_ea_simple("rtlink_thunk")
assert rtlink_thunk_ea != idc.BADADDR, "rtlink_thunk not found -- wrong IDB?"


def find_thunk_sites():
    for ea in sorted(set(idautils.CodeRefsTo(rtlink_thunk_ea, 0))):
        if ida_bytes.get_byte(ea) != 0xE8:
            continue
        jmp_ea = ea + 3
        jmp_b0 = ida_bytes.get_byte(jmp_ea)
        if jmp_b0 not in (0xE9, 0xEA):
            continue
        jmp_size = idc.get_item_size(jmp_ea)
        if jmp_size <= 0:
            # not currently disassembled as an instruction at all
            jmp_size = 3 if jmp_b0 == 0xE9 else 5
        targets = list(idautils.CodeRefsFrom(jmp_ea, 0))
        if not targets:
            continue
        target_ea = targets[0]
        # Defensive: skip a jmp landing in a tail chunk already
        # attributed to a function starting at this exact call site
        # (a split body, not a real cross-segment thunk).
        if idc.get_func_attr(target_ea, idc.FUNCATTR_START) == ea:
            continue
        thunk_end = jmp_ea + jmp_size
        yield ea, thunk_end, target_ea


def main():
    print(f"DRY_RUN = {DRY_RUN}")

    sites = list(find_thunk_sites())
    print(f"{len(sites)} candidate rtlink_thunk call sites found")

    targets_needing_func = 0
    targets_created = 0
    targets_create_failed = []

    calls_needing_func = 0
    calls_created = 0
    calls_create_failed = []

    renamed = 0
    already_named = 0
    rename_failed = []

    # First pass: make sure every jump target has a function, so the
    # rename pass below can pick up a real name (possibly one created
    # just now) rather than a bare loc_/off_ label.
    seen_targets = set()
    for ea, thunk_end, target_ea in sites:
        if target_ea in seen_targets:
            continue
        seen_targets.add(target_ea)
        if idc.get_func_attr(target_ea, idc.FUNCATTR_START) == target_ea:
            continue
        targets_needing_func += 1
        if DRY_RUN:
            continue
        ok = ida_funcs.add_func(target_ea, idc.BADADDR)
        if ok:
            targets_created += 1
        else:
            targets_create_failed.append(target_ea)

    # Second pass: make sure every call site itself has a function
    # (exact, deterministic size), then rename it.
    for ea, thunk_end, target_ea in sites:
        if idc.get_func_attr(ea, idc.FUNCATTR_START) != ea:
            calls_needing_func += 1
            if not DRY_RUN:
                ok = ida_funcs.add_func(ea, thunk_end)
                if ok:
                    calls_created += 1
                else:
                    calls_create_failed.append(ea)
                    continue

        target_name = idc.get_func_name(target_ea) or idc.get_name(target_ea) or f"{target_ea:#x}"
        new_name = f"thunk_{target_name}"
        old_name = idc.get_name(ea)
        if old_name == new_name:
            already_named += 1
            continue
        if DRY_RUN:
            continue
        ok = idc.set_name(ea, new_name, idc.SN_NOCHECK)
        if ok:
            renamed += 1
        else:
            rename_failed.append((ea, new_name))

    print(f"\n{targets_needing_func} jump targets need a function created")
    if not DRY_RUN:
        print(f"  {targets_created} created ok, {len(targets_create_failed)} failed")
        for ea in targets_create_failed[:30]:
            print(f"    FAILED target: {ea:#x}")

    print(f"\n{calls_needing_func} call sites need a function created")
    if not DRY_RUN:
        print(f"  {calls_created} created ok, {len(calls_create_failed)} failed")
        for ea in calls_create_failed[:30]:
            print(f"    FAILED call site: {ea:#x}")

    if not DRY_RUN:
        print(f"\n{renamed} call sites renamed, {already_named} already correctly named")
        for ea, new_name in rename_failed[:30]:
            print(f"    FAILED rename: {ea:#x} -> {new_name!r}")


main()
