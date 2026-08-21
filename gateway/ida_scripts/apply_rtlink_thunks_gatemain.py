"""
One-off structural script (not a curated RENAMES list -- see
find_rtlink_thunks.py's docstring and docs/overview.md's "RTLink overlay
architecture" section for the full discovery writeup): batch-renames
every gatemain.idb function matching the RTLink call-thunk shape

    call near ptr rtlink_thunk
    jmp  <real target, in another overlay segment>

to thunk_<target-name>, e.g. a stub jumping to save_game becomes
thunk_save_game; one jumping to a still-unnamed sub_674A7 becomes
thunk_sub_674A7. 955 functions matched in the survey this was written
against, all with distinct targets (no collisions to resolve).

**Maintenance note**: many targets are themselves not yet named
(sub_XXXXX), so a good fraction of the names this produces
(thunk_sub_XXXXX) will go stale text-wise once that target function
later gets a real name -- the EA reference stays correct, only the
thunk's own label becomes a missed opportunity for a better name. Safe
to just re-run this script after a future pass renames more targets;
already-renamed thunks get harmlessly re-set to the same or an updated
name (idc.set_name is idempotent here since the scan re-derives the
target fresh each time).

Convention: DRY_RUN starts True. Run once with DRY_RUN True, check the
output, then flip and re-run.

    .\\run_ida_script.ps1 -Idb gatemain -ScriptName apply_rtlink_thunks_gatemain.py -NoExport
"""

import idc
import idautils
import ida_bytes

DRY_RUN = False

rtlink_thunk_ea = idc.get_name_ea_simple("rtlink_thunk")
assert rtlink_thunk_ea != idc.BADADDR, "rtlink_thunk not found -- wrong IDB?"


def find_thunks():
    for ea in idautils.Functions():
        fname = idc.get_func_name(ea)
        if not fname.startswith("sub_"):
            continue
        end = idc.get_func_attr(ea, idc.FUNCATTR_END)
        size = end - ea
        if size > 12 or size < 5:
            continue
        if ida_bytes.get_byte(ea) != 0xE8:  # call rel16
            continue
        jmp_ea = ea + 3
        if rtlink_thunk_ea not in idautils.CodeRefsFrom(ea, 0):
            continue
        targets = list(idautils.CodeRefsFrom(jmp_ea, 0))
        if not targets:
            continue
        target_ea = targets[0]
        # Some far jmps land in a tail chunk IDA attributes back to this
        # SAME function (a relocated continuation, not a call to a
        # different function) -- get_func_name(target_ea) then just
        # returns fname again. That's not a thunk-to-another-function,
        # skip it.
        target_func_start = idc.get_func_attr(target_ea, idc.FUNCATTR_START)
        if target_func_start == ea:
            continue
        target_name = idc.get_func_name(target_ea) or idc.get_name(target_ea)
        if not target_name:
            continue
        yield ea, target_name


def main():
    print(f"DRY_RUN = {DRY_RUN}")
    count = 0
    skipped = 0
    for ea, target_name in find_thunks():
        new_name = f"thunk_{target_name}"
        old_name = idc.get_name(ea)
        if old_name == new_name:
            skipped += 1
            continue
        if DRY_RUN:
            print(f"{ea:#x}: {old_name!r} -> {new_name!r}")
        else:
            ok = idc.set_name(ea, new_name, idc.SN_NOCHECK)
            if not ok:
                print(f"{ea:#x}: FAILED to rename to {new_name!r}")
        count += 1
    print(f"\n{count} thunks {'would be ' if DRY_RUN else ''}renamed, "
          f"{skipped} already correctly named")


main()
