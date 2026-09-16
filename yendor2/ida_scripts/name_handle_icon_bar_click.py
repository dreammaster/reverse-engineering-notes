"""
Names sub_270FE, called from `start` and HandleDungeonInput: hit-tests
the same region table (0x636C) TryCureAilmentFromIconClick uses (with
a different mouse-position pair, word_2E76E/word_2E770 vs. that
function's word_2E772/word_2E774). Hit zones 1-3 dispatch to the
still-untraced sub_271DC; zones 4+ compute an index into a table at
0x950D (stride 4, `(hit-1)*4 + 0x950D`) -- which for hit=4..9 lands on
the *exact same addresses* (0x9519, 0x951D, ..., 0x952D)
TryCureAilmentFromIconClick's 6-entry ailment table uses. This implies
the underlying array is really 9 contiguous slots starting at 0x950D,
of which the last 6 are the confirmed active-ailment slots and the
first 3 (handled here via sub_271DC, not via the ailment-cure logic)
are something else -- plausibly equipment-icon slots, matching the
very first (later-corrected) guess about this UI region. Neither
sub_271DC nor sub_219FA (called for a nonzero slot 4+) are traced, so
not asserting the icons' exact identity. -> HandleStatusIconBarClick

Run via:
    .\run_ida_script.ps1 name_handle_icon_bar_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x270FE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleStatusIconBarClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleStatusIconBarClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Hit-tests region table 0x636C (also used by "
    "TryCureAilmentFromIconClick, with a different mouse-position "
    "pair). Zones 1-3 -> sub_271DC (not traced); zones 4+ -> table "
    "0x950D, stride 4 -- which for zone 4..9 is the exact same memory "
    "as TryCureAilmentFromIconClick's 6-slot ailment table, implying a "
    "9-slot array whose first 3 entries are something else (equipment "
    "icons?), not confirmed.",
    False,
)
