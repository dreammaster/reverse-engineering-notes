"""
IDA Pro script: fixes the long-flagged IDB hygiene issue in OUT.EXE
where `_nheapinit`'s proc boundary visually swallowed 3 unrelated
pieces of code that just happened to sit contiguously after it with no
gap:

  0x19418 - 0x1949D   the real _nheapinit (heap-init / SETBLOCK dance)
  0x1949E - 0x194CE   an unlabeled, uncalled block that loops through a
                       candidate-name list and calls execProgramEntry --
                       confirmed via idautils.CodeRefsTo that NOTHING
                       calls into 0x1949E (zero code refs, zero data
                       refs) and the preceding instruction is a retn,
                       so it can't be reached by fallthrough either.
                       Genuinely dead code, same family as the
                       already-documented dead sub_192AE. Left with
                       its auto-generated sub_ name -- not asserting
                       an identity with zero evidence.
  0x194CF - 0x1950C   execProgramEntry -- real function, called from
                       chainToExecutable, chainToExecutableAlt, AND
                       (harmlessly, since that call site is itself
                       unreachable) the dead block above.
  0x1950D - 0x19568   translateDosErrorToErrno -- real function,
                       called from every _dos_* primitive
                       (_dos_open/_read/_lseek/_write/_getfileattr/
                       _creat/_creattemp) plus _nheapinit's own error
                       path.

Both `execProgramEntry` and `translateDosErrorToErrno` already had
their names (as `loc_`-turned-named locations, not real functions) --
this script just gives them real function boundaries so the function
list and call graph read correctly, without touching any name.

    .\\run_ida_script.ps1 -Idb ultima1_out -ScriptName fix_nheapinit_boundary.py
"""

import idc
import ida_funcs

DRY_RUN = False

SPLITS = [
    (0x19418, 0x1949E, "_nheapinit (unchanged name, corrected end)"),
    (0x1949E, 0x194CF, "dead/uncalled block -- left as auto sub_ name"),
    (0x194CF, 0x1950D, "execProgramEntry (unchanged name, now a real function)"),
    (0x1950D, 0x19569, "translateDosErrorToErrno (unchanged name, now a real function)"),
]


def main():
    merged = ida_funcs.get_func(0x19418)
    if merged is None:
        print("[!] no function at 0x19418 -- already split? aborting")
        return
    if merged.start_ea != 0x19418 or merged.end_ea != 0x19569:
        print(f"[!] existing function bounds {merged.start_ea:#x}-{merged.end_ea:#x} "
              f"don't match expected 0x19418-0x19569 -- aborting, re-investigate")
        return

    # preserve names at the 2 already-labeled interior points before del_func
    names_before = {ea: idc.get_name(ea) for _, ea, _ in [(0, s, "") for s, e, n in SPLITS]}
    print("names before split:", {hex(k): v for k, v in names_before.items()})

    print(f"Deleting merged function 0x19418-0x19569")
    if DRY_RUN:
        print("[dry] would delete and re-add 4 separate functions:")
        for s, e, note in SPLITS:
            print(f"  {s:#x}-{e:#x} ({e-s} bytes): {note}")
        return

    ok = ida_funcs.del_func(0x19418)
    print(f"  del_func ok={ok}")

    for s, e, note in SPLITS:
        ok = ida_funcs.add_func(s, e)
        print(f"  add_func({s:#x}, {e:#x}) ok={ok}  -- {note}")

    print("\nnames after split:")
    for s, e, note in SPLITS:
        print(f"  {s:#x}: {idc.get_name(s)!r}")

    print("\nDone. Re-export the .asm/.idc and check the function list/names took.")


if __name__ == "__main__":
    main()
