"""
IDA Pro script: un-collapses (expands) the 7 functions found folded in
the IDB -- set_cga_mode, setPalette, draw_tile, draw_map_content,
animate_water, animate_forcefield, animate_tile (all clustered right
after access_file, asm ~8659-8665). Collapsed functions show only a
"[NN BYTES: COLLAPSED FUNCTION ...]" placeholder in the .asm export
instead of their real instructions -- found while investigating DOS/
BIOS interrupt dependencies (couldn't read set_cga_mode/setPalette's
bodies to check for int 10h calls).

Collapsed functions are controlled by a per-function flag
(ida_funcs.FUNC_HIDDEN on func_t.flags), not the generic "hidden
range" mechanism (checked first -- 0 hidden ranges existed in this
idb, despite 7 visibly collapsed functions in the .asm export).

USAGE: DRY_RUN=False -- this is a pure visibility/presentation change
(same underlying bytes/instructions, just toggling whether they're
folded in the view), not a data or naming change, so no dry-run
ambiguity to review.
"""

import ida_funcs
import idc

DRY_RUN = False

NAMES = [
    "set_cga_mode",
    "setPalette",
    "draw_tile",
    "draw_map_content",
    "animate_water",
    "animate_forcefield",
    "animate_tile",
]


def main():
    expanded = 0
    for name in NAMES:
        ea = idc.get_name_ea_simple(name)
        if ea == idc.BADADDR:
            print(f"{name!r}: [!] not found, skipping")
            continue
        pfn = ida_funcs.get_func(ea)
        if pfn is None:
            print(f"{name!r} @ {ea:X}: [!] no function object, skipping")
            continue
        if not (pfn.flags & ida_funcs.FUNC_HIDDEN):
            print(f"{name!r} @ {ea:X}: already expanded -- skipping")
            continue
        print(f"{name!r} @ {ea:X}: collapsed (FUNC_HIDDEN set)")
        if DRY_RUN:
            print("    [dry] would clear FUNC_HIDDEN")
            continue
        pfn.flags &= ~ida_funcs.FUNC_HIDDEN
        if ida_funcs.update_func(pfn):
            print("    expanded")
            expanded += 1
        else:
            print("    [!] update_func FAILED")

    print(f"\nDone. Expanded {expanded} function(s).")


if __name__ == "__main__":
    main()
