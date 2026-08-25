"""
IDA Pro script: fixes a disassembly-resolution gap in GEN.EXE's
character-creation attribute code, flagged in docs/overview.md as a
possible struct-field mixup.

Background: `decreaseAttribute`, `increaseAttribute`, and
`updateAttribute` all index the same 7-word run of Savegame fields
(_hits, _strength, _agility, _stamina, _charisma, _wisdom,
_intelligence) via `[si]` where si = attr*2 -- i.e. a single array
that starts at _hits (index 0), with the 6 point-buy attributes at
indices 1-6. `decreaseAttribute` already displayed this correctly as
`_savegame._hits[si]`, but `increaseAttribute` and `updateAttribute`
showed the same address as a raw `word ptr [si+0A46h]` -- not because
the underlying computation differs (both are byte-identical: 0xA46 is
dseg's base (0x14260) + _hits' offset (0x16) = 0x14CA6, exactly
_savegame._hits), but because those 2 instructions were simply never
given the "offset" operand type IDA needs to resolve a bare immediate
back to a symbol, while decreaseAttribute's was (probably from
whichever earlier session first named `_hits`/`_strength` etc.).

**This confirms there is no real struct-field bug** -- the earlier
"stale/incorrect alternate field name" suspicion in overview.md was a
misdiagnosis of a display-only gap, not a genuine inconsistency. Once
these 2 instructions get the same operand type as the third, all three
functions read identically as indexing the same _hits-based array. See
the corrected writeup in docs/overview.md.

    .\\run_ida_script.ps1 -Idb ultima1_gen -ScriptName apply_structs_gen.py -NoExport
"""

import idc
import ida_segment

DRY_RUN = False

# (ea, note) -- the 2 unresolved operands. Both are operand 0 of their
# instruction.
FIXES = [
    (0x10E6E, "increaseAttribute's range-check cmp against _hits[si]"),
    (0x10E83, "increaseAttribute's inc of _hits[si]"),
    (0x107D4, "updateAttribute's push of _hits[si] for display"),
]


def main():
    dseg = ida_segment.get_segm_by_name("dseg")
    if dseg is None:
        print("[!] dseg segment not found -- skipping")
        return
    base = dseg.start_ea

    for ea, note in FIXES:
        before = idc.print_operand(ea, 0)
        if before.lower().startswith("_savegame"):
            print(f"{ea:#x}: already resolved ({before!r}) -- skipping")
            continue
        print(f"{ea:#x}: {before!r}  ({note})")
        if DRY_RUN:
            continue
        ok = idc.op_plain_offset(ea, 0, base)
        after = idc.print_operand(ea, 0)
        if not ok or not after.lower().startswith("_savegame"):
            print(f"    [!] op_plain_offset FAILED or didn't resolve -- got {after!r}")
        else:
            print(f"    -> {after!r}")

    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new display took.")


if __name__ == "__main__":
    main()
