"""
One-off structural fix for ultima.idb: two regions of real code that IDA
never recognized as functions (no proc/endp boundary), discovered while
reading through ultima.asm but only confirmed as legitimate, reachable
code once ultima_bootup.idb turned out to contain byte-for-byte
identical logic with real, confirmed callers there (promptForNumberEntry
and printHexWord -- see apply_renames_bootup.py's notes on those two
names, and docs/overview.md's findings log).

Both regions sit immediately before an already-named function and have
no preceding proc/endp of their own -- classic case for
ida_funcs.add_func(start, end) using the already-named function's start
as the end boundary, then idc.set_name on the newly-created function.

Region 1: printHexWord, ends where printHexByte (0x18B43) begins.
Region 2: promptForNumberEntry, ends where openFileWithRetry (0x18D63)
begins -- this one also depends on accumulateInputDigit (0x18B6E) and
printHexByte, both already named, confirming the call graph matches
BOOTUP.BIN's promptForNumberEntry exactly.

Run against ultima.idb specifically:
    .\\run_ida_script.ps1 -Idb ultima -ScriptName fix_orphaned_functions.py
"""

import idc
import ida_funcs

DRY_RUN = False

# (function_end_ea, new_name, note)
#
# NOTE: only printHexWord is fixed here. There's also a promptForNumberEntry-
# equivalent orphan in this IDB (calls accumulateInputDigit/readLine, around
# loc_18C39/loc_18C7F per the CODE XREF comments on swapCursorPos/
# playSoundEffect) plus a separate save-file-writer orphan (calls
# openFileWithRetry from ~0x18CD9, mirrors loadFile with AH=15h write) --
# both confirmed real via ultima_bootup.idb's identical, properly-bounded
# copies (promptForNumberEntry/saveFile), but this script's naive
# "walk back to the nearest retn" heuristic got their boundaries wrong on
# the first attempt (it doesn't handle nested loops/branches within the
# orphan correctly) and a wrong add_func call risks corrupting the
# flow graph. Left as a documented, unfixed finding in docs/roadmap.md
# rather than risk it -- do this one by hand in the IDA GUI (temporarily,
# read-only) or with a more careful boundary-finding script next time.
FIXUPS = [
    (0x18B43, "printHexWord",
     "orphaned code ending right before printHexByte -- same logic as "
     "ultima_bootup.idb's printHexWord (0x14858), which has a "
     "confirmed real caller there. Prints AX as 4 hex digits. "
     "Boundary double-checked: exactly 11 bytes (0x18B38-0x18B43), "
     "matching xchg+call+xchg+call+retn's expected encoded length."),
]


def find_function_start(end_ea):
    """The orphan block itself ends in its own `retn` (the instruction
    right before end_ea) -- skip past that one, then walk backward
    looking for the NEXT `retn`, which belongs to whatever function
    precedes the orphan. The orphan starts right after that one."""
    ea = idc.prev_head(end_ea)  # the orphan's own closing retn
    ea = idc.prev_head(ea)      # start the real search before that
    while ea != idc.BADADDR:
        if idc.print_insn_mnem(ea) == "retn":
            return idc.next_head(ea)
        ea = idc.prev_head(ea)
    return None


def main():
    for func_end, new_name, note in FIXUPS:
        cur_name_at_end = idc.get_func_name(func_end)
        print(f"target boundary: {func_end:#x} (currently {cur_name_at_end!r})")

        start = find_function_start(func_end)
        if start is None:
            print(f"  [!] could not find a preceding retn -- skipping")
            continue
        print(f"  found orphan start: {start:#x}")

        existing_name = idc.get_func_name(start)
        if existing_name == new_name:
            print(f"  already named {new_name!r} -- skipping")
            continue

        print(f"  {start:#x}..{func_end:#x} -> add_func + rename to {new_name!r}")
        print(f"    {note}")
        if DRY_RUN:
            continue

        ok = ida_funcs.add_func(start, func_end)
        if not ok:
            print(f"    [!] add_func FAILED (already a function? overlapping?)")
            continue
        ok = idc.set_name(start, new_name, idc.SN_NOWARN)
        if not ok:
            print(f"    [!] set_name FAILED")

    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names took.")


if __name__ == "__main__":
    main()
